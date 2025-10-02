/**
 * Implementation of BMSSP algorithm (Algorithm 3)
 */

#include "../include/bmssp.hpp"
#include "../include/duan_sssp.hpp"
#include <algorithm>
#include <cassert>

namespace duan{

// Thread-local recursion depth tracking
static thread_local int current_recursion_depth = 0;

BMSSPResult BMSSP::Execute(
    const Graph& graph,
    Labels& labels,
    int l,
    long double B,
    const vector<int>& S,
    const Params& params) {

    // Instrumentation: track recursion depth
    current_recursion_depth++;
    if (g_collect_stats) {
        g_stats.bmssp_calls++;
        g_stats.max_recursion_depth = std::max(
            g_stats.max_recursion_depth,
            (size_t)current_recursion_depth);
    }

    BMSSPResult result;

    // Base case: layer 0
    if (l == 0) {
        auto base_result = BaseCase::Execute(graph, labels, B, S, params.k);
        result.b = base_result.b;
        result.U = base_result.U;
        current_recursion_depth--;
        return result;
    }

    // Recursive case: layer l > 0

    // Step 1: FindPivots to reduce source set
    auto pivots_result = FindPivots::Execute(graph, labels, B, S, params.k);
    vector<int>& P = pivots_result.P;
    vector<int>& W = pivots_result.W;

    // Step 2: Initialize partial order data structure
    int M = (1 << ((l - 1) * params.t));  // M = 2^((l-1)t)
    PartialOrderDS DS;
    DS.Initialize(M, B);

    // Step 3: Insert pivots into DS
    for (int x : P) {
        DS.Insert(x, labels.dist[x]);
        if (g_collect_stats) g_stats.ds_inserts++;
    }

    // Step 4: Initialize iteration variables
    long double b_i = B;  // Current boundary

    // Compute b_0 = min_{x∈P} d[x]
    if (!P.empty()) {
        b_i = INF;
        for (int x : P) {
            b_i = std::min(b_i, labels.dist[x]);
        }
    }

    vector<int> U;  // Accumulated result set

    // Step 5: Main loop - process recursive subproblems
    int k_limit = params.k * (1 << (l * params.t));  // k * 2^(lt)

    while ((int)U.size() < k_limit && !DS.empty()) {

        // Pull next batch from DS
        auto [S_i, B_i] = DS.Pull();
        if (g_collect_stats) g_stats.ds_pulls++;

        // Recursive call to BMSSP at layer l-1
        auto recursive_result = Execute(graph, labels, l - 1, B_i, S_i, params);
        long double b_i_new = recursive_result.b;
        vector<int>& U_i = recursive_result.U;

        // Add U_i to accumulated result
        U.insert(U.end(), U_i.begin(), U_i.end());

        // Relax edges from U_i and classify by distance range
        vector<KeyValuePair> K;
        RelaxAndClassify(graph, labels, U_i, b_i_new, B_i, B, DS, K);

        // Collect vertices from S_i in range [b_i, B_i)
        CollectVerticesInRange(S_i, labels, b_i_new, B_i, K);

        // BatchPrepend collected vertices
        if (!K.empty()) {
            DS.BatchPrepend(K);
            if (g_collect_stats) g_stats.ds_batch_prepends++;
        }

        // Update boundary
        b_i = b_i_new;
    }

    // Step 6: Finalize result
    // b = min{b_i, B}
    result.b = std::min(b_i, B);

    // U = U ∪ {x ∈ W : d[x] < b}
    result.U = U;
    for (int x : W) {
        if (labels.dist[x] < result.b) {
            result.U.push_back(x);
        }
    }

    current_recursion_depth--;
    return result;
}

void BMSSP::RelaxAndClassify(
    const Graph& graph,
    Labels& labels,
    const vector<int>& U_i,
    long double b_i,
    long double B_i,
    long double B,
    PartialOrderDS& DS,
    vector<KeyValuePair>& K) {

    // For each edge (u,v) where u ∈ U_i
    for (int u : U_i) {
        if (u < 0 || u >= (int)graph.size()) continue;

        for (const Edge& edge : graph[u]) {
            int v = edge.to;
            long double new_dist = labels.dist[u] + edge.weight;

            // Relaxation condition (line 209 in paper)
            // Use <= to allow reusing edges from lower levels
            if (new_dist <= labels.dist[v]) {
                // Check if this is an improvement or tie-break
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

                    // Instrumentation: count edge relaxation
                    if (g_collect_stats) g_stats.edge_relaxations++;

                    // Classify by distance range (lines 211-217 in paper)
                    if (new_dist >= B_i && new_dist < B) {
                        // Range [B_i, B): insert into DS
                        DS.Insert(v, new_dist);
                        if (g_collect_stats) g_stats.ds_inserts++;
                    } else if (new_dist >= b_i && new_dist < B_i) {
                        // Range [b_i, B_i): add to K for batch prepend
                        K.push_back({v, new_dist});
                    }
                }
            }
        }
    }
}

void BMSSP::CollectVerticesInRange(
    const vector<int>& S_i,
    const Labels& labels,
    long double b_i,
    long double B_i,
    vector<KeyValuePair>& K) {

    // Add vertices from S_i with distance in range [b_i, B_i)
    for (int x : S_i) {
        if (labels.dist[x] >= b_i && labels.dist[x] < B_i) {
            K.push_back({x, labels.dist[x]});
        }
    }
}

}
