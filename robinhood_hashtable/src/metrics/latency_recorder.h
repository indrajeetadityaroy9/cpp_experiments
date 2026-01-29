#ifndef LATENCY_RECORDER_H
#define LATENCY_RECORDER_H

/**
 * @file latency_recorder.h
 * @brief Fixed-capacity latency sample buffer with HFT-grade percentile computation.
 *
 * Implements a pre-allocated buffer for nanosecond-precision latency samples.
 * Computes extended tail percentiles (p99.9, p99.99) using linear interpolation
 * for accurate tail latency measurement critical in HFT systems.
 *
 * Memory model: All pages pre-faulted at construction to avoid page faults
 * during measurement. No heap allocations during record() calls.
 *
 * Thread safety: NOT thread-safe by design (single-writer benchmark context).
 * Statistical method: Linear interpolation for percentiles, two-pass stddev.
 */

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <vector>

namespace metrics {

/**
 * Compute percentile from sorted data using linear interpolation.
 *
 * Linear interpolation provides more accurate tail percentiles than
 * nearest-rank method, which is critical for p99.9 and p99.99 in HFT.
 *
 * @param sorted_samples_ns Pre-sorted vector of latency samples in nanoseconds
 * @param percentile_fraction Percentile as fraction (e.g., 0.99 for p99)
 * @return Interpolated percentile value in nanoseconds
 */
template<typename T>
[[nodiscard]] inline double compute_percentile_interpolated(
    const std::vector<T>& sorted_samples_ns,
    double percentile_fraction) {

    if (sorted_samples_ns.empty()) return 0.0;
    if (sorted_samples_ns.size() == 1) return static_cast<double>(sorted_samples_ns[0]);

    double index = percentile_fraction * (sorted_samples_ns.size() - 1);
    size_t lower_idx = static_cast<size_t>(index);
    size_t upper_idx = std::min(lower_idx + 1, sorted_samples_ns.size() - 1);
    double interpolation_fraction = index - lower_idx;

    return static_cast<double>(sorted_samples_ns[lower_idx]) * (1.0 - interpolation_fraction) +
           static_cast<double>(sorted_samples_ns[upper_idx]) * interpolation_fraction;
}

/**
 * Research-grade latency statistics for HFT benchmarking.
 *
 * All time values are in nanoseconds (_ns suffix).
 * Extended tail percentiles (p99.9, p99.99) are critical for HFT SLA compliance.
 */
struct LatencyStats {
    // Core percentiles (nanoseconds)
    double p50_ns;    // Median latency
    double p90_ns;    // 90th percentile
    double p95_ns;    // 95th percentile
    double p99_ns;    // 99th percentile - typical SLA threshold
    double p999_ns;   // 99.9th percentile - critical for HFT
    double p9999_ns;  // 99.99th percentile - extreme tail

    // Distribution metrics (nanoseconds)
    double min_ns;
    double max_ns;
    double mean_ns;
    double stddev_ns;

    // Sample metadata
    size_t sample_count;
    size_t outlier_count;
};

/**
 * High-performance latency recorder for HFT benchmarking.
 *
 * Design rationale:
 * - Pre-allocated contiguous storage for cache-friendly sequential access
 * - No heap allocations during record() (zero-allocation hot path)
 * - Page pre-faulting to avoid TLB misses during measurement
 * - IQR-based outlier detection for robust statistics
 * - Extended percentiles (p99.9, p99.99) with linear interpolation
 */
class LatencyRecorder {
    std::vector<uint64_t> latency_samples_ns_;
    size_t sample_count_;
    size_t max_sample_count_;

public:
    /**
     * Construct recorder with fixed capacity.
     *
     * @param max_samples Maximum number of samples to record
     */
    explicit LatencyRecorder(size_t max_samples)
        : latency_samples_ns_(max_samples), sample_count_(0), max_sample_count_(max_samples) {
        // Pre-fault all pages to avoid page faults during measurement
        // Touch one element per page (4KB / 8 bytes = 512 elements)
        for (size_t page_idx = 0; page_idx < max_samples; page_idx += 4096 / sizeof(uint64_t)) {
            latency_samples_ns_[page_idx] = 0;
        }
    }

    /**
     * Record a latency sample in nanoseconds.
     *
     * Caller must ensure sample_count_ < max_sample_count_.
     * Inlined for zero overhead in hot measurement path.
     */
    inline void record(uint64_t latency_ns) noexcept {
        latency_samples_ns_[sample_count_++] = latency_ns;
    }

    /**
     * Reset recorder for reuse without reallocation.
     */
    void reset() noexcept {
        sample_count_ = 0;
    }

    [[nodiscard]] size_t sample_count() const noexcept { return sample_count_; }
    [[nodiscard]] size_t max_sample_count() const noexcept { return max_sample_count_; }

    /**
     * Compute comprehensive latency statistics.
     *
     * @param remove_outliers If true, removes samples outside 1.5*IQR range
     *                        using Tukey's fence method for robust statistics
     * @return LatencyStats with percentiles, distribution metrics, and metadata
     */
    [[nodiscard]] LatencyStats compute_stats(bool remove_outliers = false) const {
        LatencyStats stats{};
        if (sample_count_ == 0) return stats;

        // Copy and sort samples for percentile computation
        std::vector<uint64_t> sorted_samples_ns(
            latency_samples_ns_.begin(),
            latency_samples_ns_.begin() + sample_count_);
        std::sort(sorted_samples_ns.begin(), sorted_samples_ns.end());

        size_t outlier_count = 0;

        if (remove_outliers && sorted_samples_ns.size() > 100) {
            // Tukey's fence method: remove samples outside [Q1 - 1.5*IQR, Q3 + 1.5*IQR]
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

        // Compute percentiles with linear interpolation
        stats.p50_ns = compute_percentile_interpolated(sorted_samples_ns, 0.50);
        stats.p90_ns = compute_percentile_interpolated(sorted_samples_ns, 0.90);
        stats.p95_ns = compute_percentile_interpolated(sorted_samples_ns, 0.95);
        stats.p99_ns = compute_percentile_interpolated(sorted_samples_ns, 0.99);
        stats.p999_ns = compute_percentile_interpolated(sorted_samples_ns, 0.999);
        stats.p9999_ns = compute_percentile_interpolated(sorted_samples_ns, 0.9999);

        // Min/Max from sorted endpoints
        stats.min_ns = static_cast<double>(sorted_samples_ns.front());
        stats.max_ns = static_cast<double>(sorted_samples_ns.back());

        // Mean (first pass)
        double sum_ns = 0.0;
        for (uint64_t sample_ns : sorted_samples_ns) {
            sum_ns += static_cast<double>(sample_ns);
        }
        stats.mean_ns = sum_ns / sorted_samples_ns.size();

        // Standard deviation (second pass for numerical stability)
        // Uses Bessel's correction (N-1) for unbiased sample variance
        double sum_squared_deviation = 0.0;
        for (uint64_t sample_ns : sorted_samples_ns) {
            double deviation_ns = static_cast<double>(sample_ns) - stats.mean_ns;
            sum_squared_deviation += deviation_ns * deviation_ns;
        }
        // Use N-1 for sample stddev; fall back to N=1 edge case (returns 0)
        size_t divisor = sorted_samples_ns.size() > 1 ? sorted_samples_ns.size() - 1 : 1;
        stats.stddev_ns = std::sqrt(sum_squared_deviation / divisor);

        stats.sample_count = sorted_samples_ns.size();
        stats.outlier_count = outlier_count;

        return stats;
    }
};

} // namespace metrics

#endif // LATENCY_RECORDER_H
