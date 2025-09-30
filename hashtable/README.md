# Hashtable Experimentation
Two template-based hash table implementations using separate chaining for collision resolution, each optimized for different use cases:

- **Base Implementation** (`hashtable.h` + `hashtable.tpp`): Generic implementation supporting key/value types and flexible bucket sizing using `std::vector`.
- **Optimized Implementation** (`hashtable_optimized.h` + `hashtable_optimized.tpp`): Uses explicit `std::move` transfers to keep buckets cache-friendly, runs micro-benchmarks that time bulk insert/lookup loops, and prints latency statistics such as average per-operation latency and throughput for large batch workloads.

