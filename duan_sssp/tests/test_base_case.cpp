/**
 * Unit tests for BaseCase algorithm
 */

#include "../include/algorithms/base_case.hpp"
#include "../test_helpers/graph_generators.hpp"
#include "../test_helpers/test_utils.hpp"
#include <catch_amalgamated.hpp>
#include <algorithm>

using namespace duan;
using namespace duan::test;

TEST_CASE("BaseCase small k with few vertices", "[base_case]") {
    Graph g = create_path_graph();
    Labels labels(5);

    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 10;  // Large k
    long double B = 2.5;  // Only reach vertices 0, 1, 2
    vector<int> S = {0};

    auto result_exp = BaseCase::Execute(g, labels, B, S, k);
    REQUIRE(result_exp.has_value());
    auto result = *result_exp;

    // Should find vertices 0, 1, 2 (distances 0, 1, 2)
    REQUIRE(approx_equal(result.b, B));
    REQUIRE(result.U.size() == 3);

    // Check all returned vertices have dist < B
    for (int v : result.U) {
        REQUIRE(labels.dist[v] < B);
    }
}

TEST_CASE("BaseCase exactly k+1 vertices", "[base_case]") {
    Graph g = create_star_graph(5);
    Labels labels(6);

    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 2;  // Will find k+1 = 3 vertices
    long double B = 10.0;
    vector<int> S = {0};

    auto result_exp = BaseCase::Execute(g, labels, B, S, k);
    REQUIRE(result_exp.has_value());
    auto result = *result_exp;

    REQUIRE(result.U.size() == 1);
    REQUIRE(result.U[0] == 0);
}

TEST_CASE("BaseCase path graph boundary", "[base_case]") {
    Graph g = create_path_graph();
    Labels labels(5);

    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 2;  // Find 3 vertices: 0, 1, 2
    long double B = 10.0;
    vector<int> S = {0};

    auto result_exp = BaseCase::Execute(g, labels, B, S, k);
    REQUIRE(result_exp.has_value());
    auto result = *result_exp;

    REQUIRE(approx_equal(result.b, 2.0));
    REQUIRE(result.U.size() == 2);

    std::sort(result.U.begin(), result.U.end());
    REQUIRE(result.U[0] == 0);
    REQUIRE(result.U[1] == 1);
}

TEST_CASE("BaseCase bounded by B", "[base_case]") {
    Graph g = create_path_graph();
    Labels labels(5);

    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 10;
    long double B = 1.5;  // Only vertex 1 is reachable (dist 1.0 < 1.5)
    vector<int> S = {0};

    auto result_exp = BaseCase::Execute(g, labels, B, S, k);
    REQUIRE(result_exp.has_value());
    auto result = *result_exp;

    REQUIRE(approx_equal(result.b, B));
    REQUIRE(result.U.size() == 2);
}

TEST_CASE("BaseCase diamond graph with tie-breaking", "[base_case]") {
    Graph g = create_diamond_graph();
    Labels labels(4);

    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 3;
    long double B = 10.0;
    vector<int> S = {0};

    auto result_exp = BaseCase::Execute(g, labels, B, S, k);
    REQUIRE(result_exp.has_value());
    auto result = *result_exp;

    // Lexicographic tie-breaking should prefer smaller predecessor (1 < 2)
    REQUIRE(labels.pred[3] == 1);
    REQUIRE(approx_equal(result.b, 2.0));
    REQUIRE(result.U.size() == 3);
}

TEST_CASE("BaseCase isolated source", "[base_case]") {
    Graph g(3);
    g[1].push_back({2, 1.0});

    Labels labels(3);

    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 5;
    long double B = 10.0;
    vector<int> S = {0};

    auto result_exp = BaseCase::Execute(g, labels, B, S, k);
    REQUIRE(result_exp.has_value());
    auto result = *result_exp;

    REQUIRE(approx_equal(result.b, B));
    REQUIRE(result.U.size() == 1);
    REQUIRE(result.U[0] == 0);
}

TEST_CASE("BaseCase large star graph", "[base_case]") {
    Graph g = create_star_graph(20);
    Labels labels(21);

    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 5;
    long double B = 10.0;
    vector<int> S = {0};

    auto result_exp = BaseCase::Execute(g, labels, B, S, k);
    REQUIRE(result_exp.has_value());
    auto result = *result_exp;

    REQUIRE(approx_equal(result.b, 1.0));
    REQUIRE(result.U.size() == 1);
    REQUIRE(result.U[0] == 0);
}

TEST_CASE("BaseCase weighted path", "[base_case]") {
    Graph g(4);
    g[0].push_back({1, 1.5});
    g[1].push_back({2, 2.0});
    g[2].push_back({3, 1.0});

    Labels labels(4);

    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 2;
    long double B = 10.0;
    vector<int> S = {0};

    auto result_exp = BaseCase::Execute(g, labels, B, S, k);
    REQUIRE(result_exp.has_value());
    auto result = *result_exp;

    REQUIRE(approx_equal(result.b, 3.5));
    REQUIRE(result.U.size() == 2);

    std::sort(result.U.begin(), result.U.end());
    REQUIRE(result.U[0] == 0);
    REQUIRE(result.U[1] == 1);
}

TEST_CASE("BaseCase k=0 edge case", "[base_case]") {
    Graph g = create_path_graph();
    Labels labels(5);

    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 0;
    long double B = 10.0;
    vector<int> S = {0};

    auto result_exp = BaseCase::Execute(g, labels, B, S, k);
    REQUIRE(result_exp.has_value());
    auto result = *result_exp;

    REQUIRE(approx_equal(result.b, 0.0));
    REQUIRE(result.U.empty());
}

TEST_CASE("BaseCase error: non-singleton source set", "[base_case][error]") {
    Graph g = create_path_graph();
    Labels labels(5);

    labels.dist[0] = 0.0;
    labels.hops[0] = 0;
    labels.dist[1] = 0.0;
    labels.hops[1] = 0;

    int k = 5;
    long double B = 10.0;
    vector<int> S = {0, 1};  // Multiple sources

    auto result_exp = BaseCase::Execute(g, labels, B, S, k);
    REQUIRE_FALSE(result_exp.has_value());
    REQUIRE(result_exp.error() == DuanError::NonSingletonSourceSet);
}

TEST_CASE("BaseCase error: source out of bounds", "[base_case][error]") {
    Graph g = create_path_graph();
    Labels labels(5);

    int k = 5;
    long double B = 10.0;
    vector<int> S = {10};  // Out of bounds

    auto result_exp = BaseCase::Execute(g, labels, B, S, k);
    REQUIRE_FALSE(result_exp.has_value());
    REQUIRE(result_exp.error() == DuanError::SourceOutOfBounds);
}

TEST_CASE("BaseCase error: negative source index", "[base_case][error]") {
    Graph g = create_path_graph();
    Labels labels(5);

    int k = 5;
    long double B = 10.0;
    vector<int> S = {-1};  // Negative index

    auto result_exp = BaseCase::Execute(g, labels, B, S, k);
    REQUIRE_FALSE(result_exp.has_value());
    REQUIRE(result_exp.error() == DuanError::SourceOutOfBounds);
}

TEST_CASE("BaseCase error: empty source set", "[base_case][error]") {
    Graph g = create_path_graph();
    Labels labels(5);

    int k = 5;
    long double B = 10.0;
    vector<int> S = {};  // Empty

    auto result_exp = BaseCase::Execute(g, labels, B, S, k);
    REQUIRE_FALSE(result_exp.has_value());
    REQUIRE(result_exp.error() == DuanError::NonSingletonSourceSet);
}
