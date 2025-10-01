/**
 * Implementation of BaseCase algorithm (Algorithm 2)
 */

#include "../include/base_case.hpp"
#include <cassert>
#include <algorithm>

namespace duan{

BaseCaseResult BaseCase::Execute(
    const Graph& graph,
    Labels& labels,
    long double B,
    const vector<int>& S,
    int k) {

    BaseCaseResult result;

    // Requirement: S must be a singleton {x}
    assert(S.size() == 1);
    int x = S[0];

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

            // Check relaxation condition (line 148 in paper)
            // Use ≤ for relaxation (allows reusing edges)
            if (new_dist <= labels.dist[v] && new_dist < B) {
                // Check if this is an improvement (distance or tie-break)
                bool update = false;

                if (new_dist < labels.dist[v]) {
                    // Strict improvement
                    update = true;
                } else {
                    // Equal distance: use lexicographic tie-breaking
                    int new_hops = labels.hops[u] + 1;
                    if (lex_better(u, labels.pred[v], new_hops, labels.hops[v])) {
                        update = true;
                    }
                }

                if (update) {
                    labels.dist[v] = new_dist;
                    labels.pred[v] = u;
                    labels.hops[v] = labels.hops[u] + 1;

                    // Insert or DecreaseKey in heap
                    if (in_heap.count(v) == 0) {
                        // Not in heap: insert
                        heap.push({v, labels.dist[v], labels.hops[v]});
                        in_heap.insert(v);
                    } else {
                        // Already in heap: simulate DecreaseKey by inserting again
                        // (stale entries will be filtered out during extraction)
                        heap.push({v, labels.dist[v], labels.hops[v]});
                    }
                }
            }
        }
    }

    // Determine boundary and result set
    if ((int)U_0.size() <= k) {
        // Case 1: Found ≤ k vertices, return all with boundary B
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
