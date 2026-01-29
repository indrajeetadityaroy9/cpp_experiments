#include "lru.h"
#include <catch_amalgamated.hpp>
#include <memory>
#include <string>

using namespace std;

TEST_CASE("LRUCache benchmarks", "[benchmark]") {
    BENCHMARK("String cache: 10k set operations") {
        LRUCache<string, string> cache(100);
        for (int i = 0; i < 10000; ++i) {
            (void)cache.set("key" + to_string(i), "value" + to_string(i));
        }
        return cache.has("key0");
    };

    BENCHMARK("String cache: 10k get operations") {
        LRUCache<string, string> cache(1000);
        for (int i = 0; i < 1000; ++i) {
            (void)cache.set("key" + to_string(i), "value" + to_string(i));
        }
        int found = 0;
        for (int i = 0; i < 10000; ++i) {
            if (cache.get("key" + to_string(i % 1000)).has_value()) {
                ++found;
            }
        }
        return found;
    };

    BENCHMARK("Move-only types: 10k unique_ptr set") {
        LRUCache<string, unique_ptr<int>> cache(100);
        for (int i = 0; i < 10000; ++i) {
            (void)cache.set("key" + to_string(i), make_unique<int>(i));
        }
        return cache.has("key0");
    };

    BENCHMARK("string_view lookups: 100k has() checks") {
        LRUCache<string, int> cache(1000);
        for (int i = 0; i < 1000; ++i) {
            (void)cache.set("key" + to_string(i), i);
        }
        int found = 0;
        string key500 = "key500";
        for (int i = 0; i < 100000; ++i) {
            if (cache.has(key500)) {
                ++found;
            }
        }
        return found;
    };

    BENCHMARK("Int keys: 10k set operations") {
        LRUCache<int, int> cache(100);
        for (int i = 0; i < 10000; ++i) {
            (void)cache.set(i, i * 2);
        }
        return cache.has(0);
    };
}

TEST_CASE("LRUCache eviction performance", "[benchmark]") {
    BENCHMARK("Eviction under pressure: 50k ops on size-100 cache") {
        LRUCache<string, int> cache(100);
        int evictions = 0;
        for (int i = 0; i < 50000; ++i) {
            if (!cache.has("key" + to_string(i))) {
                ++evictions;
            }
            (void)cache.set("key" + to_string(i), i);
        }
        return evictions;
    };
}
