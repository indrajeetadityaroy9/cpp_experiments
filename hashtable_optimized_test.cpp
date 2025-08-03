#include "hashtable_optimized.h"
#include <iostream>
#include <string>
#include <chrono>
#include <random>

void performance_test() {
    std::cout << "Performance test with larger dataset:\n";

    customhashtable::HashTable<std::string, int> ht(1024);

    const int num_elements = 10000;
    std::vector<std::string> keys;
    keys.reserve(num_elements);

    for (int i = 0; i < num_elements; ++i) {
        keys.push_back("key_" + std::to_string(i));
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_elements; ++i) {
        ht.put(keys[i], i);
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Inserted " << num_elements << " elements in " << duration.count() << " microseconds\n";
    std::cout << "Average insertion time: " << (double)duration.count() / num_elements << " microseconds per element\n";

    start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_elements; ++i) {
        ht.get(keys[i]);
    }
    end = std::chrono::high_resolution_clock::now();

    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Looked up " << num_elements << " elements in " << duration.count() << " microseconds\n";
    std::cout << "Average lookup time: " << (double)duration.count() / num_elements << " microseconds per element\n";

    auto config = ht.get_configuration();
    std::cout << "Final configuration:\n";
    std::cout << "  Size: " << config.current_size << "\n";
    std::cout << "  Bucket count: " << config.bucket_count << "\n";
    std::cout << "  Load factor: " << ht.get_load_factor() << "\n";

    auto collision_stats = ht.get_collision_stats();
    std::cout << "Collision statistics:\n";
    std::cout << "  Max chain length: " << collision_stats.max_chain_length << "\n";
    std::cout << "  Average chain length: " << collision_stats.average_chain_length << "\n";

    auto perf_metrics = ht.get_performance_metrics();
    std::cout << "Performance metrics:\n";
    std::cout << "  Average latency (ms): " << perf_metrics.average_latency_ms << "\n";
    std::cout << "  Throughput (ops/sec): " << perf_metrics.throughput_ops_per_sec << "\n";
}

int main() {
    std::cout << "Testing optimized custom HashTable implementation\n";

    customhashtable::HashTable<std::string, int> ht(8);

    std::cout << "\n1. Testing put and get operations:\n";
    ht.put("apple", 5);
    ht.put("banana", 3);
    ht.put("cherry", 8);
    ht.put("date", 2);

    std::cout << "apple: " << ht.get("apple") << "\n";
    std::cout << "banana: " << ht.get("banana") << "\n";
    std::cout << "cherry: " << ht.get("cherry") << "\n";
    std::cout << "date: " << ht.get("date") << "\n";

    std::cout << "\n2. Testing move operations:\n";
    std::string key = "elderberry";
    int value = 12;
    ht.put(std::move(key), std::move(value));
    std::cout << "elderberry: " << ht.get("elderberry") << "\n";

    std::cout << "\n3. Testing contains operation:\n";
    std::cout << "Contains 'apple': " << (ht.contains("apple") ? "true" : "false") << "\n";
    std::cout << "Contains 'grape': " << (ht.contains("grape") ? "true" : "false") << "\n";

    std::cout << "\n4. Testing remove operation:\n";
    ht.remove("banana");
    std::cout << "Contains 'banana' after removal: " << (ht.contains("banana") ? "true" : "false") << "\n";

    std::cout << "\n5. Testing get_configuration:\n";
    auto config = ht.get_configuration();
    std::cout << "Current size: " << config.current_size << "\n";
    std::cout << "Bucket count: " << config.bucket_count << "\n";
    std::cout << "Active hash function ID: " << config.active_hash_function_id << "\n";

    std::cout << "\n6. Testing get_load_factor:\n";
    std::cout << "Load factor: " << ht.get_load_factor() << "\n";

    std::cout << "\n7. Testing get_collision_stats:\n";
    auto collision_stats = ht.get_collision_stats();
    std::cout << "Max chain length: " << collision_stats.max_chain_length << "\n";
    std::cout << "Average chain length: " << collision_stats.average_chain_length << "\n";
    std::cout << "Variance: " << collision_stats.variance << "\n";

    std::cout << "\n8. Testing get_performance_metrics:\n";
    auto perf_metrics = ht.get_performance_metrics();
    std::cout << "Average latency (ms): " << perf_metrics.average_latency_ms << "\n";
    std::cout << "Throughput (ops/sec): " << perf_metrics.throughput_ops_per_sec << "\n";

    std::cout << "\n9. Testing execute_resize:\n";
    std::cout << "Before resize - Bucket count: " << ht.get_configuration().bucket_count << "\n";
    ht.execute_resize(32);
    std::cout << "After resize - Bucket count: " << ht.get_configuration().bucket_count << "\n";

    std::cout << "\n10. Testing execute_change_hash_function:\n";
    std::cout << "Before change - Hash function ID: " << ht.get_configuration().active_hash_function_id << "\n";
    ht.execute_change_hash_function(2);
    std::cout << "After change - Hash function ID: " << ht.get_configuration().active_hash_function_id << "\n";

    std::cout << "\n11. Testing execute_do_nothing:\n";
    ht.execute_do_nothing();
    std::cout << "execute_do_nothing completed (no effect)\n";

    std::cout << "\n12. ";
    performance_test();

    return 0;
}