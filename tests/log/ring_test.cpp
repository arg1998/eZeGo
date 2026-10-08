#include "ez/log/detail/ring.hpp"

#include <doctest/doctest.h>

#include <cstring>
#include <thread>
#include <vector>

namespace {

using namespace ez;
using namespace ez::log::detail;

struct RingFixture {
    alignas(64) u8 memory[4096];
    Ring ring;
    RingFixture() { ring.attach(memory, sizeof(memory)); }

    bool push(u32 value, usize payload = 8) {
        RecordHeader* h = ring.reserve(sizeof(RecordHeader) + payload);
        if (h == nullptr) {
            ring.count_drop();
            return false;
        }
        *h = RecordHeader{};
        h->kind = RecordKind::Text;
        h->line = value;
        h->length = u16(payload);
        h->size = u16(align_up(sizeof(RecordHeader) + payload));
        ring.commit(h);
        return true;
    }
};

TEST_CASE("log: a ring delivers records in order and frees their space") {
    RingFixture f;
    for (u32 i = 0; i < 10; ++i) {
        REQUIRE(f.push(i));
    }
    std::vector<u32> seen;
    CHECK(f.ring.drain([&](const RecordHeader& h) { seen.push_back(h.line); }) == 10);
    REQUIRE(seen.size() == 10);
    for (u32 i = 0; i < 10; ++i) {
        CHECK(seen[i] == i);
    }
    CHECK(f.ring.empty());
}

TEST_CASE("log: a full ring drops the line and counts it") {
    RingFixture f;
    u32 accepted = 0;
    while (f.push(accepted, 200)) {
        ++accepted;
    }
    CHECK(accepted >= 15);  // 4096 bytes / 232-byte records
    CHECK(f.ring.dropped() == 1);
    f.ring.drain([](const RecordHeader&) {});
    CHECK(f.push(99, 200));  // space is back after the drain
}

TEST_CASE("log: records wrap around the end of the buffer intact") {
    RingFixture f;
    u32 next = 0;
    u32 expected = 0;
    for (int round = 0; round < 50; ++round) {
        for (int i = 0; i < 7; ++i) {
            REQUIRE(f.push(next++, 100 + usize(round % 5) * 24));
        }
        f.ring.drain([&](const RecordHeader& h) {
            CHECK(h.line == expected);
            ++expected;
        });
    }
    CHECK(expected == next);
}

TEST_CASE("log: one producer and one consumer on different threads lose nothing") {
    static RingFixture g_fixture;  // large: not on the stack of two threads
    RingFixture& f = g_fixture;
    constexpr u32 total = 200000;
    std::thread producer([] {
        for (u32 i = 0; i < total;) {
            if (g_fixture.push(i, 16 + (i % 7) * 8)) {
                ++i;  // retry when full: this test checks ordering, not dropping
            }
        }
    });
    u32 expected = 0;
    bool in_order = true;
    while (expected < total) {
        f.ring.drain([&](const RecordHeader& h) {
            in_order = in_order && h.line == expected;
            ++expected;
        });
    }
    producer.join();
    CHECK(in_order);
    CHECK(expected == total);
}

}  // namespace
