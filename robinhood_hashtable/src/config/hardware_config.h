#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

/**
 * @file hardware_config.h
 * @brief Compile-time and runtime hardware capability detection.
 *
 * Provides platform-specific constants and detection functions for:
 * - Architecture identification (x86-64, ARM64, Apple Silicon)
 * - Cache line size detection (compile-time assumed, runtime verified)
 * - SIMD capability flags (AVX2, AVX512, NEON)
 *
 * Design: Zero runtime cost for compile-time constants. Runtime detection
 * functions are called once during initialization and cached if needed.
 *
 * Cache line sizes:
 * - Apple Silicon M1/M2/M3/M4: 128 bytes (performance cores)
 * - x86-64 (Intel/AMD): 64 bytes
 * - ARM64 (non-Apple): 64 bytes
 */

#include <cstddef>
#include <cstdint>

#if defined(__APPLE__)
#include <sys/sysctl.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

namespace hw {

// ============================================================================
// Compile-time Architecture Detection
// ============================================================================

#if defined(__x86_64__) || defined(_M_X64)
inline constexpr bool IS_X86_64 = true;
inline constexpr bool IS_ARM64 = false;
#elif defined(__aarch64__) || defined(_M_ARM64)
inline constexpr bool IS_X86_64 = false;
inline constexpr bool IS_ARM64 = true;
#else
inline constexpr bool IS_X86_64 = false;
inline constexpr bool IS_ARM64 = false;
#endif

#if defined(__APPLE__)
inline constexpr bool IS_MACOS = true;
inline constexpr bool IS_LINUX = false;
#elif defined(__linux__)
inline constexpr bool IS_MACOS = false;
inline constexpr bool IS_LINUX = true;
#else
inline constexpr bool IS_MACOS = false;
inline constexpr bool IS_LINUX = false;
#endif

// ============================================================================
// Cache Line Size Detection
// ============================================================================

// Default for most modern x86 CPUs
inline constexpr size_t DEFAULT_CACHE_LINE_SIZE = 64;

// Apple Silicon (M1/M2/M3) has 128-byte cache lines for performance cores
// x86 and most ARM have 64-byte cache lines
#if defined(__APPLE__) && defined(__aarch64__)
inline constexpr size_t ASSUMED_CACHE_LINE_SIZE = 128;
#else
inline constexpr size_t ASSUMED_CACHE_LINE_SIZE = 64;
#endif

/**
 * Runtime cache line size detection.
 * Falls back to compile-time default on unsupported platforms.
 */
inline size_t detect_cache_line_size() {
#if defined(__APPLE__)
    size_t line_size = 0;
    size_t size = sizeof(line_size);
    if (sysctlbyname("hw.cachelinesize", &line_size, &size, nullptr, 0) == 0) {
        return line_size;
    }
    return ASSUMED_CACHE_LINE_SIZE;
#elif defined(__linux__)
    long line_size = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
    if (line_size > 0) {
        return static_cast<size_t>(line_size);
    }
    return DEFAULT_CACHE_LINE_SIZE;
#else
    return DEFAULT_CACHE_LINE_SIZE;
#endif
}

// ============================================================================
// SIMD Capability Detection (Compile-time)
// ============================================================================

#if defined(__AVX2__)
inline constexpr bool HAS_AVX2 = true;
#else
inline constexpr bool HAS_AVX2 = false;
#endif

#if defined(__AVX512F__)
inline constexpr bool HAS_AVX512 = true;
#else
inline constexpr bool HAS_AVX512 = false;
#endif

#if defined(__ARM_NEON) || defined(__aarch64__)
inline constexpr bool HAS_NEON = true;
#else
inline constexpr bool HAS_NEON = false;
#endif

// ============================================================================
// CPU Feature String (for diagnostic output)
// ============================================================================

inline const char* platform_name() {
#if defined(__APPLE__) && defined(__aarch64__)
    return "macOS (Apple Silicon)";
#elif defined(__APPLE__) && defined(__x86_64__)
    return "macOS (Intel)";
#elif defined(__linux__) && defined(__x86_64__)
    return "Linux (x86-64)";
#elif defined(__linux__) && defined(__aarch64__)
    return "Linux (ARM64)";
#else
    return "Unknown";
#endif
}

inline const char* simd_features() {
#if defined(__AVX512F__)
    return "AVX-512";
#elif defined(__AVX2__)
    return "AVX2";
#elif defined(__ARM_NEON) || defined(__aarch64__)
    return "NEON";
#else
    return "None";
#endif
}

} // namespace hw

#endif // HARDWARE_CONFIG_H
