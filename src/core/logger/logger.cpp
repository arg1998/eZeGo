#include "logger.hpp"
#include "core/assertion/assertions.hpp"
#include "core/platform/platform.hpp"

#include <cstdarg>


b8 initLoggingSystem() {
    return true;
}

void shutdownLoggingSystem() {}


void logOutout(LogLevel log_level, const std::string& message, const char *_file, s32 _line, ...) {
    const char *level_strings[6] = {"FATAL", "ERROR", "WARN ", "INFO ", "DEBUG", "TRACE"};
    
    char messageBufffer[EZ_CONFIG_LOG_BUFFER_SIZE] = {};
    if(log_level == EZ_LOG_LEVEL_TRACE){
        sprintf(messageBufffer, "[%s]: %s() <%s:%d>\n", level_strings[log_level], message.c_str(), _file, _line);
    }else {
        va_list args {};
        va_start(args, _line);
        char variaticArgsBuffer[EZ_CONFIG_LOG_BUFFER_SIZE] = {};
        vsnprintf(variaticArgsBuffer, sizeof(variaticArgsBuffer), message.c_str(), args);
        va_end(args);
        sprintf(messageBufffer, "[%s]: %s <%s:%d>\n", level_strings[log_level], variaticArgsBuffer, _file, _line);
    }

    platformWriteConsoleOutput(messageBufffer, log_level);

}

void report_assertion_failure(const char *expression, const char* message, const char *file, s32 line){
    if(message[0] == '\0'){
        logOutout(EZ_LOG_LEVEL_FATAL, "ASSERTION FAILURE  --> Expression: %s --> File:", file, line, expression);
    } else {
        logOutout(EZ_LOG_LEVEL_FATAL, "ASSERTION FAILURE  -->  Expression: %s  -->  Message: %s  -->  File:", file, line, expression, message);
    }
    
}

