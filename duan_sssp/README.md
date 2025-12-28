# Duan et al. Deterministic SSSP

A from-scratch implementation of the Duan-Mehlhorn-Shao-Su-Zhang deterministic single-source shortest path (SSSP) algorithm with **O(m·log^(2/3)(n))** complexity.

## Algorithm Components

| Component | Description | Complexity |
|-----------|-------------|------------|
| **PartialOrderDS** (Lemma 3.1) | Block-based data structure for adaptive problem partitioning | Insert: O(max{1, log(N/M)}), BatchPrepend: O(L·log(L/M)), Pull: O(M) amortized |
| **FindPivots** (Algorithm 1) | k-step bounded relaxation to identify pivot vertices | O(min{k²|S|, k|U|}) |
| **BaseCase** (Algorithm 2) | Mini-Dijkstra for base case (layer l=0) | O(k·log(k)) |
| **BMSSP** (Algorithm 3) | Main recursive bounded multi-source shortest path | Combines all components |

## Test Results

The correctness validation tests against reference Dijkstra currently fail on larger graphs. The algorithm computes correct results for small graphs tested in unit tests, but has issues with:

- Path graphs (n > 10)
- Grid graphs
- Sparse random graphs

**Status**: The individual components (PartialOrderDS, FindPivots, BaseCase, BMSSP) pass their unit tests, but the full algorithm integration needs debugging for complete correctness on all graph types.
