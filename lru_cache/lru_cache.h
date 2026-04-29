#ifndef LRU_H
#define LRU_H

#include <concepts>
#include <cstddef>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>


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

    // Get pointer to value (null if not found). O(1).
    [[nodiscard]] V* get(const K& key);
    [[nodiscard]] const V* get(const K& key) const;

    // Set or update value for key. O(1).
    // Evicts LRU entry if capacity exceeded.
    template <typename KType, typename VType>
    bool set(KType&& key, VType&& value);


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



#endif

// ============================================================================
// Constructor / Destructor
// ============================================================================

template<Hashable K, typename V>
LRUCache<K, V>::LRUCache(std::size_t item_limit)
    : free_head_(0),
      lru_head_(INVALID_INDEX),
      lru_tail_(INVALID_INDEX),
      capacity_(item_limit),
      size_(0),
      bucket_count_(0) {

    if (capacity_ == 0) {
        // Handle zero capacity - no allocations needed
        return;
    }

    // Allocate contiguous storage for entries
    nodes_ = std::make_unique<Entry[]>(capacity_);

    // Compute bucket count: load factor ~0.7, power of 2 for fast modulo
    bucket_count_ = next_power_of_two((capacity_ * 10) / 7);
    if (bucket_count_ < 4) bucket_count_ = 4;  // Minimum bucket count

    // Allocate hash buckets
    hash_buckets_ = std::make_unique<Bucket[]>(bucket_count_);

    // Initialize free list
    init_free_list();
}

template<Hashable K, typename V>
LRUCache<K, V>::~LRUCache() {
    clear_all();
}

// ============================================================================
// Move Semantics
// ============================================================================

template<Hashable K, typename V>
LRUCache<K, V>::LRUCache(LRUCache&& other) noexcept
    : nodes_(std::move(other.nodes_)),
      hash_buckets_(std::move(other.hash_buckets_)),
      free_head_(other.free_head_),
      lru_head_(other.lru_head_),
      lru_tail_(other.lru_tail_),
      capacity_(other.capacity_),
      size_(other.size_),
      bucket_count_(other.bucket_count_) {

    // Leave other in valid empty state
    other.capacity_ = 0;
    other.size_ = 0;
    other.bucket_count_ = 0;
    other.free_head_ = INVALID_INDEX;
    other.lru_head_ = INVALID_INDEX;
    other.lru_tail_ = INVALID_INDEX;
}

template<Hashable K, typename V>
LRUCache<K, V>& LRUCache<K, V>::operator=(LRUCache&& other) noexcept {
    if (this != &other) {
        // Clean up current state
        clear_all();

        // Move from other
        nodes_ = std::move(other.nodes_);
        hash_buckets_ = std::move(other.hash_buckets_);
        free_head_ = other.free_head_;
        lru_head_ = other.lru_head_;
        lru_tail_ = other.lru_tail_;
        capacity_ = other.capacity_;
        size_ = other.size_;
        bucket_count_ = other.bucket_count_;

        // Leave other in valid empty state
        other.capacity_ = 0;
        other.size_ = 0;
        other.bucket_count_ = 0;
        other.free_head_ = INVALID_INDEX;
        other.lru_head_ = INVALID_INDEX;
        other.lru_tail_ = INVALID_INDEX;
    }
    return *this;
}

// ============================================================================
// Slab Allocator Methods
// ============================================================================

template<Hashable K, typename V>
void LRUCache<K, V>::init_free_list() {
    for (std::size_t i = 0; i < capacity_; ++i) {
        nodes_[i].next = (i + 1 < capacity_) ? i + 1 : INVALID_INDEX;
        nodes_[i].occupied = false;
    }
    free_head_ = 0;
}

template<Hashable K, typename V>
std::size_t LRUCache<K, V>::alloc_slot() {
    std::size_t slot = free_head_;
    free_head_ = nodes_[slot].next;
    nodes_[slot].occupied = true;
    return slot;
}

template<Hashable K, typename V>
void LRUCache<K, V>::free_slot(std::size_t idx) {
    // Destroy non-trivial types
    if constexpr (!std::is_trivially_destructible_v<K>) {
        nodes_[idx].key.~K();
    }
    if constexpr (!std::is_trivially_destructible_v<V>) {
        nodes_[idx].value.~V();
    }

    nodes_[idx].occupied = false;
    nodes_[idx].next = free_head_;
    free_head_ = idx;
}

// ============================================================================
// Robin Hood Hash Table Methods
// ============================================================================

template<Hashable K, typename V>
std::size_t LRUCache<K, V>::compute_hash(const K& key) const noexcept {
    return std::hash<K>{}(key);
}

template<Hashable K, typename V>
std::size_t LRUCache<K, V>::find_bucket(const K& key) const {
    return find_bucket_with_hash(key, compute_hash(key));
}

template<Hashable K, typename V>
std::size_t LRUCache<K, V>::find_bucket_with_hash(const K& key, std::size_t hash) const {
    if (bucket_count_ == 0) return INVALID_INDEX;

    std::size_t ideal = hash & (bucket_count_ - 1);
    std::size_t psl = 0;

    while (true) {
        std::size_t idx = (ideal + psl) & (bucket_count_ - 1);
        const Bucket& b = hash_buckets_[idx];

        if (b.is_empty()) {
            return INVALID_INDEX;  // Not found
        }

        if (b.is_occupied()) {
            // Robin Hood property: if we've probed farther than this bucket's PSL,
            // the key would have been placed here if it existed
            if (b.psl < psl) {
                return INVALID_INDEX;
            }

            // Check if this is our key
            if (nodes_[b.node_index].hash == hash && nodes_[b.node_index].key == key) {
                return idx;  // Found
            }
        }

        ++psl;

        // Safety: prevent infinite loop (shouldn't happen with proper load factor)
        if (psl > bucket_count_) {
            return INVALID_INDEX;
        }
    }
}

template<Hashable K, typename V>
void LRUCache<K, V>::insert_bucket(std::size_t node_idx, std::size_t hash) {
    std::size_t ideal = hash & (bucket_count_ - 1);
    std::size_t psl = 0;
    std::size_t inserting_idx = node_idx;
    std::size_t inserting_psl = 0;

    while (true) {
        std::size_t idx = (ideal + psl) & (bucket_count_ - 1);
        Bucket& b = hash_buckets_[idx];

        if (b.is_empty() || b.is_tombstone()) {
            b.node_index = inserting_idx;
            b.psl = inserting_psl;
            return;
        }

        // Robin Hood: steal from the rich (swap if current has shorter probe sequence)
        if (b.psl < inserting_psl) {
            std::swap(b.node_index, inserting_idx);
            std::swap(b.psl, inserting_psl);
        }

        ++psl;
        ++inserting_psl;
    }
}

template<Hashable K, typename V>
void LRUCache<K, V>::remove_bucket(std::size_t node_idx) {
    // Find the bucket containing this node
    std::size_t hash = nodes_[node_idx].hash;
    std::size_t ideal = hash & (bucket_count_ - 1);
    std::size_t psl = 0;

    while (true) {
        std::size_t idx = (ideal + psl) & (bucket_count_ - 1);
        Bucket& b = hash_buckets_[idx];

        if (b.is_empty()) {
            return;  // Not found (shouldn't happen)
        }

        if (b.is_occupied() && b.node_index == node_idx) {
            // Found - use backward shift deletion instead of tombstone
            // This maintains Robin Hood properties better
            std::size_t empty_idx = idx;

            while (true) {
                std::size_t next_idx = (empty_idx + 1) & (bucket_count_ - 1);
                Bucket& next_b = hash_buckets_[next_idx];

                // Stop if next bucket is empty or has PSL of 0 (at ideal position)
                if (next_b.is_empty() || next_b.is_tombstone() || next_b.psl == 0) {
                    hash_buckets_[empty_idx] = Bucket{};  // Mark as empty
                    return;
                }

                // Shift this bucket back
                hash_buckets_[empty_idx] = next_b;
                hash_buckets_[empty_idx].psl--;
                empty_idx = next_idx;
            }
        }

        ++psl;
        if (psl > bucket_count_) return;  // Safety
    }
}

// ============================================================================
// LRU List Methods (Index-Based)
// ============================================================================

template<Hashable K, typename V>
void LRUCache<K, V>::link_as_mru(std::size_t idx) {
    Entry& e = nodes_[idx];
    e.prev = INVALID_INDEX;
    e.next = lru_head_;

    if (lru_head_ != INVALID_INDEX) {
        nodes_[lru_head_].prev = idx;
    }
    lru_head_ = idx;

    if (lru_tail_ == INVALID_INDEX) {
        lru_tail_ = idx;
    }
}

template<Hashable K, typename V>
void LRUCache<K, V>::unlink(std::size_t idx) {
    Entry& e = nodes_[idx];

    if (e.prev != INVALID_INDEX) {
        nodes_[e.prev].next = e.next;
    } else {
        lru_head_ = e.next;
    }

    if (e.next != INVALID_INDEX) {
        nodes_[e.next].prev = e.prev;
    } else {
        lru_tail_ = e.prev;
    }
}

template<Hashable K, typename V>
void LRUCache<K, V>::move_to_mru(std::size_t idx) {
    if (idx == lru_head_) return;  // Already MRU
    unlink(idx);
    link_as_mru(idx);
}

template<Hashable K, typename V>
void LRUCache<K, V>::evict_lru() {
    std::size_t victim = lru_tail_;
    unlink(victim);
    remove_bucket(victim);
    free_slot(victim);
    --size_;
}

// ============================================================================
// Clear Methods
// ============================================================================

template<Hashable K, typename V>
void LRUCache<K, V>::clear_all() {
    if (!nodes_) return;

    // Destroy all occupied entries
    for (std::size_t i = 0; i < capacity_; ++i) {
        if (nodes_[i].occupied) {
            if constexpr (!std::is_trivially_destructible_v<K>) {
                nodes_[i].key.~K();
            }
            if constexpr (!std::is_trivially_destructible_v<V>) {
                nodes_[i].value.~V();
            }
            nodes_[i].occupied = false;
        }
    }

    size_ = 0;
    lru_head_ = INVALID_INDEX;
    lru_tail_ = INVALID_INDEX;
}

template<Hashable K, typename V>
void LRUCache<K, V>::clear() {
    clear_all();

    // Reset hash buckets
    if (hash_buckets_) {
        for (std::size_t i = 0; i < bucket_count_; ++i) {
            hash_buckets_[i] = Bucket{};
        }
    }

    // Re-initialize free list
    if (nodes_ && capacity_ > 0) {
        init_free_list();
    }
}

// ============================================================================
// Public API: has()
// ============================================================================

template<Hashable K, typename V>
bool LRUCache<K, V>::has(const K& key) const {
    return find_bucket(key) != INVALID_INDEX;
}

// ============================================================================
// Public API: get()
// ============================================================================

template<Hashable K, typename V>
V* LRUCache<K, V>::get(const K& key) {
    std::size_t bucket_idx = find_bucket(key);
    if (bucket_idx == INVALID_INDEX) {
        return nullptr;
    }

    std::size_t node_idx = hash_buckets_[bucket_idx].node_index;
    move_to_mru(node_idx);
    return &nodes_[node_idx].value;
}

template<Hashable K, typename V>
const V* LRUCache<K, V>::get(const K& key) const {
    std::size_t bucket_idx = find_bucket(key);
    if (bucket_idx == INVALID_INDEX) {
        return nullptr;
    }

    std::size_t node_idx = hash_buckets_[bucket_idx].node_index;
    return &nodes_[node_idx].value;
}

// ============================================================================
// Public API: set()
// ============================================================================


template<Hashable K, typename V>
template<typename KType, typename VType>
bool LRUCache<K, V>::set(KType&& key, VType&& value) {
    if (capacity_ == 0) [[unlikely]] {
        return false;
    }

    std::size_t hash = compute_hash(key);
    std::size_t bucket_idx = find_bucket_with_hash(key, hash);

    if (bucket_idx != INVALID_INDEX) {
        std::size_t node_idx = hash_buckets_[bucket_idx].node_index;
        nodes_[node_idx].value = std::forward<VType>(value);
        move_to_mru(node_idx);
        return true;
    }

    if (size_ >= capacity_) {
        evict_lru();
    }

    std::size_t slot = alloc_slot();
    new (&nodes_[slot].key) K(std::forward<KType>(key));
    new (&nodes_[slot].value) V(std::forward<VType>(value));
    nodes_[slot].hash = hash;

    insert_bucket(slot, hash);
    link_as_mru(slot);
    ++size_;

    return true;
}

// ============================================================================
// Iterator Methods
// ============================================================================

template<Hashable K, typename V>
typename LRUCache<K, V>::iterator LRUCache<K, V>::begin() noexcept {
    return iterator(nodes_.get(), lru_head_);
}

template<Hashable K, typename V>
typename LRUCache<K, V>::iterator LRUCache<K, V>::end() noexcept {
    return iterator(nodes_.get(), INVALID_INDEX);
}

template<Hashable K, typename V>
typename LRUCache<K, V>::const_iterator LRUCache<K, V>::begin() const noexcept {
    return const_iterator(nodes_.get(), lru_head_);
}

template<Hashable K, typename V>
typename LRUCache<K, V>::const_iterator LRUCache<K, V>::end() const noexcept {
    return const_iterator(nodes_.get(), INVALID_INDEX);
}

template<Hashable K, typename V>
typename LRUCache<K, V>::const_iterator LRUCache<K, V>::cbegin() const noexcept {
    return begin();
}

template<Hashable K, typename V>
typename LRUCache<K, V>::const_iterator LRUCache<K, V>::cend() const noexcept {
    return end();
}
