#pragma once


#include "core/definitions.hpp"

#include <string>

enum LogLevel {
    EZ_LOG_LEVEL_FATAL  =   0,
    EZ_LOG_LEVEL_ERROR  =   1,
    EZ_LOG_LEVEL_WARN   =   2,
    EZ_LOG_LEVEL_INFO   =   3,
    EZ_LOG_LEVEL_DEBUG  =   4,
    EZ_LOG_LEVEL_TRACE  =   5
};


b8 initLoggingSystem();
void shutdownLoggingSystem(); 
void logOutout(LogLevel log_level, const std::string& message, const char *_file, s32 _line, ...);


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


#ifndef EZ_LOG_FATAL
    #define EZ_LOG_FATAL(msg, ...)  logOutout(EZ_LOG_LEVEL_FATAL, msg, __FILENAME__, __LINE__, ##__VA_ARGS__)
#endif

#ifndef EZ_LOG_ERROR
    #define EZ_LOG_ERROR(msg, ...)  logOutout(EZ_LOG_LEVEL_ERROR, msg, __FILENAME__, __LINE__, ##__VA_ARGS__)
#endif


#if defined(EZ_CONFIG_LOGGING_ENBABLED) && (EZ_CONFIG_LOGGING_ENBABLED)

    #define EZ_LOG_WARN(msg, ...)   logOutout(EZ_LOG_LEVEL_WARN, msg, __FILENAME__, __LINE__, ##__VA_ARGS__)
    #define EZ_LOG_INFO(msg, ...)   logOutout(EZ_LOG_LEVEL_INFO, msg, __FILENAME__, __LINE__, ##__VA_ARGS__)


    #ifdef EZ_DEBUG_BUILD
        #define EZ_LOG_DEBUG(msg, ...)  logOutout(EZ_LOG_LEVEL_DEBUG, msg, __FILENAME__, __LINE__, ##__VA_ARGS__)
        #define EZ_LOG_TRACE()          logOutout(EZ_LOG_LEVEL_TRACE, __FUNCTION__, __FILENAME__, __LINE__)
    #else
        #define EZ_LOG_DEBUG(msg, ...)
        #define EZ_LOG_TRACE(msg, ...)
    #endif

#else
    // Disable all auxilary logs 
    #define EZ_LOG_WARN(msg, ...)
    #define EZ_LOG_INFO(msg, ...)
    #define EZ_LOG_DEBUG(msg, ...)
    #define EZ_LOG_TRACE(msg, ...)
#endif 
