#ifndef HASHTABLE_H
#define HASHTABLE_H
#include <cstddef>
#include <vector>
#include <functional>
#include <chrono>
#include <cmath>
#include <utility>
#include <expected>
#include <optional>
#include <concepts>

using namespace std;

namespace customhashtable {

    enum class HashTableError {
        KeyNotFound
    };

    /**
     * Thread Safety Warning:
     * This HashTable implementation is NOT thread-safe. The mutable performance
     * metrics (operation_timestamps_, operation_latencies_ms_, operation_count_,
     * operation_index_) are modified during const operations like get_checked()
     * and contains(). Concurrent access from multiple threads will cause data races.
     *
     * For multi-threaded use, external synchronization is required, or use a
     * thread-safe wrapper.
     *
     * Template Requirements:
     * - Key must be equality comparable (support operator==)
     * - Key must be hashable (std::hash<Key> must be defined)
     */
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
        expected<Value, HashTableError> get_checked(const Key& key) const;
        bool contains(const Key& key) const;
        bool remove(const Key& key);
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

        // Fixed-size circular buffer for metrics (prevents unbounded growth)
        static constexpr size_t MAX_TRACKED_OPS = 1000;
        mutable chrono::high_resolution_clock::time_point operation_timestamps_[MAX_TRACKED_OPS];
        mutable double operation_latencies_ms_[MAX_TRACKED_OPS];
        mutable size_t operation_count_ = 0;
        mutable size_t operation_index_ = 0;
    };

}

#include "hashtable.tpp"

#endif
