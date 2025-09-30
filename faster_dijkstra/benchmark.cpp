#include <algorithm>
#include <chrono>
#include <cmath>
#include <expected>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <set>
#include <span>
#include <string>
#include <tuple>
#include <vector>

using namespace std;
using namespace std::chrono;

// External APIs implemented in dijkstra.cpp and Duan_deterministic_sssp.cpp
expected<vector<long double>, string>
shortest_paths_original_graph(int n, span<const tuple<int,int,long double>> edges, int source);

expected<vector<long double>, string>
duan_shortest_paths_original_graph(int n, span<const tuple<int,int,long double>> edges, int source, bool finalize_pass);

static const long double INF_LD = numeric_limits<long double>::infinity();

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <num_vertices> <num_edges>" << '\n';
        return 1;
    }

    int n = atoi(argv[1]);
    long long m_target = atoll(argv[2]);
    if (n <= 0 || m_target < 0) {
        cerr << "Invalid parameters." << '\n';
        return 1;
    }

    cout << "--- Test Parameters ---\n";
    cout << "Target Vertices: " << n << '\n';
    cout << "Target Edges:    " << m_target << '\n';

    vector<tuple<int, int, long double>> edges;
    edges.reserve((size_t)m_target);
    mt19937 rng(12345);
    uniform_int_distribution<> node_dist(0, n - 1);
    uniform_real_distribution<long double> weight_dist(1.0L, 1000.0L);
    set<pair<int,int>> edge_set;

    if (m_target <= (long long)n * 2 && m_target > n - 1) {
        cout << "Generating special 'path-like' sparse graph...\n";
        for (int i = 0; i < n - 1; ++i) edge_set.insert({i, i + 1});
        while ((long long)edge_set.size() < m_target) {
            int u = node_dist(rng), v = node_dist(rng);
            if (u != v) edge_set.insert({u, v});
        }
    } else {
        cout << "Generating standard random graph...\n";
        while ((long long)edge_set.size() < m_target && edge_set.size() < (size_t)n * (n - 1)) {
            int u = node_dist(rng), v = node_dist(rng);
            if (u != v) edge_set.insert({u, v});
        }
    }

    for (const auto& e : edge_set) edges.emplace_back(e.first, e.second, weight_dist(rng));

    int m = (int)edges.size();
    int s = 0;

    cout << "\n--- Generated Graph ---\n";
    cout << "Actual Vertices: " << n << '\n';
    cout << "Actual Edges:    " << m << '\n';

    long double log_n = log2l((long double)max(2, n));
    long double m_crit = (long double)n * powl(log_n, 1.0L/3.0L);
    cout << "\n--- Asymptotic Analysis ---\n";
    cout << fixed << setprecision(2);
    cout << "Theoretical Crossover m_crit: ~" << m_crit << " (for n=" << n << ")\n";

    cout << "\n--- Running Duan et al. (deterministic) ---\n";
    auto t1 = high_resolution_clock::now();
    auto duan_res = duan_shortest_paths_original_graph(n, span(edges), s, /*finalize_pass=*/true);
    auto t2 = high_resolution_clock::now();
    if (!duan_res) {
        cerr << "Duan API error: " << duan_res.error() << '\n';
        return 1;
    }
    auto duan_ms = duration_cast<milliseconds>(t2 - t1);

    cout << "\n--- Running Dijkstra (baseline) ---\n";
    auto t3 = high_resolution_clock::now();
    auto dij_res = shortest_paths_original_graph(n, span(edges), s);
    auto t4 = high_resolution_clock::now();
    if (!dij_res) {
        cerr << "Dijkstra API error: " << dij_res.error() << '\n';
        return 1;
    }
    auto dij_ms = duration_cast<milliseconds>(t4 - t3);

    cout << "\n--- Verification ---\n";
    const auto& a = duan_res.value();
    const auto& b = dij_res.value();
    if (a.size() != (size_t)n || b.size() != (size_t)n) {
        cerr << "Result size mismatch." << '\n';
        return 1;
    }
    bool ok = true;
    for (int i = 0; i < n; ++i) {
        long double x = a[i], y = b[i];
        if (x == INF_LD && y == INF_LD) continue;
        if (fabsl(x - y) > 1e-9L) { ok = false; break; }
    }
    cout << (ok ? "SUCCESS: Results match." : "FAILURE: Results differ.") << '\n';

    cout << "\n--- Final Summary ---\n";
    cout << "Duan (deterministic): " << duan_ms.count() << " ms\n";
    cout << "Dijkstra (baseline): " << dij_ms.count() << " ms\n";

    return ok ? 0 : 2;
}

