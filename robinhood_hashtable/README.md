# Robin Hood Hash Table

Fixed-capacity, allocation-free open-addressing hash table with Robin Hood probing and backshift deletion, aimed at low-latency symbol lookup.

## Build & Run

```bash
make bench   
make run
```

## Design

- **Split metadata / slot layout** (Swiss-table style): occupancy and probe distance are fused into a single byte per slot (`0` = empty, `d + 1` = occupied at probe distance `d`), stored in a dense array separate from the key/value slots. Probe loops scan only metadata — one cache line covers 64+ slots — and touch the slot array once per candidate match.
- **Robin Hood insertion**: `put` probes and displaces in a single pass; when the resident's probe distance drops below the candidate's, the scan position is reused directly as the insertion point (no restart from the home bucket).
- **Backshift deletion**: `erase` slides displaced successors back one slot instead of leaving tombstones, so probe chains never degrade over time.
- **Zero allocation**: capacity is a compile-time power of two (`RobinHoodTable<Key, Value, Capacity>`); all storage is in-object. No resize — `put` returns `false` when full.
- **Hashing**: `splitmix64` finalizer for integral keys, `std::hash` otherwise; power-of-two masking for bucket selection.

## Usage

```cpp
#include "robin_hood.h"
using namespace robin_hood;

RobinHoodTable<uint64_t, uint64_t, 8192> symbols;
(void)symbols.put(symbol_id, value);
uint64_t* v = symbols.get(symbol_id);   // nullptr if absent
symbols.erase(symbol_id);
```

## Performance

Measured on Apple Silicon (macOS, `-O3`), 8192-bucket table, uniformly random `uint64` keys, 95% lookup / 5% overwrite, 1M ops x 5 interleaved trials per configuration. Mean per-op latency percentiles in ns:

| Load | Table | p50 | p99 | p99.9 | Mops |
|------|-------|-----|-----|-------|------|
| 50% | RobinHoodTable | 6.2 | 8.6 | 10.2 | 158 |
| 50% | std::unordered_map | 4.4 | 7.0 | 7.8 | 229 |
| 70% | RobinHoodTable | 11.0 | 14.2 | 16.6 | 91 |
| 70% | std::unordered_map | 6.0 | 8.0 | 9.8 | 174 |
| 90% | RobinHoodTable | 14.0 | 18.2 | 20.8 | 70 |
| 90% | std::unordered_map | 7.0 | 9.6 | 11.0 | 145 |
