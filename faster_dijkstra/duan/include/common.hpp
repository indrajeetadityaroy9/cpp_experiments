/**
 * Common data structures and types for duanSSSP implementation
 */

#ifndef DUAN_COMMON_HPP
#define DUAN_COMMON_HPP

#include <vector>
#include <limits>
#include <cmath>

namespace duan{

using std::vector;

constexpr long double INF = std::numeric_limits<long double>::infinity();

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

}

#endif
