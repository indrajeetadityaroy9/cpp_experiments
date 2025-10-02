/**
 * Unit tests for BaseCase algorithm
 */

#include "../include/base_case.hpp"
#include <iostream>
#include <cassert>
#include <algorithm>
#include <cmath>

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

// Helper function to create a simple path graph: 0 -> 1 -> 2 -> 3 -> 4
Graph create_path_graph() {
    Graph g(5);
    g[0].push_back({1, 1.0});
    g[1].push_back({2, 1.0});
    g[2].push_back({3, 1.0});
    g[3].push_back({4, 1.0});
    return g;
}

// Helper function to create a star graph: center -> spoke1, spoke2, ...
Graph create_star_graph(int num_spokes) {
    Graph g(num_spokes + 1);
    for (int i = 1; i <= num_spokes; ++i) {
        g[0].push_back({i, 1.0});
    }
    return g;
}

// Helper to check if value is approximately equal
bool approx_equal(long double a, long double b, long double eps = 1e-9) {
    return std::abs(a - b) < eps;
}

// Test 1: Small k, few vertices found (|U| <= k)
void test_small_k_few_vertices() {
    Graph g = create_path_graph();
    Labels labels(5);

    // Initialize source vertex 0
    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 10;  // Large k
    long double B = 2.5;  // Only reach vertices 0, 1, 2
    vector<int> S = {0};

    auto result = BaseCase::Execute(g, labels, B, S, k);

    // Should find vertices 0, 1, 2 (distances 0, 1, 2)
    // Since |U_0| = 3 <= k = 10, should return b = B
    assert(approx_equal(result.b, B));
    assert(result.U.size() == 3);

    // Check all returned vertices have dist < B
    for (int v : result.U) {
        assert(labels.dist[v] < B);
    }
}

// Test 2: Found exactly k+1 vertices
void test_exactly_k_plus_one() {
    Graph g = create_star_graph(5);
    Labels labels(6);

    // Initialize source vertex 0
    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 2;  // Will find k+1 = 3 vertices
    long double B = 10.0;
    vector<int> S = {0};

    auto result = BaseCase::Execute(g, labels, B, S, k);

    // Should find k+1 = 3 vertices: 0, 1, 2 (or 0 and any 2 spokes)
    // b = max distance among the 3 vertices
    // U = vertices with dist < b

    // Since all spokes have distance 1 from center (0 has dist 0),
    // U_0 will contain {0} and any 2 spokes
    // max_dist will be 1.0, so b = 1.0
    // U will contain only vertex 0 (dist 0 < 1.0)
    assert(result.U.size() == 1);
    assert(result.U[0] == 0);
}

// Test 3: Path graph with k boundary
void test_path_graph_boundary() {
    Graph g = create_path_graph();
    Labels labels(5);

    // Initialize source vertex 0
    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 2;  // Find 3 vertices: 0, 1, 2
    long double B = 10.0;
    vector<int> S = {0};

    auto result = BaseCase::Execute(g, labels, B, S, k);

    // U_0 = {0, 1, 2} with distances {0, 1, 2}
    // b = max = 2.0
    // U = {0, 1} with dist < 2.0
    assert(approx_equal(result.b, 2.0));
    assert(result.U.size() == 2);

    // Check vertices 0 and 1 are in U
    std::sort(result.U.begin(), result.U.end());
    assert(result.U[0] == 0);
    assert(result.U[1] == 1);
}

// Test 4: Bounded relaxation (B limits reachable vertices)
void test_bounded_by_B() {
    Graph g = create_path_graph();
    Labels labels(5);

    // Initialize source vertex 0
    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 10;
    long double B = 1.5;  // Only vertex 1 is reachable (dist 1.0 < 1.5)
    vector<int> S = {0};

    auto result = BaseCase::Execute(g, labels, B, S, k);

    // Vertices with dist < B: {0, 1}
    // Since |U_0| = 2 <= k, return b = B
    assert(approx_equal(result.b, B));
    assert(result.U.size() == 2);
}

// Test 5: Diamond graph with tie-breaking
void test_lexicographic_tiebreak() {
    // Create diamond graph:
    //     0
    //    / \
    //   1   2
    //    \ /
    //     3
    Graph g(4);
    g[0].push_back({1, 1.0});
    g[0].push_back({2, 1.0});
    g[1].push_back({3, 1.0});
    g[2].push_back({3, 1.0});

    Labels labels(4);

    // Initialize source vertex 0
    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 3;  // Find 4 vertices
    long double B = 10.0;
    vector<int> S = {0};

    auto result = BaseCase::Execute(g, labels, B, S, k);

    // Vertex 3 has two equal-length paths: 0->1->3 and 0->2->3
    // Lexicographic tie-breaking should prefer smaller predecessor (1 < 2)
    assert(labels.pred[3] == 1);

    // U_0 = {0, 1, 2, 3} with max dist = 2.0
    // b = 2.0, U = {0, 1, 2} (vertices with dist < 2.0)
    assert(approx_equal(result.b, 2.0));
    assert(result.U.size() == 3);
}

// Test 6: Disconnected graph (isolated source)
void test_isolated_source() {
    // Graph with isolated vertex 0, and connected component 1->2
    Graph g(3);
    g[1].push_back({2, 1.0});

    Labels labels(3);

    // Initialize isolated source vertex 0
    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 5;
    long double B = 10.0;
    vector<int> S = {0};

    auto result = BaseCase::Execute(g, labels, B, S, k);

    // Only vertex 0 is reachable
    // |U_0| = 1 <= k, so b = B
    assert(approx_equal(result.b, B));
    assert(result.U.size() == 1);
    assert(result.U[0] == 0);
}

// Test 7: Large star graph (many vertices)
void test_large_star() {
    Graph g = create_star_graph(20);
    Labels labels(21);

    // Initialize source vertex 0
    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 5;
    long double B = 10.0;
    vector<int> S = {0};

    auto result = BaseCase::Execute(g, labels, B, S, k);

    // Will find k+1 = 6 vertices: center 0 and 5 spokes
    // All spokes have distance 1, center has distance 0
    // b = max = 1.0, U = {0} (only center has dist < 1.0)
    assert(approx_equal(result.b, 1.0));
    assert(result.U.size() == 1);
    assert(result.U[0] == 0);
}

// Test 8: Weighted path graph
void test_weighted_path() {
    // Create weighted path: 0 --(1.5)--> 1 --(2.0)--> 2 --(1.0)--> 3
    Graph g(4);
    g[0].push_back({1, 1.5});
    g[1].push_back({2, 2.0});
    g[2].push_back({3, 1.0});

    Labels labels(4);

    // Initialize source vertex 0
    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 2;
    long double B = 10.0;
    vector<int> S = {0};

    auto result = BaseCase::Execute(g, labels, B, S, k);

    // Distances: 0->0.0, 1->1.5, 2->3.5, 3->4.5
    // U_0 = {0, 1, 2} (first k+1 = 3 vertices)
    // b = max = 3.5, U = {0, 1} (dist < 3.5)
    assert(approx_equal(result.b, 3.5));
    assert(result.U.size() == 2);

    std::sort(result.U.begin(), result.U.end());
    assert(result.U[0] == 0);
    assert(result.U[1] == 1);
}

// Test 9: k=0 edge case
void test_k_zero() {
    Graph g = create_path_graph();
    Labels labels(5);

    // Initialize source vertex 0
    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 0;  // Find only 1 vertex
    long double B = 10.0;
    vector<int> S = {0};

    auto result = BaseCase::Execute(g, labels, B, S, k);

    // k+1 = 1, so U_0 = {0}
    // |U_0| = 1 > k = 0, so b = max = 0.0
    // U = {} (no vertices with dist < 0.0)
    assert(approx_equal(result.b, 0.0));
    assert(result.U.empty());
}

int main() {
    std::cout << "=== BaseCase Unit Tests ===\n\n";

    TEST(test_small_k_few_vertices);
    TEST(test_exactly_k_plus_one);
    TEST(test_path_graph_boundary);
    TEST(test_bounded_by_B);
    TEST(test_lexicographic_tiebreak);
    TEST(test_isolated_source);
    TEST(test_large_star);
    TEST(test_weighted_path);
    TEST(test_k_zero);

    std::cout << "\n=== Test Summary ===\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";

    return (tests_failed == 0) ? 0 : 1;
}
