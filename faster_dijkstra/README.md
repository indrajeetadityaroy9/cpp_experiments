# Faster Dijkstra

Experiments around single-source shortest paths inspired by *Breaking the Sorting Barrier for Directed SSSP* ([arXiv:2504.17033v2](https://arxiv.org/abs/2504.17033)). The directory contrasts a classical Dijkstra implementation with a work-in-progress port of the Duan–Mehlhorn–Shao–Su–Zhang deterministic algorithm and several benchmarking harnesses.

## Components

- `dijkstra.cpp`: baseline routine with a degree-reduction pre-processing step.
- `Duan_deterministic_sssp.cpp`: prototype of the paper's algorithm including the bespoke `PartialOrderDS` structure.
- `benchmark.cpp`: generates random graphs, runs both algorithms, and cross-validates results.
- `benchmark_parallel.cpp`: OpenMP-enabled flavour for multi-thread experiments.
- `Makefile`: builds the executables (`dijkstra`, `duan_sssp`, `benchmark`, `benchmark_parallel`, and test utilities).
- `test_graph.txt`: small sample input for sanity checks.

## Observations

- The Duan prototype matches theoretical complexity on paper but currently loses to classical Dijkstra on most practical inputs.
- `PartialOrderDS` introduces deep recursion and cache-unfriendly access patterns that dominate runtime at moderate graph sizes.
- The expected crossover point (`m_crit`) occurs only on very sparse graphs with tens of thousands of edges, making empirical wins hard to reproduce.

Use these experiments as a sandbox for profiling, algorithm tweaks, or alternative data structures.
