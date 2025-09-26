# Faster LRU

Practice project that implements a templated least-recently-used cache backed by a doubly linked list and an `unordered_map`. The implementation is header-only, so the cache can be instantiated for any value type without needing explicit instantiations.

## Components

- `lru.h` / `lru.tpp`: cache declaration and inline implementation. `get` returns `std::optional<T>` to distinguish cache misses, while `has` is a const probe that does not change entry ordering.
- `main.cpp`: demonstrates eviction behaviour on a small cache of strings.
- `performance_test.cpp`: runs a microbenchmark of 100k `set` operations followed by 10k lookups.
- `Makefile`: builds the example binary (`lru_example`) and the benchmark (`performance_test`).
