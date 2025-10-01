# Duan et al. Deterministic SSSP 

A from-scratch implementation of the Duan-Mehlhorn-Shao-Su-Zhang deterministic single-source shortest path (SSSP) algorithm with **O(m·log^(2/3)(n))** complexity.

1. **PartialOrderDS** (Lemma 3.1): Block-based data structure for adaptive problem partitioning
   - Insert: O(max{1, log(N/M)}) amortized
   - BatchPrepend: O(L·log(L/M)) amortized
   - Pull: O(M) amortized

2. **FindPivots** (Algorithm 1): k-step bounded relaxation to identify pivot vertices
   - Time: O(min{k²|S|, k|U|})

3. **BaseCase** (Algorithm 2): Mini-Dijkstra for base case (layer l=0)
   - Time: O(k·log(k))

4. **BMSSP** (Algorithm 3): Main recursive bounded multi-source shortest path
   - Combines all components with adaptive recursion
