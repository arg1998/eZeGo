#pragma once 

#include "definitions.hpp"



enum PlatformOsType {
    EZ_OS_WINDOWS   = 0,
    EZ_OS_LINUX     = 1,
    EZ_OS_MAC       = 2
};


b8 initPlatform();
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
