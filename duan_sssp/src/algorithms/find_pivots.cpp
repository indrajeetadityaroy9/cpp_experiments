/**
 * Implementation of FindPivots algorithm (Algorithm 1)
 */

#include "../../include/algorithms/find_pivots.hpp"
#include <algorithm>
#include <stack>

namespace duan {

FindPivotsResult FindPivots::Execute(
    const Graph& graph,
    Labels& labels,
    long double B,
    const vector<int>& S,
    int k) {

    FindPivotsResult result;

    // Handle empty source set
    if (S.empty()) {
        return result;
    }

    // Initialize working set W with source set S
    std::unordered_set<int> W_set(S.begin(), S.end());
    vector<int> W_prev = S;  // W_0 = S

    // Perform k-step relaxation
    for (int i = 1; i <= k; ++i) {
        std::unordered_set<int> W_next_set;

        // Relax edges from W_{i-1} to build W_i
        RelaxLayer(graph, labels, B, W_prev, W_next_set);

        // Add W_i to W
        for (int v : W_next_set) {
            W_set.insert(v);
        }

        // Early exit: if |W| > k|S|, return P = S
        // Note: W_set.size() is already updated with W_i
        if ((int)W_set.size() > k * (int)S.size()) {
            result.P = S;
            result.W.assign(W_set.begin(), W_set.end());
            return result;
        }

        // Convert W_next_set to vector for next iteration
        W_prev.assign(W_next_set.begin(), W_next_set.end());

        // If no vertices were added, we can stop early
        if (W_prev.empty()) {
            break;
        }
    }

    // No early exit: build forest and identify pivots

    // Convert W_set to vector for output
    result.W.assign(W_set.begin(), W_set.end());

    // Build predecessor forest F
    auto forest = BuildForest(graph, labels, W_set);

    // Compute subtree sizes
    auto subtree_sizes = ComputeSubtreeSizes(forest, S);

    // Identify pivots (roots with subtree size >= k)
    result.P = IdentifyPivots(S, subtree_sizes, k);

    return result;
}

void FindPivots::RelaxLayer(
    const Graph& graph,
    Labels& labels,
    long double B,
    const vector<int>& W_prev,
    std::unordered_set<int>& W_next) {

    // For all edges (u,v) with u ∈ W_prev
    for (int u : W_prev) {
        if (u < 0 || u >= (int)graph.size()) continue;

        for (const Edge& edge : graph[u]) {
            int v = edge.to;
            long double new_dist = labels.dist[u] + edge.weight;

            // Try to relax edge (u -> v)
            // Note: try_relax uses <= for distance comparison (see paper line 61)
            if (try_relax(labels, u, v, new_dist)) {
                // Add to W_next if distance is within bound B
                if (new_dist < B) {
                    W_next.insert(v);
                }
            }
        }
    }
}

std::unordered_map<int, vector<int>> FindPivots::BuildForest(
    const Graph& graph,
    const Labels& labels,
    const std::unordered_set<int>& W_set) {

    std::unordered_map<int, vector<int>> forest;

    // For each vertex u in W
    for (int u : W_set) {
        if (u < 0 || u >= (int)graph.size()) continue;

        // Check all edges (u,v)
        for (const Edge& edge : graph[u]) {
            int v = edge.to;

            // Include edge in forest if:
            // 1. Both u and v are in W
            // 2. d[v] = d[u] + w(u,v) (v's current shortest path goes through u)
            if (W_set.count(v) > 0) {
                long double expected_dist = labels.dist[u] + edge.weight;

                if (std::abs(labels.dist[v] - expected_dist) < FP_EPSILON &&
                    labels.pred[v] == u) {
                    // Edge (u,v) is in forest: u is parent of v
                    forest[u].push_back(v);
                }
            }
        }
    }

    return forest;
}

std::unordered_map<int, int> FindPivots::ComputeSubtreeSizes(
    const std::unordered_map<int, vector<int>>& forest,
    const vector<int>& roots) {

    std::unordered_map<int, int> subtree_sizes;

    // DFS from each root to compute subtree sizes
    for (int root : roots) {
        // Use iterative DFS with stack
        std::stack<std::pair<int, bool>> stack;  // (vertex, children_processed)
        stack.push({root, false});

        while (!stack.empty()) {
            auto [v, processed] = stack.top();
            stack.pop();

            if (processed) {
                // Children have been processed, now compute size for v
                int size = 1;  // Count v itself

                auto it = forest.find(v);
                if (it != forest.end()) {
                    for (int child : it->second) {
                        size += subtree_sizes[child];
                    }
                }

                subtree_sizes[v] = size;
            } else {
                // Process children first
                stack.push({v, true});

                auto it = forest.find(v);
                if (it != forest.end()) {
                    for (int child : it->second) {
                        stack.push({child, false});
                    }
                }
            }
        }
    }

    return subtree_sizes;
}

vector<int> FindPivots::IdentifyPivots(
    const vector<int>& S,
    const std::unordered_map<int, int>& subtree_sizes,
    int k) {

    vector<int> pivots;

    for (int u : S) {
        // Check if u is a root with subtree size >= k
        auto it = subtree_sizes.find(u);
        if (it != subtree_sizes.end() && it->second >= k) {
            pivots.push_back(u);
        }
    }

    return pivots;
}

}
