// The instrumentation facade, profiler half (specs/observability.md O-1, O-4). The only first-party
// header that includes Tracy, and only in profile builds: elsewhere every macro compiles to nothing
// and Tracy is not on the include path, so nothing can use it by accident.
//
//   EZ_PROF_ZONE("Render Frame");   // a named zone until the end of the scope
//   EZ_PROF_FUNCTION();             // a zone named after the function
//   EZ_PROF_FRAME();                // the display frame boundary
//   EZ_PROF_FRAME_START("audio"); ... EZ_PROF_FRAME_END("audio");   // a named frame series
//   EZ_PROF_PLOT("frame ms", value);
//
// The always-on metrics half (EZ_METRIC_*, O-2) is designed but not implemented yet.
#pragma once

#include "ez/base/build.hpp"
#include "ez/base/macros.hpp"
#include "ez/base/types.hpp"

#if EZ_PROFILER
    #include <tracy/Tracy.hpp>
    // Zones and frame marks are inert outside the profiler's lifetime (manual lifetime: touching
    // Tracy before start_profiler() would crash), so instrumented code may run at any time.
    #define EZ_PROF_ZONE(name) ZoneNamedN(EZ_CONCAT(ez_prof_zone_, __LINE__), name, ::ez::metrics::profiler_running())
    #define EZ_PROF_FUNCTION() ZoneNamed(EZ_CONCAT(ez_prof_zone_, __LINE__), ::ez::metrics::profiler_running())
    #define EZ_PROF_FRAME()                          \
        do {                                         \
            if (::ez::metrics::profiler_running()) { \
                FrameMark;                           \
            }                                        \
        } while (0)
    #define EZ_PROF_FRAME_START(name)                \
        do {                                         \
            if (::ez::metrics::profiler_running()) { \
                FrameMarkStart(name);                \
            }                                        \
        } while (0)
    #define EZ_PROF_FRAME_END(name)                  \
        do {                                         \
            if (::ez::metrics::profiler_running()) { \
                FrameMarkEnd(name);                  \
            }                                        \
        } while (0)
    #define EZ_PROF_PLOT(name, value)                \
        do {                                         \
            if (::ez::metrics::profiler_running()) { \
                TracyPlot(name, value);              \
            }                                        \
        } while (0)
#else
    #define EZ_PROF_ZONE(name) \
        do {                   \
        } while (0)
    #define EZ_PROF_FUNCTION() \
        do {                   \
        } while (0)
    #define EZ_PROF_FRAME() \
        do {                \
        } while (0)
    #define EZ_PROF_FRAME_START(name) \
        do {                          \
        } while (0)
    #define EZ_PROF_FRAME_END(name) \
        do {                        \
        } while (0)
    #define EZ_PROF_PLOT(name, value) \
        do {                          \
            (void)(value);            \
        } while (0)
#endif

namespace ez::metrics {

// Manual lifetime (observability.md §3): start after the memory system and before the logger and
// any thread that registers with the profiler; stop in reverse. Starting also routes every log
// line to the profiler as a message coloured by level. No-ops outside profile builds.
void start_profiler() noexcept;
void stop_profiler() noexcept;
[[nodiscard]] bool profiler_running() noexcept;
[[nodiscard]] bool profiler_connected() noexcept;  // a viewer is attached (on-demand mode)

// All no-ops unless the profiler is compiled in and running.
void profiler_thread_name(const char* name) noexcept;
void profiler_alloc(const void* ptr, usize size, const char* pool) noexcept;  // pool: a static string
void profiler_free(const void* ptr, const char* pool) noexcept;

}  // namespace ez::metrics
