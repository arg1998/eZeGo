#include "logger.hpp"

#include <iostream>
#include <cstdarg>


b8 initLoggingSystem() {
    return true;
}

void shutdownLoggingSystem() {}


void logOutout(LogLevel log_level, const std::string message, const char *_file, s32 _line, ...) {
    const char *level_strings[6] = {"FATAL", "ERROR", "WARN ", "INFO ", "DEBUG", "TRACE"};
    va_list args {};
    va_start(args, _line);

    char messageBuffer[EZ_CONFIG_LOG_BUFFER_SIZE] = {};
    vsnprintf(messageBuffer, sizeof(messageBuffer), message.c_str(), args);

    va_end(args);

    //TODO:(Argosta): implement a platform-specific colored output mechanism when we have a platform system in place
    #if defined(EZ_CONFIG_LOG_TO_STD_ERR) && (EZ_CONFIG_LOG_TO_STD_ERR)
        if (log_level == EZ_LOG_LEVEL_FATAL || log_level == EZ_LOG_LEVEL_ERROR) {
            std::cerr << "[" << level_strings[log_level] << "]: " << messageBuffer << " <" << _file << ":L" << _line << ">" << std::endl;
        } else {
            std::cout << "[" << level_strings[log_level] << "]: " << messageBuffer << " <" << _file << ":L" << _line << ">" << std::endl;
        }
    #else
        std::cout << "[" << level_strings[log_level] << "]: " << messageBuffer << " <" << _file << ":L" << _line << ">" << std::endl;
    #endif
}

