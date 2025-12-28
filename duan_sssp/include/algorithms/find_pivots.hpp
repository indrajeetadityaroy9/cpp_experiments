/**
 * FindPivots subroutine (Algorithm 1)
 *
 * Given bound B and source set S, performs k-step relaxation to identify
 * pivot vertices that need recursive processing.
 *
 * Algorithm:
 * 1. Relax edges from S for k steps, tracking visited vertices W
 * 2. If |W| > k|S|, return early with P = S (too many vertices)
 * 3. Otherwise, build predecessor forest F and identify pivots:
 *    - P contains roots of trees with >=k vertices
 *
 * Returns:
 * - P: pivot set (subset of S), size <= |W|/k
 * - W: working set, size O(k|S|)
 *
 * Time: O(min{k²|S|, k|W|})
 */

#ifndef DUAN_FIND_PIVOTS_HPP
#define DUAN_FIND_PIVOTS_HPP

#include "../common.hpp"
#include "../labels.hpp"
#include <unordered_set>
#include <unordered_map>
#include <queue>

namespace duan {

/**
 * Result of FindPivots operation
 */
struct FindPivotsResult {
    vector<int> P;  // Pivot set (subset of S)
    vector<int> W;  // Working set (vertices visited during k-step relaxation)
};

/**
 * FindPivots implementation (Algorithm 1)
 *
 * Performs k-step bounded relaxation from source set S to identify
 * pivot vertices for recursive BMSSP calls.
 */
class FindPivots {
public:
    /**
     * Execute FindPivots algorithm
     *
     * Requirements:
     * - For every incomplete vertex v with d_true(v) < B,
     *   the shortest path to v visits some complete vertex in S
     *
     * @param graph The graph structure
     * @param labels Current distance labels (will be modified)
     * @param B Upper bound on distances to consider
     * @param S Source vertex set
     * @param k Number of relaxation steps (from Params)
     * @return FindPivotsResult containing pivot set P and working set W
     */
    static FindPivotsResult Execute(
        const Graph& graph,
        Labels& labels,
        long double B,
        const vector<int>& S,
        int k);

private:
    /**
     * Perform one step of edge relaxation from a layer
     *
     * @param graph The graph structure
     * @param labels Current distance labels (will be modified)
     * @param B Upper bound on distances
     * @param W_prev Previous layer vertices
     * @param W_next Output: next layer vertices (vertices updated with d < B)
     */
    static void RelaxLayer(
        const Graph& graph,
        Labels& labels,
        long double B,
        const vector<int>& W_prev,
        std::unordered_set<int>& W_next);

    /**
     * Build predecessor forest F from vertices in W
     *
     * F = {(u,v) ∈ E : u,v ∈ W, d[v] = d[u] + w(u,v)}
     *
     * Returns adjacency list representation of forest
     * (maps each vertex to its children in the forest)
     */
    static std::unordered_map<int, vector<int>> BuildForest(
        const Graph& graph,
        const Labels& labels,
        const std::unordered_set<int>& W_set);

    /**
     * Compute subtree sizes for all vertices in forest
     *
     * @param forest Adjacency list of forest (parent -> children)
     * @param roots Root vertices (vertices in S)
     * @return Map from vertex -> subtree size
     */
    static std::unordered_map<int, int> ComputeSubtreeSizes(
        const std::unordered_map<int, vector<int>>& forest,
        const vector<int>& roots);

    /**
     * Identify pivot vertices (roots with subtree size >= k)
     *
     * @param S Source set
     * @param subtree_sizes Map from vertex -> subtree size
     * @param k Threshold for pivot identification
     * @return Vector of pivot vertices
     */
    static vector<int> IdentifyPivots(
        const vector<int>& S,
        const std::unordered_map<int, int>& subtree_sizes,
        int k);
};

}

#endif
