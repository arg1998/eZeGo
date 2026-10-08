// The costs specs/logging.md §7 claims, measured on the calling thread.
#include "ez/cvars/registry.hpp"
#include "ez/log/log.hpp"
#include "support/bench.hpp"

#include <cstdio>
#include <memory>

namespace {

using namespace ez;

// The logger, started once for every benchmark here: a large ring and a fast drain so the producer
// measures the emit path rather than the drop path, and no console output.
void ensure_logger() {
    static std::unique_ptr<cvars::Registry> g_registry;
    if (g_registry) {
        return;
    }
    g_registry = std::make_unique<cvars::Registry>();
    log::register_cvars(*g_registry);
    char a0[] = "bench";
    char a1[] = "--log=all:info,cvars:warn";
    char a2[] = "--log.ring_kib=16384";
    char a3[] = "--log.drain_ms=1";
    char a4[] = "--log.stderr=off";
    char a5[] = "--log.history_kib=0";
    char* argv[] = {a0, a1, a2, a3, a4, a5, nullptr};
    const char* env[] = {nullptr};
    g_registry->init({.argc = 6, .argv = argv, .envp = env, .settings_path = ""});
    log::init(*g_registry);
}

void report_drops(u64 before) {
    const u64 dropped = log::stats().dropped - before;
    if (dropped > 0) {
        std::printf("  (%llu lines dropped during this benchmark: the ring filled; numbers include the drop path)\n",
                    static_cast<unsigned long long>(dropped));
    }
}

}  // namespace

EZ_BENCH("log: filtered line (category below its runtime level)") {
    ensure_logger();
    int i = 0;
    while (state.next()) {
        EZ_LOG_INFO(cvars, "filtered %d", ++i);  // cvars is at warn
    }
    test::do_not_optimize(i);
}

EZ_BENCH("log: enabled line, one integer") {
    ensure_logger();
    const u64 before = log::stats().dropped;
    int i = 0;
    while (state.next()) {
        EZ_LOG_INFO(log, "value %d", ++i);
    }
    log::flush();  // not timed: next() returned false before this
    report_drops(before);
}

EZ_BENCH("log: enabled line, integer + float + string") {
    ensure_logger();
    const u64 before = log::stats().dropped;
    int i = 0;
    while (state.next()) {
        EZ_LOG_INFO(log, "frame %d late by %.2f ms on %s", ++i, 2.31, "output");
    }
    log::flush();
    report_drops(before);
}

EZ_BENCH("log: realtime variant") {
    ensure_logger();
    const u64 before = log::stats().dropped;
    u64 i = 0;
    while (state.next()) {
        EZ_LOG_RT(log, Info, "frames missed", ++i);
    }
    log::flush();
    report_drops(before);
}

EZ_BENCH("log: flood-guarded hit, suppressed (EVERY 1000000)") {
    ensure_logger();
    while (state.next()) {
        EZ_LOG_INFO_EVERY(1000000, log, "rarely");
    }
    log::flush();
}
