// Build-time feature switches (specs/base.md §2, observability.md §6).
// cmake/modes.cmake derives them from the preset and passes them on the command line; code tests
// features, never the mode name. The defaults below only apply to a file compiled outside the
// CMake build, so every macro is always defined and -Wundef stays quiet.
#pragma once

#ifndef EZ_ASSERTS
    #define EZ_ASSERTS 1
#endif
#ifndef EZ_PROFILER
    #define EZ_PROFILER 0
#endif
#ifndef EZ_MEM_TRACE
    #define EZ_MEM_TRACE 0
#endif
#ifndef EZ_METRICS
    #define EZ_METRICS 1
#endif
#ifndef EZ_LOG_LEVEL
    #define EZ_LOG_LEVEL 0  // 0 trace, 1 debug, 2 info, 3 warn, 4 error, 5 fatal
#endif
#ifndef EZ_VERSION_STR
    #define EZ_VERSION_STR "0.0.0"
#endif
#ifndef EZ_MODE_NAME
    #define EZ_MODE_NAME "unknown"
#endif

namespace ez {

inline constexpr const char* build_version = EZ_VERSION_STR;
inline constexpr const char* build_mode = EZ_MODE_NAME;

}  // namespace ez
