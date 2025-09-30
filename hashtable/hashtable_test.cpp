#include "hashtable.h"
#include <iostream>
#include <string>

using namespace std;

int main() {
    customhashtable::HashTable<string, int> ht(8);

    cout << "\n1. Testing put and get operations:\n";
    ht.put("apple", 5);
    ht.put("banana", 3);
    ht.put("cherry", 8);
    ht.put("date", 2);
    cout << "apple: " << ht.get("apple") << "\n";
    cout << "banana: " << ht.get("banana") << "\n";
    cout << "cherry: " << ht.get("cherry") << "\n";
    cout << "date: " << ht.get("date") << "\n";

    cout << "\n2. Testing contains operation:\n";
    cout << "Contains 'apple': " << (ht.contains("apple") ? "true" : "false") << "\n";
    cout << "Contains 'grape': " << (ht.contains("grape") ? "true" : "false") << "\n";

    cout << "\n3. Testing remove operation:\n";
    ht.remove("banana");
    cout << "Contains 'banana' after removal: " << (ht.contains("banana") ? "true" : "false") << "\n";

    cout << "\n4. Testing get_configuration:\n";
    auto config = ht.get_configuration();
    cout << "Current size: " << config.current_size << "\n";
    cout << "Bucket count: " << config.bucket_count << "\n";
    cout << "Active hash function ID: " << config.active_hash_function_id << "\n";

    cout << "\n5. Testing get_load_factor:\n";
    cout << "Load factor: " << ht.get_load_factor() << "\n";

    cout << "\n6. Testing get_collision_stats:\n";
    auto collision_stats = ht.get_collision_stats();
    cout << "Max chain length: " << collision_stats.max_chain_length << "\n";
    cout << "Average chain length: " << collision_stats.average_chain_length << "\n";
    cout << "Variance: " << collision_stats.variance << "\n";

    cout << "\n7. Testing get_performance_metrics:\n";
    auto perf_metrics = ht.get_performance_metrics();
    cout << "Average latency (ms): " << perf_metrics.average_latency_ms << "\n";
    cout << "Throughput (ops/sec): " << perf_metrics.throughput_ops_per_sec << "\n";

    cout << "\n8. Testing execute_resize:\n";
    cout << "Before resize - Bucket count: " << ht.get_configuration().bucket_count << "\n";
    ht.execute_resize(32);
    cout << "After resize - Bucket count: " << ht.get_configuration().bucket_count << "\n";

    cout << "\n9. Testing execute_change_hash_function:\n";
    cout << "Before change - Hash function ID: " << ht.get_configuration().active_hash_function_id << "\n";
    ht.execute_change_hash_function(2);
    cout << "After change - Hash function ID: " << ht.get_configuration().active_hash_function_id << "\n";

    cout << "\n10. Testing execute_do_nothing:\n";
    ht.execute_do_nothing();
    cout << "execute_do_nothing completed (no effect)\n";

    cout << "\n11. Adding more elements:\n";
    for (int i = 0; i < 20; ++i) {
        ht.put("key" + to_string(i), i * 10);
    }
    cout << "New load factor: " << ht.get_load_factor() << "\n";
    auto new_collision_stats = ht.get_collision_stats();
    cout << "New max chain length: " << new_collision_stats.max_chain_length << "\n";
    cout << "New average chain length: " << new_collision_stats.average_chain_length << "\n";

    return 0;
}