## Breaking the Sorting Barrier for Directed Single-Source Shortest Paths
Based on the paper: https://arxiv.org/abs/2504.17033v2

Attempted implementation for provides a deterministic single-source shortest path algorithm
with O(m*log^(2/3)*n) time complexity, which is theoretically faster than Dijkstra's
O(m+n*log*n) algorithm on sparse graphs where m = O(n).

Despite the better theoretical complexity, the Duan implementation is not consistently faster than standard Dijkstra's algorithm in practice.The PartialOrderDS data structure is much more complex than a simple priority queue. Multiple levels of recursion add function call overhead.Complex memory access patterns affect cache performance

The theoretical advantage only applies when:
- Graphs are very sparse (m = O(n))
- The number of edges is below the crossover point (m_crit)
- Benchmarks show m_crit values in the tens of thousands, which is quite high
