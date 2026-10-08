#include <doctest/doctest.h>

#include "log/fixture.hpp"

namespace {

using namespace ez;
using namespace ez::test;
using log::Category;
using log::Level;

TEST_CASE("log: the runtime default is the build's floor") {
    LogSession session;
    const Level floor = Level(EZ_LOG_LEVEL);
    CHECK(log::enabled(Category::log, floor));
    if (floor > Level::Trace) {
        CHECK_FALSE(log::enabled(Category::log, Level(u8(floor) - 1)));
    }
}

TEST_CASE("log: --log sets every category, then specific ones; later entries win") {
    LogSession session({"--log=all:error,cvars:debug"});
    REQUIRE(session.start.ok);
    CHECK_FALSE(log::enabled(Category::log, Level::Warn));
    CHECK(log::enabled(Category::log, Level::Error));
    CHECK(log::enabled(Category::cvars, Level::Debug));
    CHECK_FALSE(log::enabled(Category::cvars, Level::Trace));
    CHECK_FALSE(log::enabled(Category::base, Level::Warn));
}

TEST_CASE("log: EZ_LOG in the environment works like --log") {
    LogSession session({}, {"EZ_LOG=all:warn"});
    REQUIRE(session.start.ok);
    CHECK_FALSE(log::enabled(Category::base, Level::Info));
    CHECK(log::enabled(Category::base, Level::Warn));
}

TEST_CASE("log: a bad --log value fails startup with a readable message") {
    LogSession session({"--log=all:loud,nosuch:debug,oops"});
    CHECK_FALSE(session.start.ok);
    const std::string_view report = session.start.report.view();
    CHECK(report.find("log.level.all: 'loud' is not accepted") != std::string_view::npos);
    CHECK(report.find("log: unknown category 'nosuch'") != std::string_view::npos);
    CHECK(report.find("log: 'oops' is not category:level") != std::string_view::npos);
}

TEST_CASE("log: a level changed at runtime takes effect within one drain") {
    LogSession session({"--log=all:info"});
    CHECK_FALSE(log::enabled(Category::cvars, Level::Debug));
    session.registry->set("log.level.cvars", "debug");
    session.registry->apply_pending();
    log::flush();  // the log thread notices the change on its next pass
    CHECK(log::enabled(Category::cvars, Level::Debug));
    CHECK_FALSE(log::enabled(Category::log, Level::Debug));
    session.registry->set("log.level.cvars", "inherit");
    session.registry->apply_pending();
    log::flush();
    CHECK_FALSE(log::enabled(Category::cvars, Level::Debug));
}

TEST_CASE("log: off silences a category") {
    LogSession session({"--log=all:off"});
    CHECK_FALSE(log::enabled(Category::log, Level::Error));
}

}  // namespace
