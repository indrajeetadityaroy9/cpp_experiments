#ifndef LRU_H
#define LRU_H

#include <concepts>
#include <cstddef>
#include <expected>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

// Error types returned by LRUCache operations
enum class CacheError {
    KeyNotFound,   // Requested key does not exist in cache
    CapacityZero   // Cache was constructed with zero capacity
};

// Concept for hashable key types
template<typename K>
concept Hashable = requires(K k) {
    { std::hash<K>{}(k) } -> std::convertible_to<std::size_t>;
};

// High-performance thread-unsafe LRU cache with O(1) get/set operations.
//
// Optimizations:
// - Contiguous storage: All entries in a flat array for cache locality
// - Array-based linked list: Uses indices instead of pointers
// - Integrated slab allocator: O(1) allocation via embedded free list
// - Robin Hood hashing: Custom hash table with bounded probe sequences
//
// Template parameters:
//   K - Key type (must be Hashable and equality comparable)
//   V - Value type
//
// Supports move-only value types (use get_ref() instead of get()).
template<Hashable K, typename V>
class LRUCache {
public:
    using key_type = K;
    using mapped_type = V;
    using value_type = std::pair<const K&, V&>;
    using const_value_type = std::pair<const K&, const V&>;

private:
    // Sentinel values for index-based linking
    static constexpr std::size_t INVALID_INDEX = std::numeric_limits<std::size_t>::max();
    static constexpr std::size_t EMPTY_SLOT = INVALID_INDEX;
    static constexpr std::size_t TOMBSTONE = INVALID_INDEX - 1;

    // Entry struct - stored contiguously in nodes_ array
    // Combines key/value storage with LRU list linkage
    struct Entry {
        K key;
        V value;
        std::size_t prev;      // Index in nodes_ (LRU list linkage)
        std::size_t next;      // Index in nodes_ (LRU list linkage)
        std::size_t hash;      // Cached hash value (avoids rehashing)
        bool occupied;         // True if slot contains valid data

        Entry() : prev(INVALID_INDEX), next(INVALID_INDEX), hash(0), occupied(false) {}
    };

    // Robin Hood hash bucket
    struct Bucket {
        std::size_t node_index;  // Index into nodes_
        std::size_t psl;         // Probe sequence length (for Robin Hood)

        Bucket() : node_index(EMPTY_SLOT), psl(0) {}

        [[nodiscard]] bool is_empty() const noexcept { return node_index == EMPTY_SLOT; }
        [[nodiscard]] bool is_tombstone() const noexcept { return node_index == TOMBSTONE; }
        [[nodiscard]] bool is_occupied() const noexcept { return !is_empty() && !is_tombstone(); }
    };

    // Storage
    std::unique_ptr<Entry[]> nodes_;           // Contiguous node storage
    std::unique_ptr<Bucket[]> hash_buckets_;   // Robin Hood hash table

    // Slab allocator free list head (embedded in nodes_)
    std::size_t free_head_;

    // LRU list pointers (indices, not pointers)
    std::size_t lru_head_;  // Most recently used
    std::size_t lru_tail_;  // Least recently used

    // Sizing
    std::size_t capacity_;
    std::size_t size_;
    std::size_t bucket_count_;  // Power of 2 for fast modulo

    // Helper: compute next power of two
    static constexpr std::size_t next_power_of_two(std::size_t n) noexcept {
        if (n == 0) return 1;
        --n;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        n |= n >> 32;
        return n + 1;
    }

    // Slab allocator methods
    void init_free_list();
    [[nodiscard]] std::size_t alloc_slot();
    void free_slot(std::size_t idx);

    // Robin Hood hash table methods
    [[nodiscard]] std::size_t compute_hash(const K& key) const noexcept;
    [[nodiscard]] std::size_t find_bucket(const K& key) const;
    [[nodiscard]] std::size_t find_bucket_with_hash(const K& key, std::size_t hash) const;
    void insert_bucket(std::size_t node_idx, std::size_t hash);
    void remove_bucket(std::size_t node_idx);

    // LRU list methods (index-based)
    void link_as_mru(std::size_t idx);
    void unlink(std::size_t idx);
    void move_to_mru(std::size_t idx);
    void evict_lru();

    // Clear all entries
    void clear_all();

public:
    // Forward iterator for traversing cache from MRU to LRU
    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::pair<const K&, V&>;
        using pointer = value_type*;
        using reference = value_type;

        iterator() : nodes_(nullptr), current_(INVALID_INDEX) {}
        iterator(Entry* nodes, std::size_t idx) : nodes_(nodes), current_(idx) {}

        reference operator*() const { return {nodes_[current_].key, nodes_[current_].value}; }

        iterator& operator++() {
            current_ = nodes_[current_].next;
            return *this;
        }

        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const iterator& other) const { return current_ == other.current_; }
        bool operator!=(const iterator& other) const { return current_ != other.current_; }

    private:
        Entry* nodes_;
        std::size_t current_;
        friend class const_iterator;
    };

    // Const forward iterator for traversing cache from MRU to LRU
    class const_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::pair<const K&, const V&>;
        using pointer = const value_type*;
        using reference = value_type;

        const_iterator() : nodes_(nullptr), current_(INVALID_INDEX) {}
        const_iterator(const Entry* nodes, std::size_t idx) : nodes_(nodes), current_(idx) {}
        const_iterator(const iterator& it) : nodes_(it.nodes_), current_(it.current_) {}

        reference operator*() const { return {nodes_[current_].key, nodes_[current_].value}; }

        const_iterator& operator++() {
            current_ = nodes_[current_].next;
            return *this;
        }

        const_iterator operator++(int) {
            const_iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const const_iterator& other) const { return current_ == other.current_; }
        bool operator!=(const const_iterator& other) const { return current_ != other.current_; }

    private:
        const Entry* nodes_;
        std::size_t current_;
    };

    explicit LRUCache(std::size_t item_limit);
    ~LRUCache();

    // Move constructor
    LRUCache(LRUCache&& other) noexcept;
    // Move assignment
    LRUCache& operator=(LRUCache&& other) noexcept;

    // Non-copyable
    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;

    // Check if key exists without updating LRU order. O(1).
    [[nodiscard]] bool has(const K& key) const;

    // Get copy of value, updates LRU order. O(1).
    // Requires V to be copy-constructible.
    [[nodiscard]] std::expected<V, CacheError> get(const K& key) requires std::is_copy_constructible_v<V>;

    // Get const reference to value, updates LRU order. O(1).
    // Use for move-only types or to avoid copies.
    [[nodiscard]] std::expected<std::reference_wrapper<const V>, CacheError> get_ref(const K& key);

    // Set or update value for key. O(1).
    // Evicts LRU entry if capacity exceeded.
    [[nodiscard]] std::expected<void, CacheError> set(const K& key, const V& value);
    [[nodiscard]] std::expected<void, CacheError> set(const K& key, V&& value);
    [[nodiscard]] std::expected<void, CacheError> set(K&& key, const V& value);
    [[nodiscard]] std::expected<void, CacheError> set(K&& key, V&& value);

    // Get value as optional (nullopt if missing). O(1).
    // Convenience wrapper around get() for value-or-default patterns.
    [[nodiscard]] std::optional<V> get_optional(const K& key) requires std::is_copy_constructible_v<V>;

    // Query current size and capacity
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }

    // Clear all entries from the cache
    void clear();

    // Iterator access (MRU to LRU order)
    // Note: Iteration does not update LRU order
    iterator begin() noexcept;
    iterator end() noexcept;
    [[nodiscard]] const_iterator begin() const noexcept;
    [[nodiscard]] const_iterator end() const noexcept;
    [[nodiscard]] const_iterator cbegin() const noexcept;
    [[nodiscard]] const_iterator cend() const noexcept;
};

#include "lru.tpp"

#endif
