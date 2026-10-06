#pragma once

#include "core/definitions.hpp"


enum PlatformOsType {
    EZ_OS_WINDOWS   = 0,
    EZ_OS_LINUX     = 1,
    EZ_OS_MAC       = 2
};

struct PlatformState {
    //TODO(Argosta): implement me
    b8 initialized;
};

/// @brief Platform independent main function. Defined by the application; called by the
/// per-OS entry point (platform/entry.cpp, compiled into executables only).
s32 PLATFORM_MAIN(s32 argc, char** argv);

PlatformState* initPlatform(const char* application_name);
PlatformState* getPlatformState();
void shutdownPlatform();

EZ_NO_DISCARD void* platformAllocateMemory(u64 size);
EZ_NO_DISCARD void* platformAllocateMemoryAligned(u64 size, u16 alignment);
void platformFreeMemory(void* buffer);
void platformFreeMemoryAligned(void* buffer);
void platformCopyMemory(void* source, void* dest, u64 size);
void platformZeroMemory(void *buffer, u64 size);
void platformSetMemory(void *buffer, u64 size, s32 value);

/// @brief Monotonic reference clock in milliseconds (one epoch for all threads).
f64 platformGetClockTickMs();
/// @brief Monotonic reference clock in nanoseconds.
u64 platformGetClockTickNs();
void platformSleep(const u64 ms);

void platformWriteConsoleOutput(const char* message, u8 color);

/// @brief Directory of the running executable, unix separators, no trailing '/'.
const char* platformGetExecutableDir();

const char* getPlatformOsTypeString();
PlatformOsType getPlatformOSType();
