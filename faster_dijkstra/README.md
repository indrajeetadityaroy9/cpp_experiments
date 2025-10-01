# Duan Deterministic SSSP Implementation

Experimental implementation of the Duan-Mehlhorn-Shao-Su-Zhang deterministic single-source shortest path algorithm achieving **O(m·log^(2/3)(n))** complexity. (([arXiv:2504.17033v2](https://arxiv.org/abs/2504.17033)))

- PartialOrderDS: Block-based partial ordering (Lemma 3.1)
- FindPivots: k-step bounded relaxation (Algorithm 1)
- BaseCase: Mini-Dijkstra for base case (Algorithm 2)
- BMSSP: Main recursive algorithm (Algorithm 3)

