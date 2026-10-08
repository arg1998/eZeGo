#include <doctest/doctest.h>

#include <string>

#include "cvars/fixtures.hpp"

namespace {

using namespace ez;
using namespace ez::test;

std::string run(cvars::Registry& r, std::string_view line) {
    std::string out;
    r.execute(line, string_writer(out));
    return out;
}

TEST_CASE("cvars: the console describes a cvar") {
    auto r = make_registry();
    const std::string out = run(*r, "cvars.test.drain_ms");
    CHECK(out.find("cvars.test.drain_ms = 20    (default)") != std::string::npos);
    CHECK(out.find("i32; live; advanced; accepts 1..1000") != std::string::npos);
    CHECK(out.find("Drain period in milliseconds.") != std::string::npos);
    CHECK(run(*r, "cvars.test.drain_ms ?").find("former names: cvars.test.old_drain") != std::string::npos);
}

TEST_CASE("cvars: the console sets with validation and shows rejections inline") {
    auto r = make_registry();
    CHECK(run(*r, "cvars.test.drain_ms 64") == "cvars.test.drain_ms will be 64 at the next frame\n");
    CHECK(run(*r, "cvars.test.drain_ms 0").find("expected 1..1000") != std::string::npos);
    r->apply_pending();
    CHECK(cv_drain_ms.value() == 64);
}

TEST_CASE("cvars: the console finds by name or help text, case-insensitively") {
    auto r = make_registry();
    const std::string out = run(*r, "find MILLISECONDS");
    CHECK(out.find("cvars.test.drain_ms = 20") != std::string::npos);
    CHECK(out.find("cvars.test.gain") == std::string::npos);
    CHECK(run(*r, "find zzz") == "no setting matches\n");
}

TEST_CASE("cvars: dump lists only values that differ from their defaults, with the source") {
    auto r = make_registry();
    CHECK(run(*r, "dump").empty());
    run(*r, "cvars.test.gain 1.5");
    r->apply_pending();
    CHECK(run(*r, "dump") == "cvars.test.gain = 1.5    # console\n");
}

TEST_CASE("cvars: reset from the console") {
    auto r = make_registry();
    run(*r, "cvars.test.gain 1.5");
    r->apply_pending();
    run(*r, "reset all");
    r->apply_pending();
    CHECK(cv_gain.value() == doctest::Approx(0.5f));
    CHECK(run(*r, "nope").find("unknown setting 'nope'") != std::string::npos);
}

}  // namespace
