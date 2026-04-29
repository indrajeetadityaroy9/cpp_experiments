#include "robin_hood.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <numeric>
#include <random>
#include <thread>
#include <unordered_map>
#include <vector>

// Platform-specific includes
#if defined(__APPLE__)
#include <mach/mach.h>
#include <mach/thread_policy.h>
#include <mach/mach_time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/sysctl.h>
#elif defined(__linux__)
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/resource.h>
#endif

// ============================================================================
// Timing
// ============================================================================
namespace timing {

inline volatile const void* escape_sink;

#if defined(__x86_64__) || defined(_M_X64)
inline uint64_t read_timestamp_ticks() {
    uint64_t lo, hi;
    __asm__ volatile(
        "lfence\n\t"
        "rdtsc\n\t"
        "lfence\n\t"
        : "=a"(lo), "=d"(hi)
        :
        : "memory"
    );
    return (hi << 32) | lo;
}
#elif defined(__APPLE__) && defined(__aarch64__)
inline uint64_t read_timestamp_ticks() {
    __asm__ volatile("isb" ::: "memory");
    return mach_absolute_time();
}
#elif defined(__aarch64__) || defined(_M_ARM64)
inline uint64_t read_timestamp_ticks() {
    uint64_t ticks;
    __asm__ volatile("isb\n\t mrs %0, cntvct_el0" : "=r"(ticks) :: "memory");
    return ticks;
}
#else
inline uint64_t read_timestamp_ticks() { return 0; }
#endif

class CycleTimer {
    uint64_t start_ticks_;
    static inline std::once_flag calibration_flag_;
    static inline std::atomic<double> ns_per_tick_{0.0};
    static inline std::atomic<double> timer_resolution_ns_{0.0};

    static void calibrate_ticks_to_ns_ratio() {
#if defined(__APPLE__) && defined(__aarch64__)
        mach_timebase_info_data_t timebase_info;
        mach_timebase_info(&timebase_info);
        double ns_per_tick = static_cast<double>(timebase_info.numer) / static_cast<double>(timebase_info.denom);
        ns_per_tick_.store(ns_per_tick, std::memory_order_release);
        timer_resolution_ns_.store(ns_per_tick, std::memory_order_release);
#else
        constexpr int CALIBRATION_DURATION_MS = 50;
        constexpr int CALIBRATION_SAMPLE_COUNT = 15;
        constexpr int WARMUP_SAMPLE_COUNT = 3;

        std::vector<double> ns_per_tick_samples;
        ns_per_tick_samples.reserve(CALIBRATION_SAMPLE_COUNT);

        for (int sample_idx = 0; sample_idx < CALIBRATION_SAMPLE_COUNT; ++sample_idx) {
            uint64_t start_ticks = read_timestamp_ticks();
            auto start_time = std::chrono::high_resolution_clock::now();
            std::this_thread::sleep_for(std::chrono::milliseconds(CALIBRATION_DURATION_MS));
            uint64_t end_ticks = read_timestamp_ticks();
            auto end_time = std::chrono::high_resolution_clock::now();

            uint64_t elapsed_ticks = end_ticks - start_ticks;
            auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();

            if (sample_idx >= WARMUP_SAMPLE_COUNT && elapsed_ticks > 0) {
                ns_per_tick_samples.push_back(static_cast<double>(elapsed_ns) / static_cast<double>(elapsed_ticks));
            }
        }
        double calibrated_ns_per_tick = 1.0;
        if (!ns_per_tick_samples.empty()) {
            std::sort(ns_per_tick_samples.begin(), ns_per_tick_samples.end());
            calibrated_ns_per_tick = ns_per_tick_samples[ns_per_tick_samples.size() / 2];
        }
        ns_per_tick_.store(calibrated_ns_per_tick, std::memory_order_release);
        timer_resolution_ns_.store(calibrated_ns_per_tick, std::memory_order_release);
#endif
    }

public:
    CycleTimer() : start_ticks_(read_timestamp_ticks()) {}
    [[nodiscard]] double elapsed_ns() const noexcept {
        uint64_t elapsed_ticks = read_timestamp_ticks() - start_ticks_;
        return static_cast<double>(elapsed_ticks) * ns_per_tick_.load(std::memory_order_acquire);
    }
    static void calibrate() { std::call_once(calibration_flag_, calibrate_ticks_to_ns_ratio); }
    static double resolution_ns() { return timer_resolution_ns_.load(std::memory_order_acquire); }
};

} // namespace timing

// ============================================================================
// Metrics
// ============================================================================
namespace metrics {

template<typename T>
[[nodiscard]] inline double compute_percentile_interpolated(const std::vector<T>& sorted_samples_ns, double percentile_fraction) {
    if (sorted_samples_ns.empty()) return 0.0;
    if (sorted_samples_ns.size() == 1) return static_cast<double>(sorted_samples_ns[0]);

    double index = percentile_fraction * (sorted_samples_ns.size() - 1);
    size_t lower_idx = static_cast<size_t>(index);
    size_t upper_idx = std::min(lower_idx + 1, sorted_samples_ns.size() - 1);
    double interpolation_fraction = index - lower_idx;

    return static_cast<double>(sorted_samples_ns[lower_idx]) * (1.0 - interpolation_fraction) +
           static_cast<double>(sorted_samples_ns[upper_idx]) * interpolation_fraction;
}

struct LatencyStats {
    double p50_ns; double p90_ns; double p95_ns; double p99_ns; double p999_ns; double p9999_ns;
    double min_ns; double max_ns; double mean_ns; double stddev_ns;
    size_t sample_count; size_t outlier_count;
};

class LatencyRecorder {
    std::vector<uint64_t> latency_samples_ns_;
    size_t sample_count_;

public:
    explicit LatencyRecorder(size_t max_samples) : latency_samples_ns_(max_samples), sample_count_(0) {
        for (size_t page_idx = 0; page_idx < max_samples; page_idx += 4096 / sizeof(uint64_t)) {
            latency_samples_ns_[page_idx] = 0;
        }
    }
    inline void record(uint64_t latency_ns) noexcept { latency_samples_ns_[sample_count_++] = latency_ns; }
    void reset() noexcept { sample_count_ = 0; }
    [[nodiscard]] LatencyStats compute_stats(bool remove_outliers = false) const {
        LatencyStats stats{};
        if (sample_count_ == 0) return stats;

        std::vector<uint64_t> sorted_samples_ns(latency_samples_ns_.begin(), latency_samples_ns_.begin() + sample_count_);
        std::sort(sorted_samples_ns.begin(), sorted_samples_ns.end());

        size_t outlier_count = 0;
        if (remove_outliers && sorted_samples_ns.size() > 100) {
            double q1_ns = compute_percentile_interpolated(sorted_samples_ns, 0.25);
            double q3_ns = compute_percentile_interpolated(sorted_samples_ns, 0.75);
            double iqr_ns = q3_ns - q1_ns;
            double lower_fence_ns = q1_ns - 1.5 * iqr_ns;
            double upper_fence_ns = q3_ns + 1.5 * iqr_ns;

            std::vector<uint64_t> filtered_samples_ns;
            filtered_samples_ns.reserve(sorted_samples_ns.size());
            for (uint64_t sample_ns : sorted_samples_ns) {
                double sample_ns_double = static_cast<double>(sample_ns);
                if (sample_ns_double >= lower_fence_ns && sample_ns_double <= upper_fence_ns) {
                    filtered_samples_ns.push_back(sample_ns);
                }
            }
            outlier_count = sorted_samples_ns.size() - filtered_samples_ns.size();
            sorted_samples_ns = std::move(filtered_samples_ns);
            std::sort(sorted_samples_ns.begin(), sorted_samples_ns.end());
        }

        if (sorted_samples_ns.empty()) return stats;

        stats.p50_ns = compute_percentile_interpolated(sorted_samples_ns, 0.50);
        stats.p90_ns = compute_percentile_interpolated(sorted_samples_ns, 0.90);
        stats.p95_ns = compute_percentile_interpolated(sorted_samples_ns, 0.95);
        stats.p99_ns = compute_percentile_interpolated(sorted_samples_ns, 0.99);
        stats.p999_ns = compute_percentile_interpolated(sorted_samples_ns, 0.999);
        stats.p9999_ns = compute_percentile_interpolated(sorted_samples_ns, 0.9999);
        stats.min_ns = static_cast<double>(sorted_samples_ns.front());
        stats.max_ns = static_cast<double>(sorted_samples_ns.back());

        double sum_ns = 0.0;
        for (uint64_t sample_ns : sorted_samples_ns) sum_ns += static_cast<double>(sample_ns);
        stats.mean_ns = sum_ns / sorted_samples_ns.size();

        double sum_squared_deviation = 0.0;
        for (uint64_t sample_ns : sorted_samples_ns) {
            double deviation_ns = static_cast<double>(sample_ns) - stats.mean_ns;
            sum_squared_deviation += deviation_ns * deviation_ns;
        }
        size_t divisor = sorted_samples_ns.size() > 1 ? sorted_samples_ns.size() - 1 : 1;
        stats.stddev_ns = std::sqrt(sum_squared_deviation / divisor);
        stats.sample_count = sorted_samples_ns.size();
        stats.outlier_count = outlier_count;

        return stats;
    }
};

} // namespace metrics

// ============================================================================
// Benchmarking Harness
// ============================================================================
namespace bench {

struct BenchResult {
    double p50_ns; double p90_ns; double p95_ns; double p99_ns; double p999_ns; double p9999_ns;
    double min_ns; double max_ns; double mean_ns; double stddev_ns; double throughput_mops;
    size_t sample_count; size_t outliers_removed; double timer_overhead_ns;
};

struct BenchConfig {
    size_t ops_per_trial = 1000000;
    size_t warmup_ops = 100000;
    int read_percent = 95;
    uint64_t rng_seed = 0xDEADBEEF;
    bool pin_cpu = true;
    int cpu_core = 0;
    bool lock_memory = true;
    bool remove_outliers = false;
    bool measure_overhead = true;
    bool verify_key_coverage = false;
    size_t batch_size = 64;
    bool flush_caches = false;
};

class BenchEnvironment {
public:
    static bool pin_to_core(int core) {
#if defined(__APPLE__)
        thread_affinity_policy_data_t policy = { core };
        thread_port_t thread = pthread_mach_thread_np(pthread_self());
        kern_return_t ret = thread_policy_set(thread, THREAD_AFFINITY_POLICY, reinterpret_cast<thread_policy_t>(&policy), THREAD_AFFINITY_POLICY_COUNT);
        return ret == KERN_SUCCESS;
#elif defined(__linux__)
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(core, &cpuset);
        return pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset) == 0;
#else
        (void)core;
        return false;
#endif
    }

    static bool lock_memory() {
#if defined(__APPLE__) || defined(__linux__)
        return mlockall(MCL_CURRENT | MCL_FUTURE) == 0;
#else
        return false;
#endif
    }

    static void flush_caches() {
        constexpr size_t FLUSH_BUFFER_SIZE = 32 * 1024 * 1024;
        static std::vector<uint8_t> flush_buffer(FLUSH_BUFFER_SIZE);
        volatile uint8_t sink = 0;
        for (size_t i = 0; i < FLUSH_BUFFER_SIZE; i += 64) {
            flush_buffer[i] = static_cast<uint8_t>(i);
            sink += flush_buffer[i];
        }
        (void)sink;
        memory_barrier();
    }

    static double measure_timer_overhead_ns() {
        constexpr int OVERHEAD_MEASUREMENT_COUNT = 100000;
        std::vector<double> overhead_samples_ns;
        overhead_samples_ns.reserve(OVERHEAD_MEASUREMENT_COUNT);

        for (int i = 0; i < 10000; ++i) {
            timing::CycleTimer t;
            compiler_barrier();
            volatile double x = t.elapsed_ns();
            (void)x;
        }

        for (int i = 0; i < OVERHEAD_MEASUREMENT_COUNT; ++i) {
            compiler_barrier();
            timing::CycleTimer t;
            compiler_barrier();
            double elapsed_ns = t.elapsed_ns();
            compiler_barrier();
            if (elapsed_ns > 0) {
                overhead_samples_ns.push_back(elapsed_ns);
            }
        }

        if (overhead_samples_ns.empty()) return timing::CycleTimer::resolution_ns();
        std::sort(overhead_samples_ns.begin(), overhead_samples_ns.end());
        return overhead_samples_ns[overhead_samples_ns.size() / 100];
    }

    static inline void memory_barrier() { std::atomic_thread_fence(std::memory_order_seq_cst); }
    static inline void compiler_barrier() { asm volatile("" ::: "memory"); }
};

class AccessPattern {
    struct alignas(8) Operation { uint32_t key_index; uint8_t is_read; uint8_t padding[3]; };
    std::vector<Operation> ops_;
    size_t pos_;
public:
    AccessPattern(size_t count, size_t num_keys, int read_percent, uint64_t seed) : ops_(count), pos_(0) {
        std::mt19937_64 rng(seed);
        std::uniform_int_distribution<uint32_t> key_dist(0, static_cast<uint32_t>(num_keys > 0 ? num_keys - 1 : 0));
        std::uniform_int_distribution<int> op_dist(0, 99);
        for (size_t i = 0; i < count; ++i) {
            ops_[i].key_index = key_dist(rng);
            ops_[i].is_read = (op_dist(rng) < read_percent) ? 1 : 0;
        }
    }
    inline size_t next_key_index() noexcept { return ops_[pos_].key_index; }
    inline bool next_is_read() noexcept { return ops_[pos_++].is_read != 0; }
    void reset() noexcept { pos_ = 0; }
};

template<typename Table, typename GetFn, typename PutFn>
BenchResult run_benchmark(Table& table, const std::vector<uint64_t>& keys, size_t num_keys, GetFn get_fn, PutFn put_fn, const BenchConfig& cfg = {}) {
    if (cfg.pin_cpu) BenchEnvironment::pin_to_core(cfg.cpu_core);
    if (cfg.lock_memory) BenchEnvironment::lock_memory();

    double timer_overhead_ns = cfg.measure_overhead ? BenchEnvironment::measure_timer_overhead_ns() : 0.0;
    AccessPattern warmup_pattern(cfg.warmup_ops, num_keys, cfg.read_percent, cfg.rng_seed);
    AccessPattern bench_pattern(cfg.ops_per_trial, num_keys, cfg.read_percent, cfg.rng_seed + 1);

    metrics::LatencyRecorder recorder(cfg.ops_per_trial);
    if (cfg.flush_caches) BenchEnvironment::flush_caches();

    BenchEnvironment::memory_barrier();
    for (size_t i = 0; i < cfg.warmup_ops / 2; ++i) {
        size_t idx = warmup_pattern.next_key_index();
        if (warmup_pattern.next_is_read()) get_fn(table, keys[idx]);
        else put_fn(table, keys[idx], keys[idx] + 1);
    }
    warmup_pattern.reset();
    for (size_t i = 0; i < cfg.warmup_ops; ++i) {
        size_t idx = warmup_pattern.next_key_index();
        if (warmup_pattern.next_is_read()) get_fn(table, keys[idx]);
        else put_fn(table, keys[idx], keys[idx] + 1);
    }
    BenchEnvironment::memory_barrier();

    const size_t batch_size = std::max(cfg.batch_size, size_t{1});
    const size_t num_batches = cfg.ops_per_trial / batch_size;
    std::vector<uint64_t> batch_keys(batch_size);
    std::vector<uint64_t> batch_values(batch_size);
    std::vector<bool> batch_is_read(batch_size);

    for (size_t batch = 0; batch < num_batches; ++batch) {
        for (size_t b = 0; b < batch_size; ++b) {
            size_t idx = bench_pattern.next_key_index();
            batch_keys[b] = keys[idx];
            batch_values[b] = keys[idx] + 1;
            batch_is_read[b] = bench_pattern.next_is_read();
        }

        BenchEnvironment::compiler_barrier();
        timing::CycleTimer batch_timer;
        for (size_t b = 0; b < batch_size; ++b) {
            if (batch_is_read[b]) get_fn(table, batch_keys[b]);
            else put_fn(table, batch_keys[b], batch_values[b]);
        }
        BenchEnvironment::compiler_barrier();
        double batch_latency_ns = batch_timer.elapsed_ns();

        double per_op_latency_ns = std::max(0.0, batch_latency_ns - timer_overhead_ns) / static_cast<double>(batch_size);
        for (size_t b = 0; b < batch_size; ++b) recorder.record(static_cast<uint64_t>(per_op_latency_ns + 0.5));
    }

    BenchEnvironment::compiler_barrier();
    auto stats = recorder.compute_stats(cfg.remove_outliers);
    double throughput_mops = stats.mean_ns > 0.0 ? 1000.0 / stats.mean_ns : 0.0;

    return { stats.p50_ns, stats.p90_ns, stats.p95_ns, stats.p99_ns, stats.p999_ns, stats.p9999_ns,
             stats.min_ns, stats.max_ns, stats.mean_ns, stats.stddev_ns, throughput_mops,
             stats.sample_count, stats.outlier_count, timer_overhead_ns };
}

struct AggregatedResult {
    BenchResult mean; BenchResult min; BenchResult max; double stddev_p99; size_t num_trials;
};

inline AggregatedResult aggregate_trials(const std::vector<BenchResult>& trials) {
    if (trials.empty()) return {};
    AggregatedResult agg{};
    agg.num_trials = trials.size();
    agg.min = trials[0]; agg.max = trials[0];

    for (const auto& t : trials) {
        agg.mean.p50_ns += t.p50_ns; agg.mean.p90_ns += t.p90_ns; agg.mean.p95_ns += t.p95_ns;
        agg.mean.p99_ns += t.p99_ns; agg.mean.p999_ns += t.p999_ns; agg.mean.p9999_ns += t.p9999_ns;
        agg.mean.min_ns += t.min_ns; agg.mean.max_ns += t.max_ns; agg.mean.mean_ns += t.mean_ns;
        agg.mean.throughput_mops += t.throughput_mops;
        if (t.p99_ns < agg.min.p99_ns) agg.min = t;
        if (t.p99_ns > agg.max.p99_ns) agg.max = t;
    }

    double n = static_cast<double>(trials.size());
    agg.mean.p50_ns /= n; agg.mean.p90_ns /= n; agg.mean.p95_ns /= n;
    agg.mean.p99_ns /= n; agg.mean.p999_ns /= n; agg.mean.p9999_ns /= n;
    agg.mean.min_ns /= n; agg.mean.max_ns /= n; agg.mean.mean_ns /= n;
    agg.mean.throughput_mops /= n;

    double sq_sum = 0.0;
    for (const auto& t : trials) { double diff = t.p99_ns - agg.mean.p99_ns; sq_sum += diff * diff; }
    agg.stddev_p99 = std::sqrt(sq_sum / n);
    return agg;
}

} // namespace bench

// ============================================================================
// Main Benchmark
// ============================================================================

using namespace robin_hood;
using namespace bench;
using timing::escape_sink;

static constexpr size_t CAPACITY = 8192;
static constexpr size_t NUM_TRIALS = 5;
static constexpr double LOAD_FACTORS[] = {0.50, 0.70, 0.85, 0.90};

template<size_t Cap>
BenchResult benchmark_robin_hood(const std::vector<uint64_t>& keys, double load_factor, const BenchConfig& cfg) {
    RobinHoodTable<uint64_t, uint64_t, Cap> table;
    size_t num_keys = static_cast<size_t>(load_factor * Cap);
    for (size_t i = 0; i < num_keys && i < keys.size(); ++i) (void)table.put(keys[i], keys[i]);
    return run_benchmark(table, keys, num_keys,
        [](auto& t, uint64_t k) { escape_sink = t.get(k); },
        [](auto& t, uint64_t k, uint64_t v) { (void)t.put(k, v); }, cfg);
}

BenchResult benchmark_std(const std::vector<uint64_t>& keys, double load_factor, const BenchConfig& cfg) {
    std::unordered_map<uint64_t, uint64_t> table;
    table.reserve(CAPACITY);
    size_t num_keys = static_cast<size_t>(load_factor * CAPACITY);
    for (size_t i = 0; i < num_keys && i < keys.size(); ++i) table[keys[i]] = keys[i];
    return run_benchmark(table, keys, num_keys,
        [](auto& t, uint64_t k) { auto it = t.find(k); escape_sink = (it != t.end()) ? &it->second : nullptr; },
        [](auto& t, uint64_t k, uint64_t v) { t[k] = v; }, cfg);
}

void print_result_header() {
    std::cout << std::left << std::setw(20) << "Table" << std::right
              << std::setw(8) << "min" << std::setw(8) << "p50" << std::setw(8) << "p90" << std::setw(8) << "p95"
              << std::setw(8) << "p99" << std::setw(9) << "p99.9" << std::setw(10) << "p99.99" << std::setw(8) << "max"
              << std::setw(8) << "Mops" << "\n" << std::string(95, '-') << "\n";
}

void print_result_row(const char* name, const BenchResult& r) {
    std::cout << std::left << std::setw(20) << name << std::right << std::fixed
              << std::setw(8) << std::setprecision(1) << r.min_ns << std::setw(8) << r.p50_ns
              << std::setw(8) << r.p90_ns << std::setw(8) << r.p95_ns << std::setw(8) << r.p99_ns
              << std::setw(9) << r.p999_ns << std::setw(10) << r.p9999_ns << std::setw(8) << r.max_ns
              << std::setw(8) << std::setprecision(2) << r.throughput_mops << "\n";
}

int main() {
    std::cout << std::string(100, '=') << "\n  RESEARCH-GRADE HFT HASH TABLE BENCHMARK\n" << std::string(100, '=') << "\n\n";

    timing::CycleTimer::calibrate();
    std::cout << "Environment:\n  Timer resolution:  " << std::fixed << std::setprecision(2) << timing::CycleTimer::resolution_ns() << " ns\n\n";

    BenchConfig cfg;
    cfg.ops_per_trial = 1000000;
    cfg.warmup_ops = 100000;
    cfg.batch_size = 64;

    std::vector<uint64_t> keys;
    keys.reserve(CAPACITY);
    std::mt19937_64 key_generator_rng(42);
    for (size_t i = 0; i < CAPACITY; ++i) keys.push_back(key_generator_rng());

    for (double lf : LOAD_FACTORS) {
        std::cout << std::string(95, '=') << "\nLoad Factor: " << static_cast<int>(lf * 100) << "% ("
                  << static_cast<size_t>(lf * CAPACITY) << " / " << CAPACITY << " buckets)\n" << std::string(95, '=') << "\n\n";

        std::vector<BenchResult> robin_trials, std_trials;
        for (size_t trial = 0; trial < NUM_TRIALS; ++trial) {
            std::cout << "Trial " << (trial + 1) << "/" << NUM_TRIALS << "...\r" << std::flush;
            if (trial % 2 == 0) {
                robin_trials.push_back(benchmark_robin_hood<CAPACITY>(keys, lf, cfg));
                std_trials.push_back(benchmark_std(keys, lf, cfg));
            } else {
                std_trials.push_back(benchmark_std(keys, lf, cfg));
                robin_trials.push_back(benchmark_robin_hood<CAPACITY>(keys, lf, cfg));
            }
        }
        std::cout << std::string(30, ' ') << "\r";

        auto robin_agg = aggregate_trials(robin_trials);
        auto std_agg = aggregate_trials(std_trials);

        print_result_header();
        print_result_row("RobinHoodTable", robin_agg.mean);
        print_result_row("std::unordered_map", std_agg.mean);
        std::cout << "\n";
    }
    return 0;
}