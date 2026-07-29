# Robin Hood Hash Table — Vectorized Invariant Probing

Fixed-capacity, allocation-free open-addressing hash table (ARM64-only: NEON + CRC32 required) where the Robin Hood ordering invariant itself is the SIMD probe predicate.

## Build & Run

```bash
make bench   # comparison benchmark
make run
make test    # Catch2 differential suite under ASan+UBSan
```

## The mechanism

One metadata byte per slot: `0` = empty, `d+1` = occupied at displacement `d`. Two invariants hold across all operations — home buckets are non-decreasing along a probe run, and runs are gap-free (backshift deletion, no tombstones). Because of them, a probe of home bucket `h` needs only one NEON comparison pair per 16 slots, against the ramp `{offset+1}`:

- `meta == ramp` lanes are exactly the entries homed at `h` (displacement identity) — the only slots ever touched for key comparison; no fingerprints needed, zero false positives;
- the first `meta < ramp` lane is the exact Robin Hood miss-termination (empty slots are the `meta == 0` case of the same test). Unlike Swiss-table designs, termination never depends on finding an empty slot, so it holds even in a completely full table.

Everything is one execution path: `__crc32cd` (hardware CRC32C, measured 2x faster than splitmix64) produces the home bucket; a single `find_slot` primitive drives `get`, `put`, and `erase`; insertion is a two-phase shift (validate, then move — a rejected insert mutates nothing); deletion backshifts to keep runs gap-free.

All bounds are derived, not chosen: NEON registers scan 16 bytes, ramp values must fit uint8, so the probe window is at most `(255/16)*16 = 240` slots and the displacement cap is `window − 2 = 238` (or `Capacity − 1` if smaller). Inserts that would exceed the cap fail cleanly ("effectively full") — the theory-backed alternative to operating near load 1 (arXiv 2501.11582). The metadata array carries a window-length mirror of its prefix so group loads never wrap.

## Usage

```cpp
#include "robin_hood.h"
using namespace robin_hood;

RobinHoodTable<uint64_t, uint64_t, 8192> symbols;   // integral keys only
symbols.put(id, value);         // true = present (insert or update); false = effectively full
uint64_t* v = symbols.get(id);  // nullptr if absent
symbols.erase(id);
```

## Measured results

Apple M3 Pro, `-O3`, 8192 slots, random `uint64` keys, 95% lookup / 5% overwrite, 1M ops x 5 interleaved trials; latency percentiles are batch-of-64 averages (timer resolution ~42ns). Mean of repeated runs, ns / Mops:

| Load | Table | p50 | p99 | p99.9 | Mops |
|------|-------|-----|-----|-------|------|
| 50% | this table | 5.0 | 8.0 | 9.1 | 185 |
| 50% | std::unordered_map | 4.5 | 6.5 | 7.3 | 231 |
| 70% | this table | 7.0 | 9.6 | 11.0 | 147 |
| 70% | std::unordered_map | 5.7 | 7.8 | 9.3 | 175 |
| 90% | this table | 8.0 | 12.0 | 13.5 | 119 |
| 90% | std::unordered_map | 7.0 | 9.3 | 10.9 | 143 |

Versus the previous scalar Robin Hood implementation of this table: +17% throughput at 50% load, +62% at 70%, +70% at 85%, +70% at 90%; p50 at 90% load fell from 14ns to 8ns. `std::unordered_map` still leads on this workload (identity hash on random keys, ~1-node chains); this table's value is the fixed footprint (~136KB at 8192 slots)
