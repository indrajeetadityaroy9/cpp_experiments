/**
 * Bounded Multi-Source Shortest Path (Algorithm 3)
 *
 * Main recursive algorithm for computing shortest paths from a bounded
 * source set S to all reachable vertices with distance < B.
 *
 * Requirements:
 * 1. |S| <= 2^(lt) where l is the layer parameter
 * 2. For every incomplete vertex x with d_true(x) < B,
 *    the shortest path to x visits some complete vertex y ∈ S
 *
 * Returns:
 * - b: boundary value (<= B)
 * - U: set of complete vertices with d[v] < b
 *
 * Time: O(m·log^(2/3)(n)) total across all recursive calls
 */

#ifndef DUAN_BMSSP_HPP
#define DUAN_BMSSP_HPP

#include "common.hpp"
#include "labels.hpp"
#include "partial_order_ds.hpp"
#include "find_pivots.hpp"
#include "base_case.hpp"

namespace duan{

/**
 * Result of BMSSP operation
 */
struct BMSSPResult {
    long double b;  // Boundary value
    vector<int> U;  // Set of complete vertices with d[v] < b
};

/**
 * BMSSP implementation (Algorithm 3)
 *
 * Recursive algorithm for bounded multi-source shortest path.
 * Uses FindPivots to reduce source set, BaseCase for layer 0,
 * and PartialOrderDS to manage recursive subproblems.
 */
class BMSSP {
public:
    /**
     * Execute BMSSP algorithm
     *
     * Requirements:
     * - |S| <= 2^(lt) where l is the layer parameter
     * - For every incomplete vertex x with d_true(x) < B,
     *   the shortest path to x visits some complete vertex y ∈ S
     *
     * @param graph The graph structure
     * @param labels Current distance labels (will be modified)
     * @param l Layer parameter (recursion depth)
     * @param B Upper bound on distances to consider
     * @param S Source vertex set
     * @param params Algorithm parameters (k, t)
     * @return BMSSPResult containing boundary b and vertex set U
     */
    static BMSSPResult Execute(
        const Graph& graph,
        Labels& labels,
        int l,
        long double B,
        const vector<int>& S,
        const Params& params);

private:
    /**
     * Relax edges from a set of vertices
     *
     * For each edge (u,v) where u ∈ U_i:
     * - If d[u] + w(u,v) <= d[v], update d[v]
     * - Classify relaxed vertices by distance range:
     *   - [B_i, B): insert into DS
     *   - [b_i, B_i): add to K for batch prepend
     *
     * @param graph The graph structure
     * @param labels Current distance labels (will be modified)
     * @param U_i Vertices to relax from
     * @param b_i Lower boundary
     * @param B_i Upper boundary (separator from Pull)
     * @param B Global upper bound
     * @param DS Partial order data structure (will be modified)
     * @param K Output: vertices in range [b_i, B_i) for batch prepend
     */
    static void RelaxAndClassify(
        const Graph& graph,
        Labels& labels,
        const vector<int>& U_i,
        long double b_i,
        long double B_i,
        long double B,
        PartialOrderDS& DS,
        vector<KeyValuePair>& K);

    /**
     * Add vertices from S_i that fall in range [b_i, B_i)
     *
     * @param S_i Vertex set from Pull
     * @param labels Current distance labels
     * @param b_i Lower boundary
     * @param B_i Upper boundary
     * @param K Output: vertices to add for batch prepend
     */
    static void CollectVerticesInRange(
        const vector<int>& S_i,
        const Labels& labels,
        long double b_i,
        long double B_i,
        vector<KeyValuePair>& K);
};

}

#endif
