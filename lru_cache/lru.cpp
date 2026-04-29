#include "lru_cache.h"

#include <array>
#include <memory>
#include <string>
#include <string_view>

using namespace std;

#ifdef LRU_CACHE_DEMO

#include <iostream>

int main() {
    cout << "1. Basic string cache:\n";
    LRUCache<string, string> cache(3);
    for (const auto& [key, value] : array{
             pair{"key1", "value1"},
             pair{"key2", "value2"},
             pair{"key3", "value3"},
         }) {
        (void)cache.set(key, value);
    }

    if (const auto* result = cache.get("key1")) {
        cout << "key1: " << *result << '\n';
    } else {
        cout << "key1 missing\n";
    }

    cout << "\n2. String lookups:\n";
    const string key2 = "key2";
    cout << "key2 present: " << (cache.has(key2) ? "yes" : "no") << '\n';

    cout << "\n3. Testing eviction (capacity=3, adding key4):\n";
    (void)cache.set("key4", "value4");
    for (const auto key : {"key1", "key2", "key3", "key4"}) {
        cout << key << " present: " << (cache.has(key) ? "yes" : "no");
        if (string_view{key} == "key2") {
            cout << " (evicted LRU)";
        }
        cout << '\n';
    }

    cout << "\n4. Move-only types (unique_ptr):\n";
    LRUCache<string, unique_ptr<int>> ptr_cache(2);
    (void)ptr_cache.set("ptr1", make_unique<int>(42));
    (void)ptr_cache.set("ptr2", make_unique<int>(100));

    if (const auto* result = ptr_cache.get("ptr1")) {
        cout << "ptr1 value: " << **result << '\n';
    }

    cout << "\n5. Iterator support (MRU to LRU order):\n";
    for (auto [k, v] : cache) {
        cout << "  " << k << " -> " << v << '\n';
    }

    cout << "\n6. Non-string keys (int -> string):\n";
    LRUCache<int, string> int_cache(3);
    for (const auto& [key, value] : array{
             pair{1, "one"},
             pair{2, "two"},
             pair{3, "three"},
         }) {
        (void)int_cache.set(key, value);
    }
    for (auto [k, v] : int_cache) {
        cout << "  " << k << " -> " << v << '\n';
    }

    cout << "\n7. Move semantics:\n";
    LRUCache<string, string> moved_cache = std::move(cache);
    cout << "Original cache size after move: " << cache.size() << '\n';
    cout << "Moved cache size: " << moved_cache.size() << '\n';

    return 0;
}

#else

#include "catch_amalgamated.hpp"

namespace {

constexpr int kSetOps = 10000;
constexpr int kLookupOps = 100000;
constexpr int kEvictionOps = 50000;
constexpr int kSetCapacity = 100;
constexpr int kLookupCapacity = 1000;

vector<string> make_strings(string_view prefix, int count) {
    vector<string> values;
    values.reserve(count);
    for (int i = 0; i < count; ++i) {
        values.push_back(string{prefix} + to_string(i));
    }
    return values;
}

vector<int> make_indices(int count, int modulus) {
    vector<int> indices;
    indices.reserve(count);
    for (int i = 0; i < count; ++i) {
        indices.push_back(i % modulus);
    }
    return indices;
}

template <typename K, typename V>
vector<LRUCache<K, V>> make_caches(int runs, size_t capacity) {
    vector<LRUCache<K, V>> caches;
    caches.reserve(runs);
    for (int i = 0; i < runs; ++i) {
        caches.emplace_back(capacity);
    }
    return caches;
}

template <typename V>
vector<vector<unique_ptr<V>>> make_unique_payloads(int runs, int count) {
    vector<vector<unique_ptr<V>>> payloads;
    payloads.reserve(runs);
    for (int run = 0; run < runs; ++run) {
        auto& values = payloads.emplace_back();
        values.reserve(count);
        for (int i = 0; i < count; ++i) {
            values.push_back(make_unique<V>(i));
        }
    }
    return payloads;
}

struct NoDefault {
    int value;

    explicit NoDefault(int initial_value) : value(initial_value) {}
    NoDefault(const NoDefault&) = default;
    NoDefault& operator=(const NoDefault&) = default;
    NoDefault(NoDefault&&) noexcept = default;
    NoDefault& operator=(NoDefault&&) noexcept = default;
};

}  // namespace

TEST_CASE("LRUCache basic operations", "[lru]") {
    LRUCache<string, string> cache(3);

    SECTION("set and get") {
        REQUIRE(cache.set("key1", "value1"));

        auto result = cache.get("key1");
        REQUIRE(result != nullptr);
        REQUIRE(*result == "value1");
    }

    SECTION("has returns true for existing key") {
        REQUIRE(cache.set("key1", "value1"));
        REQUIRE(cache.has("key1"));
    }

    SECTION("has returns false for missing key") {
        REQUIRE_FALSE(cache.has("missing"));
    }

    SECTION("get returns null for missing key") {
        auto result = cache.get("missing");
        REQUIRE(result == nullptr);
    }
}

TEST_CASE("LRUCache eviction policy", "[lru]") {
    LRUCache<string, string> cache(3);

    REQUIRE(cache.set("key1", "value1"));
    REQUIRE(cache.set("key2", "value2"));
    REQUIRE(cache.set("key3", "value3"));

    SECTION("evicts LRU when capacity exceeded") {
        REQUIRE(cache.set("key4", "value4"));

        REQUIRE_FALSE(cache.has("key1"));
        REQUIRE(cache.has("key2"));
        REQUIRE(cache.has("key3"));
        REQUIRE(cache.has("key4"));
    }

    SECTION("get updates recency") {
        [[maybe_unused]] auto _ = cache.get("key1");
        REQUIRE(cache.set("key4", "value4"));

        REQUIRE(cache.has("key1"));
        REQUIRE_FALSE(cache.has("key2"));
        REQUIRE(cache.has("key3"));
        REQUIRE(cache.has("key4"));
    }

    SECTION("set updates recency for existing key") {
        REQUIRE(cache.set("key1", "updated"));
        REQUIRE(cache.set("key4", "value4"));

        REQUIRE(cache.has("key1"));
        REQUIRE_FALSE(cache.has("key2"));
    }
}

TEST_CASE("LRUCache string literal lookups", "[lru]") {
    LRUCache<string, string> cache(3);
    REQUIRE(cache.set("key1", "value1"));

    SECTION("has works with string literal") {
        REQUIRE(cache.has("key1"));
    }

    SECTION("get works with string literal") {
        auto result = cache.get("key1");
        REQUIRE(result != nullptr);
        REQUIRE(*result == "value1");
    }

    SECTION("string_view lookups avoid temporary strings") {
        const string_view key = "key1";
        REQUIRE(cache.has(key));

        auto result = cache.get(key);
        REQUIRE(result != nullptr);
        REQUIRE(*result == "value1");
    }
}

TEST_CASE("LRUCache get returns pointer", "[lru]") {
    LRUCache<string, string> cache(3);
    REQUIRE(cache.set("key1", "value1"));

    const auto* result = cache.get("key1");
    REQUIRE(result != nullptr);
    REQUIRE(*result == "value1");
}

TEST_CASE("LRUCache get", "[lru]") {
    LRUCache<string, string> cache(3);
    REQUIRE(cache.set("key1", "value1"));

    SECTION("returns value for existing key") {
        auto result = cache.get("key1");
        REQUIRE(result != nullptr);
        REQUIRE(*result == "value1");
    }

    SECTION("returns null for missing key") {
        auto result = cache.get("missing");
        REQUIRE(result == nullptr);
    }
}

TEST_CASE("LRUCache with move-only types", "[lru]") {
    LRUCache<string, unique_ptr<int>> cache(2);

    SECTION("set and get with unique_ptr") {
        REQUIRE(cache.set("ptr1", make_unique<int>(42)));

        auto result = cache.get("ptr1");
        REQUIRE(result != nullptr);
        REQUIRE(**result == 42);
    }

    SECTION("eviction works with move-only types") {
        REQUIRE(cache.set("ptr1", make_unique<int>(1)));
        REQUIRE(cache.set("ptr2", make_unique<int>(2)));
        REQUIRE(cache.set("ptr3", make_unique<int>(3)));

        REQUIRE_FALSE(cache.has("ptr1"));
        REQUIRE(cache.has("ptr2"));
        REQUIRE(cache.has("ptr3"));
    }
}

TEST_CASE("LRUCache move semantics", "[lru]") {
    LRUCache<string, string> source(2);
    REQUIRE(source.set("key1", "value1"));
    REQUIRE(source.set("key2", "value2"));

    auto moved = std::move(source);
    REQUIRE(source.size() == 0);
    REQUIRE_FALSE(source.has("key1"));
    REQUIRE(moved.size() == 2);
    REQUIRE(moved.has("key1"));
    REQUIRE(moved.has("key2"));

    LRUCache<string, string> assigned(1);
    REQUIRE(assigned.set("other", "value"));
    assigned = std::move(moved);

    REQUIRE(moved.size() == 0);
    REQUIRE_FALSE(moved.has("key1"));
    REQUIRE(assigned.size() == 2);
    REQUIRE(assigned.has("key1"));
    REQUIRE(assigned.has("key2"));
}

TEST_CASE("LRUCache supports non-default-constructible values", "[lru]") {
    LRUCache<string, NoDefault> cache(2);

    REQUIRE(cache.set("key1", NoDefault{7}));
    auto* result = cache.get("key1");
    REQUIRE(result != nullptr);
    REQUIRE(result->value == 7);
}

TEST_CASE("LRUCache update existing key", "[lru]") {
    LRUCache<string, string> cache(3);

    REQUIRE(cache.set("key1", "value1"));
    REQUIRE(cache.set("key1", "updated"));

    const auto* result = cache.get("key1");
    REQUIRE(result != nullptr);
    REQUIRE(*result == "updated");
}

TEST_CASE("LRUCache capacity edge cases", "[lru]") {
    SECTION("capacity 1 cache") {
        LRUCache<string, string> cache(1);

        REQUIRE(cache.set("key1", "value1"));
        REQUIRE(cache.has("key1"));

        REQUIRE(cache.set("key2", "value2"));
        REQUIRE_FALSE(cache.has("key1"));
        REQUIRE(cache.has("key2"));
    }

    SECTION("capacity 0 cache returns false") {
        LRUCache<string, string> cache(0);

        const auto result = cache.set("key", "value");
        REQUIRE_FALSE(result);
        REQUIRE_FALSE(cache.has("key"));
    }
}

TEST_CASE("LRUCache iterators", "[lru]") {
    LRUCache<string, string> cache(3);

    REQUIRE(cache.set("key1", "value1"));
    REQUIRE(cache.set("key2", "value2"));
    REQUIRE(cache.set("key3", "value3"));

    SECTION("iteration order is MRU to LRU") {
        auto it = cache.begin();
        REQUIRE(it != cache.end());
        REQUIRE((*it).first == "key3");

        ++it;
        REQUIRE(it != cache.end());
        REQUIRE((*it).first == "key2");

        ++it;
        REQUIRE(it != cache.end());
        REQUIRE((*it).first == "key1");

        ++it;
        REQUIRE(it == cache.end());
    }
}

TEST_CASE("LRUCache clear", "[lru]") {
    LRUCache<string, string> cache(3);

    REQUIRE(cache.set("key1", "value1"));
    REQUIRE(cache.set("key2", "value2"));

    cache.clear();

    REQUIRE(cache.size() == 0);
    REQUIRE_FALSE(cache.has("key1"));
    REQUIRE_FALSE(cache.has("key2"));
    REQUIRE(cache.set("key3", "value3"));
    REQUIRE(cache.has("key3"));
    REQUIRE(cache.size() == 1);
}

TEST_CASE("LRUCache benchmarks", "[benchmark]") {
    const auto keys = make_strings("key", kSetOps);
    const auto values = make_strings("value", kSetOps);
    const auto lookup_keys = make_strings("key", kLookupCapacity);
    const auto lookup_values = make_strings("value", kLookupCapacity);
    const auto lookup_indices = make_indices(kSetOps, kLookupCapacity);
    vector<string_view> lookup_views;
    lookup_views.reserve(lookup_keys.size());
    for (const auto& key : lookup_keys) {
        lookup_views.push_back(key);
    }

    BENCHMARK_ADVANCED("String cache: 10k set misses")(Catch::Benchmark::Chronometer meter) {
        auto caches = make_caches<string, string>(meter.runs(), kSetCapacity);

        meter.measure([&](int run) {
            auto& cache = caches[run];
            for (int i = 0; i < kSetOps; ++i) {
                (void)cache.set(keys[i], values[i]);
            }
            return cache.size();
        });
    };

    BENCHMARK_ADVANCED("String cache: 10k get hits")(Catch::Benchmark::Chronometer meter) {
        auto caches = make_caches<string, string>(meter.runs(), kLookupCapacity);
        for (auto& cache : caches) {
            for (int i = 0; i < kLookupCapacity; ++i) {
                (void)cache.set(lookup_keys[i], lookup_values[i]);
            }
        }

        meter.measure([&](int run) {
            auto& cache = caches[run];
            int found = 0;
            for (const auto index : lookup_indices) {
                if (cache.get(lookup_keys[index]) != nullptr) {
                    ++found;
                }
            }
            return found;
        });
    };

    BENCHMARK_ADVANCED("Move-only values: 10k set misses")(Catch::Benchmark::Chronometer meter) {
        auto caches = make_caches<string, unique_ptr<int>>(meter.runs(), kSetCapacity);
        auto payloads = make_unique_payloads<int>(meter.runs(), kSetOps);

        meter.measure([&](int run) {
            auto& cache = caches[run];
            for (int i = 0; i < kSetOps; ++i) {
                (void)cache.set(keys[i], std::move(payloads[run][i]));
            }
            return cache.size();
        });
    };

    BENCHMARK_ADVANCED("String has() hits: 100k checks")(Catch::Benchmark::Chronometer meter) {
        auto caches = make_caches<string, int>(meter.runs(), kLookupCapacity);
        for (auto& cache : caches) {
            for (int i = 0; i < kLookupCapacity; ++i) {
                (void)cache.set(lookup_keys[i], i);
            }
        }

        meter.measure([&](int run) {
            auto& cache = caches[run];
            int found = 0;
            for (int i = 0; i < kLookupOps; ++i) {
                if (cache.has(lookup_keys[i % kLookupCapacity])) {
                    ++found;
                }
            }
            return found;
        });
    };

    BENCHMARK_ADVANCED("string_view has() hits: 100k checks")(Catch::Benchmark::Chronometer meter) {
        auto caches = make_caches<string, int>(meter.runs(), kLookupCapacity);
        for (auto& cache : caches) {
            for (int i = 0; i < kLookupCapacity; ++i) {
                (void)cache.set(lookup_keys[i], i);
            }
        }

        meter.measure([&](int run) {
            auto& cache = caches[run];
            int found = 0;
            for (int i = 0; i < kLookupOps; ++i) {
                if (cache.has(lookup_views[i % kLookupCapacity])) {
                    ++found;
                }
            }
            return found;
        });
    };

    BENCHMARK_ADVANCED("Int keys: 10k set misses")(Catch::Benchmark::Chronometer meter) {
        auto caches = make_caches<int, int>(meter.runs(), kSetCapacity);

        meter.measure([&](int run) {
            auto& cache = caches[run];
            for (int i = 0; i < kSetOps; ++i) {
                (void)cache.set(i, i * 2);
            }
            return cache.size();
        });
    };
}

TEST_CASE("LRUCache eviction performance", "[benchmark]") {
    const auto keys = make_strings("key", kEvictionOps);

    BENCHMARK_ADVANCED("Eviction pressure: 50k has+set ops")(Catch::Benchmark::Chronometer meter) {
        auto caches = make_caches<string, int>(meter.runs(), kSetCapacity);

        meter.measure([&](int run) {
            auto& cache = caches[run];
            int misses = 0;
            for (int i = 0; i < kEvictionOps; ++i) {
                if (!cache.has(keys[i])) {
                    ++misses;
                }
                (void)cache.set(keys[i], i);
            }
            return misses;
        });
    };
}

#endif
