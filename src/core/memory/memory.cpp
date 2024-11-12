#include "memory.hpp"

#include "core/platform/platform.hpp"

EZ_NO_DISCARD void* ezAllocate(u64 size) {
    return platformAllocateMemory(size);
}

void ezFree(void* buffer) {
    platformFreeMemory(buffer);
}

void ezSetMemory(void* buffer, u64 size, s32 value) {
    platformSetMemory(buffer, size, value);
}

void ezZeroMemory(void* buffer, u64 size) {
    platformZeroMemory(buffer, size);
}

void ezCopyMemmory(void* orig, void* dest, u64 size) {
    platformCopyMemory(orig, dest, size);
}
