#include <algorithm>
#include <expected>
#include <limits>
#include <queue>
#include <ranges>
#include <set>
#include <span>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <string>
#include <functional>

using namespace std;

struct Edge {
    int to;
    long double w;
};

using Graph = vector<vector<Edge>>;

static const long double INF_LD = numeric_limits<long double>::infinity();

struct DegreeReduction {
    vector<unordered_map<int, int>> slot_id;
    vector<int> rep_of_orig;
    Graph Gt;

    explicit DegreeReduction(int n = 0) {
        slot_id.resize(n);
        rep_of_orig.assign(n, -1);
    }

    void build(int n, span<const tuple<int, int, long double>> edges) {
        slot_id.assign(n, {});
        rep_of_orig.assign(n, -1);

        set<pair<int, int>> edge_set;
        for (const auto &e : edges) {
            int u, v;
            tie(u, v, ignore) = e;
            edge_set.insert({u, v});
            edge_set.insert({v, u});
        }

        int Np = 0;
        for (const auto &p : edge_set) {
            int u = p.first, v = p.second;
            if (slot_id[u].find(v) == slot_id[u].end()) {
                slot_id[u][v] = Np++;
            }
        }

        for (int v = 0; v < n; v++) {
            if (!slot_id[v].empty()) {
                // Choose a deterministic representative: smallest neighbor id
                int min_nbr = numeric_limits<int>::max();
                for (const auto &kv : slot_id[v]) {
                    if (kv.first < min_nbr) min_nbr = kv.first;
                }
                rep_of_orig[v] = slot_id[v][min_nbr];
            } else {
                slot_id[v][v] = Np++;
                rep_of_orig[v] = slot_id[v][v];
            }
        }

        Gt.assign(Np, {});

        for (int v = 0; v < n; v++) {
            if (slot_id[v].empty()) continue;

            vector<pair<int, int>> pairs;
            pairs.reserve(slot_id[v].size());
            for (const auto &kv : slot_id[v]) {
                pairs.emplace_back(kv.first, kv.second);
            }
            ranges::sort(pairs);

            int m = static_cast<int>(pairs.size());
            for (int i = 0; i < m; i++) {
                int from = pairs[i].second;
                int to = pairs[(i + 1) % m].second;
                Gt[from].push_back({to, 0.0L});
            }
        }

        for (const auto &e : edges) {
            int u, v;
            long double w;
            tie(u, v, w) = e;
            int xu = slot_id[u][v];
            int xv = slot_id[v][u];
            Gt[xu].push_back({xv, w});
        }
    }
};

[[nodiscard]] vector<long double> dijkstra(const Graph &g, int s) {
    const int n = static_cast<int>(g.size());
    vector<long double> dist(n, INF_LD);

    using Node = pair<long double, int>;
    priority_queue<Node, vector<Node>, greater<Node>> pq;

    dist[s] = 0.0L;
    pq.emplace(0.0L, s);

    while (!pq.empty()) {
        auto [du, u] = pq.top();
        pq.pop();
        if (du > dist[u]) continue;

        for (const auto &e : g[u]) {
            long double nd = du + e.w;
            if (nd < dist[e.to]) {
                dist[e.to] = nd;
                pq.emplace(nd, e.to);
            }
        }
    }
    return dist;
}

[[nodiscard]] expected<vector<long double>, string>
shortest_paths_original_graph(
    int n,
    span<const tuple<int, int, long double>> edges,
    int source) {
    if (n < 0) return unexpected("n must be non-negative");
    if (source < 0 || source >= n) return unexpected("invalid source index");

    bool has_negative = false;
    for (const auto &e : edges) {
        int u, v;
        long double w;
        tie(u, v, w) = e;
        if (u < 0 || u >= n || v < 0 || v >= n) {
            return unexpected("edge endpoint out of range");
        }
        if (w < 0) has_negative = true;
    }
    if (has_negative) return unexpected("negative edge weights are not allowed");

    DegreeReduction dr(n);
    dr.build(n, edges);
    const int s_rep = dr.rep_of_orig[source];

    vector<long double> dist_transformed = dijkstra(dr.Gt, s_rep);

    vector<long double> dist_orig(n, INF_LD);
    for (int v = 0; v < n; ++v) {
        long double best = INF_LD;
        for (const auto &kv : dr.slot_id[v]) {
            best = min(best, dist_transformed[kv.second]);
        }
        dist_orig[v] = best;
    }
    return dist_orig;
}
