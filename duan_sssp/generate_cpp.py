import re
import os

cpp_code = """
#include "../include/duan_sssp.hpp"
#include <queue>
#include <algorithm>
#include <cmath>
#include <stack>

namespace duan {

DuanStats g_stats;
bool g_collect_stats = false;

// ---------------------------------------------------------
// Helper functions
// ---------------------------------------------------------

static void relax_layer(const Graph& graph, Labels& labels, long double B,
                        const vector<int>& W_prev, std::unordered_set<int>& W_next) {
    for (int u : W_prev) {
        if (u < 0 || u >= (int)graph.size()) continue;
        for (const Edge& edge : graph[u]) {
            int v = edge.to;
            long double new_dist = labels.dist[u] + edge.weight;
            if (try_relax(labels, u, v, new_dist)) {
                if (new_dist < B) {
                    W_next.insert(v);
                }
            }
        }
    }
}

static std::unordered_map<int, vector<int>> build_forest(
    const Graph& graph, const Labels& labels, const std::unordered_set<int>& W_set) {
    std::unordered_map<int, vector<int>> forest;
    for (int u : W_set) {
        if (u < 0 || u >= (int)graph.size()) continue;
        for (const Edge& edge : graph[u]) {
            int v = edge.to;
            if (W_set.count(v) > 0) {
                long double expected_dist = labels.dist[u] + edge.weight;
                if (std::abs(labels.dist[v] - expected_dist) < FP_EPSILON && labels.pred[v] == u) {
                    forest[u].push_back(v);
                }
            }
        }
    }
    return forest;
}

static std::unordered_map<int, int> compute_subtree_sizes(
    const std::unordered_map<int, vector<int>>& forest, const vector<int>& roots) {
    std::unordered_map<int, int> subtree_sizes;
    for (int root : roots) {
        std::stack<std::pair<int, bool>> stack;
        stack.push({root, false});
        while (!stack.empty()) {
            auto [v, processed] = stack.top();
            stack.pop();
            if (processed) {
                int size = 1;
                auto it = forest.find(v);
                if (it != forest.end()) {
                    for (int child : it->second) {
                        size += subtree_sizes[child];
                    }
                }
                subtree_sizes[v] = size;
            } else {
                stack.push({v, true});
                auto it = forest.find(v);
                if (it != forest.end()) {
                    for (int child : it->second) {
                        stack.push({child, false});
                    }
                }
            }
        }
    }
    return subtree_sizes;
}

static vector<int> identify_pivots(const vector<int>& S, const std::unordered_map<int, int>& subtree_sizes, int k) {
    vector<int> pivots;
    for (int u : S) {
        auto it = subtree_sizes.find(u);
        if (it != subtree_sizes.end() && it->second >= k) {
            pivots.push_back(u);
        }
    }
    return pivots;
}

static void relax_and_classify(
    const Graph& graph, Labels& labels, const vector<int>& U_i, long double b_i,
    long double B_i, long double B, PartialOrderDS& DS, vector<KeyValuePair>& K) {
    for (int u : U_i) {
        if (u < 0 || u >= (int)graph.size()) continue;
        for (const Edge& edge : graph[u]) {
            int v = edge.to;
            long double new_dist = labels.dist[u] + edge.weight;
            if (try_relax(labels, u, v, new_dist)) {
                if (g_collect_stats) g_stats.edge_relaxations++;
                if (new_dist >= B_i && new_dist < B) {
                    DS.Insert(v, new_dist);
                    if (g_collect_stats) g_stats.ds_inserts++;
                } else if (new_dist >= b_i && new_dist < B_i) {
                    K.push_back({v, new_dist});
                }
            }
        }
    }
}

static void collect_vertices_in_range(
    const vector<int>& S_i, const Labels& labels, long double b_i,
    long double B_i, vector<KeyValuePair>& K) {
    for (int x : S_i) {
        if (labels.dist[x] >= b_i && labels.dist[x] < B_i) {
            K.push_back({x, labels.dist[x]});
        }
    }
}

// ---------------------------------------------------------
// Algorithms
// ---------------------------------------------------------

struct HeapElement {
    int vertex;
    long double dist;
    int hops;
    bool operator>(const HeapElement& other) const {
        if (dist != other.dist) return dist > other.dist;
        if (hops != other.hops) return hops > other.hops;
        return vertex > other.vertex;
    }
};

std::expected<BaseCaseResult, DuanError> execute_base_case(
    const Graph& graph, Labels& labels, long double B, const vector<int>& S, int k) {
    
    if (S.size() != 1) return std::unexpected(DuanError::NonSingletonSourceSet);
    int x = S[0];
    if (x < 0 || x >= static_cast<int>(graph.size())) return std::unexpected(DuanError::SourceOutOfBounds);

    std::unordered_set<int> U_0_set;
    vector<int> U_0;
    U_0.push_back(x);
    U_0_set.insert(x);

    std::priority_queue<HeapElement, vector<HeapElement>, std::greater<HeapElement>> heap;
    heap.push({x, labels.dist[x], labels.hops[x]});

    std::unordered_set<int> in_heap;
    in_heap.insert(x);

    while (!heap.empty() && (int)U_0.size() < k + 1) {
        HeapElement min_elem = heap.top();
        heap.pop();
        int u = min_elem.vertex;

        if (U_0_set.count(u) > 0 && u != x) continue;
        if (min_elem.dist > labels.dist[u]) continue;

        if (u != x) {
            U_0.push_back(u);
            U_0_set.insert(u);
        }
        in_heap.erase(u);

        if (u < 0 || u >= (int)graph.size()) continue;

        for (const Edge& edge : graph[u]) {
            int v = edge.to;
            long double new_dist = labels.dist[u] + edge.weight;
            if (new_dist >= B) continue;
            if (try_relax(labels, u, v, new_dist)) {
                if (in_heap.count(v) == 0) {
                    heap.push({v, labels.dist[v], labels.hops[v]});
                    in_heap.insert(v);
                } else {
                    heap.push({v, labels.dist[v], labels.hops[v]});
                }
            }
        }
    }

    BaseCaseResult result;
    if ((int)U_0.size() <= k) {
        result.b = B;
        result.U = U_0;
    } else {
        long double max_dist = -INF;
        for (int v : U_0) max_dist = std::max(max_dist, labels.dist[v]);
        result.b = max_dist;
        for (int v : U_0) {
            if (labels.dist[v] < result.b) {
                result.U.push_back(v);
            }
        }
    }
    return result;
}


FindPivotsResult execute_find_pivots(
    const Graph& graph, Labels& labels, long double B, const vector<int>& S, int k) {
    
    FindPivotsResult result;
    if (S.empty()) return result;

    std::unordered_set<int> W_set(S.begin(), S.end());
    vector<int> W_prev = S;

    for (int i = 1; i <= k; ++i) {
        std::unordered_set<int> W_next_set;
        relax_layer(graph, labels, B, W_prev, W_next_set);
        for (int v : W_next_set) W_set.insert(v);

        if ((int)W_set.size() > k * (int)S.size()) {
            result.P = S;
            result.W.assign(W_set.begin(), W_set.end());
            return result;
        }
        W_prev.assign(W_next_set.begin(), W_next_set.end());
        if (W_prev.empty()) break;
    }

    result.W.assign(W_set.begin(), W_set.end());
    auto forest = build_forest(graph, labels, W_set);
    auto subtree_sizes = compute_subtree_sizes(forest, S);
    result.P = identify_pivots(S, subtree_sizes, k);
    return result;
}

static thread_local int current_recursion_depth = 0;

BMSSPResult execute_bmssp(
    const Graph& graph, Labels& labels, int l, long double B, const vector<int>& S, const Params& params) {
    
    BMSSPResult result;
    if (S.empty()) return BMSSPResult{B, {}};

    current_recursion_depth++;
    if (g_collect_stats) {
        g_stats.bmssp_calls++;
        g_stats.max_recursion_depth = std::max(g_stats.max_recursion_depth, (size_t)current_recursion_depth);
    }

    if (l == 0) {
        auto base_result_exp = execute_base_case(graph, labels, B, S, params.k);
        if (!base_result_exp) {
            current_recursion_depth--;
            return BMSSPResult{B, {}};
        }
        auto base_result = *base_result_exp;
        result.b = base_result.b;
        result.U = base_result.U;
        current_recursion_depth--;
        return result;
    }

    auto pivots_result = execute_find_pivots(graph, labels, B, S, params.k);
    vector<int>& P = pivots_result.P;
    vector<int>& W = pivots_result.W;

    int M = safe_power_of_2((l - 1) * params.t);
    PartialOrderDS DS;
    DS.Initialize(M, B);

    for (int x : P) {
        DS.Insert(x, labels.dist[x]);
        if (g_collect_stats) g_stats.ds_inserts++;
    }

    long double b_i = B;
    if (!P.empty()) {
        b_i = INF;
        for (int x : P) b_i = std::min(b_i, labels.dist[x]);
    }

    vector<int> U;
    int k_limit = safe_multiply(params.k, safe_power_of_2(l * params.t));

    while ((int)U.size() < k_limit && !DS.empty()) {
        auto [S_i, B_i] = DS.Pull();
        if (g_collect_stats) g_stats.ds_pulls++;

        auto recursive_result = execute_bmssp(graph, labels, l - 1, B_i, S_i, params);
        long double b_i_new = recursive_result.b;
        vector<int>& U_i = recursive_result.U;

        U.insert(U.end(), U_i.begin(), U_i.end());

        vector<KeyValuePair> K;
        relax_and_classify(graph, labels, U_i, b_i_new, B_i, B, DS, K);
        collect_vertices_in_range(S_i, labels, b_i_new, B_i, K);

        if (!K.empty()) {
            DS.BatchPrepend(K);
            if (g_collect_stats) g_stats.ds_batch_prepends++;
        }
        b_i = b_i_new;
    }

    result.b = std::min(b_i, B);
    result.U = U;
    for (int x : W) {
        if (labels.dist[x] < result.b) {
            result.U.push_back(x);
        }
    }
    current_recursion_depth--;
    return result;
}

static int compute_initial_layer(int n, const Params& params) {
    if (n <= 1) return 0;
    if (params.t == 0) return 1;
    long double log_n = std::log2((long double)n);
    return std::max(1, (int)std::ceil(log_n / (long double)params.t));
}

DuanSSSPResult compute_sssp(const Graph& graph, int source, bool collect_stats) {
    DuanSSSPResult result;
    int n = graph.size();

    g_collect_stats = collect_stats;
    if (collect_stats) g_stats.reset();

    auto start_time = std::chrono::high_resolution_clock::now();

    Labels labels(n);
    labels.dist[source] = 0.0;
    labels.hops[source] = 0;

    Params params = Params::compute(n);
    int initial_layer = compute_initial_layer(n, params);

    vector<int> S = {source};
    long double B = INF;

    execute_bmssp(graph, labels, initial_layer, B, S, params);

    auto end_time = std::chrono::high_resolution_clock::now();

    result.dist = labels.dist;
    result.pred = labels.pred;

    if (collect_stats) {
        g_stats.total_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        result.stats = g_stats;
    }

    return result;
}

vector<long double> compute_dijkstra_sssp(const Graph& graph, int source) {
    int n = graph.size();
    vector<long double> dist(n, INF);
    vector<bool> visited(n, false);
    using PQElement = std::pair<long double, int>;
    std::priority_queue<PQElement, vector<PQElement>, std::greater<PQElement>> pq;

    dist[source] = 0.0;
    pq.push({0.0, source});

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();
        if (visited[u]) continue;
        visited[u] = true;

        for (const Edge& edge : graph[u]) {
            int v = edge.to;
            long double new_dist = dist[u] + edge.weight;
            if (new_dist < dist[v]) {
                dist[v] = new_dist;
                pq.push({new_dist, v});
            }
        }
    }
    return dist;
}

} // namespace duan
"""

with open("src/duan_sssp.cpp", "w") as f:
    f.write(cpp_code)
    
# Now we read the original src/algorithms/partial_order_ds.cpp and append it to src/duan_sssp.cpp
with open("src/algorithms/partial_order_ds.cpp", "r") as f:
    pods_code = f.read()

# remove #include lines and the namespace start from pods_code
pods_code = re.sub(r'#include.*', '', pods_code)
pods_code = re.sub(r'namespace duan \{', '', pods_code, count=1)
# remove the last '}' 
pods_code = pods_code.rstrip()
if pods_code.endswith('}'):
    pods_code = pods_code[:-1]

# Append the partial order ds methods
with open("src/duan_sssp.cpp", "a") as f:
    f.write("\nnamespace duan {\n")
    f.write(pods_code)
    f.write("\n}\n")
