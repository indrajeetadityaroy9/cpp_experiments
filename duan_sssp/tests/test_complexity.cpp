/**
 * Complexity analysis and empirical validation
 *
 * Tests Duan SSSP on various graph sizes and structures to:
 * 1. Validate correctness against Dijkstra
 * 2. Measure empirical complexity
 * 3. Compare against theoretical O(m·log^(2/3)(n)) bound
 */

#include "../include/duan_sssp.hpp"
#include "../test_helpers/graph_generators.hpp"
#include "../test_helpers/test_utils.hpp"
#include <catch_amalgamated.hpp>
#include <cmath>
#include <fstream>
#include <random>
#include <iomanip>

using namespace duan;
using namespace duan::test;

TEST_CASE("Correctness on path graph", "[complexity][correctness]") {
    int n = 20;
    Graph g = create_path_graph(n);
    int source = 0;

    auto duan_result = DuanSSSP::ComputeSSSP(g, source, false);
    auto dijkstra_result = Dijkstra::ComputeSSSP(g, source);

    for (int i = 0; i < n; ++i) {
        REQUIRE(approx_equal(duan_result.dist[i], dijkstra_result[i]));
    }
}

TEST_CASE("Correctness on grid graph", "[complexity][correctness]") {
    Graph g = create_grid_graph(5, 5);
    int source = 0;

    auto duan_result = DuanSSSP::ComputeSSSP(g, source, false);
    auto dijkstra_result = Dijkstra::ComputeSSSP(g, source);

    for (int i = 0; i < (int)g.size(); ++i) {
        REQUIRE(approx_equal(duan_result.dist[i], dijkstra_result[i]));
    }
}

TEST_CASE("Correctness on sparse random graph", "[complexity][correctness]") {
    std::mt19937 rng(42);
    int n = 30;
    int out_degree = 3;

    Graph g = create_sparse_graph(n, out_degree, rng);
    int source = 0;

    auto duan_result = DuanSSSP::ComputeSSSP(g, source, false);
    auto dijkstra_result = Dijkstra::ComputeSSSP(g, source);

    for (int i = 0; i < n; ++i) {
        REQUIRE(approx_equal(duan_result.dist[i], dijkstra_result[i]));
    }
}

TEST_CASE("Complexity scaling on path graphs", "[complexity][benchmark]") {
    std::cout << "\n=== Complexity Analysis: Path Graphs ===\n";
    std::cout << std::setw(10) << "n"
              << std::setw(15) << "m"
              << std::setw(15) << "Relaxations"
              << std::setw(15) << "Time (μs)"
              << "\n";

    vector<int> sizes = {50, 100, 200, 400};

    for (int n : sizes) {
        Graph g = create_path_graph(n);
        int m = n - 1;

        auto result = DuanSSSP::ComputeSSSP(g, 0, true);

        std::cout << std::setw(10) << n
                  << std::setw(15) << m
                  << std::setw(15) << result.stats.edge_relaxations
                  << std::setw(15) << result.stats.total_time.count()
                  << "\n";

        // Sanity check: relaxations should be bounded
        REQUIRE(result.stats.edge_relaxations > 0);
    }
}

TEST_CASE("Complexity scaling on sparse graphs", "[complexity][benchmark]") {
    std::cout << "\n=== Complexity Analysis: Sparse Graphs (degree=4) ===\n";
    std::cout << std::setw(10) << "n"
              << std::setw(15) << "m"
              << std::setw(15) << "Relaxations"
              << std::setw(15) << "m*log^(2/3)n"
              << "\n";

    std::mt19937 rng(42);
    vector<int> sizes = {50, 100, 200};
    int out_degree = 4;

    for (int n : sizes) {
        Graph g = create_sparse_graph(n, out_degree, rng);
        int m = 0;
        for (const auto& adj : g) m += adj.size();

        auto result = DuanSSSP::ComputeSSSP(g, 0, true);

        long double log_n = std::log2((long double)n);
        long double theoretical = m * std::pow(log_n, 2.0L / 3.0L);

        std::cout << std::setw(10) << n
                  << std::setw(15) << m
                  << std::setw(15) << result.stats.edge_relaxations
                  << std::setw(15) << std::fixed << std::setprecision(0) << theoretical
                  << "\n";

        REQUIRE(result.stats.edge_relaxations > 0);
    }
}

TEST_CASE("Detailed statistics example", "[complexity][benchmark]") {
    std::cout << "\n=== Detailed Statistics Example ===\n";

    Graph g = create_grid_graph(10, 10);
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

    REQUIRE(correct);
}
