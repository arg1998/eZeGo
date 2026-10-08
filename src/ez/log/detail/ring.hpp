// The per-thread log ring (specs/logging.md §4.2, §4.3): one producer, one consumer, variable-length
// records in a byte buffer. The producer never blocks, locks or allocates: it reserves bytes, writes
// the record and publishes it with one release store. A full ring drops the record and counts it.
#pragma once

#include "ez/base/detect.h"
#include "ez/base/fixed_string.hpp"
#include "ez/base/types.hpp"

#include <atomic>
#include <cstring>

namespace ez::log::detail {

enum class RecordKind : u8 {
    Text,  // payload: the formatted line
    Rt,    // payload: RtPayload (a literal and one value), formatted by the log thread
    Skip,  // padding to the end of the buffer; the next record starts at offset 0
};

// The record contract (LG-8). Fields are fixed; the encoding of the payload may change.
struct RecordHeader {
    u16 size;    // header + payload, rounded up to 8 bytes
    u16 length;  // payload bytes in use
    u8 level;
    u8 category;
    RecordKind kind;
    u8 truncated;  // the text was cut at log.max_line
    u64 ticks;     // reference clock, nanoseconds
    u32 frame;     // engine frame counter at emit time
    u32 line;
    const char* file;  // __FILE__, static storage
};
static_assert(sizeof(RecordHeader) == 32);

struct RtPayload {
    const char* literal;
    u64 value;
};

constexpr usize record_align = 8;
constexpr usize align_up(usize n) noexcept {
    return (n + record_align - 1) & ~(record_align - 1);
}

class Ring {
public:
    // `capacity` must be a power of two and at least 1 KiB. Memory is owned by the caller.
    void attach(u8* memory, usize capacity) noexcept {
        data_ = memory;
        capacity_ = capacity;
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
        cached_tail_ = 0;
        dropped_.store(0, std::memory_order_relaxed);
    }

    // ---------------------------------------------------------------- producer
    // Reserves room for a record of up to `max_size` bytes (header included). Returns nullptr when
    // the ring is full; the caller counts the drop. The record is invisible until commit().
    [[nodiscard]] RecordHeader* reserve(usize max_size) noexcept {
        const usize need = align_up(max_size);
        if (need > capacity_ / 2) {
            return nullptr;
        }
        const u64 head = head_.load(std::memory_order_relaxed);
        const usize offset = usize(head & (capacity_ - 1));
        const usize to_end = capacity_ - offset;
        const usize pad = to_end < need ? to_end : 0;  // wrap: skip the tail end of the buffer
        if (!has_room(head, pad + need)) {
            return nullptr;
        }
        if (pad != 0) {
            // A Skip header fits whenever to_end >= 8, which alignment guarantees; the consumer
            // reads only `size` and `kind` from it.
            auto* skip = reinterpret_cast<RecordHeader*>(data_ + offset);
            skip->size = u16(pad > 0xFFFF ? 0 : pad);
            skip->kind = RecordKind::Skip;
            pending_pad_ = pad;
            return reinterpret_cast<RecordHeader*>(data_);
        }
        pending_pad_ = 0;
        return reinterpret_cast<RecordHeader*>(data_ + offset);
    }

    // Publishes the record returned by reserve(); `record->size` must be set and <= what was reserved.
    void commit(const RecordHeader* record) noexcept {
        const u64 head = head_.load(std::memory_order_relaxed);
        head_.store(head + pending_pad_ + record->size, std::memory_order_release);
    }

    void count_drop() noexcept { dropped_.fetch_add(1, std::memory_order_relaxed); }

    // ---------------------------------------------------------------- consumer
    // Calls fn(const RecordHeader&) for every published record, then frees their space.
    template <class Fn>
    usize drain(Fn&& fn) noexcept {
        u64 tail = tail_.load(std::memory_order_relaxed);
        const u64 head = head_.load(std::memory_order_acquire);
        usize n = 0;
        while (tail < head) {
            const usize offset = usize(tail & (capacity_ - 1));
            const auto* record = reinterpret_cast<const RecordHeader*>(data_ + offset);
            if (record->kind == RecordKind::Skip) {
                tail += capacity_ - offset;
                continue;
            }
            fn(*record);
            tail += record->size;
            ++n;
        }
        tail_.store(tail, std::memory_order_release);
        return n;
    }

    [[nodiscard]] bool empty() const noexcept {
        return tail_.load(std::memory_order_acquire) == head_.load(std::memory_order_acquire);
    }
    [[nodiscard]] u64 dropped() const noexcept { return dropped_.load(std::memory_order_relaxed); }
    [[nodiscard]] usize capacity() const noexcept { return capacity_; }

private:
    bool has_room(u64 head, usize bytes) noexcept {
        if (head + bytes - cached_tail_ <= capacity_) {
            return true;
        }
        cached_tail_ = tail_.load(std::memory_order_acquire);  // refresh only when it looks full
        return head + bytes - cached_tail_ <= capacity_;
    }

    // Producer line.
    alignas(EZ_CACHE_LINE) std::atomic<u64> head_{0};
    u64 cached_tail_ = 0;
    usize pending_pad_ = 0;
    u8* data_ = nullptr;
    usize capacity_ = 0;
    std::atomic<u64> dropped_{0};
    // Consumer line.
    alignas(EZ_CACHE_LINE) std::atomic<u64> tail_{0};
};

}  // namespace ez::log::detail
