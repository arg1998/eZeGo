#pragma once

/*
    This file contains all the compile configurations and flags.

    Build-mode switches come from CMake (cmake/modes.cmake) as feature macros:
        EZ_ASSERTS, EZ_PROFILER, EZ_MEM_TRACE, EZ_METRICS, EZ_LOG_LEVEL, EZ_MODE_NAME, EZ_VERSION_STR
    The defaults below only apply when a file is compiled outside the CMake build (e.g. a quick
    experiment); inside the build every value is set by the preset.
*/

#ifndef EZ_VERSION_STR
    #define EZ_VERSION_STR "0.0.0"
#endif
#ifndef EZ_MODE_NAME
    #define EZ_MODE_NAME "unknown"
#endif
#ifndef EZ_ASSERTS
    #define EZ_ASSERTS 1
#endif
#ifndef EZ_PROFILER
    #define EZ_PROFILER 0
#endif
#ifndef EZ_MEM_TRACE
    #define EZ_MEM_TRACE 0
#endif
#ifndef EZ_LOG_LEVEL
    #define EZ_LOG_LEVEL 0   // 0 trace, 1 debug, 2 info, 3 warn, 4 error
#endif

// Kept for existing code: derived from the build, not edited by hand.
#define EZ_CONFIG_ASSERTION_ENABLED         (EZ_ASSERTS != 0)
#define EZ_CONFIG_LOGGING_ENBABLED          true

// define the size of the buffer that is used to extract variadic arguments
// this size represent the number of ASCII characters
#define EZ_CONFIG_LOG_BUFFER_SIZE           (2048 * 10)

// whether to spwan a separate console/terminal while running the GUI application
#define EZ_CONFIG_CONSOLE_ENABLED           false //TODO(Argosta): implement me

// whether the log system should output filename only or file name + absolute path
#define EZ_CONFIG_LOG_FILE_ABSOLUTE_PATH    false
