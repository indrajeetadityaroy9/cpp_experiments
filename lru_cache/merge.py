import re

with open('lru.h', 'r') as f:
    h = f.read()

with open('lru.tpp', 'r') as f:
    tpp = f.read()

# Refactor lru.h
h = h.replace('#include "lru.tpp"', '')
h = re.sub(r'#include <expected>\n', '', h)
h = re.sub(r'// Error types returned by LRUCache operations\nenum class CacheError \{.*?\};\n', '', h, flags=re.DOTALL)

# Replace get, get_ref, get_optional
gets = r'''    // Get copy of value, updates LRU order\. O\(1\)\.\n    // Requires V to be copy-constructible\.\n    \[\[nodiscard\]\] std::expected<V, CacheError> get\(const K& key\) requires std::is_copy_constructible_v<V>;\n\n    // Get const reference to value, updates LRU order\. O\(1\)\.\n    // Use for move-only types or to avoid copies\.\n    \[\[nodiscard\]\] std::expected<std::reference_wrapper<const V>, CacheError> get_ref\(const K& key\);\n'''
h = re.sub(gets, '    // Get pointer to value (null if not found). O(1).\n    [[nodiscard]] V* get(const K& key);\n    [[nodiscard]] const V* get(const K& key) const;\n', h)

opts = r'''    // Get value as optional \(nullopt if missing\)\. O\(1\)\.\n    // Convenience wrapper around get\(\) for value-or-default patterns\.\n    \[\[nodiscard\]\] std::optional<V> get_optional\(const K& key\) requires std::is_copy_constructible_v<V>;\n'''
h = re.sub(opts, '', h)

# Replace sets
sets = r'''    // Set or update value for key\. O\(1\)\.\n    // Evicts LRU entry if capacity exceeded\.\n    \[\[nodiscard\]\] std::expected<void, CacheError> set\(const K& key, const V& value\);\n    \[\[nodiscard\]\] std::expected<void, CacheError> set\(const K& key, V&& value\);\n    \[\[nodiscard\]\] std::expected<void, CacheError> set\(K&& key, const V& value\);\n    \[\[nodiscard\]\] std::expected<void, CacheError> set\(K&& key, V&& value\);\n'''
h = re.sub(sets, '    // Set or update value for key. O(1).\n    // Evicts LRU entry if capacity exceeded.\n    template <typename KType, typename VType>\n    bool set(KType&& key, VType&& value);\n', h)

# In lru.tpp: remove get, get_ref, get_optional
tpp = re.sub(r'// ============================================================================\n// Public API: get\(\)\n// ============================================================================\n.*?// ============================================================================\n// Public API: set\(\) - All overloads\n', '// ============================================================================\n// Public API: set() - All overloads\n', tpp, flags=re.DOTALL)

# Add new get
new_get = '''// ============================================================================
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
// ============================================================================'''

tpp = tpp.replace('// ============================================================================\n// Public API: set() - All overloads\n// ============================================================================', new_get)

# Remove set implementations
tpp = re.sub(r'template<Hashable K, typename V>\nstd::expected<void, CacheError> LRUCache<K, V>::set\(const K& key, const V& value\) \{.*?// ============================================================================\n// Iterator Methods', '// Iterator Methods', tpp, flags=re.DOTALL)

new_set = '''
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
// Iterator Methods'''

tpp = tpp.replace('// Iterator Methods', new_set)

with open('lru_cache.h', 'w') as f:
    f.write(h)
    f.write('\n')
    f.write(tpp)
