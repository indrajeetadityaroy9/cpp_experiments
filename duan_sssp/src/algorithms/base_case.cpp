/**
 * Implementation of BaseCase algorithm (Algorithm 2)
 */

#include "../../include/algorithms/base_case.hpp"
#include <algorithm>

namespace duan {

std::expected<BaseCaseResult, DuanError> BaseCase::Execute(
    const Graph& graph,
    Labels& labels,
    long double B,
    const vector<int>& S,
    int k) {

    BaseCaseResult result;

    // Requirement: S must be a singleton {x}
    if (S.size() != 1) {
        return std::unexpected(DuanError::NonSingletonSourceSet);
    }
    int x = S[0];

    // Validate source vertex is within graph bounds
    if (x < 0 || x >= static_cast<int>(graph.size())) {
        return std::unexpected(DuanError::SourceOutOfBounds);
    }

    // Initialize U_0 with source
    std::unordered_set<int> U_0_set;
    vector<int> U_0;
    U_0.push_back(x);
    U_0_set.insert(x);

    // Initialize binary min-heap with source ⟨x, d[x]⟩
    std::priority_queue<HeapElement, vector<HeapElement>, std::greater<HeapElement>> heap;
    heap.push({x, labels.dist[x], labels.hops[x]});

    // Track which vertices are in heap to avoid duplicates
    std::unordered_set<int> in_heap;
    in_heap.insert(x);

    // Run bounded Dijkstra until finding k+1 closest vertices
    // or heap becomes empty
    while (!heap.empty() && (int)U_0.size() < k + 1) {
        // Extract minimum element
        HeapElement min_elem = heap.top();
        heap.pop();
        int u = min_elem.vertex;

        // Skip if already processed (can happen with DecreaseKey simulation)
        if (U_0_set.count(u) > 0 && u != x) {
            continue;
        }

        // Skip if distance changed since insertion (stale entry)
        if (min_elem.dist > labels.dist[u]) {
            continue;
        }

        // Add u to U_0 (unless it's the source, already added)
        if (u != x) {
            U_0.push_back(u);
            U_0_set.insert(u);
        }

        // Remove from in_heap tracking
        in_heap.erase(u);

        // Relax edges from u
        if (u < 0 || u >= (int)graph.size()) continue;

        for (const Edge& edge : graph[u]) {
            int v = edge.to;
            long double new_dist = labels.dist[u] + edge.weight;

            // Only relax if within bound B
            if (new_dist >= B) continue;

            // Try to relax edge (u -> v)
            if (try_relax(labels, u, v, new_dist)) {
                // Update heap: insert or simulate DecreaseKey
                if (in_heap.count(v) == 0) {
                    heap.push({v, labels.dist[v], labels.hops[v]});
                    in_heap.insert(v);
                } else {
                    // Already in heap: insert again (stale entries filtered on extraction)
                    heap.push({v, labels.dist[v], labels.hops[v]});
                }
            }
        }
    }

    // Determine boundary and result set
    if ((int)U_0.size() <= k) {
        // Case 1: Found <= k vertices, return all with boundary B
        result.b = B;
        result.U = U_0;
    } else {
        // Case 2: Found k+1 vertices
        // b = max_{v ∈ U_0} d[v]
        // U = {v ∈ U_0 : d[v] < b}

        // Find maximum distance in U_0
        long double max_dist = -INF;
        for (int v : U_0) {
            max_dist = std::max(max_dist, labels.dist[v]);
        }

        result.b = max_dist;

        // Collect vertices with d[v] < b
        for (int v : U_0) {
            if (labels.dist[v] < result.b) {
                result.U.push_back(v);
            }
        }
    }

    return result;
}

}
