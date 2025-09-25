#include "sum.cpp"

#include <iostream>

static long long naiveChecksum(int n) {
    long long total = 0;
    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= n; ++j) {
            total += (i % j) + (j % i);
        }
    }
    return total % MOD;
}

int main() {
    for (int n = 1; n <= 200; ++n) {
        long long expected = naiveChecksum(n);
        long long observed = computeChecksumAggregation(n);
        if (expected != observed) {
            std::cout << "Mismatch at n=" << n << ": expected " << expected
                      << ", got " << observed << '\n';
            return 1;
        }
    }
    std::cout << "checksum aggregation sanity check passed (n up to 200)\n";
    return 0;
}
