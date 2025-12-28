#include "hashtable_optimized.h"
#include <catch_amalgamated.hpp>
#include <string>
#include <chrono>
#include <iostream>

using namespace customhashtable;

TEST_CASE("OptimizedHashTable basic put and get", "[hashtable_optimized]") {
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

TEST_CASE("OptimizedHashTable move operations", "[hashtable_optimized]") {
    HashTable<std::string, int> ht(8);

    std::string key = "elderberry";
    int value = 12;
    ht.put(std::move(key), std::move(value));

    auto result = ht.get_checked("elderberry");
    REQUIRE(result.has_value());
    REQUIRE(result.value() == 12);
}

TEST_CASE("OptimizedHashTable contains operation", "[hashtable_optimized]") {
    HashTable<std::string, int> ht(8);

    ht.put("apple", 5);

    REQUIRE(ht.contains("apple"));
    REQUIRE_FALSE(ht.contains("grape"));
}

TEST_CASE("OptimizedHashTable remove operation", "[hashtable_optimized]") {
    HashTable<std::string, int> ht(8);

    ht.put("apple", 5);
    ht.put("banana", 3);

    REQUIRE(ht.contains("banana"));
    REQUIRE(ht.remove("banana") == true);
    REQUIRE_FALSE(ht.contains("banana"));
    REQUIRE(ht.contains("apple"));
}

TEST_CASE("OptimizedHashTable remove returns correct status", "[hashtable_optimized]") {
    HashTable<std::string, int> ht(8);

    ht.put("apple", 5);

    REQUIRE(ht.remove("apple") == true);   // Key existed
    REQUIRE(ht.remove("apple") == false);  // Key no longer exists
    REQUIRE(ht.remove("missing") == false); // Never existed
}

TEST_CASE("OptimizedHashTable update existing key", "[hashtable_optimized]") {
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

TEST_CASE("OptimizedHashTable configuration", "[hashtable_optimized]") {
    HashTable<std::string, int> ht(8);

    ht.put("apple", 5);
    ht.put("banana", 3);

    auto config = ht.get_configuration();
    REQUIRE(config.current_size == 2);
    // Bucket count may be rounded to power of 2
    REQUIRE(config.bucket_count >= 8);
}

TEST_CASE("OptimizedHashTable load factor", "[hashtable_optimized]") {
    HashTable<std::string, int> ht(8);

    REQUIRE(ht.get_load_factor() == 0.0);

    ht.put("apple", 5);
    ht.put("banana", 3);

    REQUIRE(ht.get_load_factor() > 0.0);
}

TEST_CASE("OptimizedHashTable load factor after move", "[hashtable_optimized]") {
    HashTable<std::string, int> ht(8);
    ht.put("apple", 5);

    HashTable<std::string, int> moved = std::move(ht);

    // Moved-from object should be safe to query
    REQUIRE(ht.get_load_factor() == 0.0);  // Should not crash, returns 0

    // Moved-to object should work normally
    REQUIRE(moved.get_load_factor() > 0.0);
    REQUIRE(moved.contains("apple"));
}

TEST_CASE("OptimizedHashTable collision stats", "[hashtable_optimized]") {
    HashTable<std::string, int> ht(8);

    for (int i = 0; i < 5; ++i) {
        ht.put("key" + std::to_string(i), i);
    }

    auto stats = ht.get_collision_stats();
    REQUIRE(stats.max_chain_length >= 0);
    REQUIRE(stats.average_chain_length >= 0.0);
}

TEST_CASE("OptimizedHashTable resize operation", "[hashtable_optimized]") {
    HashTable<std::string, int> ht(8);

    ht.put("apple", 5);
    ht.put("banana", 3);

    ht.execute_resize(32);

    REQUIRE(ht.get_configuration().bucket_count >= 32);
    auto apple = ht.get_checked("apple");
    auto banana = ht.get_checked("banana");
    REQUIRE(apple.has_value());
    REQUIRE(apple.value() == 5);
    REQUIRE(banana.has_value());
    REQUIRE(banana.value() == 3);
}

TEST_CASE("OptimizedHashTable change hash function", "[hashtable_optimized]") {
    HashTable<std::string, int> ht(8);

    ht.put("apple", 5);
    ht.put("banana", 3);

    ht.execute_change_hash_function(2);

    auto config = ht.get_configuration();
    REQUIRE(config.active_hash_function_id == 2);
    auto apple = ht.get_checked("apple");
    auto banana = ht.get_checked("banana");
    REQUIRE(apple.has_value());
    REQUIRE(apple.value() == 5);
    REQUIRE(banana.has_value());
    REQUIRE(banana.value() == 3);
}

TEST_CASE("OptimizedHashTable auto-resize on high load", "[hashtable_optimized]") {
    HashTable<std::string, int> ht(4);

    for (int i = 0; i < 10; ++i) {
        ht.put("key" + std::to_string(i), i);
    }

    REQUIRE(ht.get_configuration().bucket_count > 4);

    for (int i = 0; i < 10; ++i) {
        REQUIRE(ht.contains("key" + std::to_string(i)));
        auto result = ht.get_checked("key" + std::to_string(i));
        REQUIRE(result.has_value());
        REQUIRE(result.value() == i);
    }
}

TEST_CASE("OptimizedHashTable performance metrics", "[hashtable_optimized]") {
    HashTable<std::string, int> ht(8);

    for (int i = 0; i < 20; ++i) {
        ht.put("key" + std::to_string(i), i);
    }

    auto metrics = ht.get_performance_metrics();
    REQUIRE(metrics.average_latency_ms >= 0.0);
}

TEST_CASE("OptimizedHashTable bulk operations benchmark", "[hashtable_optimized][benchmark]") {
    HashTable<std::string, int> ht(1024);

    const int num_elements = 1000;
    std::vector<std::string> keys;
    keys.reserve(num_elements);

    for (int i = 0; i < num_elements; ++i) {
        keys.push_back("key_" + std::to_string(i));
    }

    // Insertion
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_elements; ++i) {
        ht.put(keys[i], i);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto insert_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Lookup
    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_elements; ++i) {
        [[maybe_unused]] auto result = ht.get_checked(keys[i]);
    }
    end = std::chrono::high_resolution_clock::now();
    auto lookup_duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // Just verify operations completed
    REQUIRE(ht.get_configuration().current_size == num_elements);

    // Output timing info
    std::cout << "\nBulk operation benchmark (" << num_elements << " elements):\n";
    std::cout << "  Insert: " << insert_duration.count() << " us\n";
    std::cout << "  Lookup: " << lookup_duration.count() << " us\n";
}

TEST_CASE("OptimizedHashTable get_checked returns expected", "[hashtable_optimized]") {
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
