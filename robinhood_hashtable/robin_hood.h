#ifndef ROBIN_HOOD_H
#define ROBIN_HOOD_H

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>

namespace robin_hood {

// Hash Functions

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


// Concepts and Traits


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


// Robin Hood Hash Table


#if defined(__APPLE__) && defined(__aarch64__)
inline constexpr size_t DEFAULT_CACHE_LINE_SIZE = 128;
#else
inline constexpr size_t DEFAULT_CACHE_LINE_SIZE = 64;
#endif

// Open-addressing table with Robin Hood probing and backshift deletion.
//
// Layout: metadata and key/value slots live in two separate dense arrays.
// meta_[i] encodes both occupancy and probe distance in one byte:
//   0            -> empty
//   d + 1        -> occupied, displaced d slots from its home bucket
//                   (saturates at 255 for pathological chains)
// Probe loops scan only the metadata array (1 byte per slot, so an entire
// cache line covers 64+ slots) and touch the slot array once per candidate.
template<TableKey Key, TableValue Value, size_t Capacity,
         size_t CacheLineSize = DEFAULT_CACHE_LINE_SIZE>
    requires (Capacity >= 16) && is_power_of_two<Capacity> &&
             is_power_of_two<CacheLineSize>
class RobinHoodTable {

    static constexpr size_t INDEX_MASK = Capacity - 1;
    static constexpr uint8_t META_EMPTY = 0;
    static constexpr uint8_t META_MAX = 255;

    struct Slot {
        Key key;
        Value value;
    };

    alignas(CacheLineSize) std::array<uint8_t, Capacity> meta_;
    alignas(CacheLineSize) std::array<Slot, Capacity> slots_;
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

    // Robin Hood displacement starting at idx with the candidate's current
    // meta (probe distance + 1). Callers resume from wherever their probe
    // scan stopped instead of restarting at the home bucket.
    bool insert_with_displacement(size_t idx, Key key, Value value, uint8_t meta) {
        for (size_t iterations = 0; iterations < Capacity; ++iterations) {
            if (meta_[idx] == META_EMPTY) {
                slots_[idx].key = std::move(key);
                slots_[idx].value = std::move(value);
                meta_[idx] = meta;
                return true;
            }

            if (meta > meta_[idx]) {
                std::swap(key, slots_[idx].key);
                std::swap(value, slots_[idx].value);
                std::swap(meta, meta_[idx]);
            }

            idx = (idx + 1) & INDEX_MASK;
            if (meta < META_MAX) ++meta;
        }
        return false;
    }

public:
    RobinHoodTable() : size_(0) {
        meta_.fill(META_EMPTY);
    }

    RobinHoodTable(const RobinHoodTable&) = delete;
    RobinHoodTable& operator=(const RobinHoodTable&) = delete;
    RobinHoodTable(RobinHoodTable&&) = delete;
    RobinHoodTable& operator=(RobinHoodTable&&) = delete;

    [[nodiscard]] bool put(const Key& key, const Value& value) {
        size_t idx = compute_bucket_index(key);
        uint8_t meta = 1;

        // While the resident's probe distance is >= ours, our key may still
        // be further along the chain; once it drops below, the Robin Hood
        // invariant guarantees the key is absent and this is the insert point.
        while (meta_[idx] >= meta) {
            if (slots_[idx].key == key) {
                slots_[idx].value = value;
                return false;
            }
            idx = (idx + 1) & INDEX_MASK;
            if (meta < META_MAX) ++meta;
        }

        if (!insert_with_displacement(idx, key, value, meta)) {
            return false;
        }
        ++size_;
        return true;
    }

    [[nodiscard]] Value* get(const Key& key) noexcept {
        size_t idx = compute_bucket_index(key);
        uint8_t meta = 1;

        while (meta_[idx] >= meta) {
            if (slots_[idx].key == key) {
                return &slots_[idx].value;
            }
            idx = (idx + 1) & INDEX_MASK;
            if (meta < META_MAX) ++meta;
        }
        return nullptr;
    }

    [[nodiscard]] const Value* get(const Key& key) const noexcept {
        size_t idx = compute_bucket_index(key);
        uint8_t meta = 1;

        while (meta_[idx] >= meta) {
            if (slots_[idx].key == key) {
                return &slots_[idx].value;
            }
            idx = (idx + 1) & INDEX_MASK;
            if (meta < META_MAX) ++meta;
        }
        return nullptr;
    }

    // Backshift deletion: instead of tombstones, slide every displaced
    // successor back one slot until a hole or a home-positioned entry.
    bool erase(const Key& key) noexcept {
        size_t idx = compute_bucket_index(key);
        uint8_t meta = 1;

        while (meta_[idx] >= meta) {
            if (slots_[idx].key == key) {
                size_t hole = idx;
                size_t next = (hole + 1) & INDEX_MASK;
                while (meta_[next] > 1) {
                    slots_[hole] = std::move(slots_[next]);
                    meta_[hole] = meta_[next] - 1;
                    hole = next;
                    next = (next + 1) & INDEX_MASK;
                }
                meta_[hole] = META_EMPTY;
                --size_;
                return true;
            }
            idx = (idx + 1) & INDEX_MASK;
            if (meta < META_MAX) ++meta;
        }
        return false;
    }

    [[nodiscard]] size_t size() const noexcept { return size_; }
    [[nodiscard]] static constexpr size_t capacity() noexcept { return Capacity; }
    [[nodiscard]] static constexpr size_t cache_line_size() noexcept { return CacheLineSize; }
};

} // namespace robin_hood

#endif // ROBIN_HOOD_H
