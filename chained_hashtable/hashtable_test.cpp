#include "hashtable.h"
#include <catch_amalgamated.hpp>
#include <string>

using namespace customhashtable;

TEST_CASE("HashTable basic put and get", "[hashtable]") {
    HashTable<std::string, int> ht(8);

    ht.put("apple", 5);
    ht.put("banana", 3);
    ht.put("cherry", 8);

    auto apple = ht.get_checked("apple");
    auto banana = ht.get_checked("banana");
    auto cherry = ht.get_checked("cherry");

    REQUIRE(apple.has_value());
    REQUIRE(apple.value() == 5);
    REQUIRE(banana.has_value());
    REQUIRE(banana.value() == 3);
    REQUIRE(cherry.has_value());
    REQUIRE(cherry.value() == 8);
}

TEST_CASE("HashTable contains operation", "[hashtable]") {
    HashTable<std::string, int> ht(8);

    ht.put("apple", 5);

    REQUIRE(ht.contains("apple"));
    REQUIRE_FALSE(ht.contains("grape"));
}

TEST_CASE("HashTable remove operation", "[hashtable]") {
    HashTable<std::string, int> ht(8);

    ht.put("apple", 5);
    ht.put("banana", 3);

    REQUIRE(ht.contains("banana"));
    REQUIRE(ht.remove("banana") == true);
    REQUIRE_FALSE(ht.contains("banana"));
    REQUIRE(ht.contains("apple"));  // Other keys unaffected
}

TEST_CASE("HashTable remove returns correct status", "[hashtable]") {
    HashTable<std::string, int> ht(8);

    ht.put("apple", 5);

    REQUIRE(ht.remove("apple") == true);   // Key existed
    REQUIRE(ht.remove("apple") == false);  // Key no longer exists
    REQUIRE(ht.remove("missing") == false); // Never existed
}

TEST_CASE("HashTable update existing key", "[hashtable]") {
    HashTable<std::string, int> ht(8);

    ht.put("apple", 5);
    auto result1 = ht.get_checked("apple");
    REQUIRE(result1.has_value());
    REQUIRE(result1.value() == 5);

    ht.put("apple", 10);
    auto result2 = ht.get_checked("apple");
    REQUIRE(result2.has_value());
    REQUIRE(result2.value() == 10);
}

TEST_CASE("HashTable configuration", "[hashtable]") {
    HashTable<std::string, int> ht(8);

    ht.put("apple", 5);
    ht.put("banana", 3);

    auto config = ht.get_configuration();
    REQUIRE(config.current_size == 2);
    REQUIRE(config.bucket_count == 8);
}

TEST_CASE("HashTable load factor", "[hashtable]") {
    HashTable<std::string, int> ht(8);

    REQUIRE(ht.get_load_factor() == 0.0);

    ht.put("apple", 5);
    ht.put("banana", 3);
    ht.put("cherry", 8);
    ht.put("date", 2);

    REQUIRE(ht.get_load_factor() == Catch::Approx(0.5));
}

TEST_CASE("HashTable load factor after move", "[hashtable]") {
    HashTable<std::string, int> ht(8);
    ht.put("apple", 5);

    HashTable<std::string, int> moved = std::move(ht);

    // Moved-from object should be safe to query
    REQUIRE(ht.get_load_factor() == 0.0);  // Should not crash, returns 0

    // Moved-to object should work normally
    REQUIRE(moved.get_load_factor() > 0.0);
    REQUIRE(moved.contains("apple"));
}

TEST_CASE("HashTable collision stats", "[hashtable]") {
    HashTable<std::string, int> ht(8);

    for (int i = 0; i < 5; ++i) {
        ht.put("key" + std::to_string(i), i);
    }

    auto stats = ht.get_collision_stats();
    REQUIRE(stats.max_chain_length >= 0);
    REQUIRE(stats.average_chain_length >= 0.0);
}

TEST_CASE("HashTable resize operation", "[hashtable]") {
    HashTable<std::string, int> ht(8);

    ht.put("apple", 5);
    ht.put("banana", 3);

    REQUIRE(ht.get_configuration().bucket_count == 8);

    ht.execute_resize(32);

    REQUIRE(ht.get_configuration().bucket_count == 32);
    // Data should be preserved after resize
    auto apple = ht.get_checked("apple");
    auto banana = ht.get_checked("banana");
    REQUIRE(apple.has_value());
    REQUIRE(apple.value() == 5);
    REQUIRE(banana.has_value());
    REQUIRE(banana.value() == 3);
}

TEST_CASE("HashTable change hash function", "[hashtable]") {
    HashTable<std::string, int> ht(8);

    ht.put("apple", 5);
    ht.put("banana", 3);

    ht.execute_change_hash_function(2);

    auto config_after = ht.get_configuration();
    REQUIRE(config_after.active_hash_function_id == 2);

    // Data should be preserved after hash function change
    auto apple = ht.get_checked("apple");
    auto banana = ht.get_checked("banana");
    REQUIRE(apple.has_value());
    REQUIRE(apple.value() == 5);
    REQUIRE(banana.has_value());
    REQUIRE(banana.value() == 3);
}

TEST_CASE("HashTable auto-resize on high load", "[hashtable]") {
    HashTable<std::string, int> ht(4);

    // Add enough elements to trigger resize (load factor > 0.75)
    for (int i = 0; i < 10; ++i) {
        ht.put("key" + std::to_string(i), i);
    }

    // Should have resized
    REQUIRE(ht.get_configuration().bucket_count > 4);

    // All data should be preserved
    for (int i = 0; i < 10; ++i) {
        REQUIRE(ht.contains("key" + std::to_string(i)));
        auto result = ht.get_checked("key" + std::to_string(i));
        REQUIRE(result.has_value());
        REQUIRE(result.value() == i);
    }
}

TEST_CASE("HashTable performance metrics", "[hashtable]") {
    HashTable<std::string, int> ht(8);

    for (int i = 0; i < 20; ++i) {
        ht.put("key" + std::to_string(i), i);
    }

    auto metrics = ht.get_performance_metrics();
    // Just verify we can get metrics without crashing
    REQUIRE(metrics.average_latency_ms >= 0.0);
}

TEST_CASE("HashTable with integer keys", "[hashtable]") {
    HashTable<int, std::string> ht(8);

    ht.put(1, "one");
    ht.put(2, "two");
    ht.put(3, "three");

    auto one = ht.get_checked(1);
    auto two = ht.get_checked(2);
    auto three = ht.get_checked(3);

    REQUIRE(one.has_value());
    REQUIRE(one.value() == "one");
    REQUIRE(two.has_value());
    REQUIRE(two.value() == "two");
    REQUIRE(three.has_value());
    REQUIRE(three.value() == "three");
}

TEST_CASE("HashTable get_checked returns expected", "[hashtable]") {
    HashTable<std::string, int> ht(8);

    ht.put("apple", 5);
    ht.put("banana", 3);

    SECTION("returns value for existing key") {
        auto result = ht.get_checked("apple");
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 5);
    }

    SECTION("returns error for missing key") {
        auto result = ht.get_checked("grape");
        REQUIRE_FALSE(result.has_value());
        REQUIRE(result.error() == HashTableError::KeyNotFound);
    }

    SECTION("supports monadic operations") {
        auto result = ht.get_checked("apple")
            .transform([](int v) { return v * 2; });
        REQUIRE(result.has_value());
        REQUIRE(result.value() == 10);

        auto missing = ht.get_checked("missing")
            .transform([](int v) { return v * 2; })
            .value_or(-1);
        REQUIRE(missing == -1);
    }
}
