# HFT Hash Table

Fast Hash table for symbol lookup.

## Build & Run

```bash
make bench
```

## Performance

- **p99 lookup:** 42ns (target: <50ns)
- **Throughput:** ~20M ops/sec
- **Workload:** 70% lookup, 20% insert, 10% delete

## Usage

```cpp
#include "include/hft_hashtable.h"
using namespace hft;

HFTHashTable<uint64_t, OrderBook*, 2048> symbols;
symbols.put(symbol_id, &book);
OrderBook** book = symbols.get(symbol_id);
```

## Design

- Robin Hood hashing with backshift deletion
- Cache-line aligned (64 bytes)
- Zero allocation in hot path
- Software prefetch on lookup
