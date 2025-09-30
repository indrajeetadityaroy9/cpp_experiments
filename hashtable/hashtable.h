#ifndef HASHTABLE_H
#define HASHTABLE_H
#include <cstddef>
#include <vector>
#include <functional>
#include <chrono>
#include <cmath>
#include <utility>

using namespace std;

namespace customhashtable {

    template <typename Key, typename Value>
    class HashTable {
    public:
        explicit HashTable(size_t initial_bucket_count = 16);
        HashTable(const HashTable& other);
        HashTable(HashTable&& other) noexcept;
        HashTable& operator=(const HashTable& other);
        HashTable& operator=(HashTable&& other) noexcept;
        ~HashTable();

        void put(const Key& key, const Value& value);
        Value get(const Key& key) const;
        bool contains(const Key& key) const;
        void remove(const Key& key);
        double get_load_factor() const;

        struct CollisionStats {
            size_t max_chain_length;
            double average_chain_length;
            double variance;
        };
        CollisionStats get_collision_stats() const;

        struct PerformanceMetrics {
            double average_latency_ms;
            double throughput_ops_per_sec;
        };
        PerformanceMetrics get_performance_metrics(size_t last_n_ops = 100) const;

        struct Configuration {
            size_t current_size;
            size_t bucket_count;
            int active_hash_function_id;
        };
        Configuration get_configuration() const;

        void execute_resize(size_t new_size);
        void execute_change_hash_function(int new_function_id);
        void execute_do_nothing();

    private:
        struct Node {
            Key key;
            Value value;
            Node* next;
            Node(const Key& k, const Value& v) : key(k), value(v), next(nullptr) {}
        };

        size_t hash_function_1(const Key& key) const;
        size_t hash_function_2(const Key& key) const;
        size_t hash_function_3(const Key& key) const;

        void grow();
        void rehash();
        void destroy_buckets();
        void copy_from(const HashTable& other);
        size_t get_bucket_index(const Key& key) const;

        vector<Node*> buckets_;
        size_t size_;
        size_t bucket_count_;
        int active_hash_function_id_;
        mutable vector<chrono::high_resolution_clock::time_point> operation_timestamps_;
        mutable vector<double> operation_latencies_ms_;
    };

}

#include "hashtable.tpp"

#endif
