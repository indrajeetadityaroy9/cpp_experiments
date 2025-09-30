# Faster Dijkstra

Experiment implementing *Breaking the Sorting Barrier for Directed SSSP* ([arXiv:2504.17033v2](https://arxiv.org/abs/2504.17033)). Compares baseline Dijkstra implementation with an attempted port of the Duan–Mehlhorn–Shao–Su–Zhang deterministic algorithm.

- `dijkstra.cpp`: baseline routine with a degree-reduction pre-processing step.
- `Duan_deterministic_sssp.cpp`: prototype of the paper's algorithm including the `PartialOrderDS` structure.
- `benchmark.cpp`: generates random graphs, runs both algorithms, and cross-validates results.

## Observations
- The Duan prototype matches theoretical complexity on paper but currently loses to baseline Dijkstra on practical inputs.
- `PartialOrderDS` introduces deep recursion and cache-unfriendly access patterns that dominate runtime at moderate graph sizes.
