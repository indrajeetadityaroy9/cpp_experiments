#include <iostream>
using namespace std;

static const long long MOD = 1000000007;

inline long long mul(long long a, long long b) { 
    return ((a % MOD) * (b % MOD)) % MOD; 
}

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

int computeChecksumAggregation(int n) {
    long long inv2 = (MOD + 1) / 2, total = 0;
    for (long long j = 1; j <= n; ++j) {
        long long q = n / j;
        long long r = n % j;
        long long j_mod = j % MOD;
        long long jm1 = sub(j_mod, 1);
        long long full = mul(mul(j_mod, jm1), inv2);
        long long t1 = mul(q % MOD, full);
        long long r_mod = r % MOD;
        long long rp1 = add(r_mod, 1);
        long long t2 = mul(mul(r_mod, rp1), inv2);
        long long block = add(t1, t2);
        total = add(total, block);
    }
    long long answer = mul(2, total);
    return static_cast<int>(answer);
}
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int n;
    cin >> n;
    cout << computeChecksumAggregation(n) << "\n";
    return 0;
}