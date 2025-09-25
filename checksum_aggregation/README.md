# Checksum Aggregation

Efficient checksum calculator that keeps every operation in modular arithmetic to avoid overflow. The implementation focuses on tight integer math and precomputes reusable terms inside the loop for better cache behaviour.

## Highlights

- Uses the prime modulus `1_000_000_007` with lightweight inline helpers for add, subtract, and multiply.
- Computes the aggregation in `O(n)` by grouping contributions into blocks derived from `floor(n / j)`.
- Exposes the core routine via `computeChecksumAggregation(int n)` and a tiny CLI that reads `n` from stdin and prints the checksum.

## Build & Run

From this directory:

```sh
make
./sum < input.txt
```

You can also compile the file directly:

```sh
g++ -std=c++17 -O3 -Wall -Wextra sum.cpp -o sum
```

## Example

```
echo 1000000 | ./sum
```

Use the module as a reference for integer-heavy loops or embed `computeChecksumAggregation` in larger code bases.
