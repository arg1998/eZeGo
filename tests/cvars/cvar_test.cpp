#include <doctest/doctest.h>

#include <atomic>
#include <cstddef>
#include <thread>

#include "cvars/fixtures.hpp"

namespace {

using namespace ez;
using namespace ez::test;

TEST_CASE("cvars: a cvar is constant-initialised with its default") {
    CHECK(cv_watch.value() == true);
    CHECK(cv_drain_ms.value() == 20);
    CHECK(cv_gain.value() == doctest::Approx(0.5f));
    CHECK(cv_big.value() == 5000000000);
    CHECK(cv_backend.value() == Backend::Auto);
    CHECK(cv_dir.value().empty());
    CHECK(cv_drain_ms.generation() == 0);
    CHECK(cv_drain_ms.source() == cvars::Source::Default);
}

TEST_CASE("cvars: the value sits at offset 0 so a read is one load from a fixed address") {
    CHECK(static_cast<const void*>(&cv_drain_ms) == static_cast<const void*>(&cvars::Access::value(cv_drain_ms)));
    CHECK(static_cast<const void*>(&cv_gain) == static_cast<const void*>(&cvars::Access::value(cv_gain)));
    static_assert(sizeof(std::atomic<i32>) == sizeof(i32));
    static_assert(std::atomic<i32>::is_always_lock_free);
    static_assert(std::atomic<f64>::is_always_lock_free);
    static_assert(std::atomic<i64>::is_always_lock_free);
}

TEST_CASE("cvars: metadata is reachable from the object and holds the declaration") {
    const cvars::Meta& m = cv_drain_ms.meta();
    CHECK(std::string_view(m.name) == "cvars.test.drain_ms");
    CHECK(m.type == cvars::Type::I32);
    CHECK(m.default_int == 20);
    CHECK(m.options.min == 1);
    CHECK(m.options.max == 1000);
    CHECK(m.options.tier == cvars::Tier::Advanced);
    CHECK(cvars::has(m.options.flags, cvars::Flags::Persist));
    CHECK(cv_backend.meta().enum_count == 3);
}

TEST_CASE("cvars: readers on other threads see applied values without locks") {
    auto r = make_registry();
    std::atomic<bool> stop{false};
    std::atomic<i64> sum{0};
    std::thread reader([&] {
        while (!stop.load()) {
            sum += cv_drain_ms.value();  // relaxed load; never torn
        }
    });
    for (int i = 1; i <= 200; ++i) {
        r->set("cvars.test.drain_ms", std::to_string(i % 1000 + 1));
        r->apply_pending();
    }
    stop = true;
    reader.join();
    CHECK(cv_drain_ms.generation() == 200);
    CHECK(sum.load() > 0);
}

}  // namespace
