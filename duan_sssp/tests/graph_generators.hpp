#ifndef GRAPH_GENERATORS_HPP
#define GRAPH_GENERATORS_HPP

#include "../include/duan_sssp.hpp"
#include <vector>
#include <random>
#include <cmath>

namespace duan {
namespace test {

inline bool approx_equal(long double a, long double b, long double epsilon = 1e-9) {
    return std::abs(a - b) < epsilon;
}

inline Graph create_path_graph() {
    // 0 -> 1 -> 2 -> 3 -> 4
    Graph g(5);
    g[0].push_back({1, 1.0});
    g[1].push_back({2, 1.0});
    g[2].push_back({3, 1.0});
    g[3].push_back({4, 1.0});
    return g;
}

inline Graph create_path_graph(int n) {
    Graph g(n);
    for (int i = 0; i < n - 1; ++i) {
        g[i].push_back({i + 1, 1.0});
    }
    return g;
}

inline Graph create_star_graph(int n_leaves) {
    Graph g(n_leaves + 1);
    for (int i = 1; i <= n_leaves; ++i) {
        g[0].push_back({i, 1.0});
    }
    return g;
}

inline Graph create_diamond_graph() {
    // 0 -> 1 -> 3
    // 0 -> 2 -> 3
    Graph g(4);
    g[0].push_back({1, 1.0});
    g[0].push_back({2, 1.0});
    g[1].push_back({3, 1.0});
    g[2].push_back({3, 1.0});
    return g;
}

inline Graph create_grid_graph(int rows, int cols) {
    int n = rows * cols;
    Graph g(n);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int u = r * cols + c;
            if (c + 1 < cols) g[u].push_back({u + 1, 1.0});
            if (r + 1 < rows) g[u].push_back({u + cols, 1.0});
        }
    }
    return g;
}

inline Graph create_sparse_graph(int n, int m) {
    Graph g(n);
    std::mt19937 gen(42);
    std::uniform_int_distribution<> dis_node(0, n - 1);
    std::uniform_real_distribution<long double> dis_weight(1.0, 10.0);
    for (int i = 0; i < m; ++i) {
        int u = dis_node(gen);
        int v = dis_node(gen);
        if (u != v) {
            g[u].push_back({v, dis_weight(gen)});
        }
    }
    return g;
}

inline Graph create_sparse_graph(int n, int m, std::mt19937& gen) {
    Graph g(n);
    std::uniform_int_distribution<> dis_node(0, n - 1);
    std::uniform_real_distribution<long double> dis_weight(1.0, 10.0);
    for (int i = 0; i < m; ++i) {
        int u = dis_node(gen);
        int v = dis_node(gen);
        if (u != v) {
            g[u].push_back({v, dis_weight(gen)});
        }
    }
    return g;
}

} // namespace test
} // namespace duan

#endif // GRAPH_GENERATORS_HPP
