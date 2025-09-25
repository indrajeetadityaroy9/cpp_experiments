#include <iostream>
using namespace std;

static const long long MOD = 1000000007;
static const long long INV2 = 500000004;
static const long long INV6 = 166666668;

#if defined(__GNUC__)
inline long long mul(long long a, long long b) {
    return static_cast<long long>(
        (static_cast<unsigned __int128>(a) * static_cast<unsigned __int128>(b)) % MOD);
}
#else
inline long long mul(long long a, long long b) {
    return ((a % MOD) * (b % MOD)) % MOD;
}
#endif

inline long long add(long long a, long long b) {
    long long s = a + b;
    if (s >= MOD)
        s -= MOD;
    return s;
}

inline long long sub(long long a, long long b) {
    long long s = a - b;
    if (s < 0)
        s += MOD;
    return s;
}

inline long long normalize(long long v) {
    long long res = v % MOD;
    if (res < 0)
        res += MOD;
    return res;
}

inline long long sumUpTo(long long x) {
    if (x <= 0)
        return 0;
    long long x_mod = normalize(x);
    long long xp1 = add(x_mod, 1);
    return mul(mul(x_mod, xp1), INV2);
}

inline long long sumSquaresUpTo(long long x) {
    if (x <= 0)
        return 0;
    long long x_mod = normalize(x);
    long long xp1 = add(x_mod, 1);
    long long two_x = mul(2, x_mod);
    long long two_x_p1 = add(two_x, 1);
    return mul(mul(mul(x_mod, xp1), two_x_p1), INV6);
}

inline long long sumRange(long long l, long long r) {
    return sub(sumUpTo(r), sumUpTo(l - 1));
}

inline long long sumSquaresRange(long long l, long long r) {
    return sub(sumSquaresUpTo(r), sumSquaresUpTo(l - 1));
}

#if defined(__GNUC__)
__attribute__((hot))
#endif
int computeChecksumAggregation(int n) {
    long long total = 0;
    long long n_mod = normalize(n);
    long long n_sq_plus_n = add(mul(n_mod, n_mod), n_mod);
    long long n_plus1 = add(n_mod, 1);
    // Collapse ranges where floor(n / j) stays constant.
    for (long long j = 1; j <= n;) {
        long long q = n / j;
        long long next = n / q;
        long long count = next - j + 1;

        long long sum_j = sumRange(j, next);
        long long sum_j2 = sumSquaresRange(j, next);

        long long q_mod = q % MOD;
        long long q_plus1 = add(q_mod, 1);
        long long q_pair = mul(q_mod, q_plus1);
        long long term1 = mul(q_pair, sum_j2);

        long long two_q = mul(2, q_mod);
        long long two_q_n_plus1 = mul(two_q, n_plus1);
        long long term2 = mul(two_q_n_plus1, sum_j);

        long long term3 = mul(n_sq_plus_n, count % MOD);

        long long bracket = add(sub(term1, term2), term3);
        long long block = mul(INV2, bracket);
        total = add(total, block);
        j = next + 1;
    }
    long long answer = mul(2, total);
    return static_cast<int>(answer);
}
#ifndef CHECKSUM_AGGREGATION_NO_MAIN
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int n;
    cin >> n;
    cout << computeChecksumAggregation(n) << "\n";
    return 0;
}
#endif
