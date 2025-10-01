/**
 * BaseCase subroutine (Algorithm 2)
 *
 * Base case of BMSSP for layer l=0.
 * Runs mini-Dijkstra from a single complete source vertex until
 * finding k+1 closest vertices with distance < B.
 *
 * Requirements:
 * 1. S = {x} is a singleton, and x is complete
 * 2. For every incomplete vertex v with d_true(v) < B,
 *    the shortest path to v visits x
 *
 * Returns:
 * - b: boundary value (≤ B)
 * - U: set of vertices with d[v] < b
 *
 * Time: O(k·log(k)) using binary heap
 */

#ifndef DUAN_BASE_CASE_HPP
#define DUAN_BASE_CASE_HPP

#include "common.hpp"
#include "labels.hpp"
#include <queue>
#include <unordered_set>

namespace duan{

/**
 * Result of BaseCase operation
 */
struct BaseCaseResult {
    long double b;  // Boundary value
    vector<int> U;  // Set of vertices with d[v] < b
};

/**
 * BaseCase implementation (Algorithm 2)
 *
 * Runs bounded Dijkstra from a single source until finding k+1
 * closest vertices, then returns boundary and vertex set.
 */
class BaseCase {
public:
    /**
     * Execute BaseCase algorithm
     *
     * Requirements:
     * - S must be a singleton {x} where x is complete
     * - For every incomplete vertex v with d_true(v) < B,
     *   the shortest path to v visits x
     *
     * @param graph The graph structure
     * @param labels Current distance labels (will be modified)
     * @param B Upper bound on distances to consider
     * @param S Source vertex set (must be singleton)
     * @param k Parameter for identifying boundary
     * @return BaseCaseResult containing boundary b and vertex set U
     */
    static BaseCaseResult Execute(
        const Graph& graph,
        Labels& labels,
        long double B,
        const vector<int>& S,
        int k);

private:
    /**
     * Priority queue element for Dijkstra's algorithm
     * Ordered by distance (min-heap)
     */
    struct HeapElement {
        int vertex;
        long double dist;
        int hops;  // For tie-breaking

        // Comparator for min-heap (greater means lower priority)
        bool operator>(const HeapElement& other) const {
            if (dist != other.dist) {
                return dist > other.dist;
            }
            // Tie-break by hops
            if (hops != other.hops) {
                return hops > other.hops;
            }
            // Tie-break by vertex ID
            return vertex > other.vertex;
        }
    };
};

}

#endif
