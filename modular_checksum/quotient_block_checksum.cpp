#include <iostream>

constexpr long long MOD = 1'000'000'007;
constexpr long long INV2 = 500'000'004;  // Modular inverse of 2
constexpr long long INV6 = 166'666'668;  // Modular inverse of 6

// Computes 1 + 2 + ... + x (mod MOD) using formula: x(x+1)/2
inline long long sum_1_to_n(long long x) {
    if (x <= 0) return 0;
    long long x_mod = x % MOD;
    return x_mod * (x_mod + 1) % MOD * INV2 % MOD;
}

// Computes 1^2 + 2^2 + ... + x^2 (mod MOD) using formula: x(x+1)(2x+1)/6
inline long long sum_squares_1_to_n(long long x) {
    if (x <= 0) return 0;
    long long x_mod = x % MOD;
    return x_mod * (x_mod + 1) % MOD * (2 * x_mod + 1) % MOD * INV6 % MOD;
}

// Computes l + (l+1) + ... + r (mod MOD)
inline long long sum_range(long long left, long long right) {
    long long res = (sum_1_to_n(right) - sum_1_to_n(left - 1)) % MOD;
    return res < 0 ? res + MOD : res;
}

// Computes l^2 + (l+1)^2 + ... + r^2 (mod MOD)
inline long long sum_squares_range(long long left, long long right) {
    long long res = (sum_squares_1_to_n(right) - sum_squares_1_to_n(left - 1)) % MOD;
    return res < 0 ? res + MOD : res;
}

// Computes sum of (i % j) + (j % i) for all pairs 1 <= i,j <= n
// Uses O(sqrt(n)) block decomposition where floor(n/j) is constant
#if defined(__GNUC__) || defined(__clang__)
__attribute__((hot))
#endif
int compute(int n) {
    if (n <= 0) return 0;

    long long total = 0;
    long long n_mod = n % MOD;
    long long n_squared_plus_n = (n_mod * n_mod + n_mod) % MOD;
    long long n_plus_1 = n_mod + 1;

    // Process blocks where floor(n/j) = q is constant
    for (long long j = 1; j <= n; ) {
        long long quotient = n / j;
        long long block_end = n / quotient;  // Last j with same quotient
        long long block_size = block_end - j + 1;

        // Sum of j values and j^2 values in this block
        long long sum_j = sum_range(j, block_end);
        long long sum_j_squared = sum_squares_range(j, block_end);

        // Compute block contribution using derived formula
        long long q_mod = quotient % MOD;
        long long q_plus_1 = q_mod + 1;

        // term1 = q(q+1) * sum(j^2)
        long long term1 = q_mod * q_plus_1 % MOD * sum_j_squared % MOD;

        // term2 = 2q(n+1) * sum(j)
        long long term2 = 2 * q_mod % MOD * n_plus_1 % MOD * sum_j % MOD;

        // term3 = (n^2 + n) * block_size
        long long term3 = n_squared_plus_n * (block_size % MOD) % MOD;

        // block_contribution = (term1 - term2 + term3) / 2
        long long bracket = (term1 - term2 + term3) % MOD;
        if (bracket < 0) bracket += MOD;
        long long block_contribution = INV2 * bracket % MOD;

        total = (total + block_contribution) % MOD;
        j = block_end + 1;
    }

    // Final answer is 2 * total (accounts for symmetry in i,j pairs)
    return static_cast<int>((2 * total) % MOD);
}

#ifndef CHECKSUM_AGGREGATION_NO_MAIN
int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    int n;
    if (std::cin >> n) {
        std::cout << compute(n) << '\n';
    }
    return 0;
}
#endif
