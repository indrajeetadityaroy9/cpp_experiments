#ifndef ROBIN_HOOD_H
#define ROBIN_HOOD_H

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>

namespace robin_hood {

// ============================================================================
// Hash Functions
// ============================================================================

inline uint64_t splitmix64_hash(uint64_t key) noexcept {
    uint64_t z = key + 0x9e3779b97f4a7c15ULL;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

inline uint64_t fnv1a_hash(uint64_t key) noexcept {
    constexpr uint64_t FNV_PRIME = 1099511628211ULL;
    constexpr uint64_t FNV_OFFSET = 14695981039346656037ULL;

    uint64_t hash = FNV_OFFSET;
    for (int i = 0; i < 8; ++i) {
        hash ^= (key >> (i * 8)) & 0xFF;
        hash *= FNV_PRIME;
    }
    return hash;
}

// ============================================================================
// Concepts and Traits
// ============================================================================

template<size_t N>
constexpr bool is_power_of_two = (N > 0) && ((N & (N - 1)) == 0);

template<typename T>
concept Hashable = std::integral<T> || requires(T a) {
    { std::hash<T>{}(a) } -> std::convertible_to<size_t>;
};

template<typename T>
concept TableKey = Hashable<T> && std::equality_comparable<T>;

template<typename T>
concept TableValue = std::movable<T> && std::copyable<T>;

// ============================================================================
// Robin Hood Hash Table
// ============================================================================

#if defined(__APPLE__) && defined(__aarch64__)
inline constexpr size_t DEFAULT_CACHE_LINE_SIZE = 128;
#else
inline constexpr size_t DEFAULT_CACHE_LINE_SIZE = 64;
#endif

template<TableKey Key, TableValue Value, size_t Capacity,
         size_t CacheLineSize = DEFAULT_CACHE_LINE_SIZE>
    requires (Capacity >= 16) && is_power_of_two<Capacity>
class RobinHoodTable {

    static constexpr size_t INDEX_MASK = Capacity - 1;
    static constexpr uint8_t BUCKET_EMPTY = 0;
    static constexpr uint8_t BUCKET_OCCUPIED = 1;

    struct TableBucket {
        Key key;
        Value value;
        uint8_t state;
        uint8_t probe_distance;

        static constexpr size_t USED_SIZE = sizeof(Key) + sizeof(Value) + 2;
        static constexpr size_t PAD_SIZE =
            (CacheLineSize > USED_SIZE && CacheLineSize <= 128)
            ? (CacheLineSize - USED_SIZE) % CacheLineSize
            : 6;

        uint8_t padding[PAD_SIZE > 0 ? PAD_SIZE : 1];
    };

    alignas(CacheLineSize) std::array<TableBucket, Capacity> buckets_;
    size_t size_;

    size_t compute_hash(const Key& key) const noexcept {
        if constexpr (std::is_integral_v<Key>) {
            return splitmix64_hash(static_cast<uint64_t>(key));
        } else {
            return std::hash<Key>{}(key);
        }
    }

    size_t compute_bucket_index(const Key& key) const noexcept {
        return compute_hash(key) & INDEX_MASK;
    }

    bool insert_with_displacement(size_t idx, Key key, Value value, uint8_t distance) {
        size_t iterations = 0;
        while (iterations < Capacity) {
            TableBucket& bucket = buckets_[idx];

            if (bucket.state != BUCKET_OCCUPIED) {
                bucket.key = key;
                bucket.value = value;
                bucket.state = BUCKET_OCCUPIED;
                bucket.probe_distance = distance;
                return true;
            }

            if (distance > bucket.probe_distance) {
                std::swap(key, bucket.key);
                std::swap(value, bucket.value);
                std::swap(distance, bucket.probe_distance);
            }

            idx = (idx + 1) & INDEX_MASK;
            if (distance < 255) ++distance;
            ++iterations;
        }
        return false;
    }

public:
    RobinHoodTable() : size_(0) {
        for (auto& bucket : buckets_) {
            bucket.state = BUCKET_EMPTY;
            bucket.probe_distance = 0;
        }
    }

    RobinHoodTable(const RobinHoodTable&) = delete;
    RobinHoodTable& operator=(const RobinHoodTable&) = delete;
    RobinHoodTable(RobinHoodTable&&) = delete;
    RobinHoodTable& operator=(RobinHoodTable&&) = delete;

    [[nodiscard]] bool put(const Key& key, const Value& value) {
        size_t idx = compute_bucket_index(key);
        uint8_t distance = 0;

        __builtin_prefetch(&buckets_[idx], 1, 3);

        size_t probe_idx = idx;
        while (buckets_[probe_idx].state == BUCKET_OCCUPIED) {
            if (buckets_[probe_idx].probe_distance < distance) {
                break;
            }

            if (buckets_[probe_idx].key == key) {
                buckets_[probe_idx].value = value;
                return false;
            }

            probe_idx = (probe_idx + 1) & INDEX_MASK;
            if (distance < 255) ++distance;

            __builtin_prefetch(&buckets_[(probe_idx + 1) & INDEX_MASK], 0, 3);
        }

        if (!insert_with_displacement(idx, key, value, 0)) {
            return false;
        }
        ++size_;
        return true;
    }

    [[nodiscard]] Value* get(const Key& key) noexcept {
        size_t idx = compute_bucket_index(key);

        __builtin_prefetch(&buckets_[idx], 0, 3);

        uint8_t distance = 0;
        while (buckets_[idx].state == BUCKET_OCCUPIED) {
            if (distance > buckets_[idx].probe_distance) {
                return nullptr;
            }

            if (buckets_[idx].key == key) {
                return &buckets_[idx].value;
            }

            idx = (idx + 1) & INDEX_MASK;
            if (distance < 255) ++distance;

            __builtin_prefetch(&buckets_[(idx + 1) & INDEX_MASK], 0, 3);
        }

        return nullptr;
    }

    [[nodiscard]] const Value* get(const Key& key) const noexcept {
        size_t idx = compute_bucket_index(key);

        uint8_t distance = 0;
        while (buckets_[idx].state == BUCKET_OCCUPIED) {
            if (distance > buckets_[idx].probe_distance) {
                return nullptr;
            }

            if (buckets_[idx].key == key) {
                return &buckets_[idx].value;
            }

            idx = (idx + 1) & INDEX_MASK;
            if (distance < 255) ++distance;
        }

        return nullptr;
    }

    [[nodiscard]] size_t size() const noexcept { return size_; }
    [[nodiscard]] static constexpr size_t capacity() noexcept { return Capacity; }
    [[nodiscard]] static constexpr size_t cache_line_size() noexcept { return CacheLineSize; }
};

} // namespace robin_hood

#endif // ROBIN_HOOD_H
