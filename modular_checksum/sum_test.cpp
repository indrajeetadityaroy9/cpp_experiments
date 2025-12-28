#define CHECKSUM_AGGREGATION_NO_MAIN
#include "sum.cpp"

#include <catch_amalgamated.hpp>

using namespace checksum;

// =============================================================================
// Reference Implementation
// =============================================================================

namespace {

// Naive O(n^2) implementation for correctness verification
long long naive_checksum(int n) {
    long long total = 0;
    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= n; ++j) {
            total += (i % j) + (j % i);
        }
    }
    return total % MOD;
}

}  // namespace

// =============================================================================
// Modular Arithmetic Tests
// =============================================================================

TEST_CASE("Modular addition", "[modular][add]") {
    SECTION("basic addition") {
        REQUIRE(add(2, 3) == 5);
        REQUIRE(add(0, 0) == 0);
        REQUIRE(add(MOD - 1, 0) == MOD - 1);
    }

    SECTION("wraps around at MOD") {
        REQUIRE(add(MOD - 1, 1) == 0);
        REQUIRE(add(MOD - 1, 2) == 1);
    }

    SECTION("large values near MOD") {
        long long a = MOD - 1;
        long long b = MOD - 1;
        REQUIRE(add(a, b) == (2 * MOD - 2) % MOD);
    }
}

TEST_CASE("Modular subtraction", "[modular][sub]") {
    SECTION("basic subtraction") {
        REQUIRE(sub(5, 3) == 2);
        REQUIRE(sub(10, 0) == 10);
    }

    SECTION("handles negative wrap") {
        REQUIRE(sub(0, 1) == MOD - 1);
        REQUIRE(sub(1, 2) == MOD - 1);
        REQUIRE(sub(0, MOD - 1) == 1);
    }
}

TEST_CASE("Modular multiplication", "[modular][mul]") {
    SECTION("basic multiplication") {
        REQUIRE(mul(2, 3) == 6);
        REQUIRE(mul(0, 1000) == 0);
        REQUIRE(mul(1, MOD - 1) == MOD - 1);
    }

    SECTION("large values") {
        // Verify no overflow for large inputs
        REQUIRE(mul(MOD - 1, MOD - 1) == 1);  // (-1)^2 = 1 mod MOD
    }
}

TEST_CASE("Normalize function", "[modular][normalize]") {
    SECTION("positive values") {
        REQUIRE(normalize(5) == 5);
        REQUIRE(normalize(MOD) == 0);
        REQUIRE(normalize(MOD + 5) == 5);
    }

    SECTION("negative values") {
        REQUIRE(normalize(-1) == MOD - 1);
        REQUIRE(normalize(-MOD) == 0);
    }
}

// =============================================================================
// Sum Formula Tests
// =============================================================================

TEST_CASE("Triangle number sum (1 + 2 + ... + n)", "[sums][sum_1_to_n]") {
    SECTION("small values") {
        REQUIRE(sum_1_to_n(1) == 1);
        REQUIRE(sum_1_to_n(2) == 3);
        REQUIRE(sum_1_to_n(3) == 6);
        REQUIRE(sum_1_to_n(10) == 55);
        REQUIRE(sum_1_to_n(100) == 5050);
    }

    SECTION("edge cases") {
        REQUIRE(sum_1_to_n(0) == 0);
        REQUIRE(sum_1_to_n(-5) == 0);
    }
}

TEST_CASE("Sum of squares (1^2 + 2^2 + ... + n^2)", "[sums][sum_squares]") {
    SECTION("small values") {
        REQUIRE(sum_squares_1_to_n(1) == 1);
        REQUIRE(sum_squares_1_to_n(2) == 5);      // 1 + 4
        REQUIRE(sum_squares_1_to_n(3) == 14);     // 1 + 4 + 9
        REQUIRE(sum_squares_1_to_n(10) == 385);   // well-known value
    }

    SECTION("edge cases") {
        REQUIRE(sum_squares_1_to_n(0) == 0);
        REQUIRE(sum_squares_1_to_n(-1) == 0);
    }
}

TEST_CASE("Range sums", "[sums][range]") {
    SECTION("sum_range") {
        REQUIRE(sum_range(1, 10) == 55);       // same as sum_1_to_n(10)
        REQUIRE(sum_range(5, 10) == 45);       // 5+6+7+8+9+10
        REQUIRE(sum_range(10, 10) == 10);      // single element
    }

    SECTION("sum_squares_range") {
        REQUIRE(sum_squares_range(1, 3) == 14);   // 1+4+9
        REQUIRE(sum_squares_range(2, 4) == 29);   // 4+9+16
    }
}

// =============================================================================
// Checksum Algorithm Tests
// =============================================================================

TEST_CASE("Checksum edge cases", "[checksum][edge]") {
    SECTION("n = 0") {
        REQUIRE(compute(0) == 0);
    }

    SECTION("n = 1: only pair (1,1) gives 0") {
        // (1 % 1) + (1 % 1) = 0 + 0 = 0
        REQUIRE(compute(1) == 0);
    }

    SECTION("n = 2: four pairs") {
        // (1,1): 0+0=0, (1,2): 1+0=1, (2,1): 0+1=1, (2,2): 0+0=0
        // Total = 2
        REQUIRE(compute(2) == 2);
    }

    SECTION("n = 3: verify by hand") {
        // All pairs sum to 8
        REQUIRE(compute(3) == naive_checksum(3));
    }

    SECTION("negative input") {
        REQUIRE(compute(-1) == 0);
        REQUIRE(compute(-100) == 0);
    }
}

TEST_CASE("Checksum matches naive implementation", "[checksum][correctness]") {
    SECTION("small values n = 1 to 50") {
        for (int n = 1; n <= 50; ++n) {
            INFO("n = " << n);
            REQUIRE(compute(n) == naive_checksum(n));
        }
    }

    SECTION("medium values n = 51 to 200") {
        for (int n = 51; n <= 200; ++n) {
            INFO("n = " << n);
            REQUIRE(compute(n) == naive_checksum(n));
        }
    }
}

TEST_CASE("Checksum known values", "[checksum][regression]") {
    // Pre-computed values for regression testing
    REQUIRE(compute(10) == 430);
    REQUIRE(compute(100) == 450152);
    REQUIRE(compute(1000) == 451542898);
}
