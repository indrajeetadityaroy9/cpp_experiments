import os
import re

# We will construct include/duan_sssp.hpp
# and src/duan_sssp.cpp

hpp_content = """#ifndef DUAN_SSSP_HPP
#define DUAN_SSSP_HPP

#include <vector>
#include <limits>
#include <cmath>
#include <expected>
#include <chrono>
#include <iostream>
#include <unordered_set>
#include <climits>
#include <list>
#include <map>
#include <unordered_map>
#include <utility>

namespace duan {

using std::vector;

constexpr long double INF = std::numeric_limits<long double>::infinity();
constexpr long double FP_EPSILON = 1e-12L;
constexpr int MAX_SAFE_SHIFT = 30;

inline int safe_power_of_2(int exponent) {
    if (exponent < 0 || exponent >= MAX_SAFE_SHIFT) {
        return std::numeric_limits<int>::max();
    }
    return 1 << exponent;
}

inline int safe_multiply(int a, int b) {
    if (a == 0 || b == 0) return 0;
    if (a > std::numeric_limits<int>::max() / b) {
        return std::numeric_limits<int>::max();
    }
    return a * b;
}

struct Edge {
    int to;
    long double weight;
    Edge(int t, long double w) : to(t), weight(w) {}
};

using Graph = vector<vector<Edge>>;

struct Params {
    int k;
    int t;
    static Params compute(int n) {
        Params p;
        long double log_n = std::log2((long double)std::max(2, n));
        p.k = std::max(1, (int)std::floor(std::pow(log_n, 1.0L / 3.0L)));
        p.t = std::max(1, (int)std::floor(std::pow(log_n, 2.0L / 3.0L)));
        return p;
    }
};

enum class DuanError {
    NonSingletonSourceSet,
    SourceOutOfBounds,
    InvalidParameter,
    EmptyGraph
};

inline const char* error_message(DuanError err) {
    switch (err) {
        case DuanError::NonSingletonSourceSet: return "BaseCase requires singleton source set";
        case DuanError::SourceOutOfBounds: return "Source vertex out of bounds";
        case DuanError::InvalidParameter: return "Invalid parameter";
        case DuanError::EmptyGraph: return "Graph is empty";
    }
    return "Unknown error";
}

class Labels {
public:
    vector<long double> dist;
    vector<int> pred;
    vector<int> hops;

    explicit Labels(int n) { reset(n); }
    void reset(int n) {
        dist.assign(n, INF);
        pred.assign(n, -1);
        hops.assign(n, INT_MAX / 2);
    }
    std::size_t size() const { return dist.size(); }
    bool is_finite(int v) const { return dist[v] < INF; }
};

inline bool lex_better(int u, int old_pred, int new_hops, int old_hops) {
    if (new_hops != old_hops) return new_hops < old_hops;
    return u < old_pred;
}

inline bool try_relax(Labels& labels, int u, int v, long double new_dist) {
    if (new_dist > labels.dist[v]) return false;
    bool should_update = false;
    if (new_dist < labels.dist[v]) {
        should_update = true;
    } else {
        int new_hops = labels.hops[u] + 1;
        should_update = lex_better(u, labels.pred[v], new_hops, labels.hops[v]);
    }
    if (should_update) {
        labels.dist[v] = new_dist;
        labels.pred[v] = u;
        labels.hops[v] = labels.hops[u] + 1;
        return true;
    }
    return false;
}

using KeyValuePair = std::pair<int, long double>;

struct Block {
    std::list<KeyValuePair> elements;
    long double upper_bound;
    Block() : upper_bound(INF) {}
    explicit Block(long double ub) : upper_bound(ub) {}
    int size() const { return elements.size(); }
    bool empty() const { return elements.empty(); }
};

class PartialOrderDS {
public:
    PartialOrderDS() : M_(0), B_(INF), total_inserts_(0) {}
    void Initialize(int M, long double B);
    void Insert(int key, long double value);
    void BatchPrepend(const vector<KeyValuePair>& L);
    std::pair<vector<int>, long double> Pull();
    bool empty() const {
        for (const auto& block : D0_) if (!block.empty()) return false;
        for (const auto& block : D1_) if (!block.empty()) return false;
        return true;
    }
    int total_elements() const;
private:
    std::list<Block> D0_;
    std::list<Block> D1_;
    std::map<long double, std::list<Block>::iterator> D1_bounds_;
    struct ElementLocation {
        int sequence_id;
        std::list<Block>::iterator block_it;
        std::list<KeyValuePair>::iterator elem_it;
    };
    std::unordered_map<int, ElementLocation> key_locations_;
    int M_;
    long double B_;
    int total_inserts_;

    void Delete(int key, const ElementLocation& loc);
    void SplitBlock(std::list<Block>::iterator block_it);
    long double FindMedian(std::list<KeyValuePair>& elements);
    std::pair<Block, Block> PartitionByMedian(std::list<KeyValuePair>& elements, long double median);
    std::list<Block> CreateBlocksFromList(vector<KeyValuePair>& L);
    std::list<Block>::iterator FindBlockForValue(long double value);
    void UpdateKeyLocation(int key, int seq_id, std::list<Block>::iterator block_it, std::list<KeyValuePair>::iterator elem_it);
    void RemoveKeyLocation(int key);
    std::pair<vector<KeyValuePair>, std::list<Block>::iterator> CollectPrefix(std::list<Block>& sequence, int target_size);
    void RebuildD1Bounds();
    void RebuildKeyLocations();
    long double ComputeMinRemainingValue() const;
};

struct BaseCaseResult {
    long double b;
    vector<int> U;
};
std::expected<BaseCaseResult, DuanError> execute_base_case(
    const Graph& graph, Labels& labels, long double B, const vector<int>& S, int k);

struct FindPivotsResult {
    vector<int> P;
    vector<int> W;
};
FindPivotsResult execute_find_pivots(
    const Graph& graph, Labels& labels, long double B, const vector<int>& S, int k);

struct BMSSPResult {
    long double b;
    vector<int> U;
};
BMSSPResult execute_bmssp(
    const Graph& graph, Labels& labels, int l, long double B, const vector<int>& S, const Params& params);

struct DuanStats {
    size_t edge_relaxations = 0;
    size_t ds_inserts = 0;
    size_t ds_batch_prepends = 0;
    size_t ds_pulls = 0;
    size_t bmssp_calls = 0;
    size_t max_recursion_depth = 0;
    std::chrono::microseconds total_time{0};
    void reset() {
        edge_relaxations = 0; ds_inserts = 0; ds_batch_prepends = 0;
        ds_pulls = 0; bmssp_calls = 0; max_recursion_depth = 0;
        total_time = std::chrono::microseconds{0};
    }
    void print() const {
        std::cout << "=== Duan SSSP Statistics ===\n";
        std::cout << "Edge relaxations:     " << edge_relaxations << "\n";
        std::cout << "DS Inserts:           " << ds_inserts << "\n";
        std::cout << "DS BatchPrepends:     " << ds_batch_prepends << "\n";
        std::cout << "DS Pulls:             " << ds_pulls << "\n";
        std::cout << "BMSSP calls:          " << bmssp_calls << "\n";
        std::cout << "Max recursion depth:  " << max_recursion_depth << "\n";
        std::cout << "Total time:           " << total_time.count() << " us\n";
    }
};

extern DuanStats g_stats;
extern bool g_collect_stats;

struct DuanSSSPResult {
    vector<long double> dist;
    vector<int> pred;
    DuanStats stats;
};

DuanSSSPResult compute_sssp(const Graph& graph, int source, bool collect_stats = false);
vector<long double> compute_dijkstra_sssp(const Graph& graph, int source);

}
#endif
"""

with open("include/duan_sssp.hpp", "w") as f:
    f.write(hpp_content)

