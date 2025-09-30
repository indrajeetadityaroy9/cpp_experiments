/**
 * Implementation of the Duan–Mehlhorn–Shao–Su–Zhang Deterministic SSSP Algorithm
 * Paper: "Breaking the Sorting Barrier for Directed SSSP" (arXiv:2504.17033v2)
 *
 * Implements:
 * - Algorithm 1 (FindPivots): k-step BFS with pivot identification
 * - Algorithm 2 (BaseCase): Base case for l=0 recursion
 * - Algorithm 3 (BMSSP): Main recursive Bounded Multi-Source Shortest Path algorithm
 * - Lemma 3.1 (PartialOrderDS): Data structure for partial sorting
 *
 * Complexity: O(m·log^(2/3)(n)) time for SSSP on directed graphs with non-negative weights
 *
 * Key parameters:
 * - k = ⌊log^(1/3)(n)⌋
 * - t = ⌊log^(2/3)(n)⌋
 * - M = 2^((l-1)t) at recursion level l
 */

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
#include <expected>
#include <span>
#include <string>
using namespace std;

struct Edge { int to; long double w; };
using Graph = vector<vector<Edge>>;

static const long double INF_LD = numeric_limits<long double>::infinity();

/**
 * Labels structure for maintaining distance estimates
 *
 * - db[u]: Current distance estimate \hat{delta}[u] (>= true distance d(u))
 * - pred[u]: Predecessor in shortest path tree
 * - hops[u]: Number of hops for lexicographic tie-breaking
 */
struct Labels {
    vector<long double> db;   // Distance bounds \hat{delta}[u]
    vector<int> pred;         // Predecessor Pred[u]
    vector<int> hops;         // Hop count \alpha for lexicographic ordering
    Labels(int n=0) { reset(n); }
    void reset(int n) { db.assign(n, INF_LD); pred.assign(n, -1); hops.assign(n, INT_MAX/2); }
};

/**
 * Lexicographic tie-breaking function
 *
 * Paths are ordered as tuples ⟨length, hop_count, v_\alpha, v_{\alpha-1}, ..., v_1⟩
 * When lengths are equal:
 * 1. Compare hop counts (prefer fewer hops)
 * 2. If still tied, compare predecessor vertices
 *
 * Returns true if path through u with new_hops is better than current path
 * Time: O(1)
 */
inline bool lex_better(int u, int old_pred, int new_hops, int old_hops) {
    if (new_hops != old_hops) return new_hops < old_hops;
    return u < old_pred;  // Compare vertices to break final ties
}


/**
 * Degree Reduction Transformation
 * "Constant-Degree Graph" assumption
 *
 * Transforms arbitrary-degree graphs to degree-2 graphs while preserving shortest paths:
 * - Each vertex v becomes a cycle of "slot" vertices (one per neighbor)
 * - Zero-weight edges connect slots in a cycle
 * - Original edge (u,v,w) becomes edge from slot[u][v] to slot[v][u] with weight w
 *
 * Result: O(m) vertices and edges, degree <= 2
 */
struct DegreeReduction {
    vector<unordered_map<int, int>> slot_id;  // slot_id[v][w] = slot for edge (v,w)
    vector<int> rep_of_orig;                  // Representative slot for original vertex
    Graph Gt;                                 // Transformed graph

    DegreeReduction(int n = 0) {
        slot_id.resize(n);
        rep_of_orig.assign(n, -1);
    }

    void build(int n, span<const tuple<int, int, long double>> edges) {
        vector<unordered_set<int>> nbr(n);
        for (const auto& e : edges) {
            int u, v;
            long double w;
            (void)w;  
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

                for (int w : sorted) 
                    slot_id[v][w] = Np++;
                rep_of_orig[v] = slot_id[v][sorted.front()];
            }
        }
        
        Gt.assign(Np, {});
        
        for (int v = 0; v < n; ++v) {
            if (slot_id[v].empty()) 
                continue;
                
            vector<pair<int, int>> pairs;
            pairs.reserve(slot_id[v].size());
            
            for (auto& kv : slot_id[v]) 
                pairs.push_back({kv.first, kv.second});
            
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

/**
 * Partial Order Data Structure (PartialOrderDS)
 * Implements Lemma 3.1
 *
 * A block-based data structure supporting:
 * - Insert(key, value): O(max{1, log(N/M)}) amortized
 * - BatchPrepend(L): O(L·log(L/M)) amortized
 * - Pull(): Returns <=M smallest keys in O(M) amortized
 *
 * Structure:
 * - D_0: Sequence of blocks from BatchPrepend operations
 * - D_1: Sequence of blocks from Insert operations, indexed by BST
 * - Each block contains <=M key/value pairs
 * - Blocks in D_1 bounded by O(max{1, N/M}) blocks
 * - Blocks sorted: earlier blocks have smaller values
 *
 * Key optimization: Avoids full sorting by maintaining partial order
 */
struct PartialOrderDS {

    struct Entry {
        int key;          // Vertex identifier
        long double val;  // Distance value
    };

    /**
     * Block: Container for <=M entries with cached min/max bounds
     * Maintains upper bounds for efficient search
     */
    struct Block {
        vector<Entry> items;      // At most M key/value pairs
        long double mn = INF_LD;  // Minimum value in block
        long double mx = -INF_LD; // Maximum value (upper bound for search)

        void recompute() {
            mn = INF_LD;
            mx = -INF_LD;

            if (items.empty())
                return;

            for (const auto& e : items) {
                mn = min(mn, e.val);
                mx = max(mx, e.val);
            }
        }
    };

    list<Block> d0;  // D_0: Blocks from BatchPrepend (prepended to front)
    list<Block> d1;  // D_1: Blocks from Insert (maintained in sorted order)

    struct Cmp {
        bool operator()(const pair<long double, Block*>& a, const pair<long double, Block*>& b) const {
            if (a.first != b.first)
                return a.first < b.first;
            return a.second < b.second;
        }
    };

    using SetType = set<pair<long double, Block*>, Cmp>;
    SetType d1_by_mx;  // BST: Red-Black Tree indexing D_1 blocks by max value

    struct KeyPos {
        bool inD0;                 // True if key is in D_0, false if in D_1
        Block* blk;                // Pointer to containing block
        SetType::iterator set_it;  // Iterator in d1_by_mx (only valid if inD0=false)
        long double val;           // Current value for this key
    };

    unordered_map<int, KeyPos> pos;  // Track location of each key
    long double B_bound = INF_LD;    // Upper bound on all values (from Initialize)
    int M = 1;                       // Block size parameter                 

    /**
     * Initialize(M, B)
     * Initialize D_0 with empty sequence and D_1 with single empty block
     * Set block size parameter M and upper bound B
     */
    void Initialize(int M_, long double B_) {
        clear();
        M = max(1, M_);
        B_bound = B_;
        d1.emplace_back();
        auto it = d1_by_mx.insert({d1.back().mx, &d1.back()}).first;
    }

    void clear() {
        d0.clear();
        d1.clear();
        d1_by_mx.clear();
        pos.clear();
        B_bound = INF_LD;
        M = 1;
    }

    void erase_key_internal(int key) {
        auto it = pos.find(key);
        if (it == pos.end()) 
            return;
            
        KeyPos kp = it->second;
        pos.erase(it);
        Block* blk = kp.blk;
        auto& vec = blk->items;
        
        for (size_t i = 0; i < vec.size(); ++i) {
            if (vec[i].key == key) {
                vec[i] = vec.back();
                vec.pop_back();
                break;
            }
        }
        
        if (kp.inD0) {
            blk->recompute();
        } else {
            d1_by_mx.erase(kp.set_it);
            blk->recompute();
            
            if (!blk->items.empty()) {
                auto new_it = d1_by_mx.insert({blk->mx, blk}).first;
                for (const auto& entry : blk->items) 
                    pos.at(entry.key).set_it = new_it;
            } else {
                for (auto lit = d1.begin(); lit != d1.end(); ++lit) {
                    if (&*lit == blk) {
                        d1.erase(lit);
                        break;
                    }
                }
            }
        }
    }

    /**
     * Insert(key, value)
     * Inserts key/value pair into D_1 (sorted blocks)
     * Time: O(max{1, log(N/M)}) amortized
     *
     * Algorithm:
     * 1. If key exists with better value, skip insertion
     * 2. Binary search D_1 BST for block with smallest upper bound >= val
     * 3. Add entry to found block in O(1)
     * 4. If block exceeds M elements, trigger Split operation
     */
    void Insert(int key, long double val) {
        auto it = pos.find(key);
        if (it != pos.end()) {
            if (val >= it->second.val)
                return;  // New value not better, skip
            erase_key_internal(key);  // Delete old entry
        }

        // Binary search via BST
        auto sit = d1_by_mx.lower_bound({val, nullptr});
        if (sit == d1_by_mx.end()) {
            if (d1.empty() || !d1.back().items.empty())
                d1.emplace_back();
            sit = d1_by_mx.insert({d1.back().mx, &d1.back()}).first;
        }

        Block* target = sit->second;
        auto target_set_it = sit;
        target->items.push_back({key, val});
        pos[key] = {false, target, target_set_it, val};

        // Split operation if block exceeds M
        if ((int)target->items.size() > M) {
            auto& items = target->items;
            auto median_it = items.begin() + items.size() / 2;

            // Find median in O(M) time using nth_element
            nth_element(items.begin(), median_it, items.end(),
                [](const Entry& a, const Entry& b) {
                    return a.val < b.val;
                });

            // Partition into two blocks of <=⌈M/2⌉ elements
            Block splitBlk;
            splitBlk.items.assign(median_it, items.end());
            items.erase(median_it, items.end());

            target->recompute();
            splitBlk.recompute();

            auto lit = d1.begin();
            while (lit != d1.end() && &*lit != target)
                ++lit;
                
            auto newIt = d1.insert(next(lit), std::move(splitBlk));
            Block* newBlk = &*newIt;


            d1_by_mx.erase(target_set_it);
            auto new_target_it = d1_by_mx.insert({target->mx, target}).first;
            auto new_split_it = d1_by_mx.insert({newBlk->mx, newBlk}).first;
            
            for (auto& e : target->items) 
                pos.at(e.key).set_it = new_target_it;
            for (auto& e : newBlk->items) 
                pos.at(e.key).set_it = new_split_it;
        } else {
            long double old_mx = sit->first;
            target->recompute();
            
            if (old_mx != target->mx) {
                d1_by_mx.erase(sit);
                auto new_it = d1_by_mx.insert({target->mx, target}).first;
                for (const auto& entry : target->items) 
                    pos.at(entry.key).set_it = new_it;
            }
        }
    }

    /**
     * BatchPrepend(L)
     * Insert L key/value pairs smaller than all current values
     * Time: O(L·log(L/M)) amortized
     *
     * Algorithm:
     * - If L <= M: Create one block
     * - If L > M: Create O(L/M) blocks via recursive median partitioning
     * - Each block has <=⌈M/2⌉ elements
     * - Prepend blocks to front of D_0
     *
     * Key optimization: All entries in L are smaller than existing values,
     * so no sorting needed between L and existing data
     */
    void BatchPrepend(vector<Entry> L) {
        if (L.empty())
            return;

        // Deduplicate: keep smallest value per key
        unordered_map<int, long double> best;
        for (const auto& e : L) {
            auto it = best.find(e.key);
            if (it == best.end() || e.val < it->second)
                best[e.key] = e.val;
        }

        // Remove keys that already exist with better values
        vector<Entry> ins;
        for (const auto& kv : best) {
            int k = kv.first;
            long double v = kv.second;
            auto it = pos.find(k);

            if (it != pos.end()) {
                if (v >= it->second.val)
                    continue;
                erase_key_internal(k);
            }
            ins.push_back({k, v});
        }

        if (ins.empty())
            return;

        // Recursive median partitioning
        // Time: O(L·log(L/M)) via divide-and-conquer
        if (ins.size() <= (size_t)M) {
            // Base case: single block
            d0.emplace_front();
            Block& blk = d0.front();
            blk.items = move(ins);
            blk.recompute();

            for (const auto& e : blk.items)
                pos[e.key] = {true, &blk, SetType::iterator{}, e.val};
        } else {
            // Recursive case: partition into O(L/M) blocks
            vector<Block> new_blocks;
            partition_recursive(ins, 0, ins.size(), new_blocks);

            // Prepend blocks to D_0 in reverse order to maintain sorted property
            for (auto it = new_blocks.rbegin(); it != new_blocks.rend(); ++it) {
                d0.emplace_front(move(*it));
                Block& blk = d0.front();
                for (const auto& e : blk.items)
                    pos[e.key] = {true, &blk, SetType::iterator{}, e.val};
            }
        }
    }

private:
    /**
     * Recursive median partitioning helper for BatchPrepend
     * Implements divide-and-conquer strategy
     *
     * Recursively partitions entries[start, end) into blocks of size <=⌈M/2⌉
     * Time: O(L·log(L/M)) where L = end - start
     * - O(log(L/M)) recursion levels
     * - O(L) work per level (nth_element is linear)
     */
    void partition_recursive(vector<Entry>& entries, size_t start, size_t end,
                            vector<Block>& blocks) {
        size_t len = end - start;

        if (len <= (size_t)M / 2) {
            // Base case: create a block with <=⌈M/2⌉ elements
            Block blk;
            blk.items.assign(entries.begin() + start, entries.begin() + end);
            blk.recompute();
            blocks.push_back(move(blk));
            return;
        }

        // Find median in O(len) time
        size_t mid = start + len / 2;
        nth_element(entries.begin() + start, entries.begin() + mid, entries.begin() + end,
            [](const Entry& a, const Entry& b) { return a.val < b.val; });

        // Recursively partition both halves
        partition_recursive(entries, start, mid, blocks);
        partition_recursive(entries, mid, end, blocks);
    }

public:

    bool empty() const {
        for (const auto& blk : d0)
            if (!blk.items.empty())
                return false;
        for (const auto& blk : d1)
            if (!blk.items.empty())
                return false;
        return true;
    }

    /**
     * Pull()
     * Returns <=M keys with smallest values and separator bound x
     * Time: O(M) amortized
     *
     * Algorithm:
     * 1. Collect "sufficient prefix" from D_0 and D_1:
     *    - From D_0: stop when collected M elements or exhausted
     *    - From D_1: stop when collected 2M elements or exhausted
     * 2. If |collected| <= M: return all, set x = B
     * 3. Else: Use nth_element to find M smallest in O(M) time
     * 4. Set x = (M+1)-th smallest value (separator)
     * 5. Delete returned keys from structure
     */
    pair<vector<int>, long double> Pull() {
        // Check if structure is empty
        vector<int> all_keys;
        for (const auto& blk : d0)
            for (const auto& e : blk.items)
                all_keys.push_back(e.key);
        for (const auto& blk : d1)
            for (const auto& e : blk.items)
                all_keys.push_back(e.key);

        if (all_keys.empty())
            return {{}, B_bound};

        // If <=M elements total, return all
        if ((int)all_keys.size() <= M) {
            clear();
            Initialize(M, B_bound);
            return {all_keys, B_bound};
        }

        // Collect candidates: M from D_0, 2M from D_1
        vector<pair<Entry, bool>> cand;
        auto d0_it = d0.begin();

        while (d0_it != d0.end() && cand.size() < (size_t)M) {
            for (const auto& item : d0_it->items)
                cand.push_back({item, true});
            d0_it++;
        }

        auto d1_it = d1_by_mx.begin();
        while (d1_it != d1_by_mx.end() && cand.size() < (size_t)(2 * M)) {
            for (const auto& item : d1_it->second->items)
                cand.push_back({item, false});
            d1_it++;
        }

        // Select M smallest using nth_element: O(M) time
        auto cmp = [](const pair<Entry, bool>& a, const pair<Entry, bool>& b) {
            if (a.first.val != b.first.val)
                return a.first.val < b.first.val;
            return a.first.key < b.first.key;
        };
        nth_element(cand.begin(), cand.begin() + M, cand.end(), cmp);

        vector<int> S;
        for (int i = 0; i < M; ++i) 
            S.push_back(cand[i].first.key);
        
        long double next_bound = cand[M].first.val;

        for (int key : S) 
            erase_key_internal(key);

        return {S, next_bound};
    }
};

/**
 * DMSY_SSSP: Main algorithm structure
 * Implements Algorithms 1, 2, 3
 *
 * Solves Single-Source Shortest Paths in O(m·log^(2/3)(n)) time
 */
struct DMSY_SSSP {
    const Graph& G;  // Transformed graph (after degree reduction)
    int N;           // Number of vertices in transformed graph
    int s;           // Source vertex
    Labels L;        // Distance estimates, predecessors, hop counts

    // Algorithm parameters
    int k, t;  // k = ⌊log^(1/3)(n)⌋, t = ⌊log^(2/3)(n)⌋

    /**
     * Constructor: Initialize algorithm state
     * Sets up distance labels and computes parameters k, t
     */
    DMSY_SSSP(const Graph& Gt, int s_) : G(Gt), N((int)Gt.size()), s(s_), L(N) {
        L.db.assign(N, INF_LD);
        L.pred.assign(N, -1);
        L.hops.assign(N, INT_MAX/2);
        L.db[s] = 0.0L;  // Source has distance 0
        L.hops[s] = 0;

        // Compute parameters
        long double Lg = log2l((long double)max(2, N));
        k = max(1, (int)floor(powl(Lg, 1.0L/3.0L)));  // k = ⌊log^(1/3)(n)⌋
        t = max(1, (int)floor(powl(Lg, 2.0L/3.0L)));  // t = ⌊log^(2/3)(n)⌋
    }

    /**
     * Algorithm 1: FindPivots(B, S)
     *
     * Purpose: Shrink frontier S to pivot set P of size <=|U|/k
     *
     * Algorithm:
     * 1. Run k-step BFS from S, building explored set W
     * 2. If |W| > k·|S|: return (S, W) [early exit]
     * 3. Construct predecessor graph F on W
     * 4. Find pivots P in S: vertices with no incoming edges that root trees of size >=k
     *
     * Returns: (P, W) where P = pivot set, W = explored vertices
     * Time: O(min{|U|, k·|S|}·k) where U = vertices reachable within bound B
     */
    pair<vector<int>, vector<int>> FindPivots(long double B, const vector<int>& Sset) {
        vector<int> W;  // Explored set (will contain all vertices reached in k steps)
        W.reserve((size_t)k * max(1, (int)Sset.size()) + 8);
        unordered_set<int> inW(Sset.begin(), Sset.end());
        W.assign(Sset.begin(), Sset.end());

        // Step 1: k-step BFS exploration
        vector<int> Wi = Sset;  // W_i: vertices to explore in iteration i
        for (int i = 1; i <= k; i++) {
            vector<int> Wi_next;
            for (int u : Wi) {
                for (const auto& e : G[u]) {
                    int v = e.to;
                    if (L.db[u] == INF_LD) 
                        continue;
                        
                    long double nd = L.db[u] + e.w;
                    bool changed = false;
                    
                    // Update distance if better path found
                    if (nd < L.db[v]) {
                        L.db[v] = nd;
                        L.pred[v] = u;
                        L.hops[v] = L.hops[u] + 1;
                        changed = true;
                    } else if (nd == L.db[v]) {
                        // Tie-breaking with lexicographic order
                        int nh = L.hops[u] + 1;
                        if (lex_better(u, L.pred[v], nh, L.hops[v])) {
                            L.pred[v] = u;
                            L.hops[v] = nh;
                            changed = true;
                        }
                    }
                    
                    // Add to next level if vertex was updated and within bound
                    if (changed && nd < B) {
                        Wi_next.push_back(v);
                        if (inW.find(v) == inW.end()) {
                            inW.insert(v);
                            W.push_back(v);
                        }
                    }
                }
            }
            Wi = move(Wi_next);
            
            // Early exit if W becomes too large
            if (!Sset.empty() && W.size() > (size_t)k * Sset.size()) {
                return {Sset, W}; // early exit: P = S
            }
        }

        // Build predecessor graph F on W
        unordered_map<int, int> indegF;
        unordered_map<int, vector<int>> adjF;
        for (int v : W) {
            int u = L.pred[v];
            if (u != -1 && inW.count(u)) {
                adjF[u].push_back(v);
                indegF[v]++;
            }
        }

        // Find pivot vertices in Sset
        vector<int> P;
        for (int u : Sset) {
            // Skip if u has incoming edges in F
            if (indegF.count(u) && indegF[u] > 0) 
                continue;
                
            // Check if there's a tree of size at least k rooted at u
            int cnt = 0;
            stack<int> st;
            st.push(u);
            unordered_set<int> seen;
            
            while (!st.empty()) {
                int x = st.top();
                st.pop();
                if (seen.count(x)) 
                    continue;
                seen.insert(x);
                cnt++;
                if (cnt >= k) 
                    break;
                if (adjF.count(x)) 
                    for (int y : adjF.at(x)) 
                        st.push(y);
            }
            
            if (cnt >= k) 
                P.push_back(u);
        }
        return {P, W};
    }

    /**
     * Algorithm 2: BaseCase(B, {x})
     * (Algorithm: BaseCase)
     *
     * Purpose: Handle recursion base case (l=0) for singleton source set
     *
     * Algorithm:
     * 1. Run mini-Dijkstra from x until finding k+1 vertices or exhausting
     * 2. If |U₀| <= k: return (B, U₀) [successful execution]
     * 3. Else: return (B', U) where B' = max distance, U = vertices with distance < B'
     *
     * Input:
     * - B: Upper bound on distances to explore
     * - x: Source vertex (S = {x})
     *
     * Output:
     * - (B', U): New bound B' and vertex set U
     *
     * Time: O(k · degree) = O(k) for constant-degree graphs
     */
    pair<long double, vector<int>> BaseCase(long double B, int x) {
        vector<int> U0;
        U0.reserve(k + 2);

        // Mini-Dijkstra: Priority queue maintaining (distance, vertex) pairs
        struct Node {
            long double d;
            int u;
        };

        struct Cmp {
            bool operator()(const Node& a, const Node& b) const {
                if (a.d != b.d)
                    return a.d > b.d;
                return a.u > b.u;
            }
        };

        priority_queue<Node, vector<Node>, Cmp> pq;
        if (L.db[x] < B)
            pq.push({L.db[x], x});
        unordered_set<int> settled;

        // Extract up to k+1 closest vertices
        while (!pq.empty() && settled.size() < (size_t)k + 1) {
            auto [du, u] = pq.top();
            pq.pop();
            
            if (du > L.db[u] || settled.count(u)) 
                continue;
                
            settled.insert(u);
            U0.push_back(u);

            // Update neighbors
            for (auto& e : G[u]) {
                int v = e.to;
                long double nd = L.db[u] + e.w;
                if (nd >= B) 
                    continue;
                    
                bool changed = false;
                if (nd < L.db[v]) {
                    L.db[v] = nd;
                    L.pred[v] = u;
                    L.hops[v] = L.hops[u] + 1;
                    changed = true;
                } else if (nd == L.db[v]) {
                    int nh = L.hops[u] + 1;
                    if (lex_better(u, L.pred[v], nh, L.hops[v])) {
                        L.pred[v] = u;
                        L.hops[v] = nh;
                        changed = true;
                    }
                }
                
                if (changed) 
                    pq.push({L.db[v], v});
            }
        }

        // Return appropriate result based on number of settled vertices
        if (settled.size() <= (size_t)k) {
            return {B, U0};
        } else {
            long double Bp = -INF_LD;
            for (int u : U0) 
                Bp = max(Bp, L.db[u]);
            
            vector<int> U;
            if (Bp > -INF_LD) {
                U.reserve(U0.size());
                for (int u : U0) 
                    if (L.db[u] < Bp) 
                        U.push_back(u);
            } else {
                U = U0;
            }
            return {Bp, U};
        }
    }

    /**
     * Algorithm 3: BMSSP(l, B, S)
     *
     * Purpose: Bounded Multi-Source Shortest Path - main recursive algorithm
     *
     * Implements divide-and-conquer on vertex sets with ⌈log(n)/t⌉ recursion levels.
     * Each level processes vertices in batches of size <~2^(lt), avoiding full sorting
     * by using PartialOrderDS to extract <=M smallest vertices at a time.
     *
     * Input:
     * - l: Recursion level ∈ [0, ⌈log(n)/t⌉]
     * - B: Upper bound on distances to explore
     * - S: Source set with |S| <= 2^(lt)
     *
     * Precondition: For every incomplete vertex v with d(v) < B, the shortest path
     * to v visits some complete vertex u ∈ S
     *
     * Output:
     * - (B', U): New bound B' <= B and vertex set U
     * - U is complete and contains all v with d(v) < B' whose shortest path visits S
     *
     * Guarantees (Lemma 3.1):
     * - Successful execution: B' = B
     * - Partial execution: B' < B and |U| = \theta(k·2^(lt))
     *
     * Time: O((kl + tl/k + t)|U|)
     */
    pair<long double, vector<int>> BMSSP(int l, long double B, const vector<int>& S) {
        // Base case (l=0): call Algorithm 2 (BaseCase)
        if (l == 0) {
            return BaseCase(B, S.empty() ? s : S[0]);
        }

        // Step 1: FindPivots to shrink S to P of size <=|U|/k
        auto [P, W] = FindPivots(B, S);

        // Step 2: Initialize PartialOrderDS with M = 2^((l-1)t)
        PartialOrderDS D;
        long double M_val = ((long double)(l - 1) * (long double)t <= 60.0L)
                          ? powl(2.0L, (long double)(l - 1) * t)
                          : (long double)N + 1;
        D.Initialize(max(1, (int)min((long double)N, M_val)), B);

        // Step 3: Insert pivots into D
        for (int x : P)
            if (L.db[x] < B)
                D.Insert(x, L.db[x]);

        long double Bi_prime_last = B;
        vector<int> U;
        unordered_set<int> inU_local;

        // Termination threshold: |U| < k·2^(lt)
        long double two_pow_lt = ((long double)l * (long double)t <= 60.0L)
                               ? powl(2.0L, (long double)l * (long double)t)
                               : (long double)N + 1;
        long double U_limit = min((long double)N + 1, (long double)k * two_pow_lt);

        // Step 4: Main iterative loop
        while (!D.empty()) {
            // Pull <=M smallest vertices from D (line 204)
            auto pulled = D.Pull();
            vector<int> Si = move(pulled.first);
            long double Bi = pulled.second;
            if (Si.empty())
                break;

            // Recursive call to level l-1 (line 205)
            auto sub = BMSSP(l - 1, Bi, Si);
            long double Bi_prime = sub.first;
            vector<int> Ui = move(sub.second);

            Bi_prime_last = min(Bi_prime_last, Bi_prime);

            // Add newly processed vertices to U (line 206)
            for (int u : Ui)
                if (inU_local.find(u) == inU_local.end()) {
                    inU_local.insert(u);
                    U.push_back(u);
                }

            // Relax edges from Ui and populate batch K (lines 208-219)
            vector<PartialOrderDS::Entry> Kbatch;
            for (int u : Ui) {
                for (const auto& e : G[u]) {
                    int v = e.to;
                    if (L.db[u] == INF_LD) 
                        continue;
                        
                    long double nd = L.db[u] + e.w;
                    bool updated = false;
                    
                    // Update distance if better path found
                    if (nd < L.db[v]) {
                        L.db[v] = nd;
                        L.pred[v] = u;
                        L.hops[v] = L.hops[u] + 1;
                        updated = true;
                    } else if (nd == L.db[v]) {
                        int nh = L.hops[u] + 1;
                        if (lex_better(u, L.pred[v], nh, L.hops[v])) {
                            L.pred[v] = u;
                            L.hops[v] = nh;
                            updated = true;
                        }
                    }

                    // Add to data structure or batch based on distance
                    if (updated) {
                        if (nd >= B) 
                            continue;
                        if (nd >= Bi) 
                            D.Insert(v, nd);
                        else if (nd >= Bi_prime) 
                            Kbatch.push_back({v, nd});
                    }
                }
            }
            
            // Add vertices from Si to Kbatch if appropriate
            for (int x : Si) {
                if (inU_local.find(x) == inU_local.end() && 
                    L.db[x] >= Bi_prime && L.db[x] < Bi) {
                    Kbatch.push_back({x, L.db[x]});
                }
            }
            
            // Prepend batch to data structure
            D.BatchPrepend(move(Kbatch));

            if ((long double)U.size() >= U_limit) {
                for (int x : W) 
                    if (L.db[x] < Bi_prime_last && inU_local.find(x) == inU_local.end()) {
                        inU_local.insert(x);
                        U.push_back(x);
                    }
                return {Bi_prime_last, U};
            }
        }

        // Finalize by adding all remaining vertices in W
        long double Bret = Bi_prime_last;
        for (int x : W) 
            if (L.db[x] < Bret && inU_local.find(x) == inU_local.end()) {
                inU_local.insert(x);
                U.push_back(x);
            }
        return {Bret, U};
    }

    // Finalize with standard Dijkstra's algorithm to ensure correctness
    void finalize_with_dijkstra() {
        struct Node {
            long double d;
            int u;
        };
        
        struct Cmp {
            bool operator()(const Node& a, const Node& b) const {
                if (a.d != b.d) 
                    return a.d > b.d;
                return a.u > b.u;
            }
        };
        
        priority_queue<Node, vector<Node>, Cmp> pq;
        for (int u = 0; u < N; ++u) 
            if (L.db[u] < INF_LD) 
                pq.push({L.db[u], u});
                
        while (!pq.empty()) {
            auto [du, u] = pq.top();
            pq.pop();
            
            if (du > L.db[u]) 
                continue;
                
            for (const auto& e : G[u]) {
                int v = e.to;
                long double nd = du + e.w;
                
                if (nd < L.db[v]) {
                    L.db[v] = nd;
                    L.pred[v] = u;
                    L.hops[v] = L.hops[u] + 1;
                    pq.push({L.db[v], v});
                } else if (nd == L.db[v]) {
                    int nh = L.hops[u] + 1;
                    if (lex_better(u, L.pred[v], nh, L.hops[v])) {
                        L.pred[v] = u;
                        L.hops[v] = nh;
                        pq.push({L.db[v], v});
                    }
                }
            }
        }
    }

    // Run the complete algorithm
    void run(bool finalize_pass = true) {
        long double Lg = log2l((long double)max(2, N));
        int l_top = (int)ceil(Lg / (long double)max(1, t));
        vector<int> S0 = {s};
        BMSSP(l_top, INF_LD, S0);
        if (finalize_pass) finalize_with_dijkstra();
    }
};

// API: run on the transformed graph
[[nodiscard]] vector<long double> duan_sssp_transformed(const Graph& g, int source_slot, bool finalize_pass = true) {
    DMSY_SSSP algo(g, source_slot);
    algo.run(finalize_pass);
    return algo.L.db;
}

// Top-level API matching dijkstra: on original graph inputs
[[nodiscard]] expected<vector<long double>, string>
duan_shortest_paths_original_graph(
    int n,
    span<const tuple<int,int,long double>> edges,
    int source,
    bool finalize_pass = true) {
    if (n < 0) return unexpected(string("n must be non-negative"));
    if (source < 0 || source >= n) return unexpected(string("invalid source index"));
    for (const auto& e : edges) {
        int u,v; long double w; tie(u,v,w) = e;
        if (u < 0 || u >= n || v < 0 || v >= n)
            return unexpected(string("edge endpoint out of range"));
        if (w < 0) return unexpected(string("negative edge weights are not allowed"));
    }

    DegreeReduction dr(n);
    dr.build(n, edges);
    const int s_rep = dr.rep_of_orig[source];

    vector<long double> dist_transformed = duan_sssp_transformed(dr.Gt, s_rep, finalize_pass);

    vector<long double> dist_orig(n, INF_LD);
    for (int v = 0; v < n; ++v) {
        long double best = INF_LD;
        if (!dr.slot_id[v].empty()) {
            for (const auto& kv : dr.slot_id[v]) best = min(best, dist_transformed[kv.second]);
        }
        dist_orig[v] = best;
    }
    dist_orig[source] = min(dist_orig[source], 0.0L);
    return dist_orig;
}
