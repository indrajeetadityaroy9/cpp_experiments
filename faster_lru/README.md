# Faster LRU

Practice project that implements a templated least-recently-used cache backed by a doubly linked list and an unordered_map. The folder contains both a minimal usage example and a simple microbenchmark.

## Components

- `lru.h` / `lru.cpp`: cache definition with node pooling, `set`, `get`, and `has` operations, plus explicit template instantiations for a few common value types.
- `main.cpp`: demonstrates eviction behaviour on a small cache of strings.
- `performance_test.cpp`: times 100k `set` operations followed by 10k lookups.
- `Makefile`: builds the example binary (`lru_example`) and the benchmark (`performance_test`).

