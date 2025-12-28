/**
 * Common data structures and types for Duan SSSP implementation
 *
 * Notation mapping from Duan et al. paper (arXiv:2504.17033):
 * - δ̂[v] / dist[v]: Current distance estimate (upper bound)
 * - Pred[v] / pred[v]: Predecessor in shortest path tree
 * - α[v] / hops[v]: Hop count for lexicographic ordering
 * - S: Source set, B: Distance bound, P: Pivot set, U: Result set
 */

#ifndef DUAN_COMMON_HPP
#define DUAN_COMMON_HPP

#include <vector>
#include <limits>
#include <cmath>
#include <expected>

namespace duan {

using std::vector;

// Distance infinity sentinel
constexpr long double INF = std::numeric_limits<long double>::infinity();

// Floating-point comparison tolerance for distance equality checks
constexpr long double FP_EPSILON = 1e-12L;

// Maximum safe bit shift to prevent integer overflow
constexpr int MAX_SAFE_SHIFT = 30;

// Safe power-of-2 computation with overflow protection
// Returns 2^exponent, or INT_MAX if result would overflow
inline int safe_power_of_2(int exponent) {
    if (exponent < 0 || exponent >= MAX_SAFE_SHIFT) {
        return std::numeric_limits<int>::max();
    }
    return 1 << exponent;
}

// Safe multiplication with overflow protection
// Returns a * b, or INT_MAX if result would overflow
inline int safe_multiply(int a, int b) {
    if (a == 0 || b == 0) return 0;
    if (a > std::numeric_limits<int>::max() / b) {
        return std::numeric_limits<int>::max();
    }
    return a * b;
}

/**
 * Edge structure representing directed weighted edge
 */
struct Edge {
    int to;           // Destination vertex
    long double weight;  // Edge weight

    Edge(int t, long double w) : to(t), weight(w) {}
};

/**
 * Graph representation as adjacency list
 */
using Graph = vector<vector<Edge>>;

/**
 * Algorithm parameters from paper
 * k = ⌊log^(1/3)(n)⌋
 * t = ⌊log^(2/3)(n)⌋
 */
struct Params {
    int k;  // Controls pivot selection and base case size
    int t;  // Controls recursion depth and data structure block size

    /**
     * Compute parameters for given number of vertices
     * As specified in paper Section 3
     */
    static Params compute(int n) {
        Params p;
        long double log_n = std::log2((long double)std::max(2, n));
        p.k = std::max(1, (int)std::floor(std::pow(log_n, 1.0L / 3.0L)));
        p.t = std::max(1, (int)std::floor(std::pow(log_n, 2.0L / 3.0L)));
        return p;
    }
};

/**
 * Error types for algorithm operations
 */
enum class DuanError {
    NonSingletonSourceSet,   // BaseCase requires exactly one source
    SourceOutOfBounds,       // Source vertex index exceeds graph size
    InvalidParameter,        // Generic parameter validation failure
    EmptyGraph               // Graph has no vertices
};

/**
 * Convert error to descriptive string
 */
inline const char* error_message(DuanError err) {
    switch (err) {
        case DuanError::NonSingletonSourceSet:
            return "BaseCase requires singleton source set";
        case DuanError::SourceOutOfBounds:
            return "Source vertex out of bounds";
        case DuanError::InvalidParameter:
            return "Invalid parameter";
        case DuanError::EmptyGraph:
            return "Graph is empty";
    }
    return "Unknown error";
}

}

#endif
