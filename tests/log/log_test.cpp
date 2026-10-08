#include "ez/base/assert.hpp"
#include "support/subprocess.hpp"

#include <doctest/doctest.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>

#include "log/fixture.hpp"

namespace {

using namespace ez;
using namespace ez::test;

TEST_CASE("log: an enabled line reaches the sinks with its metadata") {
    LogSession session({"--log.color=off"});
    REQUIRE(session.start.ok);
    CapturedLog captured;
    log::set_frame(42);
    EZ_LOG_WARN(log, "port %u is busy: %s", 6454u, "in use");
    const auto lines = captured.lines();
    REQUIRE(lines.size() == 1);
    CHECK(lines[0].level == log::Level::Warn);
    CHECK(lines[0].category == log::Category::log);
    CHECK(lines[0].text == "port 6454 is busy: in use");
    CHECK(lines[0].thread == "main");
    CHECK(lines[0].frame == 42);
    CHECK(lines[0].rendered.find("  W  log          main       port 6454 is busy: in use    <log_test.cpp:") !=
          std::string::npos);
}

TEST_CASE("log: lines from several threads arrive in timestamp order with thread names") {
    LogSession session;
    CapturedLog captured;
    std::thread worker([] {
        log::register_thread("worker");
        for (int i = 0; i < 50; ++i) {
            EZ_LOG_INFO(log, "worker %d", i);
        }
    });
    for (int i = 0; i < 50; ++i) {
        EZ_LOG_INFO(log, "main %d", i);
    }
    worker.join();
    const auto lines = captured.lines();
    REQUIRE(lines.size() == 100);
    usize worker_lines = 0;
    for (usize i = 1; i < lines.size(); ++i) {
        CHECK(lines[i - 1].ticks <= lines[i].ticks);
    }
    for (const auto& l : lines) {
        worker_lines += l.thread == "worker" ? 1 : 0;
    }
    CHECK(worker_lines == 50);
}

TEST_CASE("log: a line longer than log.max_line is cut and ends with ...") {
    LogSession session({"--log.max_line=64"});
    CapturedLog captured;
    const std::string long_text(200, 'x');
    EZ_LOG_INFO(log, "%s", long_text.c_str());
    const auto lines = captured.lines();
    REQUIRE(lines.size() == 1);
    CHECK(lines[0].text.size() == 64);
    CHECK(lines[0].text.substr(61) == "...");
}

TEST_CASE("log: the realtime variant is rendered by the log thread as literal = value") {
    LogSession session;
    CapturedLog captured;
    EZ_LOG_RT(log, Warn, "dropout, frames missed", 128);
    const auto lines = captured.lines();
    REQUIRE(lines.size() == 1);
    CHECK(lines[0].text == "dropout, frames missed = 128");
    CHECK(lines[0].level == log::Level::Warn);
}

TEST_CASE("log: flood guards let the first hit or every n-th hit through") {
    LogSession session;
    CapturedLog captured;
    for (int i = 0; i < 10; ++i) {
        EZ_LOG_WARN_ONCE(log, "once");
        EZ_LOG_INFO_EVERY(4, log, "every fourth %d", i);
    }
    usize once = 0;
    usize every = 0;
    for (const auto& l : captured.lines()) {
        once += l.text == "once" ? 1 : 0;
        every += l.text.rfind("every fourth", 0) == 0 ? 1 : 0;
    }
    CHECK(once == 1);
    CHECK(every == 3);  // hits 0, 4, 8
}

TEST_CASE("log: a full ring drops lines, counts them and says so once") {
    LogSession session({"--log.ring_kib=16", "--log.max_line=1024", "--log.drain_ms=1000"});
    CapturedLog captured;
    for (int i = 0; i < 2000; ++i) {
        EZ_LOG_INFO(log, "line %d", i);  // about 48 bytes each: far more than 16 KiB before the next drain
    }
    const auto lines = captured.lines();
    const log::Stats stats = log::stats();
    CHECK(stats.dropped > 0);
    bool reported = false;
    for (const auto& l : lines) {
        reported = reported || l.text.find("dropped on thread main") != std::string::npos;
    }
    CHECK(reported);
}

TEST_CASE("log: arguments are not evaluated when the line is filtered out") {
    LogSession session({"--log=all:error"});
    int evaluations = 0;
    EZ_LOG_INFO(log, "%d", ++evaluations);
    CHECK(evaluations == 0);
    EZ_LOG_ERROR(log, "%d", ++evaluations);
    CHECK(evaluations == 1);
}

TEST_CASE("log: cvars changes are reported through the logger") {
    LogSession session;
    CapturedLog captured;
    session.registry->set("log.drain_ms", "30");
    session.registry->apply_pending();
    CHECK(captured.contains("cvar log.drain_ms: 20 -> 30 (console)"));
}

TEST_CASE("log: the profiler hook sees each line at the call site") {
    LogSession session;
    static std::string g_seen;
    log::set_profiler_hook([](const char* text, usize length, log::Level) { g_seen.assign(text, length); });
    EZ_LOG_INFO(log, "zone %d", 7);
    CHECK(g_seen == "zone 7");  // no flush needed: the hook runs on this thread, now
    log::set_profiler_hook(nullptr);
}

EZ_TEST_SUBPROCESS(log_before_init) {
    EZ_LOG_ERROR(log, "logged before init: %d", 5);
    return 0;
}

TEST_CASE("log: a line before init goes to stderr directly") {
    const ProcessResult r = run_subprocess("log_before_init");
    CHECK(r.succeeded());
    CHECK(r.output.find("  E  log          -          logged before init: 5") != std::string::npos);
}

EZ_TEST_SUBPROCESS(log_fatal) {
    LogSession session({"--log.color=off"});
    EZ_LOG_INFO(log, "the line before");
    EZ_LOG_FATAL(log, "cannot continue: %s", "project missing");
}

TEST_CASE("log: fatal writes everything pending, then its own line, then ends the process") {
    const ProcessResult r = run_subprocess("log_fatal");
    CHECK_FALSE(r.exited_normally());
    const usize before = r.output.find("the line before");
    const usize fatal = r.output.find("  F  log          main       cannot continue: project missing");
    CHECK(before != std::string::npos);
    CHECK(fatal != std::string::npos);
    CHECK(before < fatal);
}

#if EZ_ASSERTS
EZ_TEST_SUBPROCESS(log_assert) {
    LogSession session({"--log.color=off", "--log.drain_ms=1000"});
    EZ_LOG_INFO(log, "queued before the assert");
    EZ_ASSERT_MSG(argc == 100, "argc is never 100");
    return 0;
}

TEST_CASE("log: a failed assert writes the pending lines, then the assertion, through the logger") {
    const ProcessResult r = run_subprocess("log_assert");
    CHECK_FALSE(r.exited_normally());
    const usize before = r.output.find("queued before the assert");
    const usize assertion =
        r.output.find("  F  base         main       assertion failed: argc == 100 (argc is never 100)");
    CHECK(before != std::string::npos);
    CHECK(assertion != std::string::npos);
    CHECK(before < assertion);
}
#endif

EZ_TEST_SUBPROCESS(log_sync) {
    LogSession session({"--log.sync=on", "--log.color=off", "--log.drain_ms=1000"});
    EZ_LOG_INFO(log, "immediately");
    std::fflush(stderr);
    _Exit(0);  // no shutdown, no drain: only the synchronous write can have printed the line
}

TEST_CASE("log: log.sync writes each line on the calling thread") {
    const ProcessResult r = run_subprocess("log_sync");
    CHECK(r.output.find("immediately") != std::string::npos);
}

}  // namespace
