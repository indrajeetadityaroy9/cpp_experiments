#ifndef LRU_H
#define LRU_H

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace std;

template <typename K>
concept Hashable = requires(const K& key) {
    { hash<K>{}(key) } -> convertible_to<size_t>;
};

template <Hashable K, typename V>
class LRUCache {
public:
    using key_type = K;
    using mapped_type = V;
    using value_type = pair<const K&, V&>;
    using const_value_type = pair<const K&, const V&>;

private:
    static constexpr size_t INVALID_INDEX = numeric_limits<size_t>::max();

    template <typename T>
    struct Storage {
        alignas(T) byte bytes[sizeof(T)];

        T* ptr() noexcept { return std::launder(reinterpret_cast<T*>(bytes)); }
        const T* ptr() const noexcept { return std::launder(reinterpret_cast<const T*>(bytes)); }

        template <typename... Args>
        void construct(Args&&... args) {
            std::construct_at(ptr(), std::forward<Args>(args)...);
        }

        void destroy() noexcept { std::destroy_at(ptr()); }
    };

    struct Entry {
        Storage<K> key_storage;
        Storage<V> value_storage;
        size_t prev = INVALID_INDEX;
        size_t next = INVALID_INDEX;
        size_t hash = 0;
        size_t bucket_index = INVALID_INDEX;

        K& key() noexcept { return *key_storage.ptr(); }
        const K& key() const noexcept { return *key_storage.ptr(); }

        V& value() noexcept { return *value_storage.ptr(); }
        const V& value() const noexcept { return *value_storage.ptr(); }

        template <typename KArg, typename VArg>
        void construct(KArg&& key_arg, VArg&& value_arg) {
            key_storage.construct(std::forward<KArg>(key_arg));
            try {
                value_storage.construct(std::forward<VArg>(value_arg));
            } catch (...) {
                key_storage.destroy();
                throw;
            }
        }

        void destroy() noexcept {
            value_storage.destroy();
            key_storage.destroy();
        }
    };

    struct Bucket {
        size_t node_index = INVALID_INDEX;
        size_t psl = 0;

        bool is_empty() const noexcept { return node_index == INVALID_INDEX; }
    };

    vector<Entry> nodes_;
    vector<Bucket> hash_buckets_;
    size_t free_head_ = INVALID_INDEX;
    size_t lru_head_ = INVALID_INDEX;
    size_t lru_tail_ = INVALID_INDEX;
    size_t size_ = 0;

    static constexpr size_t next_power_of_two(size_t n) noexcept {
        if (n == 0) {
            return 1;
        }

        --n;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        n |= n >> 32;
        return n + 1;
    }

    static size_t hash_lookup(const K& key) { return hash<K>{}(key); }
    static bool keys_equal(const K& stored, const K& key) { return stored == key; }

    static size_t hash_lookup(string_view key) requires same_as<K, string> {
        return hash<string_view>{}(key);
    }

    static size_t hash_lookup(const char* key) requires same_as<K, string> {
        return hash<string_view>{}(key);
    }

    static bool keys_equal(const K& stored, string_view key) requires same_as<K, string> {
        return stored == key;
    }

    static bool keys_equal(const K& stored, const char* key) requires same_as<K, string> {
        return stored == key;
    }

    void init_free_list();
    template <typename KeyLike>
    size_t find_bucket_with_hash(const KeyLike& key, size_t hash_value) const
        requires requires(const K& stored, const KeyLike& lookup) {
            { hash_lookup(lookup) } -> convertible_to<size_t>;
            { keys_equal(stored, lookup) } -> convertible_to<bool>;
        };
    void insert_bucket(size_t node_index, size_t hash_value);
    void remove_bucket(size_t node_index);
    void link_as_mru(size_t node_index);
    void unlink(size_t node_index);
    void move_to_mru(size_t node_index);
    void evict_lru();
    void destroy_all() noexcept;

public:
    class iterator {
    public:
        using iterator_category = forward_iterator_tag;
        using difference_type = ptrdiff_t;
        using value_type = pair<const K&, V&>;
        using pointer = value_type*;
        using reference = value_type;

        iterator() = default;
        iterator(Entry* nodes, size_t index) : nodes_(nodes), current_(index) {}

        reference operator*() const { return {nodes_[current_].key(), nodes_[current_].value()}; }

        iterator& operator++() {
            current_ = nodes_[current_].next;
            return *this;
        }

        iterator operator++(int) {
            auto copy = *this;
            ++(*this);
            return copy;
        }

        bool operator==(const iterator& other) const {
            return nodes_ == other.nodes_ && current_ == other.current_;
        }

        bool operator!=(const iterator& other) const { return !(*this == other); }

    private:
        Entry* nodes_ = nullptr;
        size_t current_ = INVALID_INDEX;

        friend class const_iterator;
    };

    class const_iterator {
    public:
        using iterator_category = forward_iterator_tag;
        using difference_type = ptrdiff_t;
        using value_type = pair<const K&, const V&>;
        using pointer = const value_type*;
        using reference = value_type;

        const_iterator() = default;
        const_iterator(const Entry* nodes, size_t index) : nodes_(nodes), current_(index) {}
        const_iterator(const iterator& other) : nodes_(other.nodes_), current_(other.current_) {}

        reference operator*() const { return {nodes_[current_].key(), nodes_[current_].value()}; }

        const_iterator& operator++() {
            current_ = nodes_[current_].next;
            return *this;
        }

        const_iterator operator++(int) {
            auto copy = *this;
            ++(*this);
            return copy;
        }

        bool operator==(const const_iterator& other) const {
            return nodes_ == other.nodes_ && current_ == other.current_;
        }

        bool operator!=(const const_iterator& other) const { return !(*this == other); }

    private:
        const Entry* nodes_ = nullptr;
        size_t current_ = INVALID_INDEX;
    };

    explicit LRUCache(size_t item_limit);
    ~LRUCache();
    LRUCache(LRUCache&& other) noexcept;
    LRUCache& operator=(LRUCache&& other) noexcept;
    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;

    template <typename KeyLike>
    bool has(const KeyLike& key) const
        requires requires(const K& stored, const KeyLike& lookup) {
            { hash_lookup(lookup) } -> convertible_to<size_t>;
            { keys_equal(stored, lookup) } -> convertible_to<bool>;
        };

    template <typename KeyLike>
    V* get(const KeyLike& key)
        requires requires(const K& stored, const KeyLike& lookup) {
            { hash_lookup(lookup) } -> convertible_to<size_t>;
            { keys_equal(stored, lookup) } -> convertible_to<bool>;
        };

    template <typename KeyLike>
    const V* get(const KeyLike& key) const
        requires requires(const K& stored, const KeyLike& lookup) {
            { hash_lookup(lookup) } -> convertible_to<size_t>;
            { keys_equal(stored, lookup) } -> convertible_to<bool>;
        };

    template <typename KType, typename VType>
    bool set(KType&& key, VType&& value);

    size_t size() const noexcept { return size_; }
    size_t capacity() const noexcept { return nodes_.size(); }

    void clear();

    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;
    const_iterator cbegin() const noexcept;
    const_iterator cend() const noexcept;
};

template <Hashable K, typename V>
LRUCache<K, V>::LRUCache(size_t item_limit) : nodes_(item_limit) {
    if (nodes_.empty()) {
        return;
    }

    auto bucket_count = next_power_of_two((item_limit * 10) / 7);
    if (bucket_count < 4) {
        bucket_count = 4;
    }

    hash_buckets_.resize(bucket_count);
    init_free_list();
}

template <Hashable K, typename V>
LRUCache<K, V>::~LRUCache() {
    destroy_all();
}

template <Hashable K, typename V>
LRUCache<K, V>::LRUCache(LRUCache&& other) noexcept
    : nodes_(std::move(other.nodes_)),
      hash_buckets_(std::move(other.hash_buckets_)),
      free_head_(other.free_head_),
      lru_head_(other.lru_head_),
      lru_tail_(other.lru_tail_),
      size_(other.size_) {
    other.free_head_ = INVALID_INDEX;
    other.lru_head_ = INVALID_INDEX;
    other.lru_tail_ = INVALID_INDEX;
    other.size_ = 0;
}

template <Hashable K, typename V>
LRUCache<K, V>& LRUCache<K, V>::operator=(LRUCache&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    destroy_all();
    nodes_ = std::move(other.nodes_);
    hash_buckets_ = std::move(other.hash_buckets_);
    free_head_ = other.free_head_;
    lru_head_ = other.lru_head_;
    lru_tail_ = other.lru_tail_;
    size_ = other.size_;

    other.free_head_ = INVALID_INDEX;
    other.lru_head_ = INVALID_INDEX;
    other.lru_tail_ = INVALID_INDEX;
    other.size_ = 0;
    return *this;
}

template <Hashable K, typename V>
void LRUCache<K, V>::init_free_list() {
    for (size_t i = 0; i < nodes_.size(); ++i) {
        auto& node = nodes_[i];
        node.prev = INVALID_INDEX;
        node.next = i + 1 < nodes_.size() ? i + 1 : INVALID_INDEX;
        node.bucket_index = INVALID_INDEX;
    }
    free_head_ = nodes_.empty() ? INVALID_INDEX : 0;
}

template <Hashable K, typename V>
void LRUCache<K, V>::destroy_all() noexcept {
    for (auto node_index = lru_head_; node_index != INVALID_INDEX;) {
        auto& node = nodes_[node_index];
        const auto next_index = node.next;
        node.destroy();
        node_index = next_index;
    }
}

template <Hashable K, typename V>
template <typename KeyLike>
size_t LRUCache<K, V>::find_bucket_with_hash(const KeyLike& key, size_t hash_value) const
    requires requires(const K& stored, const KeyLike& lookup) {
        { hash_lookup(lookup) } -> convertible_to<size_t>;
        { keys_equal(stored, lookup) } -> convertible_to<bool>;
    } {
    if (hash_buckets_.empty()) {
        return INVALID_INDEX;
    }

    const auto mask = hash_buckets_.size() - 1;
    const auto ideal = hash_value & mask;

    for (size_t psl = 0; psl < hash_buckets_.size(); ++psl) {
        const auto index = (ideal + psl) & mask;
        const auto& bucket = hash_buckets_[index];

        if (bucket.is_empty() || bucket.psl < psl) {
            return INVALID_INDEX;
        }

        const auto& node = nodes_[bucket.node_index];
        if (node.hash == hash_value && keys_equal(node.key(), key)) {
            return index;
        }
    }

    return INVALID_INDEX;
}

template <Hashable K, typename V>
void LRUCache<K, V>::insert_bucket(size_t node_index, size_t hash_value) {
    const auto mask = hash_buckets_.size() - 1;
    const auto ideal = hash_value & mask;
    Bucket pending{node_index, 0};

    for (size_t psl = 0;; ++psl, ++pending.psl) {
        const auto index = (ideal + psl) & mask;
        auto& bucket = hash_buckets_[index];

        if (bucket.is_empty()) {
            bucket = pending;
            nodes_[bucket.node_index].bucket_index = index;
            return;
        }

        if (bucket.psl < pending.psl) {
            swap(bucket, pending);
            nodes_[bucket.node_index].bucket_index = index;
        }
    }
}

template <Hashable K, typename V>
void LRUCache<K, V>::remove_bucket(size_t node_index) {
    if (hash_buckets_.empty()) {
        return;
    }

    const auto mask = hash_buckets_.size() - 1;
    auto empty_index = nodes_[node_index].bucket_index;
    if (empty_index == INVALID_INDEX) {
        return;
    }

    nodes_[node_index].bucket_index = INVALID_INDEX;
    while (true) {
        const auto next_index = (empty_index + 1) & mask;
        auto& next_bucket = hash_buckets_[next_index];
        if (next_bucket.is_empty() || next_bucket.psl == 0) {
            hash_buckets_[empty_index] = {};
            return;
        }

        hash_buckets_[empty_index] = next_bucket;
        --hash_buckets_[empty_index].psl;
        nodes_[hash_buckets_[empty_index].node_index].bucket_index = empty_index;
        empty_index = next_index;
    }
}

template <Hashable K, typename V>
void LRUCache<K, V>::link_as_mru(size_t node_index) {
    auto& node = nodes_[node_index];
    node.prev = INVALID_INDEX;
    node.next = lru_head_;

    if (lru_head_ != INVALID_INDEX) {
        nodes_[lru_head_].prev = node_index;
    } else {
        lru_tail_ = node_index;
    }

    lru_head_ = node_index;
}

template <Hashable K, typename V>
void LRUCache<K, V>::unlink(size_t node_index) {
    const auto& node = nodes_[node_index];

    if (node.prev != INVALID_INDEX) {
        nodes_[node.prev].next = node.next;
    } else {
        lru_head_ = node.next;
    }

    if (node.next != INVALID_INDEX) {
        nodes_[node.next].prev = node.prev;
    } else {
        lru_tail_ = node.prev;
    }
}

template <Hashable K, typename V>
void LRUCache<K, V>::move_to_mru(size_t node_index) {
    if (node_index == lru_head_) {
        return;
    }

    unlink(node_index);
    link_as_mru(node_index);
}

template <Hashable K, typename V>
void LRUCache<K, V>::evict_lru() {
    const auto victim = lru_tail_;
    unlink(victim);
    remove_bucket(victim);

    auto& node = nodes_[victim];
    node.destroy();
    node.prev = INVALID_INDEX;
    node.next = free_head_;
    free_head_ = victim;
    --size_;
}

template <Hashable K, typename V>
void LRUCache<K, V>::clear() {
    destroy_all();
    size_ = 0;
    lru_head_ = INVALID_INDEX;
    lru_tail_ = INVALID_INDEX;
    fill(hash_buckets_.begin(), hash_buckets_.end(), Bucket{});
    init_free_list();
}

template <Hashable K, typename V>
template <typename KeyLike>
bool LRUCache<K, V>::has(const KeyLike& key) const
    requires requires(const K& stored, const KeyLike& lookup) {
        { hash_lookup(lookup) } -> convertible_to<size_t>;
        { keys_equal(stored, lookup) } -> convertible_to<bool>;
    } {
    return find_bucket_with_hash(key, hash_lookup(key)) != INVALID_INDEX;
}

template <Hashable K, typename V>
template <typename KeyLike>
V* LRUCache<K, V>::get(const KeyLike& key)
    requires requires(const K& stored, const KeyLike& lookup) {
        { hash_lookup(lookup) } -> convertible_to<size_t>;
        { keys_equal(stored, lookup) } -> convertible_to<bool>;
    } {
    const auto bucket_index = find_bucket_with_hash(key, hash_lookup(key));
    if (bucket_index == INVALID_INDEX) {
        return nullptr;
    }

    const auto node_index = hash_buckets_[bucket_index].node_index;
    move_to_mru(node_index);
    return &nodes_[node_index].value();
}

template <Hashable K, typename V>
template <typename KeyLike>
const V* LRUCache<K, V>::get(const KeyLike& key) const
    requires requires(const K& stored, const KeyLike& lookup) {
        { hash_lookup(lookup) } -> convertible_to<size_t>;
        { keys_equal(stored, lookup) } -> convertible_to<bool>;
    } {
    const auto bucket_index = find_bucket_with_hash(key, hash_lookup(key));
    if (bucket_index == INVALID_INDEX) {
        return nullptr;
    }

    return &nodes_[hash_buckets_[bucket_index].node_index].value();
}

template <Hashable K, typename V>
template <typename KType, typename VType>
bool LRUCache<K, V>::set(KType&& key, VType&& value) {
    if (nodes_.empty()) [[unlikely]] {
        return false;
    }

    const auto hash_value = hash<K>{}(key);
    const auto bucket_index = find_bucket_with_hash(key, hash_value);
    if (bucket_index != INVALID_INDEX) {
        auto& node = nodes_[hash_buckets_[bucket_index].node_index];
        node.value() = std::forward<VType>(value);
        move_to_mru(hash_buckets_[bucket_index].node_index);
        return true;
    }

    if (size_ == nodes_.size()) {
        evict_lru();
    }

    const auto slot = free_head_;
    auto& node = nodes_[slot];
    free_head_ = node.next;

    try {
        node.construct(std::forward<KType>(key), std::forward<VType>(value));
    } catch (...) {
        node.next = free_head_;
        free_head_ = slot;
        throw;
    }
    node.hash = hash_value;

    insert_bucket(slot, hash_value);
    link_as_mru(slot);
    ++size_;
    return true;
}

template <Hashable K, typename V>
typename LRUCache<K, V>::iterator LRUCache<K, V>::begin() noexcept {
    return iterator(nodes_.data(), lru_head_);
}

template <Hashable K, typename V>
typename LRUCache<K, V>::iterator LRUCache<K, V>::end() noexcept {
    return iterator(nodes_.data(), INVALID_INDEX);
}

template <Hashable K, typename V>
typename LRUCache<K, V>::const_iterator LRUCache<K, V>::begin() const noexcept {
    return const_iterator(nodes_.data(), lru_head_);
}

template <Hashable K, typename V>
typename LRUCache<K, V>::const_iterator LRUCache<K, V>::end() const noexcept {
    return const_iterator(nodes_.data(), INVALID_INDEX);
}

template <Hashable K, typename V>
typename LRUCache<K, V>::const_iterator LRUCache<K, V>::cbegin() const noexcept {
    return begin();
}

template <Hashable K, typename V>
typename LRUCache<K, V>::const_iterator LRUCache<K, V>::cend() const noexcept {
    return end();
}

#endif
