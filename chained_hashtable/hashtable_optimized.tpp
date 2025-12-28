#ifndef HASHTABLE_OPTIMIZED_TPP
#define HASHTABLE_OPTIMIZED_TPP

namespace customhashtable {

    template <typename Key, typename Value>
    HashTable<Key, Value>::HashTable(std::size_t initial_bucket_count)
        : size_(0), bucket_count_(initial_bucket_count), active_hash_function_id_(1),
          operation_count_(0), operation_index_(0) {
        // Round up to power of two with overflow protection
        std::size_t power = 1;
        while (power < initial_bucket_count && power <= SIZE_MAX / 2) {
            power <<= 1;
        }
        // Ensure minimum bucket count of 1
        bucket_count_ = std::max(power, static_cast<std::size_t>(1));
        buckets_.resize(bucket_count_, nullptr);

        std::fill(operation_latencies_ms_, operation_latencies_ms_ + MAX_TRACKED_OPS, 0.0);
    }

    template <typename Key, typename Value>
    HashTable<Key, Value>::HashTable(const HashTable& other)
        : buckets_(0), operation_count_(0), operation_index_(0) {
        copy_from(other);
    }

    template <typename Key, typename Value>
    HashTable<Key, Value>::HashTable(HashTable&& other) noexcept
        : buckets_(std::move(other.buckets_)),
          size_(other.size_),
          bucket_count_(other.bucket_count_),
          active_hash_function_id_(other.active_hash_function_id_),
          operation_count_(other.operation_count_),
          operation_index_(other.operation_index_) {
        for (std::size_t i = 0; i < MAX_TRACKED_OPS; ++i) {
            operation_timestamps_[i] = other.operation_timestamps_[i];
            operation_latencies_ms_[i] = other.operation_latencies_ms_[i];
        }

        other.size_ = 0;
        other.bucket_count_ = 0;
        other.active_hash_function_id_ = 1;
        other.operation_count_ = 0;
        other.operation_index_ = 0;
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

            operation_count_ = other.operation_count_;
            operation_index_ = other.operation_index_;
            for (std::size_t i = 0; i < MAX_TRACKED_OPS; ++i) {
                operation_timestamps_[i] = other.operation_timestamps_[i];
                operation_latencies_ms_[i] = other.operation_latencies_ms_[i];
            }

            other.size_ = 0;
            other.bucket_count_ = 0;
            other.active_hash_function_id_ = 1;
            other.operation_count_ = 0;
            other.operation_index_ = 0;
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
                auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);

                operation_timestamps_[operation_index_] = start_time;
                operation_latencies_ms_[operation_index_] = duration.count() / 1000000.0;
                operation_index_ = (operation_index_ + 1) % MAX_TRACKED_OPS;
                if (operation_count_ < MAX_TRACKED_OPS) {
                    operation_count_++;
                }
                return;
            }
            current = current->next;
        }

        Node* new_node = new Node(key, value);
        new_node->next = buckets_[index];
        buckets_[index] = new_node;
        size_++;

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);

        operation_timestamps_[operation_index_] = start_time;
        operation_latencies_ms_[operation_index_] = duration.count() / 1000000.0;
        operation_index_ = (operation_index_ + 1) % MAX_TRACKED_OPS;
        if (operation_count_ < MAX_TRACKED_OPS) {
            operation_count_++;
        }
    }

    template <typename Key, typename Value>
    void HashTable<Key, Value>::put(Key&& key, Value&& value) {
        auto start_time = std::chrono::high_resolution_clock::now();

        if (get_load_factor() > 0.75) {
            grow();
        }

        std::size_t index = get_bucket_index(key);
        Node* current = buckets_[index];

        while (current != nullptr) {
            if (current->key == key) {
                current->value = std::move(value);
                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);

                operation_timestamps_[operation_index_] = start_time;
                operation_latencies_ms_[operation_index_] = duration.count() / 1000000.0;
                operation_index_ = (operation_index_ + 1) % MAX_TRACKED_OPS;
                if (operation_count_ < MAX_TRACKED_OPS) {
                    operation_count_++;
                }
                return;
            }
            current = current->next;
        }

        Node* new_node = new Node(std::move(key), std::move(value));
        new_node->next = buckets_[index];
        buckets_[index] = new_node;
        size_++;

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);

        operation_timestamps_[operation_index_] = start_time;
        operation_latencies_ms_[operation_index_] = duration.count() / 1000000.0;
        operation_index_ = (operation_index_ + 1) % MAX_TRACKED_OPS;
        if (operation_count_ < MAX_TRACKED_OPS) {
            operation_count_++;
        }
    }

    template <typename Key, typename Value>
    std::expected<Value, HashTableError> HashTable<Key, Value>::get_checked(const Key& key) const {
        auto start_time = std::chrono::high_resolution_clock::now();

        std::size_t index = get_bucket_index(key);
        Node* current = buckets_[index];

        while (current != nullptr) {
            if (current->key == key) {
                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);

                operation_timestamps_[operation_index_] = start_time;
                operation_latencies_ms_[operation_index_] = duration.count() / 1000000.0;
                operation_index_ = (operation_index_ + 1) % MAX_TRACKED_OPS;
                if (operation_count_ < MAX_TRACKED_OPS) {
                    operation_count_++;
                }

                return current->value;
            }
            current = current->next;
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);

        operation_timestamps_[operation_index_] = start_time;
        operation_latencies_ms_[operation_index_] = duration.count() / 1000000.0;
        operation_index_ = (operation_index_ + 1) % MAX_TRACKED_OPS;
        if (operation_count_ < MAX_TRACKED_OPS) {
            operation_count_++;
        }

        return std::unexpected(HashTableError::KeyNotFound);
    }

    template <typename Key, typename Value>
    bool HashTable<Key, Value>::contains(const Key& key) const {
        auto start_time = std::chrono::high_resolution_clock::now();

        std::size_t index = get_bucket_index(key);
        Node* current = buckets_[index];

        while (current != nullptr) {
            if (current->key == key) {
                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);

                operation_timestamps_[operation_index_] = start_time;
                operation_latencies_ms_[operation_index_] = duration.count() / 1000000.0;
                operation_index_ = (operation_index_ + 1) % MAX_TRACKED_OPS;
                if (operation_count_ < MAX_TRACKED_OPS) {
                    operation_count_++;
                }

                return true;
            }
            current = current->next;
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);

        operation_timestamps_[operation_index_] = start_time;
        operation_latencies_ms_[operation_index_] = duration.count() / 1000000.0;
        operation_index_ = (operation_index_ + 1) % MAX_TRACKED_OPS;
        if (operation_count_ < MAX_TRACKED_OPS) {
            operation_count_++;
        }

        return false;
    }

    template <typename Key, typename Value>
    bool HashTable<Key, Value>::remove(const Key& key) {
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
                auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);

                operation_timestamps_[operation_index_] = start_time;
                operation_latencies_ms_[operation_index_] = duration.count() / 1000000.0;
                operation_index_ = (operation_index_ + 1) % MAX_TRACKED_OPS;
                if (operation_count_ < MAX_TRACKED_OPS) {
                    operation_count_++;
                }

                return true;
            }
            prev = current;
            current = current->next;
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);

        operation_timestamps_[operation_index_] = start_time;
        operation_latencies_ms_[operation_index_] = duration.count() / 1000000.0;
        operation_index_ = (operation_index_ + 1) % MAX_TRACKED_OPS;
        if (operation_count_ < MAX_TRACKED_OPS) {
            operation_count_++;
        }

        return false;
    }

    template <typename Key, typename Value>
    double HashTable<Key, Value>::get_load_factor() const {
        if (bucket_count_ == 0) return 0.0;
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

        if (operation_count_ == 0) {
            metrics.average_latency_ms = 0.0;
            metrics.throughput_ops_per_sec = 0.0;
            return metrics;
        }

        std::size_t ops_to_consider = std::min({last_n_ops, operation_count_, static_cast<std::size_t>(MAX_TRACKED_OPS)});

        double total_latency = 0.0;
        std::size_t start_idx = (operation_count_ >= MAX_TRACKED_OPS) ?
                                ((operation_index_ >= ops_to_consider) ?
                                 (operation_index_ - ops_to_consider) :
                                 (MAX_TRACKED_OPS - (ops_to_consider - operation_index_))) :
                                0;

        for (std::size_t i = 0; i < ops_to_consider; ++i) {
            std::size_t idx = (start_idx + i) % MAX_TRACKED_OPS;
            total_latency += operation_latencies_ms_[idx];
        }
        metrics.average_latency_ms = total_latency / ops_to_consider;

        if (operation_count_ >= 2) {
            std::size_t first_idx, last_idx;

            if (operation_count_ >= MAX_TRACKED_OPS) {
                first_idx = start_idx;
                last_idx = (operation_index_ > 0) ? (operation_index_ - 1) : (MAX_TRACKED_OPS - 1);
            } else {
                first_idx = 0;
                last_idx = operation_count_ - 1;
            }

            auto start_time = operation_timestamps_[first_idx];
            auto end_time = operation_timestamps_[last_idx];
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
        // Round up to power of two with overflow protection
        std::size_t power = 1;
        while (power < new_size && power <= SIZE_MAX / 2) {
            power <<= 1;
        }
        // Ensure minimum bucket count of 1
        bucket_count_ = std::max(power, static_cast<std::size_t>(1));
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
        if constexpr (std::is_same_v<Key, std::string>) {
            return fnv1a_hash(key.c_str(), key.length());
        } else {
            std::hash<Key> hasher;
            return hasher(key);
        }
    }

    template <typename Key, typename Value>
    std::size_t HashTable<Key, Value>::hash_function_2(const Key& key) const {
        if constexpr (std::is_same_v<Key, std::string>) {
            const std::size_t prime = 1099511628211ULL;
            std::size_t hash = 14695981039346656037ULL;

            for (std::size_t i = 0; i < key.length(); ++i) {
                hash ^= static_cast<unsigned char>(key[i]);
                hash *= prime;
                hash ^= (hash >> 23);
            }

            return hash;
        } else {
            std::hash<Key> hasher;
            std::size_t hash = hasher(key);
            hash ^= (hash << 13);
            hash ^= (hash >> 7);
            hash ^= (hash << 17);
            return hash;
        }
    }

    template <typename Key, typename Value>
    std::size_t HashTable<Key, Value>::hash_function_3(const Key& key) const {
        if constexpr (std::is_same_v<Key, std::string>) {
            const std::size_t prime = 1099511628211ULL;
            std::size_t hash = 9275218103934665603ULL;

            for (std::size_t i = 0; i < key.length(); ++i) {
                hash ^= static_cast<unsigned char>(key[i]);
                hash *= prime;
                hash ^= (hash >> 17);
            }

            return hash;
        } else {
            std::hash<Key> hasher;
            std::size_t hash = hasher(key);
            hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
            hash = ((hash >> 16) ^ hash) * 0x45d9f3b;
            hash = (hash >> 16) ^ hash;
            return hash;
        }
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
        // Create new buckets with exception safety
        std::vector<Node*> new_buckets(other.bucket_count_, nullptr);
        try {
            for (std::size_t i = 0; i < other.bucket_count_; ++i) {
                Node* current = other.buckets_[i];
                Node** new_current = &new_buckets[i];
                while (current != nullptr) {
                    *new_current = new Node(current->key, current->value);
                    new_current = &((*new_current)->next);
                    current = current->next;
                }
            }
            // Success - commit changes
            bucket_count_ = other.bucket_count_;
            size_ = other.size_;
            active_hash_function_id_ = other.active_hash_function_id_;
            buckets_ = std::move(new_buckets);

            // Copy metrics (fixed-size arrays)
            operation_count_ = other.operation_count_;
            operation_index_ = other.operation_index_;
            for (std::size_t i = 0; i < MAX_TRACKED_OPS; ++i) {
                operation_timestamps_[i] = other.operation_timestamps_[i];
                operation_latencies_ms_[i] = other.operation_latencies_ms_[i];
            }
        } catch (...) {
            // Cleanup on exception
            for (auto* bucket : new_buckets) {
                while (bucket) {
                    auto* next = bucket->next;
                    delete bucket;
                    bucket = next;
                }
            }
            throw;
        }
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
        return hash & (bucket_count_ - 1);
    }
}

#endif
