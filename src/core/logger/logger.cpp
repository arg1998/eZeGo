#include "logger.hpp"
#include "core/assertion/assertions.hpp"
#include "core/platform/platform.hpp"
#include "core/profiler/profiler.hpp"

#include <cstdarg>
#include <cstdio>


b8 initLoggingSystem() {
    return true;
}

void shutdownLoggingSystem() {}


void logOutout(LogLevel log_level, const char* message, const char *_file, s32 _line, ...) {
    static const char *level_strings[6] = {"FATAL", "ERROR", "WARN ", "INFO ", "DEBUG", "TRACE"};

    char messageBufffer[EZ_CONFIG_LOG_BUFFER_SIZE];
    if(log_level == EZ_LOG_LEVEL_TRACE){
        snprintf(messageBufffer, sizeof(messageBufffer), "[%s]: %s() <%s:%d>\n", level_strings[log_level], message, _file, _line);
    } else {
        va_list args;
        va_start(args, _line);
        char variaticArgsBuffer[EZ_CONFIG_LOG_BUFFER_SIZE];
        vsnprintf(variaticArgsBuffer, sizeof(variaticArgsBuffer), message, args);
        va_end(args);
        snprintf(messageBufffer, sizeof(messageBufffer), "[%s]: %s <%s:%d>\n", level_strings[log_level], variaticArgsBuffer, _file, _line);
    }

    // Send every log line to Tracy as a colored message (profile builds only; no-op otherwise)
    static const u32 TRACY_COLORS[6] = {
        0xFF0000, // FATAL      (Red)
        0xFF4D00, // Error      (Orange)
        0xFFFF00, // Warning    (Yellow)
        0x88E788, // Info       (Green),
        0x90D5FF, // Debug      (Blue),
        0xACADA5  // Trace      (Gray)
    };
    profilerMessage(messageBufffer, strlen(messageBufffer), TRACY_COLORS[log_level]);

    // ...and always to the platform console
    platformWriteConsoleOutput(messageBufffer, (u8)log_level);
}

void report_assertion_failure(const char *expression, const char* message, const char *file, s32 line){
    if(message[0] == '\0'){
        logOutout(EZ_LOG_LEVEL_FATAL, "ASSERTION FAILURE  -->  Expression: %s", file, line, expression);
    } else {
        logOutout(EZ_LOG_LEVEL_FATAL, "ASSERTION FAILURE  -->  Expression: %s  -->  Message: %s", file, line, expression, message);
    }
}
