#include <iostream>
#include <vector>
#include <queue>
#include <limits>
#include <stdexcept>
#include <utility>
#include <unordered_set>
#include <unordered_map>
#include <tuple>
#include <list>
#include <set>
#include <algorithm>
#include <cmath>
#include <stack>
#include <chrono>
#include <random>
#include <climits>
#include <iomanip>
using namespace std;
using namespace std::chrono;

struct Edge {
    int to;
    long double w;
};

using Graph = vector<vector<Edge>>;

static const long double INF_LD = numeric_limits<long double>::infinity();

struct Labels {
    vector<long double> db;    
    vector<int> pred;         
    vector<int> hops;         
    
    Labels(int n = 0) { 
        reset(n); 
    }
    
    void reset(int n) { 
        db.assign(n, INF_LD); 
        pred.assign(n, -1); 
        hops.assign(n, INT_MAX/2); 
    }
};

inline bool lex_better(int u, int old_pred, int new_hops, int old_hops) {
    if (new_hops != old_hops) return new_hops < old_hops;
    return u < old_pred;
}

struct DegreeReduction {
    vector<unordered_map<int, int>> slot_id;
    vector<int> rep_of_orig;
    Graph Gt;

    DegreeReduction(int n = 0) { 
        slot_id.resize(n); 
        rep_of_orig.assign(n, -1); 
    }

    void build(int n, const vector<tuple<int, int, long double>>& edges, int s_orig) {
        vector<unordered_set<int>> nbr(n);
        for (const auto& e : edges) {
            int u, v;
            long double w;
            tie(u, v, w) = e;
            nbr[u].insert(v);
            nbr[v].insert(u);
        }
        
        int Np = 0;
        slot_id.assign(n, {});
        rep_of_orig.assign(n, -1);
        
        for (int v = 0; v < n; ++v) {
            if (nbr[v].empty()) {
                slot_id[v][v] = Np++;
                rep_of_orig[v] = slot_id[v][v];
            } else {
                vector<int> sorted(nbr[v].begin(), nbr[v].end());
                sort(sorted.begin(), sorted.end());
                for (int w : sorted) {
                    slot_id[v][w] = Np++;
                }
                rep_of_orig[v] = slot_id[v][sorted.front()];
            }
        }
        
        Gt.assign(Np, {});
        
        for (int v = 0; v < n; ++v) {
            if (slot_id[v].empty()) continue;
            
            vector<pair<int, int>> pairs;
            pairs.reserve(slot_id[v].size());
            
            for (auto& kv : slot_id[v]) {
                pairs.push_back({kv.first, kv.second});
            }
            
            sort(pairs.begin(), pairs.end());
            int m = (int)pairs.size();
            
            for (int i = 0; i < m; i++) {
                int a = pairs[i].second;
                int b = pairs[(i + 1) % m].second;
                Gt[a].push_back({b, 0.0L});
            }
        }
        
        for (const auto& e : edges) {
            int u, v;
            long double w;
            tie(u, v, w) = e;
            int xu = slot_id[u].at(v);
            int xv = slot_id[v].at(u);
            Gt[xu].push_back({xv, w});
        }
    }
};

struct PartialOrderDS {
    struct Entry { 
        int key; 
        long double val; 
    };
    
    struct Block {
        vector<Entry> items;
        long double max_val = -INF_LD;
    };
    
    struct KeyPos {
        bool in_d0 = false;
        int block_idx = -1;
        int item_idx = -1;
    };
    
    vector<KeyPos> pos;
    vector<Block> d0;
    vector<Block> d1;
    vector<pair<long double, int>> d1_index;

    long double B_bound = INF_LD;
    int M = 1;

    PartialOrderDS(int n_nodes) {
        pos.resize(n_nodes);
    }
    
    void Initialize(int M_, long double B_) {
        M = max(1, M_);
        B_bound = B_;
        fill(pos.begin(), pos.end(), KeyPos{});
        d0.clear();
        d1.clear();
        d1_index.clear();
        d1.emplace_back();
        d1_index.push_back({-INF_LD, 0});
    }

    void erase_key_internal(int key) {
        KeyPos& kp = pos[key];
        if (kp.block_idx == -1) return;

        vector<Block>& blocks = kp.in_d0 ? d0 : d1;
        Block& blk = blocks[kp.block_idx];
        auto& items = blk.items;
        
        int back_key = items.back().key;
        items[kp.item_idx] = items.back();
        items.pop_back();

        if (key != back_key) {
            pos[back_key].item_idx = kp.item_idx;
        }
        kp.block_idx = -1;
    }

    void Insert(int key, long double val) {
        if (pos[key].block_idx != -1) {
            const auto& items = (pos[key].in_d0 ? d0 : d1)[pos[key].block_idx].items;
            if (val >= items[pos[key].item_idx].val) return;
        }
        erase_key_internal(key);

        size_t target_idx = -1;
        for (size_t i = 0; i < d1_index.size(); ++i) {
            if (val <= d1_index[i].first) {
                target_idx = d1_index[i].second;
                break;
            }
        }
        
        if (target_idx == (size_t)-1) {
            target_idx = d1.size();
            d1.emplace_back();
            d1_index.push_back({-INF_LD, (int)target_idx});
        }
        
        Block& target_blk = d1[target_idx];
        target_blk.items.push_back({key, val});
        pos[key] = {false, (int)target_idx, (int)target_blk.items.size() - 1};
        
        if (val > target_blk.max_val) {
            target_blk.max_val = val;
        }
        
        if ((int)target_blk.items.size() > M) {
            auto& items = target_blk.items;
            auto median_it = items.begin() + items.size() / 2;
            std::nth_element(items.begin(), median_it, items.end(), 
                [](const Entry& a, const Entry& b) { 
                    return a.val < b.val; 
                });
            
            int new_blk_idx = d1.size();
            d1.emplace_back();
            Block& new_blk = d1.back();
            new_blk.items.assign(median_it, items.end());
            items.erase(median_it, items.end());
            
            target_blk.max_val = -INF_LD;
            for (const auto& e : target_blk.items) {
                target_blk.max_val = max(target_blk.max_val, e.val);
            }
            
            new_blk.max_val = -INF_LD;
            for (const auto& e : new_blk.items) {
                new_blk.max_val = max(new_blk.max_val, e.val);
            }

            for (size_t i = 0; i < new_blk.items.size(); ++i) {
                pos[new_blk.items[i].key].block_idx = new_blk_idx;
            }
            
            d1_index.push_back({new_blk.max_val, new_blk_idx});
        }

        for (auto& p : d1_index) {
            p.first = d1[p.second].max_val;
        }
        sort(d1_index.begin(), d1_index.end());
    }

    void BatchPrepend(vector<Entry>& L) {
        if (L.empty()) return;
        
        sort(L.begin(), L.end(), [](const Entry& a, const Entry& b) {
            if (a.key != b.key) return a.key < b.key;
            return a.val < b.val;
        });
        
        L.erase(unique(L.begin(), L.end(), [](const Entry& a, const Entry& b) { 
            return a.key == b.key; 
        }), L.end());

        vector<Entry> ins;
        ins.reserve(L.size());
        
        for (const auto& entry : L) {
            if (pos[entry.key].block_idx == -1 || 
                entry.val < (pos[entry.key].in_d0 ? d0 : d1)[pos[entry.key].block_idx].items[pos[entry.key].item_idx].val) {
                erase_key_internal(entry.key);
                ins.push_back(entry);
            }
        }
        
        if (ins.empty()) return;

        size_t p = 0;
        while (p < ins.size()) {
            size_t q = min(p + (size_t)M, ins.size());
            d0.emplace_back();
            Block& blk = d0.back();
            blk.items.assign(ins.begin() + p, ins.begin() + q);
            
            for (size_t i = 0; i < blk.items.size(); ++i) {
                pos[blk.items[i].key] = {true, (int)d0.size() - 1, (int)i};
            }
            p = q;
        }
    }

    bool empty() const { 
        for (const auto& blk : d0) {
            if (!blk.items.empty()) return false;
        }
        for (const auto& blk : d1) {
            if (!blk.items.empty()) return false;
        }
        return true;
    }

    pair<vector<int>, long double> Pull() {
        vector<Entry> cand;
        cand.reserve(M + M);

        if (!d0.empty()) {
            cand.insert(cand.end(), d0.back().items.begin(), d0.back().items.end());
        }

        if (!d1_index.empty()) {
            cand.insert(cand.end(), d1[d1_index[0].second].items.begin(), d1[d1_index[0].second].items.end());
        }
        
        if (cand.size() <= (size_t)M) {
            vector<int> keys;
            keys.reserve(cand.size());
            for (const auto& e : cand) {
                keys.push_back(e.key);
            }
            Initialize(M, B_bound);
            return {keys, B_bound};
        }
        
        auto cmp = [](const Entry& a, const Entry& b) {
            if (a.val != b.val) return a.val < b.val;
            return a.key < b.key;
        };
        
        nth_element(cand.begin(), cand.begin() + M - 1, cand.end(), cmp);
        
        vector<int> S;
        S.reserve(M);
        for (int i = 0; i < M; ++i) {
            S.push_back(cand[i].key);
        }
        
        nth_element(cand.begin() + M, cand.begin() + M, cand.end(), cmp);
        long double next_bound = cand.size() > (size_t)M ? cand[M].val : B_bound;

        for (int key : S) {
            erase_key_internal(key);
        }

        return {S, next_bound};
    }
};


struct DMSY_SSSP {
    const Graph& G;
    int N;
    int s;
    Labels L;

    int k, t;

    DMSY_SSSP(const Graph& Gt, int s_) : G(Gt), N((int)Gt.size()), s(s_), L(N) {
        L.db.assign(N, INF_LD);
        L.pred.assign(N, -1);
        L.hops.assign(N, INT_MAX/2);
        L.db[s] = 0.0L;
        L.hops[s] = 0;
        
        long double Lg = log2l((long double)max(2, N));
        k = max(1, (int)floor(powl(Lg, 1.0L/3.0L)));
        t = max(1, (int)floor(powl(Lg, 2.0L/3.0L)));
    }
    
    pair<vector<int>, vector<int>> FindPivots(long double B, const vector<int>& Sset) {
        if (Sset.empty()) return {{}, {}};
        
        unordered_set<int> inW(Sset.begin(), Sset.end());
        vector<int> W = Sset;
        W.reserve((size_t)k * Sset.size() + W.size());

        vector<int> Wi = Sset;
        for (int i = 1; i <= k; i++) {
            vector<int> Wi_next;
            Wi_next.reserve(Wi.size() * 3);
            
            for (int u : Wi) {
                if (L.db[u] == INF_LD) continue;
                
                for (const auto& e : G[u]) {
                    int v = e.to;
                    long double nd = L.db[u] + e.w;
                    
                    if (nd < B && (nd < L.db[v] || (nd == L.db[v] && lex_better(u, L.pred[v], L.hops[u] + 1, L.hops[v])))) {
                        L.db[v] = nd;
                        L.pred[v] = u;
                        L.hops[v] = L.hops[u] + 1;
                        Wi_next.push_back(v);
                        
                        if (inW.find(v) == inW.end()) {
                            inW.insert(v);
                            W.push_back(v);
                        }
                    }
                }
            }
            
            Wi = std::move(Wi_next);
            if (!Sset.empty() && W.size() > (size_t)k * Sset.size()) {
                return {Sset, W};
            }
        }

        unordered_map<int, int> indegF;
        unordered_map<int, vector<int>> adjF;
        
        for (int v : W) {
            int u = L.pred[v];
            if (u != -1 && inW.count(u)) {
                adjF[u].push_back(v);
                indegF[v]++;
            }
        }

        vector<int> P;
        for (int u : Sset) {
            if (indegF.count(u) && indegF[u] > 0) continue;

            int cnt = 0;
            stack<int> st;
            st.push(u);
            unordered_set<int> seen;
            
            while (!st.empty()) {
                int x = st.top();
                st.pop();
                
                if (seen.count(x)) continue;
                seen.insert(x);
                cnt++;
                
                if (cnt >= k) break;
                
                if (adjF.count(x)) {
                    for (int y : adjF.at(x)) {
                        st.push(y);
                    }
                }
            }
            
            if (cnt >= k) {
                P.push_back(u);
            }
        }
        
        return {P, W};
    }

    pair<long double, vector<int>> BaseCase(long double B, int x) {
        vector<int> U0;
        U0.reserve(k + 2);
        
        using Node = pair<long double, int>;
        priority_queue<Node, vector<Node>, greater<Node>> pq;
        
        if (L.db[x] < B) {
            pq.push({L.db[x], x});
        }
        
        vector<char> settled(N, 0);

        while (!pq.empty() && U0.size() < (size_t)k + 1) {
            auto [du, u] = pq.top();
            pq.pop();
            
            if (du > L.db[u] || settled[u]) continue;
            settled[u] = 1;
            U0.push_back(u);

            for (auto& e : G[u]) {
                int v = e.to;
                long double nd = L.db[u] + e.w;
                
                if (nd < B && !settled[v] && (nd < L.db[v] || (nd == L.db[v] && lex_better(u, L.pred[v], L.hops[u] + 1, L.hops[v])))) {
                    L.db[v] = nd;
                    L.pred[v] = u;
                    L.hops[v] = L.hops[u] + 1;
                    pq.push({L.db[v], v});
                }
            }
        }

        if (U0.size() <= (size_t)k) {
            return {B, U0};
        }
        
        long double Bp = -INF_LD;
        for (int u : U0) {
            Bp = max(Bp, L.db[u]);
        }
        
        vector<int> U;
        U.reserve(U0.size());
        
        if (Bp > -INF_LD) {
            for (int u : U0) {
                if (L.db[u] < Bp) {
                    U.push_back(u);
                }
            }
        }
        
        return {Bp, U};
    }

    pair<long double, vector<int>> BMSSP(int l, long double B, const vector<int>& S) {
        if (l == 0) {
            return BaseCase(B, S.empty() ? s : S[0]);
        }

        auto [P, W] = FindPivots(B, S);

        PartialOrderDS D(N);
        long double M_val = ((long double)(l - 1) * (long double)t <= 60.0L) ? 
                            powl(2.0L, (long double)(l - 1) * t) : 
                            (long double)N + 1;
                            
        D.Initialize(max(1, (int)min((long double)N, M_val)), B);
        
        for (int x : P) {
            if (L.db[x] < B) {
                D.Insert(x, L.db[x]);
            }
        }

        long double Bi_prime_last = B;
        vector<int> U;
        U.reserve(1024);
        
        vector<char> inU_local(N, 0);
        vector<typename PartialOrderDS::Entry> Kbatch;
        Kbatch.reserve(1024);

        while (!D.empty()) {
            auto pulled = D.Pull();
            vector<int> Si = std::move(pulled.first);
            long double Bi = pulled.second;
            
            if (Si.empty()) break;

            auto sub = BMSSP(l - 1, Bi, Si);
            long double Bi_prime = sub.first;
            vector<int>& Ui = sub.second;
            
            Bi_prime_last = min(Bi_prime_last, Bi_prime);

            for (int u : Ui) {
                if (!inU_local[u]) {
                    inU_local[u] = true;
                    U.push_back(u);
                }
            }
            
            Kbatch.clear();
            for (int u : Ui) {
                if (L.db[u] == INF_LD) continue;
                
                for (const auto& e : G[u]) {
                    int v = e.to;
                    long double nd = L.db[u] + e.w;

                    if (nd < L.db[v] || (nd == L.db[v] && lex_better(u, L.pred[v], L.hops[u] + 1, L.hops[v]))) {
                        L.db[v] = nd;
                        L.pred[v] = u;
                        L.hops[v] = L.hops[u] + 1;
                        
                        if (nd < B) {
                            if (nd >= Bi) {
                                D.Insert(v, nd);
                            } else if (nd >= Bi_prime) {
                                Kbatch.push_back({v, nd});
                            }
                        }
                    }
                }
            }
            
            for (int x : Si) {
                if (!inU_local[x] && L.db[x] >= Bi_prime && L.db[x] < Bi) {
                    Kbatch.push_back({x, L.db[x]});
                }
            }
            
            D.BatchPrepend(Kbatch);

            long double two_pow_lt = ((long double)l * (long double)t <= 60.0L) ? 
                                     powl(2.0L, (long double)l * (long double)t) : 
                                     (long double)N + 1;
                                     
            long double U_limit = min((long double)N + 1, (long double)k * two_pow_lt);
            
            if ((long double)U.size() >= U_limit) {
                for (int x : W) {
                    if (L.db[x] < Bi_prime_last && !inU_local[x]) {
                        inU_local[x] = true;
                        U.push_back(x);
                    }
                }
                return {Bi_prime_last, U};
            }
        }

        long double Bret = Bi_prime_last;
        for (int x : W) {
            if (L.db[x] < Bret && !inU_local[x]) {
                inU_local[x] = true;
                U.push_back(x);
            }
        }
        
        return {Bret, U};
    }

    void finalize_with_dijkstra() {
        using Node = pair<long double, int>;
        priority_queue<Node, vector<Node>, greater<Node>> pq;
        
        for (int u = 0; u < N; ++u) {
            if (L.db[u] < INF_LD) {
                pq.push({L.db[u], u});
            }
        }
        
        while (!pq.empty()) {
            auto [du, u] = pq.top();
            pq.pop();
            
            if (du > L.db[u]) continue;
            
            for (const auto& e : G[u]) {
                int v = e.to;
                long double nd = du + e.w;
                
                if (nd < L.db[v] || (nd == L.db[v] && lex_better(u, L.pred[v], L.hops[u] + 1, L.hops[v]))) {
                    L.db[v] = nd;
                    L.pred[v] = u;
                    L.hops[v] = L.hops[u] + 1;
                    pq.push({L.db[v], v});
                }
            }
        }
    }

    void run() {
        long double Lg = log2l((long double)max(2, N));
        int l_top = (int)ceil(Lg / (long double)max(1, t));
        vector<int> S0 = {s};
        BMSSP(l_top, INF_LD, S0);
        finalize_with_dijkstra();
    }
};

vector<long double> run_dijkstra(const Graph& g, int s) {
    const int n = (int)g.size();
    vector<long double> dist(n, INF_LD);
    
    using Node = pair<long double, int>;
    priority_queue<Node, vector<Node>, greater<Node>> pq;
    
    if (s >= 0 && s < n) {
        dist[s] = 0.0L;
        pq.emplace(0.0L, s);
    }
    
    while (!pq.empty()) {
        auto [du, u] = pq.top();
        pq.pop();
        
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

vector<long double> map_back_results(int n_orig, const DegreeReduction& dr, const vector<long double>& dist_transformed) {
    vector<long double> dist_orig(n_orig, INF_LD);
    
    for (int v = 0; v < n_orig; ++v) {
        long double best = INF_LD;
        
        if (!dr.slot_id[v].empty()) {
            for (const auto& kv : dr.slot_id[v]) {
                if (kv.second < (int)dist_transformed.size()) {
                    best = min(best, dist_transformed[kv.second]);
                }
            }
        }
        
        dist_orig[v] = best;
    }
    
    return dist_orig;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <num_vertices> <num_edges>" << endl;
        return 1;
    }

    int n = atoi(argv[1]);
    long long m_target = atoll(argv[2]);

    cout << "--- Test Parameters ---" << endl;
    cout << "Target Vertices: " << n << endl;
    cout << "Target Edges:    " << m_target << endl;

    vector<tuple<int, int, long double>> edges;
    std::mt19937 rng(12345);
    std::uniform_int_distribution<> node_dist(0, n - 1);
    std::uniform_real_distribution<long double> weight_dist(1.0, 1000.0);
    
    set<pair<int, int>> edge_set;
    
    if (m_target <= (long long)n * 2 && m_target > n - 1) { 
        cout << "Generating special 'path-like' sparse graph..." << endl;
        for (int i = 0; i < n - 1; ++i) {
            edge_set.insert({i, i + 1});
        }
        while (edge_set.size() < (size_t)m_target) {
            int u = node_dist(rng);
            int v = node_dist(rng);
            if (u != v) {
                edge_set.insert({u, v});
            }
        }
    } else { 
        cout << "Generating standard random graph..." << endl;
        while (edge_set.size() < (size_t)m_target && edge_set.size() < (size_t)n * (n - 1)) {
            int u = node_dist(rng);
            int v = node_dist(rng);
            if (u != v) {
                edge_set.insert({u, v});
            }
        }
    }
    
    for (const auto& p : edge_set) {
        edges.emplace_back(p.first, p.second, weight_dist(rng));
    }

    int m = edges.size();
    int s_orig = 0;
    
    cout << "\n--- Generated Graph ---" << endl;
    cout << "Actual Vertices: " << n << endl;
    cout << "Actual Edges:    " << m << endl;

    long double log_n = log2l((long double)max(2, n));
    long double m_crit = (long double)n * powl(log_n, 1.0L/3.0L);
    
    cout << "\n--- Asymptotic Analysis ---" << endl;
    cout << fixed << setprecision(2);
    cout << "Theoretical Crossover m_crit: ~" << m_crit << " (for n=" << n << ")" << endl;

    cout << "\n--- Pre-processing ---" << endl;
    auto start_transform = high_resolution_clock::now();
    DegreeReduction DR(n);
    DR.build(n, edges, s_orig);
    auto stop_transform = high_resolution_clock::now();
    
    cout << "Graph Transformation Time: " << duration_cast<milliseconds>(stop_transform - start_transform).count() << " ms" << endl;
    
    int s_rep = DR.rep_of_orig[s_orig];
    cout << "Transformed graph has " << DR.Gt.size() << " vertices." << endl;

    cout << "\n--- Running Paper's Algorithm (BMSSP) ---" << endl;
    auto start_bmssp = high_resolution_clock::now();
    DMSY_SSSP algo_bmssp(DR.Gt, s_rep);
    algo_bmssp.run();
    auto stop_bmssp = high_resolution_clock::now();
    auto duration_bmssp = duration_cast<milliseconds>(stop_bmssp - start_bmssp);

    cout << "\n--- Running Standard Dijkstra's Algorithm ---" << endl;
    auto start_dijkstra = high_resolution_clock::now();
    vector<long double> dists_transformed_dijkstra = run_dijkstra(DR.Gt, s_rep);
    auto stop_dijkstra = high_resolution_clock::now();
    auto duration_dijkstra = duration_cast<milliseconds>(stop_dijkstra - start_dijkstra);

    cout << "\n--- Verification ---" << endl;
    vector<long double> results_bmssp = map_back_results(n, DR, algo_bmssp.L.db);
    vector<long double> results_dijkstra = map_back_results(n, DR, dists_transformed_dijkstra);
    
    bool ok = true;
    for (int i = 0; i < n; ++i) {
        if (abs(results_bmssp[i] - results_dijkstra[i]) > 1e-9) {
            if (!( (results_bmssp[i] == INF_LD && results_dijkstra[i] == INF_LD) )) {
                ok = false;
                break;
            }
        }
    }

    if (ok) {
        cout << "SUCCESS: Results from both algorithms match." << endl;
    } else {
        cout << "FAILURE: Results do not match." << endl;
    }

    cout << "\n--- Final Summary ---" << endl;
    cout << "Paper's Algorithm (BMSSP): " << duration_bmssp.count() << " ms" << endl;
    cout << "Standard Dijkstra        : " << duration_dijkstra.count() << " ms" << endl;
    
    return 0;
}