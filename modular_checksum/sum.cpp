#include <iostream>

namespace checksum {

constexpr long long MOD = 1'000'000'007;
constexpr long long INV2 = 500'000'004;  // Modular inverse of 2
constexpr long long INV6 = 166'666'668;  // Modular inverse of 6


#if defined(__GNUC__) || defined(__clang__)
inline long long mul(long long a, long long b) {
    using u128 = unsigned __int128;
    return static_cast<long long>((static_cast<u128>(a) * static_cast<u128>(b)) % MOD);
}
#else
inline long long mul(long long a, long long b) {
    return ((a % MOD) * (b % MOD)) % MOD;
}
#endif

inline long long add(long long a, long long b) {
    long long sum = a + b;
    return (sum >= MOD) ? sum - MOD : sum;
}

inline long long sub(long long a, long long b) {
    long long diff = a - b;
    return (diff < 0) ? diff + MOD : diff;
}

inline long long normalize(long long value) {
    long long result = value % MOD;
    return (result < 0) ? result + MOD : result;
}


// Computes 1 + 2 + ... + x (mod MOD) using formula: x(x+1)/2
inline long long sum_1_to_n(long long x) {
    if (x <= 0) return 0;
    long long x_mod = normalize(x);
    long long x_plus_1 = add(x_mod, 1);
    return mul(mul(x_mod, x_plus_1), INV2);
}

// Computes 1^2 + 2^2 + ... + x^2 (mod MOD) using formula: x(x+1)(2x+1)/6
inline long long sum_squares_1_to_n(long long x) {
    if (x <= 0) return 0;
    long long x_mod = normalize(x);
    long long x_plus_1 = add(x_mod, 1);
    long long two_x_plus_1 = add(mul(2, x_mod), 1);
    return mul(mul(mul(x_mod, x_plus_1), two_x_plus_1), INV6);
}

// Computes l + (l+1) + ... + r (mod MOD)
inline long long sum_range(long long left, long long right) {
    return sub(sum_1_to_n(right), sum_1_to_n(left - 1));
}

// Computes l^2 + (l+1)^2 + ... + r^2 (mod MOD)
inline long long sum_squares_range(long long left, long long right) {
    return sub(sum_squares_1_to_n(right), sum_squares_1_to_n(left - 1));
}

// Computes sum of (i % j) + (j % i) for all pairs 1 <= i,j <= n
// Uses O(sqrt(n)) block decomposition where floor(n/j) is constant
#if defined(__GNUC__) || defined(__clang__)
__attribute__((hot))
#endif
int compute(int n) {
    if (n <= 0) return 0;

    long long total = 0;
    long long n_mod = normalize(n);

    // Precompute terms that are constant across all blocks
    long long n_squared_plus_n = add(mul(n_mod, n_mod), n_mod);  // n^2 + n
    long long n_plus_1 = add(n_mod, 1);

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
        long long q_plus_1 = add(q_mod, 1);

        // term1 = q(q+1) * sum(j^2)
        long long q_times_q_plus_1 = mul(q_mod, q_plus_1);
        long long term1 = mul(q_times_q_plus_1, sum_j_squared);

        // term2 = 2q(n+1) * sum(j)
        long long two_q_times_n_plus_1 = mul(mul(2, q_mod), n_plus_1);
        long long term2 = mul(two_q_times_n_plus_1, sum_j);

        // term3 = (n^2 + n) * block_size
        long long term3 = mul(n_squared_plus_n, block_size % MOD);

        // block_contribution = (term1 - term2 + term3) / 2
        long long bracket = add(sub(term1, term2), term3);
        long long block_contribution = mul(INV2, bracket);

        total = add(total, block_contribution);
        j = block_end + 1;
    }

    // Final answer is 2 * total (accounts for symmetry in i,j pairs)
    return static_cast<int>(mul(2, total));
}

}  // namespace checksum


#ifndef CHECKSUM_AGGREGATION_NO_MAIN
int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    int n;
    std::cin >> n;

    std::cout << checksum::compute(n) << '\n';
    return 0;
}
#endif
