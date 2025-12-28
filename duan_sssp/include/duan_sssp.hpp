/**
 * Top-level SSSP interface for Duan algorithm
 *
 * Simplified implementation without degree reduction.
 * Computes single-source shortest paths from source s to all vertices.
 *
 * Note: This implementation works on any graph but achieves optimal
 * O(m·log^(2/3)(n)) complexity only on constant-degree graphs.
 */

#ifndef DUAN_DUAN_SSSP_HPP
#define DUAN_DUAN_SSSP_HPP

#include "common.hpp"
#include "labels.hpp"
#include "algorithms/bmssp.hpp"
#include <chrono>
#include <iostream>
#include <unordered_set>

namespace duan {

/**
 * Statistics for complexity analysis
 */
struct DuanStats {
    // Operation counts
    size_t edge_relaxations = 0;
    size_t ds_inserts = 0;
    size_t ds_batch_prepends = 0;
    size_t ds_pulls = 0;
    size_t bmssp_calls = 0;
    size_t max_recursion_depth = 0;

    // Timing
    std::chrono::microseconds total_time{0};

    void reset() {
        edge_relaxations = 0;
        ds_inserts = 0;
        ds_batch_prepends = 0;
        ds_pulls = 0;
        bmssp_calls = 0;
        max_recursion_depth = 0;
        total_time = std::chrono::microseconds{0};
    }

    void print() const {
        std::cout << "=== Duan SSSP Statistics ===\n";
        std::cout << "Edge relaxations:     " << edge_relaxations << "\n";
        std::cout << "DS Inserts:           " << ds_inserts << "\n";
        std::cout << "DS BatchPrepends:     " << ds_batch_prepends << "\n";
        std::cout << "DS Pulls:             " << ds_pulls << "\n";
        std::cout << "BMSSP calls:          " << bmssp_calls << "\n";
        std::cout << "Max recursion depth:  " << max_recursion_depth << "\n";
        std::cout << "Total time:           " << total_time.count() << " μs\n";
    }
};

// Global statistics (for instrumentation)
// Thread-safety: g_stats and g_collect_stats are NOT thread-safe.
// Do not use collect_stats=true in multi-threaded contexts.
// Each thread should use its own DuanSSSP instance with collect_stats=false.
extern DuanStats g_stats;
extern bool g_collect_stats;

/**
 * Result of SSSP computation
 */
struct DuanSSSPResult {
    vector<long double> dist;  // dist[v] = shortest distance from source to v
    vector<int> pred;          // pred[v] = predecessor of v in shortest path tree
    DuanStats stats;           // Performance statistics (valid if collect_stats=true)
};

/**
 * DuanSSSP - Top-level interface for Duan's algorithm
 */
class DuanSSSP {
public:
    /**
     * Compute single-source shortest paths
     *
     * @param graph The input graph
     * @param source Source vertex
     * @param collect_stats Whether to collect detailed statistics
     * @return DuanSSSPResult containing distances and predecessors
     */
    static DuanSSSPResult ComputeSSSP(
        const Graph& graph,
        int source,
        bool collect_stats = false);

private:
    /**
     * Compute algorithm parameters based on graph size
     */
    static Params ComputeParams(int n);

    /**
     * Compute initial recursion layer based on graph size
     */
    static int ComputeInitialLayer(int n, const Params& params);
};

/**
 * Reference Dijkstra implementation for validation
 */
class Dijkstra {
public:
    /**
     * Compute single-source shortest paths using Dijkstra's algorithm
     *
     * @param graph The input graph
     * @param source Source vertex
     * @return Vector of shortest distances
     */
    static vector<long double> ComputeSSSP(const Graph& graph, int source);
};

}

#endif
