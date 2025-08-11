#include <iostream>
#include <vector>
#include <queue>
#include <limits>
#include <stdexcept>
#include <utility>

struct Edge {
    int to;
    long double w;
};

using Graph = std::vector<std::vector<Edge>>;

struct DijkstraResult {
    std::vector<long double> dist;
    std::vector<int> parent;
};

DijkstraResult dijkstra(const Graph& g, int s) {
    const int n = (int)g.size();
    if (s < 0 || s >= n) throw std::out_of_range("source out of range");

    const long double INF = std::numeric_limits<long double>::infinity();

    std::vector<long double> dist(n, INF);
    std::vector<int> parent(n, -1);

    using Node = std::pair<long double,int>;
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> pq;

    dist[s] = 0.0L;
    pq.emplace(0.0L, s);

    while (!pq.empty()) {
        auto [du, u] = pq.top(); pq.pop();
        if (du > dist[u]) continue;

        for (const auto& e : g[u]) {
            long double nd = du + e.w;
            if (nd < dist[e.to]) {
                dist[e.to] = nd;
                parent[e.to] = u;
                pq.emplace(nd, e.to);
            }
        }
    }
    return {std::move(dist), std::move(parent)};
}

std::vector<int> reconstruct_path(int s, int t, const std::vector<int>& parent) {
    std::vector<int> path;
    if (t < 0 || t >= (int)parent.size()) return path;
    for (int v = t; v != -1; v = parent[v]) path.push_back(v);
    for (int i = 0, j = (int)path.size() - 1; i < j; ++i, --j) std::swap(path[i], path[j]);
    if (path.empty() || path.front() != s) path.clear();
    return path;
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    int n, m, s;
    if (!(std::cin >> n >> m >> s)) return 0;

    Graph g(n);
    for (int i = 0; i < m; ++i) {
        int u, v;
        long double w;
        std::cin >> u >> v >> w;
        if (u < 0 || u >= n || v < 0 || v >= n) {
            throw std::out_of_range("edge endpoint out of range");
        }
        if (w < 0) {
            throw std::invalid_argument("negative edge weight not allowed for Dijkstra");
        }
        g[u].push_back({v, w});
    }

    auto res = dijkstra(g, s);

    const long double INF = std::numeric_limits<long double>::infinity();
    for (int i = 0; i < n; ++i) {
        if (res.dist[i] == INF) {
            std::cout << "INF\n";
        } else {
            std::cout.setf(std::ios::fixed); 
            std::cout.precision(12);
            std::cout << res.dist[i] << "\n";
        }
    }
    return 0;
}
