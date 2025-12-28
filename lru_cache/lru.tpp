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
std::expected<V, CacheError> LRUCache<K, V>::get(const K& key)
    requires std::is_copy_constructible_v<V>
{
    std::size_t bucket_idx = find_bucket(key);
    if (bucket_idx == INVALID_INDEX) {
        return std::unexpected(CacheError::KeyNotFound);
    }

    std::size_t node_idx = hash_buckets_[bucket_idx].node_index;
    move_to_mru(node_idx);
    return nodes_[node_idx].value;
}

// ============================================================================
// Public API: get_ref()
// ============================================================================

template<Hashable K, typename V>
std::expected<std::reference_wrapper<const V>, CacheError> LRUCache<K, V>::get_ref(const K& key) {
    std::size_t bucket_idx = find_bucket(key);
    if (bucket_idx == INVALID_INDEX) {
        return std::unexpected(CacheError::KeyNotFound);
    }

    std::size_t node_idx = hash_buckets_[bucket_idx].node_index;
    move_to_mru(node_idx);
    return std::cref(nodes_[node_idx].value);
}

// ============================================================================
// Public API: get_optional()
// ============================================================================

template<Hashable K, typename V>
std::optional<V> LRUCache<K, V>::get_optional(const K& key)
    requires std::is_copy_constructible_v<V>
{
    auto result = get(key);
    if (result.has_value()) {
        return result.value();
    }
    return std::nullopt;
}

// ============================================================================
// Public API: set() - All overloads
// ============================================================================

template<Hashable K, typename V>
std::expected<void, CacheError> LRUCache<K, V>::set(const K& key, const V& value) {
    if (capacity_ == 0) [[unlikely]] {
        return std::unexpected(CacheError::CapacityZero);
    }

    std::size_t hash = compute_hash(key);
    std::size_t bucket_idx = find_bucket_with_hash(key, hash);

    if (bucket_idx != INVALID_INDEX) {
        // Update existing entry
        std::size_t node_idx = hash_buckets_[bucket_idx].node_index;
        nodes_[node_idx].value = value;
        move_to_mru(node_idx);
        return {};
    }

    // Evict if at capacity
    if (size_ >= capacity_) {
        evict_lru();
    }

    // Allocate and insert new entry
    std::size_t slot = alloc_slot();
    new (&nodes_[slot].key) K(key);
    new (&nodes_[slot].value) V(value);
    nodes_[slot].hash = hash;

    insert_bucket(slot, hash);
    link_as_mru(slot);
    ++size_;

    return {};
}

template<Hashable K, typename V>
std::expected<void, CacheError> LRUCache<K, V>::set(const K& key, V&& value) {
    if (capacity_ == 0) [[unlikely]] {
        return std::unexpected(CacheError::CapacityZero);
    }

    std::size_t hash = compute_hash(key);
    std::size_t bucket_idx = find_bucket_with_hash(key, hash);

    if (bucket_idx != INVALID_INDEX) {
        std::size_t node_idx = hash_buckets_[bucket_idx].node_index;
        nodes_[node_idx].value = std::move(value);
        move_to_mru(node_idx);
        return {};
    }

    if (size_ >= capacity_) {
        evict_lru();
    }

    std::size_t slot = alloc_slot();
    new (&nodes_[slot].key) K(key);
    new (&nodes_[slot].value) V(std::move(value));
    nodes_[slot].hash = hash;

    insert_bucket(slot, hash);
    link_as_mru(slot);
    ++size_;

    return {};
}

template<Hashable K, typename V>
std::expected<void, CacheError> LRUCache<K, V>::set(K&& key, const V& value) {
    if (capacity_ == 0) [[unlikely]] {
        return std::unexpected(CacheError::CapacityZero);
    }

    std::size_t hash = compute_hash(key);
    std::size_t bucket_idx = find_bucket_with_hash(key, hash);

    if (bucket_idx != INVALID_INDEX) {
        std::size_t node_idx = hash_buckets_[bucket_idx].node_index;
        nodes_[node_idx].value = value;
        move_to_mru(node_idx);
        return {};
    }

    if (size_ >= capacity_) {
        evict_lru();
    }

    std::size_t slot = alloc_slot();
    new (&nodes_[slot].key) K(std::move(key));
    new (&nodes_[slot].value) V(value);
    nodes_[slot].hash = hash;

    insert_bucket(slot, hash);
    link_as_mru(slot);
    ++size_;

    return {};
}

template<Hashable K, typename V>
std::expected<void, CacheError> LRUCache<K, V>::set(K&& key, V&& value) {
    if (capacity_ == 0) [[unlikely]] {
        return std::unexpected(CacheError::CapacityZero);
    }

    std::size_t hash = compute_hash(key);
    std::size_t bucket_idx = find_bucket_with_hash(key, hash);

    if (bucket_idx != INVALID_INDEX) {
        std::size_t node_idx = hash_buckets_[bucket_idx].node_index;
        nodes_[node_idx].value = std::move(value);
        move_to_mru(node_idx);
        return {};
    }

    if (size_ >= capacity_) {
        evict_lru();
    }

    std::size_t slot = alloc_slot();
    new (&nodes_[slot].key) K(std::move(key));
    new (&nodes_[slot].value) V(std::move(value));
    nodes_[slot].hash = hash;

    insert_bucket(slot, hash);
    link_as_mru(slot);
    ++size_;

    return {};
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
