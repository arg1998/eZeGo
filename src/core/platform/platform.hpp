#pragma once 

#include "core/definitions.hpp"

#include <GLFW/glfw3.h>
#include <string>


enum PlatformOsType {
    EZ_OS_WINDOWS   = 0,
    EZ_OS_LINUX     = 1,
    EZ_OS_MAC       = 2
};

struct PlatformState {
    std::string application_name;
    std::string application_version_str;
    GLFWwindow *main_window;
    PlatformOsType os_type;
};

/// @brief Platform independent main function. Only called by platform implementations.
void PLATFORM_MAIN();

PlatformState* initPlatform(const char* application_name);
PlatformState* getPlatformState();
void shutdownPlatform();

EZ_NO_DISCARD void* platformAllocateMemory(u64 size);
EZ_NO_DISCARD void* platformAllocateMemoryAligned(u64 size, u16 alignment);
void platformFreeMemory(void* buffer);
void platformCopyMemory(void* source, void* dest, u64 size);
void platformZeroMemory(void *buffer, u64 size);
void platformSetMemory(void *buffer, u64 size, s32 value);

f64 platformGetClockTickMs();
void platformSleep(const u64 ms);

void platformWriteConsoleOutput(const char* message, u8 color);
void platformWriteConsoleError(const char* message, u8 color);


inline const char* getPlatformOsTypeString();
inline PlatformOsType getPlatformOSType();

