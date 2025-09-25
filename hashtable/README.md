# Hashtable Playground

Two flavours of separate-chaining hash tables explore different trade-offs between simplicity and throughput:

- `hashtable.h` implements a straightforward version with dynamically sized vectors for metrics tracking.
- `optimized/` hosts a more performance-lean variant that favours power-of-two bucket counts, string-specific hashing, and fixed-size telemetry buffers.

Both are paired with console drivers (`hashtable_test.cpp` and `optimized/hashtable_optimized_test.cpp`) that exercise the full API and print diagnostic information.

## Core API

- `put`, `get`, `contains`, `remove`
- Load-factor inspection and collision statistics
- Runtime instrumentation: rolling latency and throughput metrics
- Administrative helpers to resize, swap hash functions, or no-op

## Build & Run

From this directory:

```sh
make           # builds hashtable_test
./hashtable_test

make optimized # builds the optimized variant via sub-make
./optimized/hashtable_optimized_test
```

See [`optimized/README.md`](optimized/README.md) for details specific to the tuned implementation.
