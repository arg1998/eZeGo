#include "logger.hpp"
#include "core/assertion/assertions.hpp"
#include "core/platform/platform.hpp"

#include <cstdarg>
#include <tracy/Tracy.hpp>


b8 initLoggingSystem() {
    return true;
}

void shutdownLoggingSystem() {}


void logOutout(LogLevel log_level, const std::string& message, const char *_file, s32 _line, ...) {
    const char *level_strings[6] = {"FATAL", "ERROR", "WARN ", "INFO ", "DEBUG", "TRACE"};
    
    char messageBufffer[EZ_CONFIG_LOG_BUFFER_SIZE] = {};
    if(log_level == EZ_LOG_LEVEL_TRACE){
        sprintf(messageBufffer, "[%s]: %s() <%s:%d>\n", level_strings[log_level], message.c_str(), _file, _line);
    } else {
        va_list args {};
        va_start(args, _line);
        char variaticArgsBuffer[EZ_CONFIG_LOG_BUFFER_SIZE] = {};
        vsnprintf(variaticArgsBuffer, sizeof(variaticArgsBuffer), message.c_str(), args);
        va_end(args);
        sprintf(messageBufffer, "[%s]: %s <%s:%d>\n", level_strings[log_level], variaticArgsBuffer, _file, _line);
    }

    // Send all logging data to Tracy Profiler if enabled
    #ifdef TRACY_ENABLE
        static const u32 TRACY_COLORS[6] = {
            0xFF0000, // FATAL      (Red) 
            0xFF4D00, // Error      (Orange)
            0xFFFF00, // Warning    (Yellow)
            0x88E788, // Info       (Green),
            0x90D5FF, // Debug      (Blue),
            0xACADA5  // Trace      (Gray)
        };
        TracyMessageC(messageBufffer, strlen(messageBufffer), TRACY_COLORS[log_level]);
    #else
        // Otherwise print log messages using the platform standard output
        platformWriteConsoleOutput(messageBufffer, log_level);
    #endif

}

void report_assertion_failure(const char *expression, const char* message, const char *file, s32 line){
    if(message[0] == '\0'){
        logOutout(EZ_LOG_LEVEL_FATAL, "ASSERTION FAILURE  --> Expression: %s --> File:", file, line, expression);
    } else {
        logOutout(EZ_LOG_LEVEL_FATAL, "ASSERTION FAILURE  -->  Expression: %s  -->  Message: %s  -->  File:", file, line, expression, message);
    }
    
}

