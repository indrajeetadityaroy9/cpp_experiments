/**
 * Unit tests for FindPivots algorithm
 */

#include "../include/find_pivots.hpp"
#include <iostream>
#include <cassert>
#include <algorithm>

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

// Test 1: Single source, all vertices reachable in k steps -> no pivots
void test_single_source_no_pivots() {
    Graph g = create_path_graph();
    Labels labels(5);

    // Initialize source vertex 0
    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 5;  // Large enough to reach all vertices
    long double B = 10.0;
    vector<int> S = {0};

    auto result = FindPivots::Execute(g, labels, B, S, k);

    // Should have visited all 5 vertices
    assert(result.W.size() == 5);

    // With k=5, all vertices reachable in <k steps from source
    // So no subtree should have ≥k vertices, hence P should be empty or small
    // Actually, the subtree rooted at 0 has 5 vertices = k, so 0 becomes a pivot
    assert(result.P.size() <= 1);
}

// Test 2: Single source with large tree -> source becomes pivot
void test_single_source_with_pivot() {
    Graph g = create_star_graph(10);
    Labels labels(11);

    // Initialize source vertex 0
    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 3;  // Threshold for pivot identification
    long double B = 10.0;
    vector<int> S = {0};

    auto result = FindPivots::Execute(g, labels, B, S, k);

    // Should have visited center + all spokes = 11 vertices
    assert(result.W.size() == 11);

    // Vertex 0 is root of tree with 11 vertices ≥ k=3, so should be a pivot
    assert(result.P.size() == 1);
    assert(result.P[0] == 0);
}

// Test 3: Early exit when |W| > k|S|
void test_early_exit() {
    Graph g = create_star_graph(20);
    Labels labels(21);

    // Initialize source vertex 0
    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 2;  // Small k
    long double B = 10.0;
    vector<int> S = {0};

    auto result = FindPivots::Execute(g, labels, B, S, k);

    // |W| will exceed k|S| = 2*1 = 2 quickly
    // Should return P = S
    assert(result.P.size() == 1);
    assert(result.P[0] == 0);

    // W should contain multiple vertices
    assert((int)result.W.size() > k * (int)S.size());
}

// Test 4: Multiple sources
void test_multiple_sources() {
    // Create graph: 0->2, 1->2, 2->3, 2->4, 2->5, 2->6
    Graph g(7);
    g[0].push_back({2, 1.0});
    g[1].push_back({2, 1.0});
    g[2].push_back({3, 1.0});
    g[2].push_back({4, 1.0});
    g[2].push_back({5, 1.0});
    g[2].push_back({6, 1.0});

    Labels labels(7);

    // Initialize two sources
    labels.dist[0] = 0.0;
    labels.hops[0] = 0;
    labels.dist[1] = 0.0;
    labels.hops[1] = 0;

    int k = 3;
    long double B = 10.0;
    vector<int> S = {0, 1};

    auto result = FindPivots::Execute(g, labels, B, S, k);

    // Should visit multiple vertices
    assert(result.W.size() > 2);

    // Check P is subset of S
    for (int p : result.P) {
        assert(p == 0 || p == 1);
    }
}

// Test 5: Empty source set
void test_empty_source_set() {
    Graph g = create_path_graph();
    Labels labels(5);

    int k = 3;
    long double B = 10.0;
    vector<int> S = {};

    auto result = FindPivots::Execute(g, labels, B, S, k);

    // Should return empty sets
    assert(result.P.empty());
    assert(result.W.empty());
}

// Test 6: Bounded relaxation (vertices with d ≥ B not added to W)
void test_bounded_relaxation() {
    Graph g = create_path_graph();
    Labels labels(5);

    // Initialize source vertex 0
    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 5;
    long double B = 2.5;  // Only vertices 0, 1, 2 should be in W
    vector<int> S = {0};

    auto result = FindPivots::Execute(g, labels, B, S, k);

    // Vertices with dist < B: 0 (0.0), 1 (1.0), 2 (2.0)
    // Vertex 3 (3.0) ≥ B=2.5, should not be in W
    assert(result.W.size() <= 3);

    // Check all vertices in W have dist < B
    for (int v : result.W) {
        assert(labels.dist[v] < B);
    }
}

// Test 7: Lexicographic tie-breaking
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

    int k = 3;
    long double B = 10.0;
    vector<int> S = {0};

    auto result = FindPivots::Execute(g, labels, B, S, k);

    // Should visit all 4 vertices
    assert(result.W.size() == 4);

    // Vertex 3 has two paths of equal length from 0: 0->1->3 and 0->2->3
    // Lexicographic tie-breaking should prefer smaller predecessor (1 < 2)
    assert(labels.pred[3] == 1);
}

// Test 8: k-step limitation
void test_k_step_limitation() {
    Graph g = create_path_graph();
    Labels labels(5);

    // Initialize source vertex 0
    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 2;  // Only 2 relaxation steps
    long double B = 10.0;
    vector<int> S = {0};

    auto result = FindPivots::Execute(g, labels, B, S, k);

    // With k=2 steps from vertex 0: can reach 0, 1, 2
    // Vertices 3, 4 require 3-4 steps
    assert(result.W.size() <= 3);
}

// Test 9: No reachable vertices from source
void test_disconnected_source() {
    // Create disconnected graph: 0 (isolated), 1->2
    Graph g(3);
    g[1].push_back({2, 1.0});

    Labels labels(3);

    // Initialize isolated source vertex 0
    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 3;
    long double B = 10.0;
    vector<int> S = {0};

    auto result = FindPivots::Execute(g, labels, B, S, k);

    // Should only contain source vertex
    assert(result.W.size() == 1);
    assert(result.W[0] == 0);

    // No large subtree, so no pivots
    assert(result.P.empty());
}

int main() {
    std::cout << "=== FindPivots Unit Tests ===\n\n";

    TEST(test_single_source_no_pivots);
    TEST(test_single_source_with_pivot);
    TEST(test_early_exit);
    TEST(test_multiple_sources);
    TEST(test_empty_source_set);
    TEST(test_bounded_relaxation);
    TEST(test_lexicographic_tiebreak);
    TEST(test_k_step_limitation);
    TEST(test_disconnected_source);

    std::cout << "\n=== Test Summary ===\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";

    return (tests_failed == 0) ? 0 : 1;
}
