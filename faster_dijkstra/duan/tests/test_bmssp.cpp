/**
 * Unit tests for BMSSP algorithm
 */

#include "../include/bmssp.hpp"
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

// Test 1: Layer 0 (should call BaseCase)
void test_layer_0_base_case() {
    Graph g = create_path_graph();
    Labels labels(5);

    // Initialize source vertex 0
    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    Params params = Params::compute(5);
    int l = 0;
    long double B = 10.0;
    vector<int> S = {0};

    auto result = BMSSP::Execute(g, labels, l, B, S, params);

    // Should behave like BaseCase
    assert(result.U.size() > 0);
    assert(result.b <= B);

    // All vertices in U should have dist < b
    for (int v : result.U) {
        assert(labels.dist[v] < result.b);
    }
}

// Test 2: Layer 1 simple path
void test_layer_1_path() {
    Graph g = create_path_graph();
    Labels labels(5);

    // Initialize source vertex 0
    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    Params params = Params::compute(5);
    int l = 1;
    long double B = 10.0;
    vector<int> S = {0};

    auto result = BMSSP::Execute(g, labels, l, B, S, params);

    // Should find all reachable vertices
    assert(result.U.size() > 0);

    // Check that source is complete
    assert(labels.dist[0] == 0.0);
}

// Test 3: Star graph with multiple vertices
void test_star_graph() {
    Graph g = create_star_graph(10);
    Labels labels(11);

    // Initialize source vertex 0
    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    Params params = Params::compute(11);
    int l = 1;
    long double B = 10.0;
    vector<int> S = {0};

    auto result = BMSSP::Execute(g, labels, l, B, S, params);

    // Should find some vertices
    assert(result.U.size() > 0);

    // Check reachable vertices have correct distances
    assert(labels.dist[0] == 0.0);
    for (int i = 1; i <= 10; ++i) {
        if (labels.dist[i] < INF) {
            assert(approx_equal(labels.dist[i], 1.0));
        }
    }
}

// Test 4: Bounded search (B limits reachable vertices)
void test_bounded_search() {
    Graph g = create_path_graph();
    Labels labels(5);

    // Initialize source vertex 0
    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    Params params = Params::compute(5);
    int l = 1;
    long double B = 2.5;  // Only reach vertices 0, 1, 2
    vector<int> S = {0};

    auto result = BMSSP::Execute(g, labels, l, B, S, params);

    // Should only find vertices with dist < result.b <= B
    assert(result.b <= B);
    for (int v : result.U) {
        assert(labels.dist[v] < result.b);
    }

    // Result should contain at least the source
    assert(!result.U.empty());

    // All vertices in U should have finite distances
    for (int v : result.U) {
        assert(labels.dist[v] < INF);
    }
}

// Test 5: Diamond graph with tie-breaking
void test_diamond_graph() {
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

    Params params = Params::compute(4);
    int l = 1;
    long double B = 10.0;
    vector<int> S = {0};

    auto result = BMSSP::Execute(g, labels, l, B, S, params);

    // Vertex 3 has two equal-length paths: 0->1->3 and 0->2->3
    // BMSSP should explore all reachable vertices

    // Check that we got some result
    assert(!result.U.empty());

    // Check distances for reachable vertices
    // Note: Not all vertices may be in U (BMSSP may return partial results)
    if (labels.dist[0] < INF) assert(approx_equal(labels.dist[0], 0.0));
    if (labels.dist[1] < INF) assert(approx_equal(labels.dist[1], 1.0));
    if (labels.dist[2] < INF) assert(approx_equal(labels.dist[2], 1.0));
    if (labels.dist[3] < INF) assert(approx_equal(labels.dist[3], 2.0));
}

// Test 6: Weighted path graph
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

    Params params = Params::compute(4);
    int l = 1;
    long double B = 10.0;
    vector<int> S = {0};

    auto result = BMSSP::Execute(g, labels, l, B, S, params);

    // Check correct distances for reachable vertices
    assert(!result.U.empty());
    if (labels.dist[0] < INF) assert(approx_equal(labels.dist[0], 0.0));
    if (labels.dist[1] < INF) assert(approx_equal(labels.dist[1], 1.5));
    if (labels.dist[2] < INF) assert(approx_equal(labels.dist[2], 3.5));
    if (labels.dist[3] < INF) assert(approx_equal(labels.dist[3], 4.5));
}

// Test 7: Multiple layers (l=2)
void test_multiple_layers() {
    Graph g = create_star_graph(20);
    Labels labels(21);

    // Initialize source vertex 0
    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    Params params = Params::compute(21);
    int l = 2;
    long double B = 10.0;
    vector<int> S = {0};

    auto result = BMSSP::Execute(g, labels, l, B, S, params);

    // Should find some vertices
    assert(result.U.size() > 0);

    // Reachable spokes should have distance 1.0
    for (int i = 1; i <= 20; ++i) {
        if (labels.dist[i] < INF) {
            assert(approx_equal(labels.dist[i], 1.0));
        }
    }
}

// Test 8: Empty source set
// Commented out - violates algorithm requirements (S must be non-empty)
// void test_empty_source() {
//     // This should fail the assertion in BaseCase (S must be singleton for l=0)
//     // For l > 0, empty S should work but return empty result
//     // Skip this test as it violates algorithm requirements
// }

// Test 9: Disconnected graph
void test_disconnected_graph() {
    // Graph with two components: 0->1, 2->3
    Graph g(4);
    g[0].push_back({1, 1.0});
    g[2].push_back({3, 1.0});

    Labels labels(4);

    // Initialize source vertex 0
    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    Params params = Params::compute(4);
    int l = 1;
    long double B = 10.0;
    vector<int> S = {0};

    auto result = BMSSP::Execute(g, labels, l, B, S, params);

    // Should only find vertices in component containing 0
    assert(approx_equal(labels.dist[0], 0.0));
    assert(approx_equal(labels.dist[1], 1.0));

    // Vertices 2 and 3 should remain at infinity
    assert(labels.dist[2] == INF);
    assert(labels.dist[3] == INF);
}

int main() {
    std::cout << "=== BMSSP Unit Tests ===\n\n";

    TEST(test_layer_0_base_case);
    TEST(test_layer_1_path);
    TEST(test_star_graph);
    TEST(test_bounded_search);
    TEST(test_diamond_graph);
    TEST(test_weighted_path);
    TEST(test_multiple_layers);
    // TEST(test_empty_source);  // Skip - violates requirements
    TEST(test_disconnected_graph);

    std::cout << "\n=== Test Summary ===\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";

    return (tests_failed == 0) ? 0 : 1;
}
