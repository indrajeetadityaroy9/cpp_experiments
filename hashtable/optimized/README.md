# Optimized Hashtable

Drop-in replacement for the baseline hash table that focuses on cache-friendliness and predictable throughput. The implementation lives in `hashtable_optimized.h` and is exercised by `hashtable_optimized_test.cpp`.

## What Changed Compared to the Baseline

- **Buckets**: Always rounded up to a power of two so index lookups use bit masks instead of division.
- **Hashing**: FNV-1a for strings with extra bit mixing to reduce clustering; falls back to `std::hash` for other types.
- **Move Semantics**: Overloads `put(Key&&, Value&&)` to minimize copies for heavy keys/values.
- **Telemetry**: Fixed-size circular buffers (`MAX_TRACKED_OPS = 1000`) retain recent latency samples without growing vectors.

## Build & Run

From this directory:

```sh
make
./hashtable_optimized_test
```

The console program walks through CRUD operations, hash-function swaps, and a larger-scale performance test. Use this module as a starting point for profiling experiments or integrating a custom hash function.
