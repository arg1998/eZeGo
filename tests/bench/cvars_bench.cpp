// The costs specs/cvars.md §2 claims.
#include "ez/cvars/registry.hpp"
#include "support/bench.hpp"

#include <memory>

namespace {

using namespace ez;

EZ_CVAR_I32(cv_bench_value, "cvars.bench.value", 1, {.min = 0, .max = 1000000, .help = "A benchmark value."});

cvars::Registry& bench_registry() {
    static std::unique_ptr<cvars::Registry> g_registry;
    if (!g_registry) {
        g_registry = std::make_unique<cvars::Registry>();
        g_registry->add(cv_bench_value);
    }
    return *g_registry;
}

}  // namespace

EZ_BENCH("cvars: read a scalar (the hot path)") {
    i64 sum = 0;
    while (state.next()) {
        sum += cv_bench_value.value();
        test::do_not_optimize(sum);
    }
}

EZ_BENCH("cvars: read the generation") {
    u64 sum = 0;
    while (state.next()) {
        sum += cv_bench_value.generation();
        test::do_not_optimize(sum);
    }
}

EZ_BENCH("cvars: set and apply one value (cold path)") {
    cvars::Registry& r = bench_registry();
    i32 v = 0;
    while (state.next()) {
        r.set("cvars.bench.value", (++v & 1) != 0 ? "5" : "6");
        r.apply_pending();
    }
}
