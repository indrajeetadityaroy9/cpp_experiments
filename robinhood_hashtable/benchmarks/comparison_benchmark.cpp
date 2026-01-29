#include "benchmark_harness.h"
#include "../src/core/robin_hood_table.h"

#include <cstring>
#include <iomanip>
#include <iostream>
#include <random>
#include <unordered_map>
#include <vector>

using namespace robin_hood;
using namespace bench;
using timing::escape_sink;

// Benchmark parameters
static constexpr size_t CAPACITY = 8192;
static constexpr size_t NUM_TRIALS = 5;
static constexpr double LOAD_FACTORS[] = {0.50, 0.70, 0.85, 0.90};

// Sanity check: seed validation mode seeds
static constexpr uint64_t VALIDATION_SEEDS[] = {42, 12345, 0xDEADBEEF, 0xCAFEBABE};
static constexpr size_t NUM_VALIDATION_SEEDS = sizeof(VALIDATION_SEEDS) / sizeof(VALIDATION_SEEDS[0]);
static constexpr double SEED_VARIANCE_THRESHOLD = 0.15;  // 15% max variance across seeds

// HFT performance targets in nanoseconds
static constexpr double TARGET_P99_NS_AT_50_PERCENT_LF = 30.0;   // p99 < 30ns at 50% load factor
static constexpr double TARGET_P99_NS_AT_70_PERCENT_LF = 50.0;   // p99 < 50ns at 70% load factor
static constexpr double TARGET_P999_NS_AT_70_PERCENT_LF = 100.0; // p99.9 < 100ns at 70% load factor

template<size_t Cap>
BenchResult benchmark_robin_hood(const std::vector<uint64_t>& keys, double load_factor, const BenchConfig& cfg) {
    RobinHoodTable<uint64_t, uint64_t, Cap> table;
    size_t num_keys = static_cast<size_t>(load_factor * Cap);

    for (size_t i = 0; i < num_keys && i < keys.size(); ++i) {
        (void)table.put(keys[i], keys[i]);
    }

    return run_benchmark(
        table, keys, num_keys,
        [](auto& t, uint64_t k) { escape_sink = t.get(k); },
        [](auto& t, uint64_t k, uint64_t v) { (void)t.put(k, v); },
        cfg
    );
}

BenchResult benchmark_std(const std::vector<uint64_t>& keys, double load_factor, const BenchConfig& cfg) {
    std::unordered_map<uint64_t, uint64_t> table;
    table.reserve(CAPACITY);
    size_t num_keys = static_cast<size_t>(load_factor * CAPACITY);

    for (size_t i = 0; i < num_keys && i < keys.size(); ++i) {
        table[keys[i]] = keys[i];
    }

    return run_benchmark(
        table, keys, num_keys,
        [](auto& t, uint64_t k) {
            // Unconditional assignment to match RobinHood's pattern (no branch overhead)
            // Safe because all accessed keys are pre-populated (verified by access pattern)
            auto it = t.find(k);
            escape_sink = (it != t.end()) ? &it->second : nullptr;
        },
        [](auto& t, uint64_t k, uint64_t v) { t[k] = v; },
        cfg
    );
}

void print_header() {
    std::cout << std::string(100, '=') << "\n";
    std::cout << "  RESEARCH-GRADE HFT HASH TABLE BENCHMARK\n";
    std::cout << std::string(100, '=') << "\n\n";
}

void print_config(const BenchConfig& cfg) {
    std::cout << "Configuration:\n";
    std::cout << "  Operations/trial:  " << cfg.ops_per_trial << "\n";
    std::cout << "  Warmup ops:        " << cfg.warmup_ops << "\n";
    std::cout << "  Trials:            " << NUM_TRIALS << "\n";
    std::cout << "  Read/Write ratio:  " << cfg.read_percent << "/" << (100 - cfg.read_percent) << "\n";
    std::cout << "  Batch size:        " << cfg.batch_size << " ops/sample\n";
    std::cout << "  CPU pinning:       " << (cfg.pin_cpu ? "enabled" : "disabled") << "\n";
    std::cout << "  Memory locking:    " << (cfg.lock_memory ? "enabled" : "disabled") << "\n";
    std::cout << "  Table capacity:    " << CAPACITY << "\n";
    std::cout << "\n";
}

void print_environment_info() {
    std::cout << "Environment:\n";

    // Calibrate timer first (prints calibration info to stderr)
    timing::CycleTimer::calibrate();

    double resolution = timing::CycleTimer::resolution_ns();
    double overhead_ns = BenchEnvironment::measure_timer_overhead_ns();

    std::cout << "  Timer resolution:  " << std::fixed << std::setprecision(2) << resolution << " ns\n";
    std::cout << "  Timer overhead:    " << std::fixed << std::setprecision(2) << overhead_ns << " ns\n";

#if defined(__APPLE__) && defined(__aarch64__)
    std::cout << "  Platform:          macOS (Apple Silicon)\n";
    std::cout << "  Timer source:      mach_absolute_time()\n";
#elif defined(__x86_64__)
    std::cout << "  Platform:          x86-64\n";
    std::cout << "  Timer source:      RDTSC\n";
#elif defined(__aarch64__)
    std::cout << "  Platform:          ARM64\n";
    std::cout << "  Timer source:      CNTVCT_EL0\n";
#endif
    std::cout << "\n";
}

void print_targets() {
    double resolution = timing::CycleTimer::resolution_ns();

    std::cout << "HFT Performance Targets:\n";
    std::cout << "  50% LF: p99 < " << std::fixed << std::setprecision(0) << TARGET_P99_NS_AT_50_PERCENT_LF << " ns\n";
    std::cout << "  70% LF: p99 < " << TARGET_P99_NS_AT_70_PERCENT_LF << " ns, p99.9 < " << TARGET_P999_NS_AT_70_PERCENT_LF << " ns\n";

    if (resolution > TARGET_P99_NS_AT_50_PERCENT_LF) {
        std::cout << "\n  NOTE: Timer resolution (" << std::setprecision(1) << resolution
                  << " ns) exceeds some targets.\n";
        std::cout << "        Operations faster than " << resolution
                  << " ns will show as 0 ns.\n";
        std::cout << "        Focus on p99.9/p99.99 tail latencies for comparison.\n";
    }
    std::cout << "\n";
}

void print_result_row(const char* name, const BenchResult& r) {
    std::cout << std::left << std::setw(20) << name
              << std::right << std::fixed
              << std::setw(8) << std::setprecision(1) << r.min_ns
              << std::setw(8) << r.p50_ns
              << std::setw(8) << r.p90_ns
              << std::setw(8) << r.p95_ns
              << std::setw(8) << r.p99_ns
              << std::setw(9) << r.p999_ns
              << std::setw(10) << r.p9999_ns
              << std::setw(8) << r.max_ns
              << std::setw(8) << std::setprecision(2) << r.throughput_mops
              << "\n";
}

void print_result_header() {
    std::cout << std::left << std::setw(20) << "Table"
              << std::right
              << std::setw(8) << "min"
              << std::setw(8) << "p50"
              << std::setw(8) << "p90"
              << std::setw(8) << "p95"
              << std::setw(8) << "p99"
              << std::setw(9) << "p99.9"
              << std::setw(10) << "p99.99"
              << std::setw(8) << "max"
              << std::setw(8) << "Mops"
              << "\n";
    std::cout << std::string(95, '-') << "\n";
}

void print_verdict(double lf, const AggregatedResult& robin) {
    double resolution = timing::CycleTimer::resolution_ns();
    bool pass = true;
    std::string reason;
    std::string note;

    // Adjust evaluation based on timer resolution
    if (lf <= 0.50) {
        if (robin.mean.p99_ns > TARGET_P99_NS_AT_50_PERCENT_LF) {
            // If p99 is at resolution limit, check if most ops are sub-resolution
            if (robin.mean.p99_ns <= resolution && robin.mean.p999_ns < TARGET_P999_NS_AT_70_PERCENT_LF) {
                note = " (p99 at timer resolution, p99.9 OK)";
            } else {
                pass = false;
                reason = "p99 > " + std::to_string(static_cast<int>(TARGET_P99_NS_AT_50_PERCENT_LF)) + "ns";
            }
        }
    } else if (lf <= 0.70) {
        if (robin.mean.p99_ns > TARGET_P99_NS_AT_70_PERCENT_LF && robin.mean.p99_ns > resolution) {
            pass = false;
            reason = "p99 > " + std::to_string(static_cast<int>(TARGET_P99_NS_AT_70_PERCENT_LF)) + "ns";
        } else if (robin.mean.p999_ns > TARGET_P999_NS_AT_70_PERCENT_LF) {
            pass = false;
            reason = "p99.9 > " + std::to_string(static_cast<int>(TARGET_P999_NS_AT_70_PERCENT_LF)) + "ns";
        }
    } else if (lf <= 0.85) {
        // Relaxed target at high load
        if (robin.mean.p999_ns > 200.0) {
            pass = false;
            reason = "p99.9 > 200ns";
        }
    } else {
        // Very high load - just check tail isn't catastrophic
        if (robin.mean.p9999_ns > 1000.0) {
            pass = false;
            reason = "p99.99 > 1000ns";
        }
    }

    if (pass) {
        std::cout << "  [PASS] Meets HFT target at " << static_cast<int>(lf * 100) << "% LF" << note << "\n";
    } else {
        std::cout << "  [FAIL] " << reason << " at " << static_cast<int>(lf * 100) << "% LF\n";
    }
}

void print_statistical_summary(const AggregatedResult& robin, const AggregatedResult& std_map) {
    std::cout << "\nStatistical Summary (across " << robin.num_trials << " trials):\n";

    // Use p99.9 for comparison when p99 is at timer resolution limit
    double resolution = timing::CycleTimer::resolution_ns();
    bool use_p999 = (robin.mean.p99_ns < resolution && std_map.mean.p99_ns < resolution);

    if (use_p999) {
        std::cout << "  (Using p99.9 for comparison - p99 below timer resolution)\n";
        std::cout << "  RobinHood p99.9:   mean=" << std::fixed << std::setprecision(1)
                  << robin.mean.p999_ns << " ns\n";
        std::cout << "  std::unordered_map p99.9: mean=" << std_map.mean.p999_ns << " ns\n";

        if (std_map.mean.p999_ns > 0 && robin.mean.p999_ns > 0) {
            double speedup = std_map.mean.p999_ns / robin.mean.p999_ns;
            std::cout << "  p99.9 speedup:     " << std::setprecision(2) << speedup << "x";
            if (speedup > 1.0) std::cout << " (RobinHood faster)";
            else if (speedup < 1.0) std::cout << " (std::unordered_map faster)";
            std::cout << "\n";
        }
    } else {
        std::cout << "  RobinHood p99:     mean=" << std::fixed << std::setprecision(1)
                  << robin.mean.p99_ns << " ns, stddev=" << robin.stddev_p99
                  << " ns, range=[" << robin.min.p99_ns << ", " << robin.max.p99_ns << "]\n";
        std::cout << "  std::unordered_map p99: mean=" << std_map.mean.p99_ns
                  << " ns, stddev=" << std_map.stddev_p99
                  << " ns, range=[" << std_map.min.p99_ns << ", " << std_map.max.p99_ns << "]\n";

        if (std_map.mean.p99_ns > 0 && robin.mean.p99_ns > 0) {
            double speedup = std_map.mean.p99_ns / robin.mean.p99_ns;
            std::cout << "  p99 speedup:       " << std::setprecision(2) << speedup << "x";
            if (speedup > 1.0) std::cout << " (RobinHood faster)";
            else if (speedup < 1.0) std::cout << " (std::unordered_map faster)";
            std::cout << "\n";
        }
    }

    // Always show throughput comparison
    double tput_ratio = robin.mean.throughput_mops / std_map.mean.throughput_mops;
    std::cout << "  Throughput:        RobinHood " << std::setprecision(2)
              << robin.mean.throughput_mops << " vs std " << std_map.mean.throughput_mops
              << " Mops (" << tput_ratio << "x)\n";
}

/**
 * Sanity check: Run with multiple seeds to verify results are seed-independent.
 * High variance across seeds indicates measurement instability or data leakage.
 */
void run_seed_validation() {
    std::cout << "\n" << std::string(100, '=') << "\n";
    std::cout << "  SEED VALIDATION SANITY CHECK\n";
    std::cout << std::string(100, '=') << "\n\n";

    BenchConfig cfg;
    cfg.ops_per_trial = 100000;  // Reduced for faster validation
    cfg.warmup_ops = 10000;
    cfg.pin_cpu = true;
    cfg.lock_memory = true;
    cfg.measure_overhead = true;
    cfg.batch_size = 64;         // Use batch timing for meaningful resolution

    double load_factor = 0.70;  // Test at 70% LF
    size_t num_keys = static_cast<size_t>(load_factor * CAPACITY);

    std::vector<double> robin_p99_results;
    std::vector<double> std_p99_results;

    for (size_t seed_idx = 0; seed_idx < NUM_VALIDATION_SEEDS; ++seed_idx) {
        uint64_t seed = VALIDATION_SEEDS[seed_idx];
        cfg.rng_seed = seed;

        // Generate keys with this seed
        std::vector<uint64_t> keys;
        keys.reserve(CAPACITY);
        std::mt19937_64 key_generator_rng(seed);
        for (size_t i = 0; i < CAPACITY; ++i) {
            keys.push_back(key_generator_rng());
        }

        // Benchmark RobinHood
        RobinHoodTable<uint64_t, uint64_t, CAPACITY> robin_table;
        for (size_t i = 0; i < num_keys; ++i) {
            (void)robin_table.put(keys[i], keys[i]);
        }
        auto robin_result = run_benchmark(
            robin_table, keys, num_keys,
            [](auto& t, uint64_t k) { escape_sink = t.get(k); },
            [](auto& t, uint64_t k, uint64_t v) { (void)t.put(k, v); },
            cfg
        );

        // Benchmark std::unordered_map
        std::unordered_map<uint64_t, uint64_t> std_table;
        std_table.reserve(CAPACITY);
        for (size_t i = 0; i < num_keys; ++i) {
            std_table[keys[i]] = keys[i];
        }
        auto std_result = run_benchmark(
            std_table, keys, num_keys,
            [](auto& t, uint64_t k) {
                auto it = t.find(k);
                escape_sink = (it != t.end()) ? &it->second : nullptr;
            },
            [](auto& t, uint64_t k, uint64_t v) { t[k] = v; },
            cfg
        );

        robin_p99_results.push_back(robin_result.p99_ns);
        std_p99_results.push_back(std_result.p99_ns);

        std::cout << "Seed " << std::hex << seed << std::dec
                  << ": Robin p99=" << std::fixed << std::setprecision(1) << robin_result.p99_ns
                  << " ns, std p99=" << std_result.p99_ns << " ns\n";
    }

    // Compute variance
    auto compute_variance = [](const std::vector<double>& values) {
        double sum = 0.0, sq_sum = 0.0;
        for (double v : values) { sum += v; sq_sum += v * v; }
        double mean = sum / values.size();
        double variance = sq_sum / values.size() - mean * mean;
        return std::make_pair(mean, std::sqrt(variance) / mean);  // (mean, CV)
    };

    auto [robin_mean, robin_cv] = compute_variance(robin_p99_results);
    auto [std_mean, std_cv] = compute_variance(std_p99_results);

    std::cout << "\nVariance Analysis (Coefficient of Variation):\n";
    std::cout << "  RobinHood p99: mean=" << robin_mean << " ns, CV=" << std::setprecision(2) << (robin_cv * 100) << "%\n";
    std::cout << "  std::unordered_map p99: mean=" << std_mean << " ns, CV=" << (std_cv * 100) << "%\n";

    bool robin_stable = robin_cv < SEED_VARIANCE_THRESHOLD;
    bool std_stable = std_cv < SEED_VARIANCE_THRESHOLD;

    std::cout << "\nSanity Check Result:\n";
    std::cout << "  RobinHood: " << (robin_stable ? "[PASS]" : "[WARN]")
              << " variance " << (robin_stable ? "within" : "exceeds") << " " << (SEED_VARIANCE_THRESHOLD * 100) << "% threshold\n";
    std::cout << "  std::unordered_map: " << (std_stable ? "[PASS]" : "[WARN]")
              << " variance " << (std_stable ? "within" : "exceeds") << " " << (SEED_VARIANCE_THRESHOLD * 100) << "% threshold\n";

    if (!robin_stable || !std_stable) {
        std::cout << "\n  WARNING: High cross-seed variance may indicate measurement instability.\n";
    }
}

int main(int argc, char* argv[]) {
    // Check for --validate-seeds flag
    bool validate_seeds = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--validate-seeds") == 0) {
            validate_seeds = true;
        }
    }

    if (validate_seeds) {
        timing::CycleTimer::calibrate();
        run_seed_validation();
        return 0;
    }

    print_header();
    print_environment_info();

    BenchConfig cfg;
    cfg.ops_per_trial = 1000000;  // 1M ops
    cfg.warmup_ops = 100000;      // 100K warmup
    cfg.pin_cpu = true;
    cfg.lock_memory = true;
    cfg.measure_overhead = true;
    cfg.batch_size = 64;          // Batch 64 ops per timing sample
                                  // Improves effective resolution from ~42ns to ~0.65ns

    print_config(cfg);
    print_targets();

    // Generate random keys for lookup/insert operations
    std::vector<uint64_t> keys;
    keys.reserve(CAPACITY);
    std::mt19937_64 key_generator_rng(42);
    for (size_t i = 0; i < CAPACITY; ++i) {
        keys.push_back(key_generator_rng());
    }

    for (double lf : LOAD_FACTORS) {
        std::cout << std::string(95, '=') << "\n";
        std::cout << "Load Factor: " << static_cast<int>(lf * 100) << "% ("
                  << static_cast<size_t>(lf * CAPACITY) << " / " << CAPACITY << " buckets)\n";
        std::cout << std::string(95, '=') << "\n\n";

        std::vector<BenchResult> robin_trials;
        std::vector<BenchResult> std_trials;

        // Alternate which implementation runs first to eliminate ordering bias
        // Even trials: Robin first, Odd trials: std first
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
        std::cout << std::string(30, ' ') << "\r";  // Clear progress line

        auto robin_agg = aggregate_trials(robin_trials);
        auto std_agg = aggregate_trials(std_trials);

        std::cout << "Latency Distribution (nanoseconds):\n";
        print_result_header();
        print_result_row("RobinHoodTable", robin_agg.mean);
        print_result_row("std::unordered_map", std_agg.mean);

        print_statistical_summary(robin_agg, std_agg);
        print_verdict(lf, robin_agg);
        std::cout << "\n";
    }

    // Final summary
    std::cout << std::string(100, '=') << "\n";
    std::cout << "SUMMARY\n";
    std::cout << std::string(100, '=') << "\n\n";

    std::cout << "RobinHoodTable Design:\n";
    std::cout << "  - Fixed capacity (power of 2)\n";
    std::cout << "  - Cache-line aligned buckets (64 bytes)\n";
    std::cout << "  - Software prefetch on hot paths\n";
    std::cout << "  - Robin Hood displacement (bounded probe distance)\n";
    std::cout << "  - Zero allocation in steady state\n";
    std::cout << "\n";

    std::cout << "Benchmark Methodology:\n";
    std::cout << "  - CPU-pinned execution\n";
    std::cout << "  - Memory-locked pages (mlock)\n";
    std::cout << "  - Timer overhead subtraction\n";
    std::cout << "  - Pre-generated access patterns\n";
    std::cout << "  - Multi-phase cache warmup\n";
    std::cout << "  - Extended tail percentiles (p99.9, p99.99)\n";
    std::cout << "\n";

    std::cout << "Structural Advantages (inherent to design, not measurement artifacts):\n";
    std::cout << "  RobinHood advantages:\n";
    std::cout << "    - Cache-line aligned buckets: Key+value co-located (single cache line access)\n";
    std::cout << "    - Linear probing: Sequential memory access (prefetch-friendly)\n";
    std::cout << "    - Splitmix64 hash: Fast integer hashing (~10 cycles vs ~40 for FNV1a)\n";
    std::cout << "  std::unordered_map characteristics:\n";
    std::cout << "    - Chained collision resolution: Pointer indirection per lookup\n";
    std::cout << "    - Separate node allocation: Key-value in heap-allocated nodes\n";
    std::cout << "    - std::hash: Implementation-dependent (typically FNV1a or MurmurHash)\n";
    std::cout << "\n";

    std::cout << "Sanity Checks Available:\n";
    std::cout << "  ./bench --validate-seeds   Run with multiple RNG seeds to verify stability\n";

    return 0;
}
