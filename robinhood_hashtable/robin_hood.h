#ifndef ROBIN_HOOD_H
#define ROBIN_HOOD_H

#if !defined(__ARM_NEON) || !defined(__ARM_FEATURE_CRC32)
#error "robin_hood.h requires ARMv8 NEON and CRC32 extensions"
#endif

#include <arm_acle.h>
#include <arm_neon.h>

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace robin_hood {

template<size_t N>
constexpr bool is_power_of_two = (N > 0) && ((N & (N - 1)) == 0);

template<typename T>
concept TableKey = std::integral<T>;

// Values live in a default-constructed in-object array and are moved during
// shifts, so they must be movable and default-initializable.
template<typename T>
concept TableValue = std::movable<T> && std::default_initializable<T>;

// Fixed-capacity open-addressing table: Robin Hood linear probing where the
// ordering invariant itself is the SIMD predicate ("vectorized invariant
// probing").
//
// meta_[i] is one byte: 0 = empty, d+1 = occupied with displacement d from
// its home bucket. Two invariants hold across all operations:
//   (1) home buckets are non-decreasing along any probe run,
//   (2) runs are gap-free (backshift deletion, no tombstones).
// Consequently, for a probe of home bucket h, comparing 16 metadata bytes
// against the ramp {off+1} decides everything in one NEON instruction pair:
//   meta == ramp  <=>  the entry's home bucket is exactly h (must key-compare),
//   meta <  ramp  <=>  no key homed at h can appear at or beyond this offset
//                      (empty slots are the meta==0 case of the same test).
template<TableKey Key, TableValue Value, size_t Capacity>
    requires (Capacity >= 16) && is_power_of_two<Capacity>
class RobinHoodTable {

    static constexpr size_t INDEX_MASK = Capacity - 1;
    // A NEON register scans 16 metadata bytes at a time.
    static constexpr size_t GROUP = 16;
    // Every ramp value (offset+1) must be an exact uint8: the probe window is
    // capped at the largest whole-group count whose ramp stays <= 255.
    static constexpr size_t MAX_WINDOW = (255 / GROUP) * GROUP;
    // A below-event is guaranteed at offset cap+1 (ramp cap+2 exceeds every
    // legal meta), so the displacement cap is window-2; displacement is also
    // bounded by Capacity-1 in any table.
    static constexpr size_t MAX_DISPLACEMENT = MAX_WINDOW - 2 < Capacity - 1
                                                   ? MAX_WINDOW - 2
                                                   : Capacity - 1;
    // Probe window in whole groups; also the length of the wraparound mirror.
    static constexpr size_t WINDOW =
        (MAX_DISPLACEMENT + 2 + GROUP - 1) / GROUP * GROUP;

    struct Slot {
        Key key;
        Value value;
    };

    // meta_ carries a WINDOW-byte mirror of its prefix so NEON group loads
    // never wrap; slots_ is indexed with masked indices and needs no mirror.
    alignas(16) std::array<uint8_t, Capacity + WINDOW> meta_;
    alignas(16) std::array<Slot, Capacity> slots_;
    size_t size_;

    // Sole writer of metadata: updates the base byte and every mirror copy
    // (Capacity < WINDOW means a base index can have several copies).
    void set_meta(size_t i, uint8_t v) noexcept {
        for (size_t j = i; j < Capacity + WINDOW; j += Capacity) {
            meta_[j] = v;
        }
    }

    size_t home_bucket(Key key) const noexcept {
        return __crc32cd(0, static_cast<uint64_t>(key)) & INDEX_MASK;
    }

    // One bit per lane (bit 4*lane) from a NEON byte-comparison result.
    static uint64_t lane_bits(uint8x16_t cmp) noexcept {
        const uint8x8_t nibbles = vshrn_n_u16(vreinterpretq_u16_u8(cmp), 4);
        return vget_lane_u64(vreinterpret_u64_u8(nibbles), 0) &
               0x1111111111111111ULL;
    }

    struct Probe {
        size_t idx;    // slot of the found key, or the insertion point
        uint8_t meta;  // 0 = found at idx; else displacement+1 to store there
    };

    // The single probe primitive behind get/put/erase. (A scalar
    // displacement-0 peek was measured and rejected: +26% at 50% load but
    // -6..8% at 85-90% where the peek branch mispredicts.)
    Probe find_slot(Key key, size_t h) const noexcept {
        alignas(16) static constexpr uint8_t RAMP0[GROUP] = {
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
        const uint8x16_t ramp0 = vld1q_u8(RAMP0);

        for (size_t g = 0; g < WINDOW; g += GROUP) {
            const uint8x16_t meta = vld1q_u8(meta_.data() + h + g);
            const uint8x16_t ramp =
                vaddq_u8(ramp0, vdupq_n_u8(static_cast<uint8_t>(g)));
            uint64_t match = lane_bits(vceqq_u8(meta, ramp));
            const uint64_t below = lane_bits(vcltq_u8(meta, ramp));
            if (below) {
                // Matches at or after the first below-lane belong to later
                // runs (invariant 1) and must be ignored.
                match &= (below & -below) - 1;
            }
            while (match) {
                const size_t idx =
                    (h + g + (static_cast<size_t>(__builtin_ctzll(match)) >> 2)) &
                    INDEX_MASK;
                if (slots_[idx].key == key) {
                    return {idx, 0};
                }
                match &= match - 1;
            }
            if (below) {
                const size_t off =
                    g + (static_cast<size_t>(__builtin_ctzll(below)) >> 2);
                return {(h + off) & INDEX_MASK, static_cast<uint8_t>(off + 1)};
            }
        }
        // Unreachable: every meta is <= MAX_DISPLACEMENT+1 (insert cap), and
        // the ramp exceeds that value within WINDOW.
        __builtin_unreachable();
    }

public:
    RobinHoodTable() : size_(0) { meta_.fill(0); }

    RobinHoodTable(const RobinHoodTable&) = delete;
    RobinHoodTable& operator=(const RobinHoodTable&) = delete;
    RobinHoodTable(RobinHoodTable&&) = delete;
    RobinHoodTable& operator=(RobinHoodTable&&) = delete;

    // After put returns true, key maps to value (insert or update). False
    // means the table could not place the key: it is full, or the insertion
    // would push some displacement past MAX_DISPLACEMENT ("effectively
    // full"); the table is unchanged in that case.
    bool put(Key key, Value value) {
        const Probe p = find_slot(key, home_bucket(key));
        if (p.meta == 0) {
            slots_[p.idx].value = std::move(value);
            return true;
        }
        if (size_ == Capacity || p.meta - 1u > MAX_DISPLACEMENT) {
            return false;
        }

        // Phase 1 (no mutation): find the first empty slot at or after the
        // insertion point; every resident in between will shift one slot,
        // so none may already sit at the displacement cap.
        size_t empty = p.idx;
        while (meta_[empty] != 0) {
            if (meta_[empty] > MAX_DISPLACEMENT) {
                return false;
            }
            empty = (empty + 1) & INDEX_MASK;
        }

        // Phase 2: shift [insertion point, empty) right by one, back to
        // front, then place the new entry.
        for (size_t dst = empty; dst != p.idx;) {
            const size_t src = (dst + Capacity - 1) & INDEX_MASK;
            slots_[dst] = std::move(slots_[src]);
            set_meta(dst, static_cast<uint8_t>(meta_[src] + 1));
            dst = src;
        }
        slots_[p.idx].key = key;
        slots_[p.idx].value = std::move(value);
        set_meta(p.idx, p.meta);
        ++size_;
        return true;
    }

    const Value* get(Key key) const noexcept {
        const Probe p = find_slot(key, home_bucket(key));
        return p.meta == 0 ? &slots_[p.idx].value : nullptr;
    }

    Value* get(Key key) noexcept {
        return const_cast<Value*>(std::as_const(*this).get(key));
    }

    // Backshift deletion: slide displaced successors back one slot until a
    // hole or an at-home entry, preserving both probe invariants.
    bool erase(Key key) noexcept {
        const Probe p = find_slot(key, home_bucket(key));
        if (p.meta != 0) {
            return false;
        }
        size_t hole = p.idx;
        for (size_t next = (hole + 1) & INDEX_MASK; meta_[next] > 1;
             next = (hole + 1) & INDEX_MASK) {
            slots_[hole] = std::move(slots_[next]);
            set_meta(hole, static_cast<uint8_t>(meta_[next] - 1));
            hole = next;
        }
        set_meta(hole, 0);
        --size_;
        return true;
    }

    size_t size() const noexcept { return size_; }
    static constexpr size_t capacity() noexcept { return Capacity; }
};

} // namespace robin_hood

#endif // ROBIN_HOOD_H
