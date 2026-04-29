#include "catch_amalgamated.hpp"
#include "lru_cache.h"
#include <string>
#include <memory>

using namespace std;

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

        REQUIRE_FALSE(cache.has("key1"));  // Evicted (LRU)
        REQUIRE(cache.has("key2"));
        REQUIRE(cache.has("key3"));
        REQUIRE(cache.has("key4"));
    }

    SECTION("get updates recency") {
        [[maybe_unused]] auto _ = cache.get("key1");  // Access key1, making key2 the LRU
        REQUIRE(cache.set("key4", "value4"));

        REQUIRE(cache.has("key1"));  // Not evicted (recently accessed)
        REQUIRE_FALSE(cache.has("key2"));  // Evicted (was LRU)
        REQUIRE(cache.has("key3"));
        REQUIRE(cache.has("key4"));
    }

    SECTION("set updates recency for existing key") {
        REQUIRE(cache.set("key1", "updated"));  // Update key1
        REQUIRE(cache.set("key4", "value4"));

        REQUIRE(cache.has("key1"));  // Not evicted
        REQUIRE_FALSE(cache.has("key2"));  // Evicted (was LRU)
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
}

TEST_CASE("LRUCache get returns pointer", "[lru]") {
    LRUCache<string, string> cache(3);
    REQUIRE(cache.set("key1", "value1"));

    auto result = cache.get("key1");
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
        REQUIRE(cache.set("ptr3", make_unique<int>(3)));  // Evicts ptr1

        REQUIRE_FALSE(cache.has("ptr1"));
        REQUIRE(cache.has("ptr2"));
        REQUIRE(cache.has("ptr3"));
    }
}

TEST_CASE("LRUCache update existing key", "[lru]") {
    LRUCache<string, string> cache(3);

    REQUIRE(cache.set("key1", "value1"));
    REQUIRE(cache.set("key1", "updated"));

    auto result = cache.get("key1");
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

        auto result = cache.set("key", "value");
        REQUIRE_FALSE(result);

        // Verify nothing was stored
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
    
    // Can insert again after clear
    REQUIRE(cache.set("key3", "value3"));
    REQUIRE(cache.has("key3"));
    REQUIRE(cache.size() == 1);
}
