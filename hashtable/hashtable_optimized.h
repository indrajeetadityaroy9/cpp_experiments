#ifndef HASHTABLE_H
#define HASHTABLE_H
#include <cstddef>
#include <vector>
#include <functional>
#include <chrono>
#include <cmath>
#include <algorithm>

namespace customhashtable {

    template <typename Key, typename Value>
    class HashTable {
    public:
        explicit HashTable(std::size_t initial_bucket_count = 16);

        HashTable(const HashTable& other);

        HashTable(HashTable&& other) noexcept;

        HashTable& operator=(const HashTable& other);

        HashTable& operator=(HashTable&& other) noexcept;

        ~HashTable();

        void put(const Key& key, const Value& value);
        void put(Key&& key, Value&& value);
        Value get(const Key& key) const;
        bool contains(const Key& key) const;
        void remove(const Key& key);

        double get_load_factor() const;
        struct CollisionStats {
            std::size_t max_chain_length;
            double average_chain_length;
            double variance;
        };
        CollisionStats get_collision_stats() const;

        struct PerformanceMetrics {
            double average_latency_ms;
            double throughput_ops_per_sec;
        };
        PerformanceMetrics get_performance_metrics(std::size_t last_n_ops = 100) const;

        struct Configuration {
            std::size_t current_size;
            std::size_t bucket_count;
            int active_hash_function_id;
        };
        Configuration get_configuration() const;

        void execute_resize(std::size_t new_size);
        void execute_change_hash_function(int new_function_id);
        void execute_do_nothing();

    private:
        struct Node {
            Key key;
            Value value;
            Node* next;
            Node(const Key& k, const Value& v) : key(k), value(v), next(nullptr) {}
            Node(Key&& k, Value&& v) : key(std::move(k)), value(std::move(v)), next(nullptr) {}
        };

        std::size_t hash_function_1(const Key& key) const;
        std::size_t hash_function_2(const Key& key) const;
        std::size_t hash_function_3(const Key& key) const;

        void grow();
        void rehash();
        void destroy_buckets();
        void copy_from(const HashTable& other);
        std::size_t get_bucket_index(const Key& key) const;
        bool is_power_of_two(std::size_t n) const;

        std::vector<Node*> buckets_;
        std::size_t size_;
        std::size_t bucket_count_;
        int active_hash_function_id_;

        static constexpr std::size_t MAX_TRACKED_OPS = 1000;
        mutable std::chrono::high_resolution_clock::time_point operation_timestamps_[MAX_TRACKED_OPS];
        mutable double operation_latencies_ms_[MAX_TRACKED_OPS];
        mutable std::size_t operation_count_;
        mutable std::size_t operation_index_;
    };

    inline std::size_t fnv1a_hash(const char* str, std::size_t len) {
        const std::size_t prime = 1099511628211ULL;
        std::size_t hash = 14695981039346656037ULL;

        for (std::size_t i = 0; i < len; ++i) {
            hash ^= static_cast<unsigned char>(str[i]);
            hash *= prime;
        }

        return hash;
    }
}

#include "hashtable_optimized.tpp"

#endif