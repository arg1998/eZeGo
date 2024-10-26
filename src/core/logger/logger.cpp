#include "logger.hpp"
#include "core/assertion/assertions.hpp"
#include "core/platform/platform.hpp"

#include <cstdarg>


b8 initLoggingSystem() {
    return true;
}

void shutdownLoggingSystem() {}


void logOutout(LogLevel log_level, const std::string message, const char *_file, s32 _line, ...) {
    const char *level_strings[6] = {"FATAL", "ERROR", "WARN ", "INFO ", "DEBUG", "TRACE"};
    va_list args {};
    va_start(args, _line);

    char variaticArgsBuffer[EZ_CONFIG_LOG_BUFFER_SIZE] = {};
    vsnprintf(variaticArgsBuffer, sizeof(variaticArgsBuffer), message.c_str(), args);

    va_end(args);

    char messageBufffer[EZ_CONFIG_LOG_BUFFER_SIZE] = {};
    sprintf(messageBufffer, "[%s]: %s <%s:%d>\n", level_strings[log_level], variaticArgsBuffer, _file, _line);

    //TODO:(Argosta): implement a platform-specific colored output mechanism when we have a platform system in place
    #if defined(EZ_CONFIG_LOG_TO_STD_ERR) && (EZ_CONFIG_LOG_TO_STD_ERR)
        if (log_level == EZ_LOG_LEVEL_FATAL || log_level == EZ_LOG_LEVEL_ERROR) {
            platformWriteConsoleError(messageBufffer, log_level);
        } else {
            platformWriteConsoleOutput(messageBufffer, log_level);
        }
    #else
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

