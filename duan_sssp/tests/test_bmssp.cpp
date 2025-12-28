/**
 * Unit tests for BMSSP algorithm
 */

#include "../include/algorithms/bmssp.hpp"
#include "../test_helpers/graph_generators.hpp"
#include "../test_helpers/test_utils.hpp"
#include <catch_amalgamated.hpp>
#include <algorithm>

using namespace duan;
using namespace duan::test;

TEST_CASE("BMSSP layer 0 calls BaseCase", "[bmssp]") {
    Graph g = create_path_graph();
    Labels labels(5);

    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    Params params = Params::compute(5);
    int l = 0;
    long double B = 10.0;
    vector<int> S = {0};

    auto result = BMSSP::Execute(g, labels, l, B, S, params);

    REQUIRE(result.U.size() > 0);
    REQUIRE(result.b <= B);

    for (int v : result.U) {
        REQUIRE(labels.dist[v] < result.b);
    }
}

TEST_CASE("BMSSP layer 1 path", "[bmssp]") {
    Graph g = create_path_graph();
    Labels labels(5);

    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    Params params = Params::compute(5);
    int l = 1;
    long double B = 10.0;
    vector<int> S = {0};

    auto result = BMSSP::Execute(g, labels, l, B, S, params);

    REQUIRE(result.U.size() > 0);
    REQUIRE(labels.dist[0] == 0.0);
}

TEST_CASE("BMSSP star graph", "[bmssp]") {
    Graph g = create_star_graph(10);
    Labels labels(11);

    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    Params params = Params::compute(11);
    int l = 1;
    long double B = 10.0;
    vector<int> S = {0};

    auto result = BMSSP::Execute(g, labels, l, B, S, params);

    REQUIRE(result.U.size() > 0);
    REQUIRE(labels.dist[0] == 0.0);

    for (int i = 1; i <= 10; ++i) {
        if (labels.dist[i] < INF) {
            REQUIRE(approx_equal(labels.dist[i], 1.0));
        }
    }
}

TEST_CASE("BMSSP bounded search", "[bmssp]") {
    Graph g = create_path_graph();
    Labels labels(5);

    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    Params params = Params::compute(5);
    int l = 1;
    long double B = 2.5;
    vector<int> S = {0};

    auto result = BMSSP::Execute(g, labels, l, B, S, params);

    REQUIRE(result.b <= B);
    for (int v : result.U) {
        REQUIRE(labels.dist[v] < result.b);
    }
    REQUIRE_FALSE(result.U.empty());

    for (int v : result.U) {
        REQUIRE(labels.dist[v] < INF);
    }
}

TEST_CASE("BMSSP diamond graph", "[bmssp]") {
    Graph g = create_diamond_graph();
    Labels labels(4);

    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    Params params = Params::compute(4);
    int l = 1;
    long double B = 10.0;
    vector<int> S = {0};

    auto result = BMSSP::Execute(g, labels, l, B, S, params);

    REQUIRE_FALSE(result.U.empty());

    if (labels.dist[0] < INF) REQUIRE(approx_equal(labels.dist[0], 0.0));
    if (labels.dist[1] < INF) REQUIRE(approx_equal(labels.dist[1], 1.0));
    if (labels.dist[2] < INF) REQUIRE(approx_equal(labels.dist[2], 1.0));
    if (labels.dist[3] < INF) REQUIRE(approx_equal(labels.dist[3], 2.0));
}

TEST_CASE("BMSSP weighted path", "[bmssp]") {
    Graph g(4);
    g[0].push_back({1, 1.5});
    g[1].push_back({2, 2.0});
    g[2].push_back({3, 1.0});

    Labels labels(4);

    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    Params params = Params::compute(4);
    int l = 1;
    long double B = 10.0;
    vector<int> S = {0};

    auto result = BMSSP::Execute(g, labels, l, B, S, params);

    REQUIRE_FALSE(result.U.empty());
    if (labels.dist[0] < INF) REQUIRE(approx_equal(labels.dist[0], 0.0));
    if (labels.dist[1] < INF) REQUIRE(approx_equal(labels.dist[1], 1.5));
    if (labels.dist[2] < INF) REQUIRE(approx_equal(labels.dist[2], 3.5));
    if (labels.dist[3] < INF) REQUIRE(approx_equal(labels.dist[3], 4.5));
}

TEST_CASE("BMSSP multiple layers", "[bmssp]") {
    Graph g = create_star_graph(20);
    Labels labels(21);

    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    Params params = Params::compute(21);
    int l = 2;
    long double B = 10.0;
    vector<int> S = {0};

    auto result = BMSSP::Execute(g, labels, l, B, S, params);

    REQUIRE(result.U.size() > 0);

    for (int i = 1; i <= 20; ++i) {
        if (labels.dist[i] < INF) {
            REQUIRE(approx_equal(labels.dist[i], 1.0));
        }
    }
}

TEST_CASE("BMSSP disconnected graph", "[bmssp]") {
    Graph g(4);
    g[0].push_back({1, 1.0});
    g[2].push_back({3, 1.0});

    Labels labels(4);

    labels.dist[0] = 0.0;
    labels.hops[0] = 0;

    Params params = Params::compute(4);
    int l = 1;
    long double B = 10.0;
    vector<int> S = {0};

    auto result = BMSSP::Execute(g, labels, l, B, S, params);

    REQUIRE(approx_equal(labels.dist[0], 0.0));
    REQUIRE(approx_equal(labels.dist[1], 1.0));
    REQUIRE(labels.dist[2] == INF);
    REQUIRE(labels.dist[3] == INF);
}
