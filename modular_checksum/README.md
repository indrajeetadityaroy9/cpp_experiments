# Checksum Aggregation

Efficient checksum calculator that computes `∑∑ [(i % j) + (j % i)]` for all pairs `(i, j)` where `1 ≤ i, j ≤ n` in **O(√n)** time instead of naive O(n²).

## Algorithm

The key insight is that `floor(n/j)` takes only O(√n) distinct values. For each block where the quotient is constant, contributions are computed using closed-form sum formulas:

```cpp
for (long long j = 1; j <= n;) {
    long long q = n / j;           // quotient
    long long next = n / q;        // last j with same quotient
    // compute block contribution using sum formulas
    j = next + 1;
}
```

All operations use modular arithmetic with `MOD = 10^9 + 7` and precomputed inverses (`INV2`, `INV6`) for division.


## Test Results

```
All tests passed (207 assertions in 3 test cases)
```

| Test Case | Description |
|-----------|-------------|
| Naive comparison (n=1..200) | Validates against O(n²) reference implementation |
| Edge cases | n=0, n=1, n=2 boundary conditions |
| Modular helpers | Overflow handling, triangle number formulas |

## Performance

The O(√n) complexity enables computation for very large n:

| n | Result | Time |
|---|--------|------|
| 10 | 430 | <1ms |
| 1,000 | 451542898 | <1ms |
| 1,000,000 | 291540618 | <1ms |
| 1,000,000,000 | 861363376 | ~5ms |