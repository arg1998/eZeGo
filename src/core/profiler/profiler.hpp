#pragma once
// Instrumentation facade, profiler half (specs/observability.md O-1, O-4).
// This is the ONLY first-party header that includes Tracy. In non-profile builds every macro
// compiles to nothing and Tracy is not even on the include path.

#include "core/definitions.hpp"

#if EZ_PROFILER
    #include <tracy/Tracy.hpp>
    #define EZ_PROFILE_ZONE(name)           ZoneScopedN(name)
    #define EZ_PROFILE_FUNCTION()           ZoneScoped
    #define EZ_PROFILE_FRAME()              FrameMark
    #define EZ_PROFILE_FRAME_START(name)    FrameMarkStart(name)
    #define EZ_PROFILE_FRAME_END(name)      FrameMarkEnd(name)
    #define EZ_PROFILE_PLOT(name, value)    do { if (profilerIsAlive()) TracyPlot(name, value); } while (0)
#else
    #define EZ_PROFILE_ZONE(name)           do {} while (0)
    #define EZ_PROFILE_FUNCTION()           do {} while (0)
    #define EZ_PROFILE_FRAME()              do {} while (0)
    #define EZ_PROFILE_FRAME_START(name)    do {} while (0)
    #define EZ_PROFILE_FRAME_END(name)      do {} while (0)
    #define EZ_PROFILE_PLOT(name, value)    do { (void)(value); } while (0)
#endif

// Manual lifetime. Order matters: memory -> profiler -> logger -> everything else (and the
// reverse on shutdown). The profiler must be alive before any thread registers itself with it.
void startProfiler();
void stopProfiler();
b8 profilerIsAlive();
b8 profilerIsConnected();   // a Tracy viewer is attached (on-demand mode)

// All no-ops unless the profiler is compiled in AND alive.
void profilerSetThreadName(const char* name);
void profilerMessage(const char* text, u64 length, u32 rgb);
void profilerAlloc(const void* ptr, u64 size, const char* pool);   // pool: a static string
void profilerFree(const void* ptr, const char* pool);
