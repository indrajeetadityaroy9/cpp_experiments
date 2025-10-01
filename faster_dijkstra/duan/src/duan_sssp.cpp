/**
 * Implementation of top-level SSSP interface
 */

#include "../include/duan_sssp.hpp"
#include <queue>
#include <algorithm>
#include <cmath>

namespace duan {

// Global statistics
DuanStats g_stats;
bool g_collect_stats = false;

SSSPResult DuanSSSP::ComputeSSSP(
    const Graph& graph,
    int source,
    bool collect_stats) {

    SSSPResult result;
    int n = graph.size();

    // Initialize statistics
    g_collect_stats = collect_stats;
    if (collect_stats) {
        g_stats.reset();
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    // Initialize labels
    Labels labels(n);
    labels.dist[source] = 0.0;
    labels.hops[source] = 0;

    // Compute algorithm parameters
    Params params = ComputeParams(n);
    int initial_layer = ComputeInitialLayer(n, params);

    // Initial source set
    vector<int> S = {source};

    // Upper bound (all reachable vertices)
    long double B = INF;

    // Call BMSSP
    auto bmssp_result = BMSSP::Execute(graph, labels, initial_layer, B, S, params);

    auto end_time = std::chrono::high_resolution_clock::now();

    // Copy results
    result.dist = labels.dist;
    result.pred = labels.pred;

    if (collect_stats) {
        g_stats.total_time = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - start_time);
        result.stats = g_stats;
    }

    return result;
}

Params DuanSSSP::ComputeParams(int n) {
    return Params::compute(n);
}

int DuanSSSP::ComputeInitialLayer(int n, const Params& params) {
    // Initial layer l such that 2^(lt) ≥ |S| = 1
    // We need l such that algorithm can handle single source
    // In paper, top level has |S| ≤ 2^(lt) where l = O(log^(1/3) n)

    // Compute log_2(n)
    if (n <= 1) return 0;

    long double log_n = std::log2((long double)n);

    // Initial layer: l = ⌈log(n) / t⌉
    // This ensures 2^(lt) ≥ n ≥ |S|
    int l = std::max(1, (int)std::ceil(log_n / (long double)params.t));

    return l;
}

// Reference Dijkstra implementation
vector<long double> Dijkstra::ComputeSSSP(const Graph& graph, int source) {
    int n = graph.size();
    vector<long double> dist(n, INF);
    vector<bool> visited(n, false);

    // Priority queue: (distance, vertex)
    using PQElement = std::pair<long double, int>;
    std::priority_queue<PQElement, vector<PQElement>, std::greater<PQElement>> pq;

    dist[source] = 0.0;
    pq.push({0.0, source});

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();

        if (visited[u]) continue;
        visited[u] = true;

        // Relax edges
        for (const Edge& edge : graph[u]) {
            int v = edge.to;
            long double new_dist = dist[u] + edge.weight;

            if (new_dist < dist[v]) {
                dist[v] = new_dist;
                pq.push({new_dist, v});
            }
        }
    }

    return dist;
}

}
