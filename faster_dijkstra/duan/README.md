# Duan et al. Deterministic SSSP 

A from-scratch implementation of the Duan-Mehlhorn-Shao-Su-Zhang deterministic single-source shortest path (SSSP) algorithm with **O(m·log^(2/3)(n))** complexity.

1. **PartialOrderDS** (Lemma 3.1): Block-based data structure for adaptive problem partitioning
   - Insert: O(max{1, log(N/M)}) amortized
   - BatchPrepend: O(L·log(L/M)) amortized
   - Pull: O(M) amortized

2. **FindPivots** (Algorithm 1): k-step bounded relaxation to identify pivot vertices
   - Time: O(min{k²|S|, k|U|})

3. **BaseCase** (Algorithm 2): Mini-Dijkstra for base case (layer l=0)
   - Time: O(k·log(k))

4. **BMSSP** (Algorithm 3): Main recursive bounded multi-source shortest path
   - Combines all components with adaptive recursion

## Project Structure

```
duan/
├── include/              # Header files
│   ├── common.hpp        # Core data structures (Graph, Edge, Params)
│   ├── labels.hpp        # Distance labels with tie-breaking
│   ├── partial_order_ds.hpp
│   ├── find_pivots.hpp
│   ├── base_case.hpp
│   ├── bmssp.hpp
│   └── duan_sssp.hpp     # Top-level SSSP interface
│
├── src/                  # Implementation files
│   ├── partial_order_ds.cpp
│   ├── find_pivots.cpp
│   ├── base_case.cpp
│   ├── bmssp.cpp
│   └── duan_sssp.cpp
│
├── tests/                # Unit tests
│   ├── test_partial_order_ds.cpp
│   ├── test_find_pivots.cpp
│   ├── test_base_case.cpp
│   ├── test_bmssp.cpp
│   └── test_complexity.cpp
│
├── Makefile
└── README.md
```

## Building and Testing

### Prerequisites
- C++23 compatible compiler (g++/clang++)
- Make

### Build All Tests
```bash
make
```

### Run Unit Tests
```bash
make test
```

Expected output:
```
PartialOrderDS tests:  11/11 PASSED
FindPivots tests:       9/9 PASSED
BaseCase tests:         9/9 PASSED
BMSSP tests:            8/8 PASSED
───────────────────────────────────
Total:                 37/37 PASSED ✓
```

### Run Complexity Analysis
```bash
make complexity
```

### Clean Build
```bash
make clean
```

## Test Results

All core components pass comprehensive unit tests:

- **PartialOrderDS**: 11/11 tests passing
  - Initialization, insert, pull operations
  - Batch prepend (small/large sets)
  - Duplicate handling, ordering preservation

- **FindPivots**: 9/9 tests passing
  - k-step relaxation, early exit conditions
  - Multiple sources, bounded relaxation
  - Lexicographic tie-breaking

- **BaseCase**: 9/9 tests passing
  - Bounded Dijkstra, various graph structures
  - Tie-breaking, weighted graphs

- **BMSSP**: 8/8 tests passing
  - Recursive calls, layer handling
  - Graph structures (path, star, grid, diamond)

## Implementation Notes

### Lexicographic Tie-Breaking
Paths are ordered as tuples `⟨length, hops, v_α, v_{α-1}, ..., v_1⟩` to ensure uniqueness, allowing O(1) comparison during relaxation.

### Block Structure
PartialOrderDS maintains two sequences:
- **D₀**: Elements from batch prepends (no size bound)
- **D₁**: Elements from inserts (O(N/M) blocks with BST)

### Instrumentation
The implementation includes operation counting for:
- Edge relaxations
- Data structure operations (Insert, Pull, BatchPrepend)
- BMSSP recursive calls
- Maximum recursion depth

## Known Limitations

1. **No Degree Reduction**: This implementation works on any graph but achieves optimal O(m·log^(2/3)(n)) complexity only on constant-degree graphs. The original paper includes a degree reduction transformation not implemented here.

2. **Bounded Problem Focus**: BMSSP is designed for bounded multi-source shortest path subproblems. Full standalone SSSP requires additional wrapper logic for iterative boundary management.

3. **Graph Size**: Current parameters `k = ⌊log^(1/3)(n)⌋` and `t = ⌊log^(2/3)(n)⌋` work best for graphs with n ≥ 8.

## Compilation Flags

All code compiles with strict warnings:
```bash
-std=c++23 -O2 -Wall -Wextra -Werror
```

## Reference

Duan, R., Mehlhorn, K., Shao, R., Su, H., & Zhang, R. (Year). Deterministic SSSP in O(m·log^(2/3)(n)) Time. [Paper details]

## License

Educational implementation for algorithm study and analysis.
