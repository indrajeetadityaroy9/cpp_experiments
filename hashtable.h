#ifndef HASHTABLE_H
#define HASHTABLE_H
#include <cstddef>
#include <vector>
#include <functional>
#include <chrono>
#include <cmath>

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
        };

        std::size_t hash_function_1(const Key& key) const;
        std::size_t hash_function_2(const Key& key) const;
        std::size_t hash_function_3(const Key& key) const;

        void grow();
        void rehash();
        void destroy_buckets();
        void copy_from(const HashTable& other);
        std::size_t get_bucket_index(const Key& key) const;

        std::vector<Node*> buckets_;
        std::size_t size_;
        std::size_t bucket_count_;
        int active_hash_function_id_;
        mutable std::vector<std::chrono::high_resolution_clock::time_point> operation_timestamps_;
        mutable std::vector<double> operation_latencies_ms_;
    };

    template <typename Key, typename Value>
    HashTable<Key, Value>::HashTable(std::size_t initial_bucket_count)
        : size_(0), bucket_count_(initial_bucket_count), active_hash_function_id_(1) {
        buckets_.resize(bucket_count_, nullptr);
    }

    template <typename Key, typename Value>
    HashTable<Key, Value>::HashTable(const HashTable& other) : buckets_(0) {
        copy_from(other);
    }

    template <typename Key, typename Value>
    HashTable<Key, Value>::HashTable(HashTable&& other) noexcept
        : buckets_(std::move(other.buckets_)),
          size_(other.size_),
          bucket_count_(other.bucket_count_),
          active_hash_function_id_(other.active_hash_function_id_),
          operation_timestamps_(std::move(other.operation_timestamps_)),
          operation_latencies_ms_(std::move(other.operation_latencies_ms_)) {
        other.size_ = 0;
        other.bucket_count_ = 0;
        other.active_hash_function_id_ = 1;
    }

    template <typename Key, typename Value>
    HashTable<Key, Value>& HashTable<Key, Value>::operator=(const HashTable& other) {
        if (this != &other) {
            destroy_buckets();
            copy_from(other);
        }
        return *this;
    }

    template <typename Key, typename Value>
    HashTable<Key, Value>& HashTable<Key, Value>::operator=(HashTable&& other) noexcept {
        if (this != &other) {
            destroy_buckets();
            buckets_ = std::move(other.buckets_);
            size_ = other.size_;
            bucket_count_ = other.bucket_count_;
            active_hash_function_id_ = other.active_hash_function_id_;
            operation_timestamps_ = std::move(other.operation_timestamps_);
            operation_latencies_ms_ = std::move(other.operation_latencies_ms_);
            other.size_ = 0;
            other.bucket_count_ = 0;
            other.active_hash_function_id_ = 1;
        }
        return *this;
    }

    template <typename Key, typename Value>
    HashTable<Key, Value>::~HashTable() {
        destroy_buckets();
    }

    template <typename Key, typename Value>
    void HashTable<Key, Value>::put(const Key& key, const Value& value) {
        auto start_time = std::chrono::high_resolution_clock::now();
        if (get_load_factor() > 0.75) {
            grow();
        }
        std::size_t index = get_bucket_index(key);
        Node* current = buckets_[index];
        while (current != nullptr) {
            if (current->key == key) {
                current->value = value;
                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
                operation_timestamps_.push_back(start_time);
                operation_latencies_ms_.push_back(duration.count() / 1000.0);
                return;
            }
            current = current->next;
        }
        Node* new_node = new Node(key, value);
        new_node->next = buckets_[index];
        buckets_[index] = new_node;
        size_++;
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        operation_timestamps_.push_back(start_time);
        operation_latencies_ms_.push_back(duration.count() / 1000.0);
    }

    template <typename Key, typename Value>
    Value HashTable<Key, Value>::get(const Key& key) const {
        auto start_time = std::chrono::high_resolution_clock::now();
        std::size_t index = get_bucket_index(key);
        Node* current = buckets_[index];
        while (current != nullptr) {
            if (current->key == key) {
                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
                operation_timestamps_.push_back(start_time);
                operation_latencies_ms_.push_back(duration.count() / 1000.0);
                return current->value;
            }
            current = current->next;
        }
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        operation_timestamps_.push_back(start_time);
        operation_latencies_ms_.push_back(duration.count() / 1000.0);
        return Value{};
    }

    template <typename Key, typename Value>
    bool HashTable<Key, Value>::contains(const Key& key) const {
        auto start_time = std::chrono::high_resolution_clock::now();
        std::size_t index = get_bucket_index(key);
        Node* current = buckets_[index];
        while (current != nullptr) {
            if (current->key == key) {
                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
                operation_timestamps_.push_back(start_time);
                operation_latencies_ms_.push_back(duration.count() / 1000.0);
                return true;
            }
            current = current->next;
        }
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        operation_timestamps_.push_back(start_time);
        operation_latencies_ms_.push_back(duration.count() / 1000.0);
        return false;
    }

    template <typename Key, typename Value>
    void HashTable<Key, Value>::remove(const Key& key) {
        auto start_time = std::chrono::high_resolution_clock::now();
        std::size_t index = get_bucket_index(key);
        Node* current = buckets_[index];
        Node* prev = nullptr;
        while (current != nullptr) {
            if (current->key == key) {
                if (prev == nullptr) {
                    buckets_[index] = current->next;
                } else {
                    prev->next = current->next;
                }
                delete current;
                size_--;
                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
                operation_timestamps_.push_back(start_time);
                operation_latencies_ms_.push_back(duration.count() / 1000.0);
                return;
            }
            prev = current;
            current = current->next;
        }
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        operation_timestamps_.push_back(start_time);
        operation_latencies_ms_.push_back(duration.count() / 1000.0);
    }

    template <typename Key, typename Value>
    double HashTable<Key, Value>::get_load_factor() const {
        return static_cast<double>(size_) / bucket_count_;
    }

    template <typename Key, typename Value>
    typename HashTable<Key, Value>::CollisionStats HashTable<Key, Value>::get_collision_stats() const {
        CollisionStats stats{};
        stats.max_chain_length = 0;
        std::size_t total_chain_length = 0;
        std::size_t non_empty_buckets = 0;
        std::vector<std::size_t> chain_lengths(bucket_count_, 0);
        for (std::size_t i = 0; i < bucket_count_; ++i) {
            std::size_t chain_length = 0;
            Node* current = buckets_[i];
            while (current != nullptr) {
                chain_length++;
                current = current->next;
            }
            chain_lengths[i] = chain_length;
            if (chain_length > 0) {
                non_empty_buckets++;
            }
            total_chain_length += chain_length;
            if (chain_length > stats.max_chain_length) {
                stats.max_chain_length = chain_length;
            }
        }
        if (non_empty_buckets > 0) {
            stats.average_chain_length = static_cast<double>(total_chain_length) / non_empty_buckets;
        } else {
            stats.average_chain_length = 0.0;
        }
        if (non_empty_buckets > 0) {
            double sum_squared_diff = 0.0;
            for (std::size_t i = 0; i < bucket_count_; ++i) {
                if (chain_lengths[i] > 0) {
                    double diff = static_cast<double>(chain_lengths[i]) - stats.average_chain_length;
                    sum_squared_diff += diff * diff;
                }
            }
            stats.variance = sum_squared_diff / non_empty_buckets;
        } else {
            stats.variance = 0.0;
        }
        return stats;
    }

    template <typename Key, typename Value>
    typename HashTable<Key, Value>::PerformanceMetrics 
    HashTable<Key, Value>::get_performance_metrics(std::size_t last_n_ops) const {
        PerformanceMetrics metrics{};
        if (operation_latencies_ms_.empty()) {
            metrics.average_latency_ms = 0.0;
            metrics.throughput_ops_per_sec = 0.0;
            return metrics;
        }
        std::size_t ops_to_consider = std::min(last_n_ops, operation_latencies_ms_.size());
        double total_latency = 0.0;
        for (std::size_t i = operation_latencies_ms_.size() - ops_to_consider; 
             i < operation_latencies_ms_.size(); ++i) {
            total_latency += operation_latencies_ms_[i];
        }
        metrics.average_latency_ms = total_latency / ops_to_consider;
        if (operation_timestamps_.size() >= 2) {
            auto start_time = operation_timestamps_[operation_timestamps_.size() - ops_to_consider];
            auto end_time = operation_timestamps_.back();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            if (duration.count() > 0) {
                metrics.throughput_ops_per_sec = (ops_to_consider * 1000.0) / duration.count();
            } else {
                metrics.throughput_ops_per_sec = 0.0;
            }
        } else {
            metrics.throughput_ops_per_sec = 0.0;
        }
        return metrics;
    }

    template <typename Key, typename Value>
    typename HashTable<Key, Value>::Configuration HashTable<Key, Value>::get_configuration() const {
        Configuration config{};
        config.current_size = size_;
        config.bucket_count = bucket_count_;
        config.active_hash_function_id = active_hash_function_id_;
        return config;
    }

    template <typename Key, typename Value>
    void HashTable<Key, Value>::execute_resize(std::size_t new_size) {
        bucket_count_ = new_size;
        rehash();
    }

    template <typename Key, typename Value>
    void HashTable<Key, Value>::execute_change_hash_function(int new_function_id) {
        active_hash_function_id_ = new_function_id;
        rehash();
    }

    template <typename Key, typename Value>
    void HashTable<Key, Value>::execute_do_nothing() {
    }

    template <typename Key, typename Value>
    std::size_t HashTable<Key, Value>::hash_function_1(const Key& key) const {
        std::hash<Key> hasher;
        return hasher(key);
    }

    template <typename Key, typename Value>
    std::size_t HashTable<Key, Value>::hash_function_2(const Key& key) const {
        std::hash<Key> hasher;
        std::size_t hash = hasher(key);
        hash ^= (hash << 13);
        hash ^= (hash >> 7);
        hash ^= (hash << 17);
        return hash;
    }

    template <typename Key, typename Value>
    std::size_t HashTable<Key, Value>::hash_function_3(const Key& key) const {
        std::hash<Key> hasher;
        std::size_t hash = hasher(key);
        hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
        hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
        hash = (hash >> 16) ^ hash;
        return hash;
    }

    template <typename Key, typename Value>
    void HashTable<Key, Value>::grow() {
        std::size_t new_bucket_count = bucket_count_ * 2;
        execute_resize(new_bucket_count);
    }

    template <typename Key, typename Value>
    void HashTable<Key, Value>::rehash() {
        std::vector<Node*> old_buckets = std::move(buckets_);
        std::size_t old_bucket_count = old_buckets.size();
        std::size_t new_size = 0;
        buckets_.clear();
        buckets_.resize(bucket_count_, nullptr);
        for (std::size_t i = 0; i < old_bucket_count; ++i) {
            Node* current = old_buckets[i];
            while (current != nullptr) {
                Node* next = current->next;
                std::size_t new_index = get_bucket_index(current->key);
                current->next = buckets_[new_index];
                buckets_[new_index] = current;
                new_size++;
                current = next;
            }
        }
        size_ = new_size;
    }

    template <typename Key, typename Value>
    void HashTable<Key, Value>::destroy_buckets() {
        for (std::size_t i = 0; i < buckets_.size(); ++i) {
            Node* current = buckets_[i];
            while (current != nullptr) {
                Node* next = current->next;
                delete current;
                current = next;
            }
        }
        buckets_.clear();
    }

    template <typename Key, typename Value>
    void HashTable<Key, Value>::copy_from(const HashTable& other) {
        bucket_count_ = other.bucket_count_;
        size_ = other.size_;
        active_hash_function_id_ = other.active_hash_function_id_;
        buckets_.resize(bucket_count_, nullptr);
        for (std::size_t i = 0; i < bucket_count_; ++i) {
            Node* current = other.buckets_[i];
            Node** new_current = &buckets_[i];
            while (current != nullptr) {
                *new_current = new Node(current->key, current->value);
                new_current = &((*new_current)->next);
                current = current->next;
            }
        }
        operation_timestamps_ = other.operation_timestamps_;
        operation_latencies_ms_ = other.operation_latencies_ms_;
    }

    template <typename Key, typename Value>
    std::size_t HashTable<Key, Value>::get_bucket_index(const Key& key) const {
        std::size_t hash;
        switch (active_hash_function_id_) {
            case 1:
                hash = hash_function_1(key);
                break;
            case 2:
                hash = hash_function_2(key);
                break;
            case 3:
                hash = hash_function_3(key);
                break;
            default:
                hash = hash_function_1(key);
                break;
        }
        return hash % bucket_count_;
    }

}

#endif