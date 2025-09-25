# C++ Experiments

Small C++17 playground exploring data structures and performance-minded algorithms. Each folder is self-contained with its own README and sample program.

## Projects

- [Checksum Aggregation](checksum_aggregation/README.md): modular checksum routine.
- [Custom Vector](custom_vector/README.md): handmade `vector<T>` implementation.
- [Hashtable Playground](hashtable/README.md): baseline and tuned separate-chaining maps.
- [Faster LRU](faster_lru/README.md): templated cache with a timing harness.
- [Faster Dijkstra](faster_dijkstra/README.md): Dijkstra baseline vs. Duan et al. prototype.

## Build

Requires a C++17 compiler. Each project ships with its own `Makefile`; run `make` inside the folder you want to try:

```sh
cd checksum_aggregation && make
./sum
```

See the per-project READMEs for extra flags or sample commands.
