#include "lru.h"
#include <catch_amalgamated.hpp>
#include <memory>
#include <string>
#include <vector>

using namespace std;

TEST_CASE("LRUCache basic operations", "[lru]") {
    LRUCache<string, string> cache(3);

    SECTION("set and get") {
        REQUIRE(cache.set("key1", "value1").has_value());
        auto result = cache.get("key1");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == "value1");
    }

    SECTION("has returns true for existing key") {
        REQUIRE(cache.set("key1", "value1").has_value());
        REQUIRE(cache.has("key1"));
    }

    SECTION("has returns false for missing key") {
        REQUIRE_FALSE(cache.has("missing"));
    }

    SECTION("get returns error for missing key") {
        auto result = cache.get("missing");
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == CacheError::KeyNotFound);
    }
}

TEST_CASE("LRUCache eviction policy", "[lru]") {
    LRUCache<string, string> cache(3);

    REQUIRE(cache.set("key1", "value1").has_value());
    REQUIRE(cache.set("key2", "value2").has_value());
    REQUIRE(cache.set("key3", "value3").has_value());

    SECTION("evicts LRU when capacity exceeded") {
        REQUIRE(cache.set("key4", "value4").has_value());

        REQUIRE_FALSE(cache.has("key1"));  // Evicted (LRU)
        REQUIRE(cache.has("key2"));
        REQUIRE(cache.has("key3"));
        REQUIRE(cache.has("key4"));
    }

    SECTION("get updates recency") {
        [[maybe_unused]] auto _ = cache.get("key1");  // Access key1, making key2 the LRU
        REQUIRE(cache.set("key4", "value4").has_value());

        REQUIRE(cache.has("key1"));  // Not evicted (recently accessed)
        REQUIRE_FALSE(cache.has("key2"));  // Evicted (was LRU)
        REQUIRE(cache.has("key3"));
        REQUIRE(cache.has("key4"));
    }

    SECTION("set updates recency for existing key") {
        REQUIRE(cache.set("key1", "updated").has_value());  // Update key1
        REQUIRE(cache.set("key4", "value4").has_value());

        REQUIRE(cache.has("key1"));  // Not evicted
        REQUIRE_FALSE(cache.has("key2"));  // Evicted (was LRU)
    }
}

TEST_CASE("LRUCache string literal lookups", "[lru]") {
    LRUCache<string, string> cache(3);
    REQUIRE(cache.set("key1", "value1").has_value());

    SECTION("has works with string literal") {
        REQUIRE(cache.has("key1"));
    }

    SECTION("get works with string literal") {
        auto result = cache.get("key1");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == "value1");
    }
}

TEST_CASE("LRUCache get_ref returns reference", "[lru]") {
    LRUCache<string, string> cache(3);
    REQUIRE(cache.set("key1", "value1").has_value());

    auto result = cache.get_ref("key1");
    REQUIRE(result.has_value());
    REQUIRE(result.value().get() == "value1");
}

TEST_CASE("LRUCache get_optional", "[lru]") {
    LRUCache<string, string> cache(3);
    REQUIRE(cache.set("key1", "value1").has_value());

    SECTION("returns value for existing key") {
        auto result = cache.get_optional("key1");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == "value1");
    }

    SECTION("returns nullopt for missing key") {
        auto result = cache.get_optional("missing");
        REQUIRE_FALSE(result.has_value());
    }
}

TEST_CASE("LRUCache with move-only types", "[lru]") {
    LRUCache<string, unique_ptr<int>> cache(2);

    SECTION("set and get_ref with unique_ptr") {
        REQUIRE(cache.set("ptr1", make_unique<int>(42)).has_value());

        auto result = cache.get_ref("ptr1");
        REQUIRE(result.has_value());
        REQUIRE(*result.value().get() == 42);
    }

    SECTION("eviction works with move-only types") {
        REQUIRE(cache.set("ptr1", make_unique<int>(1)).has_value());
        REQUIRE(cache.set("ptr2", make_unique<int>(2)).has_value());
        REQUIRE(cache.set("ptr3", make_unique<int>(3)).has_value());  // Evicts ptr1

        REQUIRE_FALSE(cache.has("ptr1"));
        REQUIRE(cache.has("ptr2"));
        REQUIRE(cache.has("ptr3"));
    }
}

TEST_CASE("LRUCache update existing key", "[lru]") {
    LRUCache<string, string> cache(3);

    REQUIRE(cache.set("key1", "value1").has_value());
    REQUIRE(cache.set("key1", "updated").has_value());

    auto result = cache.get("key1");
    REQUIRE(result.has_value());
    REQUIRE(result.value() == "updated");
}

TEST_CASE("LRUCache monadic operations", "[lru]") {
    LRUCache<string, string> cache(3);
    REQUIRE(cache.set("key1", "hello").has_value());

    SECTION("transform on success") {
        auto result = cache.get("key1")
            .transform([](const string& s) { return s.size(); });

        REQUIRE(result.has_value());
        REQUIRE(result.value() == 5);
    }

    SECTION("value_or on failure") {
        auto result = cache.get("missing")
            .transform([](const string& s) { return s; })
            .value_or("default");

        REQUIRE(result == "default");
    }
}

TEST_CASE("LRUCache capacity edge cases", "[lru]") {
    SECTION("capacity 1 cache") {
        LRUCache<string, string> cache(1);

        REQUIRE(cache.set("key1", "value1").has_value());
        REQUIRE(cache.has("key1"));

        REQUIRE(cache.set("key2", "value2").has_value());
        REQUIRE_FALSE(cache.has("key1"));
        REQUIRE(cache.has("key2"));
    }

    SECTION("capacity 0 cache returns error") {
        LRUCache<string, string> cache(0);

        auto result = cache.set("key", "value");
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == CacheError::CapacityZero);

        // Verify nothing was stored
        REQUIRE_FALSE(cache.has("key"));
    }
}

TEST_CASE("LRUCache set returns expected", "[lru]") {
    LRUCache<string, string> cache(3);

    SECTION("returns success for valid set") {
        auto result = cache.set("key", "value");
        REQUIRE(result.has_value());
    }

    SECTION("returns success for update") {
        REQUIRE(cache.set("key", "value1").has_value());
        auto result = cache.set("key", "value2");
        REQUIRE(result.has_value());
    }
}

TEST_CASE("LRUCache clear and size tracking", "[lru]") {
    LRUCache<string, string> cache(3);

    SECTION("size tracks insertions") {
        REQUIRE(cache.size() == 0);
        REQUIRE(cache.set("key1", "value1").has_value());
        REQUIRE(cache.size() == 1);
        REQUIRE(cache.set("key2", "value2").has_value());
        REQUIRE(cache.size() == 2);
    }

    SECTION("size tracks evictions") {
        REQUIRE(cache.set("key1", "v1").has_value());
        REQUIRE(cache.set("key2", "v2").has_value());
        REQUIRE(cache.set("key3", "v3").has_value());
        REQUIRE(cache.set("key4", "v4").has_value());  // Evicts key1
        REQUIRE(cache.size() == 3);
    }

    SECTION("clear resets size to zero") {
        REQUIRE(cache.set("key1", "value1").has_value());
        REQUIRE(cache.set("key2", "value2").has_value());
        cache.clear();
        REQUIRE(cache.size() == 0);
        REQUIRE_FALSE(cache.has("key1"));
    }

    SECTION("capacity returns correct value") {
        REQUIRE(cache.capacity() == 3);
    }
}

TEST_CASE("LRUCache move semantics", "[lru]") {
    SECTION("move constructor") {
        LRUCache<string, string> cache1(3);
        REQUIRE(cache1.set("key1", "value1").has_value());
        REQUIRE(cache1.set("key2", "value2").has_value());

        LRUCache<string, string> cache2(std::move(cache1));

        // cache2 has the data
        REQUIRE(cache2.size() == 2);
        REQUIRE(cache2.has("key1"));
        REQUIRE(cache2.has("key2"));

        // cache1 is empty but valid
        REQUIRE(cache1.size() == 0);
        REQUIRE_FALSE(cache1.has("key1"));
    }

    SECTION("move assignment") {
        LRUCache<string, string> cache1(3);
        REQUIRE(cache1.set("key1", "value1").has_value());

        LRUCache<string, string> cache2(2);
        REQUIRE(cache2.set("other", "data").has_value());

        cache2 = std::move(cache1);

        // cache2 has cache1's data
        REQUIRE(cache2.size() == 1);
        REQUIRE(cache2.has("key1"));
        REQUIRE_FALSE(cache2.has("other"));

        // cache1 is empty but valid
        REQUIRE(cache1.size() == 0);
    }
}

TEST_CASE("LRUCache iterator support", "[lru]") {
    LRUCache<string, string> cache(3);

    SECTION("empty cache iteration") {
        int count = 0;
        for ([[maybe_unused]] auto [k, v] : cache) {
            ++count;
        }
        REQUIRE(count == 0);
    }

    SECTION("iterate in MRU order") {
        REQUIRE(cache.set("first", "1").has_value());
        REQUIRE(cache.set("second", "2").has_value());
        REQUIRE(cache.set("third", "3").has_value());

        vector<string> keys;
        for (auto [k, v] : cache) {
            keys.push_back(string(k));
        }

        // MRU order: third (most recent), second, first (least recent)
        REQUIRE(keys.size() == 3);
        REQUIRE(keys[0] == "third");
        REQUIRE(keys[1] == "second");
        REQUIRE(keys[2] == "first");
    }

    SECTION("const iteration") {
        REQUIRE(cache.set("key", "value").has_value());

        const auto& const_cache = cache;
        int count = 0;
        for ([[maybe_unused]] auto [k, v] : const_cache) {
            ++count;
        }
        REQUIRE(count == 1);
    }

    SECTION("cbegin/cend") {
        REQUIRE(cache.set("key", "value").has_value());

        auto it = cache.cbegin();
        REQUIRE(it != cache.cend());
        auto [k, v] = *it;
        REQUIRE(k == "key");
        REQUIRE(v == "value");
    }
}

TEST_CASE("LRUCache with non-string keys", "[lru]") {
    SECTION("int keys") {
        LRUCache<int, string> cache(3);

        REQUIRE(cache.set(1, "one").has_value());
        REQUIRE(cache.set(2, "two").has_value());
        REQUIRE(cache.set(3, "three").has_value());

        REQUIRE(cache.has(1));
        REQUIRE(cache.has(2));
        REQUIRE(cache.has(3));

        auto result = cache.get(2);
        REQUIRE(result.has_value());
        REQUIRE(result.value() == "two");

        // Eviction
        REQUIRE(cache.set(4, "four").has_value());
        REQUIRE_FALSE(cache.has(1));  // LRU evicted
        REQUIRE(cache.has(4));
    }

    SECTION("int key iteration") {
        LRUCache<int, int> cache(3);
        REQUIRE(cache.set(10, 100).has_value());
        REQUIRE(cache.set(20, 200).has_value());

        vector<int> keys;
        for (auto [k, v] : cache) {
            keys.push_back(k);
        }

        REQUIRE(keys.size() == 2);
        REQUIRE(keys[0] == 20);  // MRU
        REQUIRE(keys[1] == 10);  // LRU
    }
}
