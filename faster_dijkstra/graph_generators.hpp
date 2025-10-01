/**
 * Graph Generators for Complexity Analysis
 * Provides various graph generation functions for testing asymptotic complexity:
 * - Random graphs (connected)
 * - Scale-free graphs (Baraasi-Albert model)
 * - Path graphs
 * - Complete graphs
 */

#ifndef GRAPH_GENERATORS_HPP
#define GRAPH_GENERATORS_HPP

#include <vector>
#include <tuple>
#include <set>
#include <random>
#include <algorithm>

using namespace std;

/**
 * Generate connected random graph
 * Creates approximately m directed edges on n vertices
 * Guarantees connectivity via initial path
 */
inline vector<tuple<int, int, long double>> generate_random_graph(
    int n, int m, mt19937& rng,
    long double min_weight = 1.0L, long double max_weight = 100.0L){
    uniform_int_distribution<int> vertex_dist(0, n - 1);
    uniform_real_distribution<long double> weight_dist(min_weight, max_weight);
    set<pair<int, int>> edge_set;

    // Ensure connectivity with path
    for (int i = 0; i < n - 1; ++i) {
        edge_set.insert({i, i + 1});
    }

    // Add random edges
    int attempts = 0;
    const int max_attempts = m * 10;
    while ((int)edge_set.size() < m && attempts < max_attempts) {
        int u = vertex_dist(rng);
        int v = vertex_dist(rng);
        if (u != v) {
            edge_set.insert({u, v});
        }
        attempts++;
    }

    // Convert to weighted edge list
    vector<tuple<int, int, long double>> edges;
    edges.reserve(edge_set.size());
    for (const auto& [u, v] : edge_set) {
        edges.emplace_back(u, v, weight_dist(rng));
    }

    return edges;
}

/**
 * Generate scale-free graph using Barabási-Albert preferential attachment
 * Power-law degree distribution (realistic for many networks)
 */
inline vector<tuple<int, int, long double>> generate_scale_free_graph(
    int n, int edges_per_node, mt19937& rng,
    long double min_weight = 1.0L, long double max_weight = 100.0L){
    uniform_real_distribution<long double> weight_dist(min_weight, max_weight);
    uniform_real_distribution<double> prob_dist(0.0, 1.0);
    set<pair<int, int>> edge_set;
    vector<int> degree(n, 0);

    // Start with small complete graph
    int initial_size = min(3, n);
    for (int i = 0; i < initial_size; ++i) {
        for (int j = i + 1; j < initial_size; ++j) {
            edge_set.insert({i, j});
            edge_set.insert({j, i});
            degree[i]++;
            degree[j]++;
        }
    }

    // Preferential attachment
    for (int new_node = initial_size; new_node < n; ++new_node) {
        set<int> targets;
        int edges_added = 0;

        while (edges_added < edges_per_node && (int)targets.size() < new_node) {
            // Preferential attachment: prob ∝ degree
            int total_degree = 0;
            for (int i = 0; i < new_node; ++i) {
                total_degree += degree[i] + 1; // +1 to avoid zero
            }

            double r = prob_dist(rng) * total_degree;
            double cumulative = 0;
            int target = -1;

            for (int i = 0; i < new_node; ++i) {
                cumulative += degree[i] + 1;
                if (r <= cumulative) {
                    target = i;
                    break;
                }
            }

            if (target >= 0 && targets.find(target) == targets.end()) {
                targets.insert(target);
                edge_set.insert({new_node, target});
                degree[new_node]++;
                degree[target]++;
                edges_added++;
            }
        }
    }

    // Convert to weighted edge list
    vector<tuple<int, int, long double>> edges;
    edges.reserve(edge_set.size());
    for (const auto& [u, v] : edge_set) {
        edges.emplace_back(u, v, weight_dist(rng));
    }

    return edges;
}

/**
 * Generate path graph: 0 -> 1 -> 2 -> ... -> n-1
 * Worst case for FindPivots (linear structure)
 */
inline vector<tuple<int, int, long double>> generate_path_graph(
    int n, mt19937& rng,
    long double min_weight = 1.0L, long double max_weight = 100.0L){
    uniform_real_distribution<long double> weight_dist(min_weight, max_weight);
    vector<tuple<int, int, long double>> edges;
    edges.reserve(n - 1);

    for (int i = 0; i < n - 1; ++i) {
        edges.emplace_back(i, i + 1, weight_dist(rng));
    }

    return edges;
}

/**
 * Generate complete directed graph (stress test)
 * All n(n-1) possible directed edges
 */
inline vector<tuple<int, int, long double>> generate_complete_graph(
    int n, mt19937& rng,
    long double min_weight = 1.0L, long double max_weight = 100.0L){
    uniform_real_distribution<long double> weight_dist(min_weight, max_weight);
    vector<tuple<int, int, long double>> edges;
    edges.reserve(n * (n - 1));

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (i != j) {
                edges.emplace_back(i, j, weight_dist(rng));
            }
        }
    }

    return edges;
}

/**
 * Generate sparse graph with controlled m/n ratio
 * Ensures m = ratio * n (approximately)
 */
inline vector<tuple<int, int, long double>> generate_sparse_graph(
    int n, double density_ratio, mt19937& rng,
    long double min_weight = 1.0L, long double max_weight = 100.0L)
{
    int m = max(n - 1, (int)(n * density_ratio));
    return generate_random_graph(n, m, rng, min_weight, max_weight);
}

#endif