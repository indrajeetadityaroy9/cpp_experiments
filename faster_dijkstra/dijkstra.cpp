#include <iostream>
#include <vector>
#include <queue>
#include <limits>
#include <tuple>
#include <unordered_map>
#include <set>
#include <algorithm>
using namespace std;

struct Edge { int to; long double w; };
using Graph = vector<vector<Edge>>;

static const long double INF_LD = numeric_limits<long double>::infinity();

struct DegreeReduction {
    vector<unordered_map<int,int>> slot_id;
    vector<int> rep_of_orig;
    Graph Gt;                

    DegreeReduction(int n=0) { slot_id.resize(n); rep_of_orig.assign(n,-1); }

    void build(int n, const vector<tuple<int,int,long double>>& edges, int s_orig) {
        slot_id.assign(n, {});
        rep_of_orig.assign(n, -1);
        
        set<pair<int,int>> edge_set;
        for (const auto& e : edges) {
            int u, v;
            tie(u, v, ignore) = e;
            edge_set.insert({u, v});
            edge_set.insert({v, u});
        }
        
        int Np = 0;
        for (const auto& p : edge_set) {
            int u = p.first, v = p.second;
            if (slot_id[u].find(v) == slot_id[u].end()) {
                slot_id[u][v] = Np++;
            }
        }

        for (int v = 0; v < n; v++) {
            if (!slot_id[v].empty()) {
                int min_nbr = slot_id[v].begin()->first;
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
            for (const auto& kv : slot_id[v]) {
                pairs.push_back({kv.first, kv.second});
            }
            sort(pairs.begin(), pairs.end());

            int m = (int)pairs.size();
            for (int i = 0; i < m; i++) {
                int from = pairs[i].second;
                int to = pairs[(i + 1) % m].second;
                Gt[from].push_back({to, 0.0L});
            }
        }
        
        for (const auto& e : edges) {
            int u, v; long double w;
            tie(u, v, w) = e;
            int xu = slot_id[u][v];
            int xv = slot_id[v][u];
            Gt[xu].push_back({xv, w});
        }
    }
};

vector<long double> dijkstra(const Graph& g, int s) {
    const int n = (int)g.size();
    vector<long double> dist(n, INF_LD);
    
    using Node = pair<long double,int>;
    priority_queue<Node, vector<Node>, greater<Node>> pq;
    
    dist[s] = 0.0L;
    pq.emplace(0.0L, s);
    
    while (!pq.empty()) {
        auto [du, u] = pq.top(); pq.pop();
        if (du > dist[u]) continue;
        
        for (const auto& e : g[u]) {
            long double nd = du + e.w;
            if (nd < dist[e.to]) {
                dist[e.to] = nd;
                pq.emplace(nd, e.to);
            }
        }
    }
    return dist;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, m, s;
    if (!(cin >> n >> m >> s)) return 0;
    vector<tuple<int,int,long double>> edges; edges.reserve(m);
    bool has_negative = false;
    for (int i=0;i<m;i++) {
        int u,v; long double w; cin >> u >> v >> w;
        if (w < 0) has_negative = true;
        edges.emplace_back(u, v, w);
    }
    if (has_negative) {
        cerr << "Error: negative edge weights are not allowed.\n";
        return 1;
    }

    DegreeReduction DR(n);
    DR.build(n, edges, s);
    int s_rep = DR.rep_of_orig[s];

    vector<long double> dist_transformed = dijkstra(DR.Gt, s_rep);

    vector<long double> dist_orig(n, INF_LD);
    for (int v=0; v<n; ++v) {
        long double best = INF_LD;
        for (auto &kv: DR.slot_id[v]) {
            best = min(best, dist_transformed[kv.second]);
        }
        dist_orig[v] = best;
    }

    cout.setf(std::ios::fixed);
    cout.precision(12);
    for (int v=0; v<n; ++v) {
        if (dist_orig[v] == INF_LD) cout << "INF\n";
        else cout << dist_orig[v] << "\n";
    }
    return 0;
}
