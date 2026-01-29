#ifndef CYCLE_TIMER_H
#define CYCLE_TIMER_H

/**
 * @file cycle_timer.h
 * @brief High-precision cycle-accurate timing for HFT latency measurement.
 *
 * Provides platform-specific timestamp acquisition and nanosecond conversion:
 * - x86-64: RDTSC with LFENCE serialization (~0.3ns resolution at 3GHz)
 * - Apple Silicon: mach_absolute_time() (~1ns resolution)
 * - Linux ARM64: CNTVCT_EL0 (~42ns resolution at 24MHz)
 *
 * Calibration: Converts raw ticks to nanoseconds via measured ratio.
 * Thread safety: Calibration is thread-safe (std::call_once).
 * Memory model: Uses acquire/release ordering for calibration values.
 */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#if defined(__APPLE__)
#include <mach/mach_time.h>
#endif

namespace timing {

// DCE prevention sink - prevents dead code elimination of get() calls in benchmarks
inline volatile const void* escape_sink;

/**
 * Platform-specific high-resolution timestamp acquisition.
 *
 * Returns raw tick count from hardware counter.
 * On x86-64: Uses RDTSC with LFENCE serialization barriers
 * On Apple ARM64: Uses mach_absolute_time() with ISB barrier
 * On Linux ARM64: Uses CNTVCT_EL0 system counter with ISB barrier
 */
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
    // ISB ensures instruction stream is synchronized before reading counter
    __asm__ volatile("isb" ::: "memory");
    return mach_absolute_time();
}

#elif defined(__aarch64__) || defined(_M_ARM64)

inline uint64_t read_timestamp_ticks() {
    uint64_t ticks;
    // ISB serializes instruction stream; MRS reads virtual counter
    __asm__ volatile("isb\n\t mrs %0, cntvct_el0" : "=r"(ticks) :: "memory");
    return ticks;
}

#else
#error "Unsupported architecture for cycle-accurate timing"
#endif

/**
 * High-precision cycle timer with automatic calibration.
 *
 * Usage:
 *   CycleTimer::calibrate();  // Call once at startup
 *   CycleTimer timer;
 *   // ... operation to measure ...
 *   double latency_ns = timer.elapsed_ns();
 *
 * Resolution (platform-dependent):
 *   x86-64:       ~0.3ns (RDTSC at 3GHz)
 *   Apple ARM64:  ~1ns (mach_absolute_time)
 *   Linux ARM64:  ~42ns (CNTVCT at 24MHz)
 */
class CycleTimer {
    uint64_t start_ticks_;

    static inline std::once_flag calibration_flag_;
    static inline std::atomic<double> ns_per_tick_{0.0};
    static inline std::atomic<double> timer_resolution_ns_{0.0};

    /**
     * Calibrate tick-to-nanosecond conversion ratio.
     *
     * Apple Silicon: Uses mach_timebase_info for exact conversion.
     * Other platforms: Empirically measures ratio against chrono clock.
     */
    static void calibrate_ticks_to_ns_ratio() {
#if defined(__APPLE__) && defined(__aarch64__)
        // Apple Silicon: Use mach_timebase_info for precise conversion
        mach_timebase_info_data_t timebase_info;
        mach_timebase_info(&timebase_info);

        double ns_per_tick = static_cast<double>(timebase_info.numer) /
                             static_cast<double>(timebase_info.denom);
        ns_per_tick_.store(ns_per_tick, std::memory_order_release);
        timer_resolution_ns_.store(ns_per_tick, std::memory_order_release);

        std::cerr << "Timer calibration (Apple Silicon):\n";
        std::cerr << "  Timebase: " << timebase_info.numer << "/" << timebase_info.denom << "\n";
        std::cerr << "  Resolution: " << ns_per_tick << " ns/tick\n";
#else
        // Other platforms: Calibrate empirically against chrono clock
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
            auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                end_time - start_time).count();

            // Skip warmup samples to allow frequency scaling to stabilize
            if (sample_idx >= WARMUP_SAMPLE_COUNT && elapsed_ticks > 0) {
                ns_per_tick_samples.push_back(
                    static_cast<double>(elapsed_ns) / static_cast<double>(elapsed_ticks));
            }
        }

        double calibrated_ns_per_tick = 1.0;
        if (!ns_per_tick_samples.empty()) {
            std::sort(ns_per_tick_samples.begin(), ns_per_tick_samples.end());
            calibrated_ns_per_tick = ns_per_tick_samples[ns_per_tick_samples.size() / 2];
        }

        ns_per_tick_.store(calibrated_ns_per_tick, std::memory_order_release);
        timer_resolution_ns_.store(calibrated_ns_per_tick, std::memory_order_release);

        std::cerr << "Timer calibration:\n";
        std::cerr << "  Resolution: " << calibrated_ns_per_tick << " ns/tick\n";

        if (calibrated_ns_per_tick < 0.1 || calibrated_ns_per_tick > 50.0) {
            std::cerr << "  WARNING: Calibration suspect (expected 0.2-42 ns/tick)\n";
        }
#endif
    }

public:
    CycleTimer() : start_ticks_(read_timestamp_ticks()) {}

    /**
     * Compute elapsed time in nanoseconds since timer construction.
     */
    [[nodiscard]] double elapsed_ns() const noexcept {
        uint64_t elapsed_ticks = read_timestamp_ticks() - start_ticks_;
        return static_cast<double>(elapsed_ticks) * ns_per_tick_.load(std::memory_order_acquire);
    }

    /**
     * Compute elapsed time in milliseconds since timer construction.
     */
    [[nodiscard]] double elapsed_ms() const noexcept {
        return elapsed_ns() / 1e6;
    }

    /**
     * Get raw elapsed tick count (no conversion to time units).
     */
    [[nodiscard]] uint64_t elapsed_ticks() const noexcept {
        return read_timestamp_ticks() - start_ticks_;
    }

    /**
     * Initialize calibration (call once at program startup).
     * Thread-safe via std::call_once.
     */
    static void calibrate() {
        std::call_once(calibration_flag_, calibrate_ticks_to_ns_ratio);
    }

    /**
     * Get timer resolution in nanoseconds per tick.
     */
    static double resolution_ns() {
        return timer_resolution_ns_.load(std::memory_order_acquire);
    }
};

/**
 * Minimal-overhead inline timer for hot-path measurement.
 *
 * Use when CycleTimer's constructor/destructor overhead is significant.
 * Returns raw ticks; caller must convert to nanoseconds if needed.
 */
struct InlineTimer {
    uint64_t start_ticks;

    inline void begin() noexcept {
        start_ticks = read_timestamp_ticks();
    }

    [[nodiscard]] inline uint64_t end_ticks() const noexcept {
        return read_timestamp_ticks() - start_ticks;
    }
};

} // namespace timing

#endif // CYCLE_TIMER_H
