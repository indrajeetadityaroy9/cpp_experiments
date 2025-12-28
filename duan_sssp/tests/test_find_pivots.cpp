/**
 * Unit tests for FindPivots algorithm
 */

#include "../include/algorithms/find_pivots.hpp"
#include "../test_helpers/graph_generators.hpp"
#include <catch_amalgamated.hpp>
#include <algorithm>

using namespace duan;
using namespace duan::test;

TEST_CASE("FindPivots single source no pivots", "[find_pivots]") {
    Graph g = create_path_graph();
    Labels labels(5);

    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 5;  // Large enough to reach all vertices
    long double B = 10.0;
    vector<int> S = {0};

    auto result = FindPivots::Execute(g, labels, B, S, k);

    // Should have visited all 5 vertices
    REQUIRE(result.W.size() == 5);
    REQUIRE(result.P.size() <= 1);
}

TEST_CASE("FindPivots single source with pivot", "[find_pivots]") {
    Graph g = create_star_graph(10);
    Labels labels(11);

    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 3;
    long double B = 10.0;
    vector<int> S = {0};

    auto result = FindPivots::Execute(g, labels, B, S, k);

    REQUIRE(result.W.size() == 11);
    REQUIRE(result.P.size() == 1);
    REQUIRE(result.P[0] == 0);
}

TEST_CASE("FindPivots early exit when |W| > k|S|", "[find_pivots]") {
    Graph g = create_star_graph(20);
    Labels labels(21);

    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 2;
    long double B = 10.0;
    vector<int> S = {0};

    auto result = FindPivots::Execute(g, labels, B, S, k);

    REQUIRE(result.P.size() == 1);
    REQUIRE(result.P[0] == 0);
    REQUIRE((int)result.W.size() > k * (int)S.size());
}

TEST_CASE("FindPivots multiple sources", "[find_pivots]") {
    Graph g(7);
    g[0].push_back({2, 1.0});
    g[1].push_back({2, 1.0});
    g[2].push_back({3, 1.0});
    g[2].push_back({4, 1.0});
    g[2].push_back({5, 1.0});
    g[2].push_back({6, 1.0});

    Labels labels(7);

    labels.dist[0] = 0.0;
    labels.hops[0] = 0;
    labels.dist[1] = 0.0;
    labels.hops[1] = 0;

    int k = 3;
    long double B = 10.0;
    vector<int> S = {0, 1};

    auto result = FindPivots::Execute(g, labels, B, S, k);

    REQUIRE(result.W.size() > 2);

    for (int p : result.P) {
        REQUIRE((p == 0 || p == 1));
    }
}

TEST_CASE("FindPivots empty source set", "[find_pivots]") {
    Graph g = create_path_graph();
    Labels labels(5);

    int k = 3;
    long double B = 10.0;
    vector<int> S = {};

    auto result = FindPivots::Execute(g, labels, B, S, k);

    REQUIRE(result.P.empty());
    REQUIRE(result.W.empty());
}

TEST_CASE("FindPivots bounded relaxation", "[find_pivots]") {
    Graph g = create_path_graph();
    Labels labels(5);

    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 5;
    long double B = 2.5;
    vector<int> S = {0};

    auto result = FindPivots::Execute(g, labels, B, S, k);

    REQUIRE(result.W.size() <= 3);

    for (int v : result.W) {
        REQUIRE(labels.dist[v] < B);
    }
}

TEST_CASE("FindPivots lexicographic tie-breaking", "[find_pivots]") {
    Graph g(4);
    g[0].push_back({1, 1.0});
    g[0].push_back({2, 1.0});
    g[1].push_back({3, 1.0});
    g[2].push_back({3, 1.0});

    Labels labels(4);

    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 3;
    long double B = 10.0;
    vector<int> S = {0};

    auto result = FindPivots::Execute(g, labels, B, S, k);

    REQUIRE(result.W.size() == 4);
    REQUIRE(labels.pred[3] == 1);
}

TEST_CASE("FindPivots k-step limitation", "[find_pivots]") {
    Graph g = create_path_graph();
    Labels labels(5);

    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 2;
    long double B = 10.0;
    vector<int> S = {0};

    auto result = FindPivots::Execute(g, labels, B, S, k);

    REQUIRE(result.W.size() <= 3);
}

TEST_CASE("FindPivots disconnected source", "[find_pivots]") {
    Graph g(3);
    g[1].push_back({2, 1.0});

    Labels labels(3);

    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    int k = 3;
    long double B = 10.0;
    vector<int> S = {0};

    auto result = FindPivots::Execute(g, labels, B, S, k);

    REQUIRE(result.W.size() == 1);
    REQUIRE(result.W[0] == 0);
    REQUIRE(result.P.empty());
}
