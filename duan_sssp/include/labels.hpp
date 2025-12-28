/**
 * Distance labels and predecessor tracking for Duan SSSP
 *
 * Maintains:
 * - δ̂[v]: Current distance estimate (upper bound on true distance)
 * - Pred[v]: Predecessor in shortest path tree
 * - α[v]: Hop count for lexicographic tie-breaking
 */

#ifndef DUAN_LABELS_HPP
#define DUAN_LABELS_HPP

#include "common.hpp"
#include <climits>

namespace duan {

/**
 * Labels structure maintaining shortest path information
 *
 * Invariant: dist[v] >= d(v) for all vertices v
 * where d(v) is the true shortest path distance
 */
class Labels {
public:
    vector<long double> dist;  // δ̂[v] = current distance estimate
    vector<int> pred;          // Pred[v] = predecessor in shortest path tree
    vector<int> hops;          // α[v] = hop count for lexicographic ordering

    /**
     * Initialize labels for n vertices
     * All distances set to INF, predecessors to -1, hops to INT_MAX
     */
    explicit Labels(int n) {
        reset(n);
    }

    /**
     * Reset labels to initial state
     */
    void reset(int n) {
        dist.assign(n, INF);
        pred.assign(n, -1);
        hops.assign(n, INT_MAX / 2);  // Prevent overflow in hop+1 operations
    }

    /**
     * Get number of vertices
     */
    std::size_t size() const {
        return dist.size();
    }

    /**
     * Check if vertex v is complete
     * A vertex is complete when dist[v] = d(v) (true distance)
     * This is tracked externally during algorithm execution
     */
    bool is_finite(int v) const {
        return dist[v] < INF;
    }
};

/**
 * Lexicographic comparison for tie-breaking
 *
 * Paths are ordered as tuples ⟨length, hop_count, v_α, v_{α-1}, ..., v_1⟩
 *
 * Returns true if new path (through u with new_hops) is better than
 * current path (with old_pred and old_hops)
 *
 * Tie-breaking rule:
 * 1. Prefer fewer hops
 * 2. If hops equal, prefer smaller predecessor vertex ID
 *
 * Time: O(1)
 */
inline bool lex_better(int u, int old_pred, int new_hops, int old_hops) {
    if (new_hops != old_hops) {
        return new_hops < old_hops;
    }
    // Tie on hop count: prefer smaller vertex ID
    return u < old_pred;
}

/**
 * Try to relax edge (u -> v) with a new distance
 *
 * Updates labels if new_dist provides a shorter path or wins lexicographic
 * tie-breaking (fewer hops, or same hops but smaller predecessor ID).
 *
 * Returns true if labels were updated, false otherwise.
 *
 * Time: O(1)
 */
inline bool try_relax(Labels& labels, int u, int v, long double new_dist) {
    if (new_dist > labels.dist[v]) {
        return false;
    }

    bool should_update = false;
    if (new_dist < labels.dist[v]) {
        // Strict distance improvement
        should_update = true;
    } else {
        // Equal distance: use lexicographic tie-breaking
        int new_hops = labels.hops[u] + 1;
        should_update = lex_better(u, labels.pred[v], new_hops, labels.hops[v]);
    }

    if (should_update) {
        labels.dist[v] = new_dist;
        labels.pred[v] = u;
        labels.hops[v] = labels.hops[u] + 1;
        return true;
    }
    return false;
}

}

#endif
