# Faster LRU

C++23 templated least-recently-used cache backed by a `doubly linked list` and `unordered_map`.

## Components
- `lru.h` / `lru.tpp`: Header-only cache declaration and template implementation
- `main.cpp`: C++23 features including `std::expected`, move semantics, and monadic operations (https://www.cppstories.com/2023/monadic-optional-ops-cpp23/)
- `performance_test.cpp`: Microbenchmark (100k sets + 10k lookups) showing ~0.09 μs/op
