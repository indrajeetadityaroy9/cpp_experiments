#include <bits/stdc++.h>
#include <iostream>
#include <vector>
#include <queue>
#include <limits>
#include <stdexcept>
#include <utility>
using namespace std;

struct Edge { int to; long double w; };
using Graph = vector<vector<Edge>>;

static const long double INF_LD = numeric_limits<long double>::infinity();

struct Labels {
    vector<long double> db;  // current upper bound distances (non-increasing updates)
    vector<int> pred; // predecessor to maintain the shortest-path tree
    vector<int> hops; // #edges on the chosen path for lexicographic tie-breaking
    Labels(int n=0) { reset(n); }
    void reset(int n) { db.assign(n, INF_LD); pred.assign(n, -1); hops.assign(n, INT_MAX/2); }
};


inline bool lex_better(int u, int old_pred, int new_hops, int old_hops) {
    if (new_hops != old_hops) return new_hops < old_hops;
    return u < old_pred;
}

inline bool relax_edge(int u, int v, long double w, Labels &L) {
    long double nd = L.db[u] + w;
    if (nd < L.db[v]) {
        L.db[v] = nd; L.pred[v] = u; L.hops[v] = L.hops[u] + 1;
        return true;
    } else if (nd == L.db[v]) { //comparison-addition
        int nh = L.hops[u] + 1;
        if (lex_better(u, L.pred[v], nh, L.hops[v])) {
            L.pred[v] = u; L.hops[v] = nh;
            return true;
        }
    }
    return false;
}

// Constant-degree reduction (Preliminaries)
// Transform each original vertex v into a zero-weight directed cycle of size deg(v),
// with a node x_{v,w} for every neighbor w (incoming or outgoing).
// For each original edge (u->v, w), add a directed edge x_{u,v} -> x_{v,u} of weight w.
// Result has in/out-degree ≤ 2 and O(m) vertices/edges. (Preserves shortest paths.)
struct DegreeReduction {
    vector<unordered_map<int,int>> slot_id;
    vector<int> rep_of_orig;
    Graph Gt;            

    DegreeReduction(int n=0) { slot_id.resize(n); rep_of_orig.assign(n,-1); }

    // edges is vector of (u,v,w).
    void build(int n, const vector<tuple<int,int,long double>>& edges, int s_orig) {
        // Gather neighbor sets per vertex (incoming or outgoing)
        vector<unordered_set<int>> nbr(n);
        for (auto &e: edges) {
            int u,v; long double w;
            tie(u,v,w) = e;
            nbr[u].insert(v);
            nbr[v].insert(u);
        }
        int Np = 0;
        slot_id.assign(n, {});
        rep_of_orig.assign(n,-1);
        for (int v=0; v<n; ++v) {
            if (nbr[v].empty()) {
                // isolated
                // create a single slot pointing to itself
                slot_id[v][v] = Np++;
                rep_of_orig[v] = slot_id[v][v];
            } else {
                for (int w: nbr[v]) {
                    slot_id[v][w] = Np++;
                }
                // representative
                rep_of_orig[v] = slot_id[v].begin()->second;
            }
        }
        Gt.assign(Np, {});
        // Add zero-weight directed cycles per original vertex v
        for (int v=0; v<n; ++v) {
            // collect
            vector<int> cyc;
            cyc.reserve(slot_id[v].size());
            for (auto &kv: slot_id[v]) cyc.push_back(kv.second);
            if (cyc.empty()) continue;
            // make a directed cycle (strongly connected)
            int m = (int)cyc.size();
            if (m==1) {
                // self-loop zero-weight
                Gt[cyc[0]].push_back({cyc[0], 0.0L});
            } else {
                for (int i=0;i<m;i++) {
                    int a = cyc[i], b = cyc[(i+1)%m];
                    Gt[a].push_back({b, 0.0L});
                }
            }
        }
        // Add transformed edges
        for (auto &e: edges) {
            int u,v; long double w;
            tie(u,v,w) = e;
            int xu = slot_id[u][v];
            int xv = slot_id[v][u];
            Gt[xu].push_back({xv, w});
        }
    }
};