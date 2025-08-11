 ## Breaking the Sorting Barrier for Directed Single-Source Shortest Paths
 Based on the paper: https://arxiv.org/abs/2504.17033v2

This implementation provides a deterministic single-source shortest path algorithm
with O(m*log^(2/3)*n) time complexity, which is theoretically faster than Dijkstra's
O(m+n*log*n) algorithm on sparse graphs where m = O(n).