#ifndef LRU_BASELINE_H
#define LRU_BASELINE_H

#include <concepts>
#include <expected>
#include <functional>
#include <iterator>
#include <list>
#include <optional>
#include <unordered_map>
#include <utility>

// Baseline LRU Cache Implementation
//
// Uses standard library containers:
// - std::list for LRU ordering (doubly-linked list with individual allocations)
// - std::unordered_map for O(1) lookups (chaining hash table)
//
// This represents the "textbook" approach to LRU cache implementation.
// Used as a baseline for performance comparison against optimized versions.

enum class BaselineCacheError {
    KeyNotFound,
    CapacityZero
};

template<typename K>
concept BaselineHashable = requires(K k) {
    { std::hash<K>{}(k) } -> std::convertible_to<std::size_t>;
};

template<BaselineHashable K, typename V>
class LRUCacheBaseline {
public:
    using key_type = K;
    using mapped_type = V;
    using value_type = std::pair<const K&, V&>;
    using const_value_type = std::pair<const K&, const V&>;

private:
    // List stores key-value pairs, front = MRU, back = LRU
    using ListType = std::list<std::pair<K, V>>;
    using ListIterator = typename ListType::iterator;

    // Map from key to list iterator for O(1) access
    using MapType = std::unordered_map<K, ListIterator>;

    ListType list_;
    MapType map_;
    std::size_t capacity_;

public:
    // Forward iterator for traversing cache from MRU to LRU
    class iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::pair<const K&, V&>;
        using pointer = value_type*;
        using reference = value_type;

        iterator() = default;
        explicit iterator(ListIterator it) : it_(it) {}

        reference operator*() const { return {it_->first, it_->second}; }

        iterator& operator++() { ++it_; return *this; }
        iterator operator++(int) { iterator tmp = *this; ++it_; return tmp; }

        bool operator==(const iterator& other) const { return it_ == other.it_; }
        bool operator!=(const iterator& other) const { return it_ != other.it_; }

    private:
        ListIterator it_;
        friend class const_iterator;
    };

    class const_iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = std::pair<const K&, const V&>;
        using pointer = const value_type*;
        using reference = value_type;

        const_iterator() = default;
        explicit const_iterator(typename ListType::const_iterator it) : it_(it) {}
        const_iterator(const iterator& other) : it_(other.it_) {}

        reference operator*() const { return {it_->first, it_->second}; }

        const_iterator& operator++() { ++it_; return *this; }
        const_iterator operator++(int) { const_iterator tmp = *this; ++it_; return tmp; }

        bool operator==(const const_iterator& other) const { return it_ == other.it_; }
        bool operator!=(const const_iterator& other) const { return it_ != other.it_; }

    private:
        typename ListType::const_iterator it_;
    };

    explicit LRUCacheBaseline(std::size_t item_limit) : capacity_(item_limit) {
        map_.reserve(capacity_);
    }

    ~LRUCacheBaseline() = default;

    // Move constructor
    LRUCacheBaseline(LRUCacheBaseline&& other) noexcept
        : list_(std::move(other.list_)),
          map_(std::move(other.map_)),
          capacity_(other.capacity_) {
        other.capacity_ = 0;
    }

    // Move assignment
    LRUCacheBaseline& operator=(LRUCacheBaseline&& other) noexcept {
        if (this != &other) {
            list_ = std::move(other.list_);
            map_ = std::move(other.map_);
            capacity_ = other.capacity_;
            other.capacity_ = 0;
        }
        return *this;
    }

    // Non-copyable
    LRUCacheBaseline(const LRUCacheBaseline&) = delete;
    LRUCacheBaseline& operator=(const LRUCacheBaseline&) = delete;

    [[nodiscard]] bool has(const K& key) const {
        return map_.find(key) != map_.end();
    }

    [[nodiscard]] std::expected<V, BaselineCacheError> get(const K& key)
        requires std::is_copy_constructible_v<V>
    {
        auto it = map_.find(key);
        if (it == map_.end()) {
            return std::unexpected(BaselineCacheError::KeyNotFound);
        }
        // Move to front (MRU position)
        list_.splice(list_.begin(), list_, it->second);
        return it->second->second;
    }

    [[nodiscard]] std::expected<std::reference_wrapper<const V>, BaselineCacheError>
    get_ref(const K& key) {
        auto it = map_.find(key);
        if (it == map_.end()) {
            return std::unexpected(BaselineCacheError::KeyNotFound);
        }
        list_.splice(list_.begin(), list_, it->second);
        return std::cref(it->second->second);
    }

    [[nodiscard]] std::expected<void, BaselineCacheError> set(const K& key, const V& value) {
        if (capacity_ == 0) [[unlikely]] {
            return std::unexpected(BaselineCacheError::CapacityZero);
        }

        auto it = map_.find(key);
        if (it != map_.end()) {
            // Update existing
            it->second->second = value;
            list_.splice(list_.begin(), list_, it->second);
            return {};
        }

        // Evict if at capacity
        if (list_.size() >= capacity_) {
            auto& lru = list_.back();
            map_.erase(lru.first);
            list_.pop_back();
        }

        // Insert new entry at front
        list_.emplace_front(key, value);
        map_[key] = list_.begin();
        return {};
    }

    [[nodiscard]] std::expected<void, BaselineCacheError> set(const K& key, V&& value) {
        if (capacity_ == 0) [[unlikely]] {
            return std::unexpected(BaselineCacheError::CapacityZero);
        }

        auto it = map_.find(key);
        if (it != map_.end()) {
            it->second->second = std::move(value);
            list_.splice(list_.begin(), list_, it->second);
            return {};
        }

        if (list_.size() >= capacity_) {
            auto& lru = list_.back();
            map_.erase(lru.first);
            list_.pop_back();
        }

        list_.emplace_front(key, std::move(value));
        map_[key] = list_.begin();
        return {};
    }

    [[nodiscard]] std::expected<void, BaselineCacheError> set(K&& key, const V& value) {
        if (capacity_ == 0) [[unlikely]] {
            return std::unexpected(BaselineCacheError::CapacityZero);
        }

        auto it = map_.find(key);
        if (it != map_.end()) {
            it->second->second = value;
            list_.splice(list_.begin(), list_, it->second);
            return {};
        }

        if (list_.size() >= capacity_) {
            auto& lru = list_.back();
            map_.erase(lru.first);
            list_.pop_back();
        }

        K key_copy = std::move(key);
        list_.emplace_front(key_copy, value);
        map_[key_copy] = list_.begin();
        return {};
    }

    [[nodiscard]] std::expected<void, BaselineCacheError> set(K&& key, V&& value) {
        if (capacity_ == 0) [[unlikely]] {
            return std::unexpected(BaselineCacheError::CapacityZero);
        }

        auto it = map_.find(key);
        if (it != map_.end()) {
            it->second->second = std::move(value);
            list_.splice(list_.begin(), list_, it->second);
            return {};
        }

        if (list_.size() >= capacity_) {
            auto& lru = list_.back();
            map_.erase(lru.first);
            list_.pop_back();
        }

        K key_copy = std::move(key);
        list_.emplace_front(key_copy, std::move(value));
        map_[key_copy] = list_.begin();
        return {};
    }

    [[nodiscard]] std::optional<V> get_optional(const K& key)
        requires std::is_copy_constructible_v<V>
    {
        auto result = get(key);
        if (result.has_value()) {
            return result.value();
        }
        return std::nullopt;
    }

    [[nodiscard]] std::size_t size() const noexcept { return list_.size(); }
    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }

    void clear() {
        list_.clear();
        map_.clear();
    }

    iterator begin() noexcept { return iterator(list_.begin()); }
    iterator end() noexcept { return iterator(list_.end()); }
    [[nodiscard]] const_iterator begin() const noexcept { return const_iterator(list_.begin()); }
    [[nodiscard]] const_iterator end() const noexcept { return const_iterator(list_.end()); }
    [[nodiscard]] const_iterator cbegin() const noexcept { return begin(); }
    [[nodiscard]] const_iterator cend() const noexcept { return end(); }
};

#endif
