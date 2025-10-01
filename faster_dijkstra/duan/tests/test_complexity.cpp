/**
 * Complexity analysis and empirical validation
 *
 * Tests Duan SSSP on various graph sizes and structures to:
 * 1. Validate correctness against Dijkstra
 * 2. Measure empirical complexity
 * 3. Compare against theoretical O(m·log^(2/3)(n)) bound
 */

#include "../include/duan_sssp.hpp"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <fstream>
#include <random>
#include <cassert>

using namespace duan;

// Test result tracking
int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) \
    std::cout << "Running test: " << #name << "... "; \
    try { \
        name(); \
        std::cout << "PASSED\n"; \
        tests_passed++; \
    } catch (const std::exception& e) { \
        std::cout << "FAILED: " << e.what() << "\n"; \
        tests_failed++; \
    }

// Graph generators

/**
 * Generate path graph: 0 -> 1 -> 2 -> ... -> (n-1)
 */
Graph generate_path_graph(int n) {
    Graph g(n);
    for (int i = 0; i < n - 1; ++i) {
        g[i].push_back({i + 1, 1.0});
    }
    return g;
}

/**
 * Generate complete graph with random weights
 */
Graph generate_complete_graph(int n, std::mt19937& rng) {
    Graph g(n);
    std::uniform_real_distribution<long double> dist(1.0, 10.0);

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (i != j) {
                g[i].push_back({j, dist(rng)});
            }
        }
    }
    return g;
}

/**
 * Generate random sparse graph (constant out-degree)
 */
Graph generate_sparse_graph(int n, int out_degree, std::mt19937& rng) {
    Graph g(n);
    std::uniform_int_distribution<int> vertex_dist(0, n - 1);
    std::uniform_real_distribution<long double> weight_dist(1.0, 10.0);

    for (int i = 0; i < n; ++i) {
        std::unordered_set<int> neighbors;
        while ((int)neighbors.size() < std::min(out_degree, n - 1)) {
            int j = vertex_dist(rng);
            if (j != i) {
                neighbors.insert(j);
            }
        }

        for (int j : neighbors) {
            g[i].push_back({j, weight_dist(rng)});
        }
    }
    return g;
}

/**
 * Generate grid graph (2D lattice)
 */
Graph generate_grid_graph(int rows, int cols) {
    int n = rows * cols;
    Graph g(n);

    auto idx = [&](int r, int c) { return r * cols + c; };

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int u = idx(r, c);

            // Right
            if (c + 1 < cols) {
                g[u].push_back({idx(r, c + 1), 1.0});
            }

            // Down
            if (r + 1 < rows) {
                g[u].push_back({idx(r + 1, c), 1.0});
            }

            // Left
            if (c - 1 >= 0) {
                g[u].push_back({idx(r, c - 1), 1.0});
            }

            // Up
            if (r - 1 >= 0) {
                g[u].push_back({idx(r - 1, c), 1.0});
            }
        }
    }

    return g;
}

// Helper to check correctness
bool approx_equal(long double a, long double b, long double eps = 1e-9) {
    if (std::isinf(a) && std::isinf(b)) return true;
    return std::abs(a - b) < eps;
}

// Test 1: Correctness on path graph
void test_correctness_path() {
    int n = 20;
    Graph g = generate_path_graph(n);
    int source = 0;

    auto duan_result = DuanSSSP::ComputeSSSP(g, source, false);
    auto dijkstra_result = Dijkstra::ComputeSSSP(g, source);

    // Compare results
    for (int i = 0; i < n; ++i) {
        assert(approx_equal(duan_result.dist[i], dijkstra_result[i]));
    }
}

// Test 2: Correctness on grid graph
void test_correctness_grid() {
    Graph g = generate_grid_graph(5, 5);
    int source = 0;

    auto duan_result = DuanSSSP::ComputeSSSP(g, source, false);
    auto dijkstra_result = Dijkstra::ComputeSSSP(g, source);

    // Compare results
    for (int i = 0; i < (int)g.size(); ++i) {
        assert(approx_equal(duan_result.dist[i], dijkstra_result[i]));
    }
}

// Test 3: Correctness on sparse random graph
void test_correctness_sparse() {
    std::mt19937 rng(42);
    int n = 30;
    int out_degree = 3;

    Graph g = generate_sparse_graph(n, out_degree, rng);
    int source = 0;

    auto duan_result = DuanSSSP::ComputeSSSP(g, source, false);
    auto dijkstra_result = Dijkstra::ComputeSSSP(g, source);

    // Compare results
    for (int i = 0; i < n; ++i) {
        assert(approx_equal(duan_result.dist[i], dijkstra_result[i]));
    }
}

// Test 4: Complexity scaling on path graphs
void test_complexity_path() {
    std::cout << "\n=== Complexity Analysis: Path Graphs ===\n";
    std::cout << std::setw(10) << "n"
              << std::setw(15) << "m"
              << std::setw(15) << "Relaxations"
              << std::setw(15) << "Time (μs)"
              << std::setw(15) << "Ratio"
              << "\n";

    vector<int> sizes = {50, 100, 200, 400, 800};
    vector<size_t> relaxations;

    for (int n : sizes) {
        Graph g = generate_path_graph(n);
        int m = n - 1;

        auto result = DuanSSSP::ComputeSSSP(g, 0, true);

        relaxations.push_back(result.stats.edge_relaxations);

        long double ratio = 0.0;
        if (relaxations.size() > 1) {
            ratio = (long double)relaxations.back() / relaxations[relaxations.size() - 2];
        }

        std::cout << std::setw(10) << n
                  << std::setw(15) << m
                  << std::setw(15) << result.stats.edge_relaxations
                  << std::setw(15) << result.stats.total_time.count()
                  << std::setw(15) << std::fixed << std::setprecision(2) << ratio
                  << "\n";
    }
}

// Test 5: Complexity scaling on sparse graphs
void test_complexity_sparse() {
    std::cout << "\n=== Complexity Analysis: Sparse Graphs (degree=4) ===\n";
    std::cout << std::setw(10) << "n"
              << std::setw(15) << "m"
              << std::setw(15) << "Relaxations"
              << std::setw(15) << "Time (μs)"
              << std::setw(15) << "m*log^(2/3)n"
              << "\n";

    std::mt19937 rng(42);
    vector<int> sizes = {50, 100, 200, 400};
    int out_degree = 4;

    for (int n : sizes) {
        Graph g = generate_sparse_graph(n, out_degree, rng);
        int m = 0;
        for (const auto& adj : g) m += adj.size();

        auto result = DuanSSSP::ComputeSSSP(g, 0, true);

        long double log_n = std::log2((long double)n);
        long double theoretical = m * std::pow(log_n, 2.0L / 3.0L);

        std::cout << std::setw(10) << n
                  << std::setw(15) << m
                  << std::setw(15) << result.stats.edge_relaxations
                  << std::setw(15) << result.stats.total_time.count()
                  << std::setw(15) << std::fixed << std::setprecision(0) << theoretical
                  << "\n";
    }
}

// Test 6: Detailed statistics
void test_detailed_stats() {
    std::cout << "\n=== Detailed Statistics Example ===\n";

    Graph g = generate_grid_graph(10, 10);
    auto result = DuanSSSP::ComputeSSSP(g, 0, true);

    result.stats.print();

    // Verify correctness
    auto dijkstra_result = Dijkstra::ComputeSSSP(g, 0);
    bool correct = true;
    for (int i = 0; i < (int)g.size(); ++i) {
        if (!approx_equal(result.dist[i], dijkstra_result[i])) {
            correct = false;
            break;
        }
    }

    std::cout << "Correctness: " << (correct ? "PASS" : "FAIL") << "\n";
    assert(correct);
}

// Export data for plotting
void export_complexity_data() {
    std::cout << "\n=== Exporting Data for Analysis ===\n";

    std::ofstream csv("duan/complexity_data.csv");
    csv << "graph_type,n,m,edge_relaxations,time_us,theoretical_bound\n";

    std::mt19937 rng(42);

    // Path graphs
    for (int n : {50, 100, 200, 400, 800}) {
        Graph g = generate_path_graph(n);
        int m = n - 1;
        auto result = DuanSSSP::ComputeSSSP(g, 0, true);

        long double log_n = std::log2((long double)n);
        long double theoretical = m * std::pow(log_n, 2.0L / 3.0L);

        csv << "path," << n << "," << m << ","
            << result.stats.edge_relaxations << ","
            << result.stats.total_time.count() << ","
            << theoretical << "\n";
    }

    // Sparse graphs
    for (int n : {50, 100, 200, 400}) {
        Graph g = generate_sparse_graph(n, 4, rng);
        int m = 0;
        for (const auto& adj : g) m += adj.size();

        auto result = DuanSSSP::ComputeSSSP(g, 0, true);

        long double log_n = std::log2((long double)n);
        long double theoretical = m * std::pow(log_n, 2.0L / 3.0L);

        csv << "sparse," << n << "," << m << ","
            << result.stats.edge_relaxations << ","
            << result.stats.total_time.count() << ","
            << theoretical << "\n";
    }

    // Grid graphs
    for (int side : {5, 7, 10, 14, 20}) {
        Graph g = generate_grid_graph(side, side);
        int n = side * side;
        int m = 0;
        for (const auto& adj : g) m += adj.size();

        auto result = DuanSSSP::ComputeSSSP(g, 0, true);

        long double log_n = std::log2((long double)n);
        long double theoretical = m * std::pow(log_n, 2.0L / 3.0L);

        csv << "grid," << n << "," << m << ","
            << result.stats.edge_relaxations << ","
            << result.stats.total_time.count() << ","
            << theoretical << "\n";
    }

    csv.close();
    std::cout << "Data exported to complexity_data.csv\n";
}

int main() {
    std::cout << "=== Duan SSSP Complexity Analysis ===\n\n";

    // Correctness tests
    TEST(test_correctness_path);
    TEST(test_correctness_grid);
    TEST(test_correctness_sparse);

    std::cout << "\n=== Correctness Summary ===\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";

    if (tests_failed == 0) {
        // Complexity analysis
        test_complexity_path();
        test_complexity_sparse();
        test_detailed_stats();
        export_complexity_data();
    }

    return (tests_failed == 0) ? 0 : 1;
}
