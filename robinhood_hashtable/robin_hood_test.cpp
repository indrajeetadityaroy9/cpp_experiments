#include "robin_hood.h"

#include <catch_amalgamated.hpp>

#include <memory>
#include <random>
#include <unordered_map>
#include <vector>

using robin_hood::RobinHoodTable;

namespace {

uint64_t mix64(uint64_t z) {
    z += 0x9e3779b97f4a7c15ULL;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

// Random mixed put/get/erase stream, checked op-for-op against
// std::unordered_map, then a full sweep and drain.
template<size_t Cap>
void differential_churn(uint64_t seed, size_t num_ops, size_t key_space,
                        int put_pct, int erase_pct) {
    auto table = std::make_unique<RobinHoodTable<uint64_t, uint64_t, Cap>>();
    std::unordered_map<uint64_t, uint64_t> ref;

    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<uint64_t> key_dist(0, key_space - 1);
    std::uniform_int_distribution<int> op_dist(0, 99);

    for (size_t i = 0; i < num_ops; ++i) {
        const uint64_t k = mix64(key_dist(rng));
        const int op = op_dist(rng);

        if (op < put_pct) {
            const uint64_t v = rng();
            if (table->put(k, v)) {
                ref[k] = v;
            } else {
                // Rejection is only legal for a NEW key when the table is
                // full or at the displacement cap; updates always succeed.
                REQUIRE(ref.find(k) == ref.end());
                REQUIRE(ref.size() >= Cap * 9 / 10);
            }
        } else if (op < put_pct + erase_pct) {
            REQUIRE(table->erase(k) == (ref.erase(k) > 0));
        } else {
            const uint64_t* got = table->get(k);
            const auto it = ref.find(k);
            if (it == ref.end()) {
                REQUIRE(got == nullptr);
            } else {
                REQUIRE(got != nullptr);
                REQUIRE(*got == it->second);
            }
        }
        REQUIRE(table->size() == ref.size());
    }

    for (const auto& [k, v] : ref) {
        const uint64_t* got = table->get(k);
        REQUIRE(got != nullptr);
        REQUIRE(*got == v);
    }
    for (const auto& [k, v] : ref) {
        REQUIRE(table->erase(k));
    }
    REQUIRE(table->size() == 0);
}

} // namespace

TEST_CASE("differential churn across capacities", "[robinhood]") {
    for (uint64_t seed = 1; seed <= 3; ++seed) {
        differential_churn<16>(seed, 100000, 12, 45, 35);      // min capacity, near-full
        differential_churn<64>(seed, 100000, 80, 45, 35);      // mirror longer than table
        differential_churn<256>(seed, 100000, 300, 45, 35);
        differential_churn<8192>(seed, 300000, 7000, 40, 20);  // ~0.85 peak load
    }
}

TEST_CASE("sustained high-load insert/erase churn", "[robinhood]") {
    // Holds the table at ~0.9 load while continuously replacing entries,
    // exercising backshift deletion and mirror maintenance for 10^6 ops.
    constexpr size_t CAP = 1024;
    auto table = std::make_unique<RobinHoodTable<uint64_t, uint64_t, CAP>>();
    std::unordered_map<uint64_t, uint64_t> ref;
    std::mt19937_64 rng(99);

    std::vector<uint64_t> live;
    while (live.size() < CAP * 9 / 10) {
        const uint64_t k = rng();
        if (table->put(k, k)) {
            ref[k] = k;
            live.push_back(k);
        }
    }
    for (size_t i = 0; i < 1'000'000; ++i) {
        const size_t victim = rng() % live.size();
        REQUIRE(table->erase(live[victim]));
        ref.erase(live[victim]);
        uint64_t k = rng();
        while (!table->put(k, k)) k = rng();
        ref[k] = k;
        live[victim] = k;
    }
    REQUIRE(table->size() == ref.size());
    for (const auto& [k, v] : ref) {
        const uint64_t* got = table->get(k);
        REQUIRE(got != nullptr);
        REQUIRE(*got == v);
    }
}

TEST_CASE("full-table lifecycle at minimum capacity", "[robinhood]") {
    RobinHoodTable<uint64_t, uint64_t, 16> t;
    // Fill completely.
    std::vector<uint64_t> keys;
    for (uint64_t k = 1; keys.size() < 16; ++k) {
        if (t.put(k, k * 10)) keys.push_back(k);
    }
    REQUIRE(t.size() == 16);
    // Full-table miss terminates; full-table update succeeds.
    REQUIRE(t.get(0xdeadbeef) == nullptr);
    REQUIRE(t.put(keys[3], 42));
    REQUIRE(*t.get(keys[3]) == 42);
    REQUIRE(t.size() == 16);
    // New key on a full table is rejected without damage.
    REQUIRE_FALSE(t.put(0xdeadbeef, 1));
    for (const uint64_t k : keys) REQUIRE(t.get(k) != nullptr);
    // Erase one, insert one.
    REQUIRE(t.erase(keys[0]));
    REQUIRE(t.put(0xdeadbeef, 1));
    REQUIRE(t.size() == 16);
}

TEST_CASE("displacement-cap rejection is non-destructive", "[robinhood]") {
    constexpr size_t CAP = 8192;
    auto t = std::make_unique<RobinHoodTable<uint64_t, uint64_t, CAP>>();
    const size_t home = 77;

    // Keys that all hash to one home bucket; the cap admits exactly 239
    // (displacements 0..238) before rejecting.
    std::vector<uint64_t> same_home;
    for (uint64_t k = 0; same_home.size() < 245; ++k) {
        if ((__crc32cd(0, k) & (CAP - 1)) == home) same_home.push_back(k);
    }

    size_t accepted = 0;
    for (const uint64_t k : same_home) {
        if (t->put(k, k * 3)) {
            ++accepted;
        } else {
            REQUIRE(t->size() == accepted);
            for (size_t j = 0; j < accepted; ++j) {
                const uint64_t* got = t->get(same_home[j]);
                REQUIRE(got != nullptr);
                REQUIRE(*got == same_home[j] * 3);
            }
        }
    }
    REQUIRE(accepted == 239);
}

TEST_CASE("signed integral keys", "[robinhood]") {
    RobinHoodTable<int32_t, int32_t, 16> t;
    REQUIRE(t.put(-5, 50));
    REQUIRE(t.put(2147483647, 1));
    REQUIRE(t.put(-2147483647 - 1, 2));
    REQUIRE(*t.get(-5) == 50);
    REQUIRE(t.erase(-5));
    REQUIRE(t.get(-5) == nullptr);
    REQUIRE(t.size() == 2);
}

TEST_CASE("move-only values", "[robinhood]") {
    RobinHoodTable<uint64_t, std::unique_ptr<int>, 16> t;
    REQUIRE(t.put(1, std::make_unique<int>(7)));
    REQUIRE(t.put(1, std::make_unique<int>(8)));  // update moves in
    REQUIRE(**t.get(1) == 8);
    REQUIRE(t.erase(1));
    REQUIRE(t.size() == 0);
}
