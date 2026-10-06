#pragma once


#include "core/definitions.hpp"

#include <cstring>   // strrchr for __FILENAME__

enum LogLevel {
    EZ_LOG_LEVEL_FATAL  =   0, // => EZ_LOG_FATAL() : when a fatal error has occurred and the application must shutdown
    EZ_LOG_LEVEL_ERROR  =   1, // => EZ_LOG_ERROR() : when an error has occurred but the application can recover and continue running
    EZ_LOG_LEVEL_WARN   =   2, // => EZ_LOG_WARN()  : when application encounters unexpected environments and resources
    EZ_LOG_LEVEL_INFO   =   3, // => EZ_LOG_INFO()  : for logging state, environment, context, etc. information
    EZ_LOG_LEVEL_DEBUG  =   4, // => EZ_LOG_DEBUG() : Used for print style debugging
    EZ_LOG_LEVEL_TRACE  =   5  // => EZ_LOG_TRACE() : Used at the beginning of function definition to trace function calls.
};


b8 initLoggingSystem();
void shutdownLoggingSystem();
// `message` is a printf-style format string (not a std::string: no allocation per log call).
void logOutout(LogLevel log_level, const char* message, const char *_file, s32 _line, ...);


#ifdef EZ_PLATFORM_WINDOWS
    #if EZ_CONFIG_LOG_FILE_ABSOLUTE_PATH == true
        #define __FILENAME__ __FILE__
    #else
        #define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
    #endif

#else
    #if EZ_CONFIG_LOG_FILE_ABSOLUTE_PATH == true
        #define __FILENAME__ __FILE__
    #else
        #define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
    #endif
#endif


// FATAL and ERROR are always compiled in.
#ifndef EZ_LOG_FATAL
    // TODO(Argosta): For Fatal logs, application must break. implement mechanisms for breaking, crash reporting, and log dumping
    #define EZ_LOG_FATAL(msg, ...)  do { logOutout(EZ_LOG_LEVEL_FATAL, msg, __FILENAME__, __LINE__, ##__VA_ARGS__); EZ_DEBUG_BREAK(); } while (0)
#endif

#ifndef EZ_LOG_ERROR
    #define EZ_LOG_ERROR(msg, ...)  logOutout(EZ_LOG_LEVEL_ERROR, msg, __FILENAME__, __LINE__, ##__VA_ARGS__)
#endif

// The rest compile out below the build's EZ_LOG_LEVEL (CMake: 0 trace, 1 debug, 2 info, 3 warn).
// debug preset: everything; profile: info and up; release: warn and up.
#if EZ_CONFIG_LOGGING_ENBABLED && EZ_LOG_LEVEL <= 3
    #define EZ_LOG_WARN(msg, ...)   logOutout(EZ_LOG_LEVEL_WARN, msg, __FILENAME__, __LINE__, ##__VA_ARGS__)
#else
    #define EZ_LOG_WARN(msg, ...)   do {} while (0)
#endif

#if EZ_CONFIG_LOGGING_ENBABLED && EZ_LOG_LEVEL <= 2
    #define EZ_LOG_INFO(msg, ...)   logOutout(EZ_LOG_LEVEL_INFO, msg, __FILENAME__, __LINE__, ##__VA_ARGS__)
#else
    #define EZ_LOG_INFO(msg, ...)   do {} while (0)
#endif

#if EZ_CONFIG_LOGGING_ENBABLED && EZ_LOG_LEVEL <= 1
    #define EZ_LOG_DEBUG(msg, ...)  logOutout(EZ_LOG_LEVEL_DEBUG, msg, __FILENAME__, __LINE__, ##__VA_ARGS__)
#else
    #define EZ_LOG_DEBUG(msg, ...)  do {} while (0)
#endif

#if EZ_CONFIG_LOGGING_ENBABLED && EZ_LOG_LEVEL <= 0
    #define EZ_LOG_TRACE()          logOutout(EZ_LOG_LEVEL_TRACE, __FUNCTION__, __FILENAME__, __LINE__)
#else
    #define EZ_LOG_TRACE()          do {} while (0)
#endif
