# Duan Deterministic SSSP Implementation

Experimental implementation of the theoretical Duan-Mehlhorn-Shao-Su-Zhang deterministic single-source shortest path algorithm achieving **O(m·log^(2/3)(n))** complexity ([arXiv:2504.17033v2](https://arxiv.org/abs/2504.17033)).

### Notes
- Current implementation achieves optimal O(m·log^(2/3)(n)) complexity on constant-degree graphs
- For general graphs, degree reduction preprocessing is needed
- All edge weights must be non-negative

### Core Components

**1. PartialOrderDS** (`partial_order_ds.hpp`) - Lemma 3.1
- Block-based data structure for adaptive problem partitioning
- **Insert**: O(max{1, log(N/M)}) amortized
- **BatchPrepend**: O(L·log(L/M)) amortized
- **Pull**: O(M) amortized
- Uses two block sequences (D_0, D_1) with median-based splitting

**2. FindPivots** (`find_pivots.hpp`) - Algorithm 1
- k-step bounded relaxation to identify pivot vertices
- Early termination when |W| > k|S| to maintain complexity
- Builds predecessor forest and identifies roots with large subtrees
- **Time**: O(min{k^2|S|, k|W|})

**3. BaseCase** (`base_case.hpp`) - Algorithm 2
- Mini-Dijkstra for base case (layer l=0)
- Runs until finding k+1 closest vertices
- **Time**: O(k·log k) using binary heap

**4. BMSSP** (`bmssp.hpp`) - Algorithm 3
- Main recursive bounded multi-source shortest path algorithm
- Combines FindPivots and PartialOrderDS for adaptive recursion
- Returns boundary value b and complete vertex set U
- **Time**: O(m·log^(2/3)(n)) total across all recursive calls

**5. DuanSSSP** (`duan_sssp.hpp`) - Top-level interface
- Computes parameters k = ⌊log^(1/3)(n)⌋, t = ⌊log^(2/3)(n)⌋
- Initializes distance labels and invokes BMSSP
- Provides result with distances, predecessors, and statistics
