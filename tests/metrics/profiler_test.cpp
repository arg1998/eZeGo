#include "ez/metrics/profiler.hpp"

#include <doctest/doctest.h>

namespace {

using namespace ez;

TEST_CASE("metrics: the profiler macros compile in every build and do nothing without the profiler") {
    EZ_PROF_FUNCTION();
    {
        EZ_PROF_ZONE("zone");
        EZ_PROF_PLOT("value", 1.5);
    }
    EZ_PROF_FRAME();
    CHECK(true);
}

TEST_CASE("metrics: the profiler is not running until started") {
    CHECK_FALSE(metrics::profiler_running());
    CHECK_FALSE(metrics::profiler_connected());
#if !EZ_PROFILER
    metrics::start_profiler();  // a no-op outside profile builds
    CHECK_FALSE(metrics::profiler_running());
#endif
}

}  // namespace
