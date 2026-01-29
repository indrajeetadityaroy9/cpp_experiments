#ifndef BENCHMARK_HARNESS_H
#define BENCHMARK_HARNESS_H

#include "../src/metrics/latency_recorder.h"
#include "../src/timing/cycle_timer.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <random>
#include <vector>

// Platform-specific includes
#if defined(__APPLE__)
#include <mach/mach.h>
#include <mach/thread_policy.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/sysctl.h>
#elif defined(__linux__)
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/resource.h>
#endif

namespace bench {

/**
 * Research-grade HFT benchmark result.
 */
struct BenchResult {
    // Percentiles (nanoseconds)
    double p50_ns;
    double p90_ns;
    double p95_ns;
    double p99_ns;
    double p999_ns;
    double p9999_ns;

    // Distribution
    double min_ns;
    double max_ns;
    double mean_ns;
    double stddev_ns;

    // Performance
    double throughput_mops;

    // Metadata
    size_t sample_count;
    size_t outliers_removed;
    double timer_overhead_ns;
};

/**
 * Benchmark configuration with HFT-grade defaults.
 */
struct BenchConfig {
    size_t ops_per_trial = 1000000;   // 1M ops for statistical significance
    size_t warmup_ops = 100000;       // 100K warmup
    int read_percent = 95;            // Read-heavy workload
    uint64_t rng_seed = 0xDEADBEEF;
    bool pin_cpu = true;              // Pin to core 0
    int cpu_core = 0;                 // Core to pin to
    bool lock_memory = true;          // mlock pages
    bool remove_outliers = false;     // IQR outlier removal
    bool measure_overhead = true;     // Subtract timer overhead
    bool verify_key_coverage = false; // Sanity check: verify all accessed keys exist
    size_t batch_size = 1;            // Batch N ops per timing sample (improves resolution)
                                      // Effective resolution = timer_resolution / batch_size
    bool flush_caches = false;        // Flush caches before measurement for fair cold-start
};

/**
 * Environment preparation for low-latency benchmarking.
 */
class BenchEnvironment {
public:
    /**
     * Pin current thread to a specific CPU core.
     * Reduces context switches and cache pollution.
     */
    static bool pin_to_core(int core) {
#if defined(__APPLE__)
        thread_affinity_policy_data_t policy = { core };
        thread_port_t thread = pthread_mach_thread_np(pthread_self());
        kern_return_t ret = thread_policy_set(
            thread,
            THREAD_AFFINITY_POLICY,
            reinterpret_cast<thread_policy_t>(&policy),
            THREAD_AFFINITY_POLICY_COUNT
        );
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

    /**
     * Set thread to high priority (best effort).
     */
    static bool set_high_priority() {
#if defined(__APPLE__) || defined(__linux__)
        struct sched_param param;
        param.sched_priority = sched_get_priority_max(SCHED_FIFO);
        // Note: SCHED_FIFO requires root. Fall back silently.
        if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &param) == 0) {
            return true;
        }
        // Try SCHED_RR as fallback
        param.sched_priority = sched_get_priority_max(SCHED_RR);
        return pthread_setschedparam(pthread_self(), SCHED_RR, &param) == 0;
#else
        return false;
#endif
    }

    /**
     * Lock memory to prevent page faults during measurement.
     */
    static bool lock_memory() {
#if defined(__APPLE__) || defined(__linux__)
        return mlockall(MCL_CURRENT | MCL_FUTURE) == 0;
#else
        return false;
#endif
    }

    /**
     * Flush CPU caches by accessing a large buffer.
     * Uses a buffer larger than typical L3 cache (32MB) to evict previous data.
     * This helps ensure fair cold-start comparisons between implementations.
     */
    static void flush_caches() {
        // 32MB buffer - larger than most L3 caches
        constexpr size_t FLUSH_BUFFER_SIZE = 32 * 1024 * 1024;
        static std::vector<uint8_t> flush_buffer;

        if (flush_buffer.empty()) {
            flush_buffer.resize(FLUSH_BUFFER_SIZE);
        }

        // Access every cache line to evict previous data
        volatile uint8_t sink = 0;
        for (size_t i = 0; i < FLUSH_BUFFER_SIZE; i += 64) {  // 64-byte cache lines
            flush_buffer[i] = static_cast<uint8_t>(i);
            sink += flush_buffer[i];
        }
        (void)sink;

        memory_barrier();
    }

    /**
     * Measure timer overhead (cost of taking a timestamp).
     * Returns median overhead in nanoseconds.
     */
    static double measure_timer_overhead_ns() {
        constexpr int OVERHEAD_MEASUREMENT_COUNT = 100000;
        std::vector<double> overhead_samples_ns;
        overhead_samples_ns.reserve(OVERHEAD_MEASUREMENT_COUNT);

        // Warmup
        for (int i = 0; i < 10000; ++i) {
            timing::CycleTimer t;
            compiler_barrier();
            volatile double x = t.elapsed_ns();
            (void)x;
        }

        // Measure with back-to-back timestamps to isolate timer call cost
        for (int i = 0; i < OVERHEAD_MEASUREMENT_COUNT; ++i) {
            compiler_barrier();
            timing::CycleTimer t;
            compiler_barrier();
            double elapsed_ns = t.elapsed_ns();
            compiler_barrier();
            if (elapsed_ns > 0) {  // Filter zero measurements from timer quantization
                overhead_samples_ns.push_back(elapsed_ns);
            }
        }

        if (overhead_samples_ns.empty()) {
            return timing::CycleTimer::resolution_ns();
        }

        std::sort(overhead_samples_ns.begin(), overhead_samples_ns.end());
        // Use p1 to get lower bound of typical overhead (closer to true minimum)
        return overhead_samples_ns[overhead_samples_ns.size() / 100];
    }

    /**
     * Force memory barrier to prevent speculative execution.
     */
    static inline void memory_barrier() {
        std::atomic_thread_fence(std::memory_order_seq_cst);
    }

    /**
     * Compiler barrier to prevent reordering.
     */
    static inline void compiler_barrier() {
        asm volatile("" ::: "memory");
    }
};

/**
 * Pre-generated access pattern for deterministic, overhead-free benchmarking.
 * Eliminates RNG calls from the hot path.
 *
 * Optimization: Uses single interleaved structure instead of two vectors.
 * This improves cache locality - both key_index and is_read are fetched
 * in the same cache line access.
 */
class AccessPattern {
    // Interleaved operation structure for cache locality
    // 8 bytes total = 8 operations per 64-byte cache line
    struct alignas(8) Operation {
        uint32_t key_index;  // 4 bytes (sufficient for most tables)
        uint8_t is_read;     // 1 byte
        uint8_t padding[3];  // Align to 8 bytes
    };
    static_assert(sizeof(Operation) == 8, "Operation should be 8 bytes");

    std::vector<Operation> ops_;
    size_t pos_;

public:
    AccessPattern(size_t count, size_t num_keys, int read_percent, uint64_t seed)
        : ops_(count), pos_(0) {

        std::mt19937_64 rng(seed);
        std::uniform_int_distribution<uint32_t> key_dist(0,
            static_cast<uint32_t>(num_keys > 0 ? num_keys - 1 : 0));
        std::uniform_int_distribution<int> op_dist(0, 99);

        for (size_t i = 0; i < count; ++i) {
            ops_[i].key_index = key_dist(rng);
            ops_[i].is_read = (op_dist(rng) < read_percent) ? 1 : 0;
        }
    }

    inline size_t next_key_index() noexcept { return ops_[pos_].key_index; }
    inline bool next_is_read() noexcept { return ops_[pos_++].is_read != 0; }
    void reset() noexcept { pos_ = 0; }
    size_t size() const noexcept { return ops_.size(); }
};

/**
 * Research-grade HFT benchmark harness.
 *
 * Methodology:
 * 1. Pin thread to isolated core
 * 2. Lock memory pages (mlock)
 * 3. Measure and subtract timer overhead
 * 4. Pre-generate access patterns (no RNG in hot path)
 * 5. Multi-phase warmup (cold → warm → hot cache)
 * 6. Compiler/memory barriers around measurements
 * 7. Extended percentiles (p99.9, p99.99)
 * 8. Statistical analysis with optional outlier removal
 */
template<typename Table, typename GetFn, typename PutFn>
BenchResult run_benchmark(
    Table& table,
    const std::vector<uint64_t>& keys,
    size_t num_keys,
    GetFn get_fn,
    PutFn put_fn,
    const BenchConfig& cfg = {}
) {
    // Environment setup
    if (cfg.pin_cpu) {
        BenchEnvironment::pin_to_core(cfg.cpu_core);
    }
    if (cfg.lock_memory) {
        BenchEnvironment::lock_memory();
    }

    // Measure timer overhead to subtract from individual operation latencies
    double timer_overhead_ns = 0.0;
    if (cfg.measure_overhead) {
        timer_overhead_ns = BenchEnvironment::measure_timer_overhead_ns();
    }

    // Pre-generate access patterns (eliminates RNG from hot path)
    AccessPattern warmup_pattern(cfg.warmup_ops, num_keys, cfg.read_percent, cfg.rng_seed);
    AccessPattern bench_pattern(cfg.ops_per_trial, num_keys, cfg.read_percent, cfg.rng_seed + 1);

    // Sanity check: verify all accessed key indices are within valid range
    // This catches key coverage bugs where benchmark accesses non-existent keys
    if (cfg.verify_key_coverage) {
        size_t max_key_idx = 0;
        AccessPattern verify_pattern(cfg.ops_per_trial, num_keys, cfg.read_percent, cfg.rng_seed + 1);
        for (size_t i = 0; i < cfg.ops_per_trial; ++i) {
            size_t idx = verify_pattern.next_key_index();
            verify_pattern.next_is_read();  // Consume operation type
            if (idx > max_key_idx) max_key_idx = idx;
            if (idx >= num_keys || idx >= keys.size()) {
                std::cerr << "KEY COVERAGE ERROR: Access index " << idx
                          << " exceeds key range [0, " << num_keys - 1 << "]\n";
            }
        }
        std::cerr << "Key coverage verified: max_idx=" << max_key_idx
                  << ", num_keys=" << num_keys << "\n";
    }

    // Pre-allocate recorder
    metrics::LatencyRecorder recorder(cfg.ops_per_trial);

    // Optional: Flush caches for fair cold-start comparison
    if (cfg.flush_caches) {
        BenchEnvironment::flush_caches();
    }

    // Phase 1: Cold cache warmup
    BenchEnvironment::memory_barrier();
    for (size_t i = 0; i < cfg.warmup_ops / 2; ++i) {
        size_t idx = warmup_pattern.next_key_index();
        if (warmup_pattern.next_is_read()) {
            get_fn(table, keys[idx]);
        } else {
            put_fn(table, keys[idx], keys[idx] + 1);
        }
    }

    // Phase 2: Hot cache warmup (same keys again)
    warmup_pattern.reset();
    for (size_t i = 0; i < cfg.warmup_ops; ++i) {
        size_t idx = warmup_pattern.next_key_index();
        if (warmup_pattern.next_is_read()) {
            get_fn(table, keys[idx]);
        } else {
            put_fn(table, keys[idx], keys[idx] + 1);
        }
    }
    BenchEnvironment::memory_barrier();

    // Measurement phase with batch timing support
    // Batch timing improves effective resolution: effective_res = timer_res / batch_size
    timing::CycleTimer total_timer;
    BenchEnvironment::compiler_barrier();

    const size_t batch_size = std::max(cfg.batch_size, size_t{1});
    const size_t num_batches = cfg.ops_per_trial / batch_size;
    const double batch_overhead_ns = timer_overhead_ns;  // Overhead per batch, not per op

    // Pre-allocate batch buffers to exclude marshalling from timing
    std::vector<uint64_t> batch_keys(batch_size);
    std::vector<uint64_t> batch_values(batch_size);
    std::vector<bool> batch_is_read(batch_size);

    for (size_t batch = 0; batch < num_batches; ++batch) {
        // Pre-fetch batch data OUTSIDE timing window (fixes key marshalling overhead)
        for (size_t b = 0; b < batch_size; ++b) {
            size_t idx = bench_pattern.next_key_index();
            batch_keys[b] = keys[idx];
            batch_values[b] = keys[idx] + 1;
            batch_is_read[b] = bench_pattern.next_is_read();
        }

        BenchEnvironment::compiler_barrier();
        timing::CycleTimer batch_timer;

        // Execute batch with pre-fetched data
        for (size_t b = 0; b < batch_size; ++b) {
            if (batch_is_read[b]) {
                get_fn(table, batch_keys[b]);
            } else {
                put_fn(table, batch_keys[b], batch_values[b]);
            }
        }

        BenchEnvironment::compiler_barrier();
        double batch_latency_ns = batch_timer.elapsed_ns();

        // Compute per-operation latency (amortized)
        double adjusted_batch_ns = std::max(0.0, batch_latency_ns - batch_overhead_ns);
        double per_op_latency_ns = adjusted_batch_ns / static_cast<double>(batch_size);

        // Record each operation's amortized latency
        for (size_t b = 0; b < batch_size; ++b) {
            recorder.record(static_cast<uint64_t>(per_op_latency_ns + 0.5));
        }
    }

    BenchEnvironment::compiler_barrier();

    // Compute statistics
    auto stats = recorder.compute_stats(cfg.remove_outliers);

    // Compute throughput from mean adjusted latency for consistency with reported latencies
    // Throughput = 1 / mean_latency (in Mops = million ops per second)
    // mean_ns is in nanoseconds, so: 1e9 ns/sec / mean_ns = ops/sec, then /1e6 = Mops
    double throughput_mops = stats.mean_ns > 0.0 ? 1000.0 / stats.mean_ns : 0.0;

    return {
        .p50_ns = stats.p50_ns,
        .p90_ns = stats.p90_ns,
        .p95_ns = stats.p95_ns,
        .p99_ns = stats.p99_ns,
        .p999_ns = stats.p999_ns,
        .p9999_ns = stats.p9999_ns,
        .min_ns = stats.min_ns,
        .max_ns = stats.max_ns,
        .mean_ns = stats.mean_ns,
        .stddev_ns = stats.stddev_ns,
        .throughput_mops = throughput_mops,
        .sample_count = stats.sample_count,
        .outliers_removed = stats.outlier_count,
        .timer_overhead_ns = timer_overhead_ns
    };
}

/**
 * Aggregate results from multiple trials with statistical analysis.
 */
struct AggregatedResult {
    BenchResult mean;
    BenchResult min;
    BenchResult max;
    double stddev_p99;
    size_t num_trials;
};

inline AggregatedResult aggregate_trials(const std::vector<BenchResult>& trials) {
    if (trials.empty()) return {};

    AggregatedResult agg{};
    agg.num_trials = trials.size();

    // Initialize min/max
    agg.min = trials[0];
    agg.max = trials[0];

    // Sum for mean
    for (const auto& t : trials) {
        agg.mean.p50_ns += t.p50_ns;
        agg.mean.p90_ns += t.p90_ns;
        agg.mean.p95_ns += t.p95_ns;
        agg.mean.p99_ns += t.p99_ns;
        agg.mean.p999_ns += t.p999_ns;
        agg.mean.p9999_ns += t.p9999_ns;
        agg.mean.min_ns += t.min_ns;
        agg.mean.max_ns += t.max_ns;
        agg.mean.mean_ns += t.mean_ns;
        agg.mean.throughput_mops += t.throughput_mops;

        // Track p99 extremes across trials to detect measurement variance
        if (t.p99_ns < agg.min.p99_ns) agg.min = t;
        if (t.p99_ns > agg.max.p99_ns) agg.max = t;
    }

    double n = static_cast<double>(trials.size());
    agg.mean.p50_ns /= n;
    agg.mean.p90_ns /= n;
    agg.mean.p95_ns /= n;
    agg.mean.p99_ns /= n;
    agg.mean.p999_ns /= n;
    agg.mean.p9999_ns /= n;
    agg.mean.min_ns /= n;
    agg.mean.max_ns /= n;
    agg.mean.mean_ns /= n;
    agg.mean.throughput_mops /= n;

    // Stddev of p99 across trials
    double sq_sum = 0.0;
    for (const auto& t : trials) {
        double diff = t.p99_ns - agg.mean.p99_ns;
        sq_sum += diff * diff;
    }
    agg.stddev_p99 = std::sqrt(sq_sum / n);

    return agg;
}

} // namespace bench

#endif // BENCHMARK_HARNESS_H
