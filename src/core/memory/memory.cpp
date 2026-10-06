#include "memory.hpp"

#include "core/assertion/assertions.hpp"
#include "core/platform/platform.hpp"
#include "core/profiler/profiler.hpp"

// Every first-party allocation is reported to Tracy in profile builds (pool "ez/general").
static const char* const k_pool = "ez/general";

EZ_NO_DISCARD void* ezAllocate(u64 size) {
    void* p = platformAllocateMemory(size);
    profilerAlloc(p, size, k_pool);
    return p;
}

EZ_NO_DISCARD void* ezAllocateAligned(u64 size, u16 alignment) {
    EZ_ASSERT_MSG(alignment > 0 && (alignment & (alignment - 1)) == 0, "alignment must be a power of two");
    void* p = platformAllocateMemoryAligned(size, alignment);
    profilerAlloc(p, size, k_pool);
    return p;
}

void ezFree(void* buffer) {
    profilerFree(buffer, k_pool);
    platformFreeMemory(buffer);
}

void ezFreeAligned(void* buffer) {
    profilerFree(buffer, k_pool);
    platformFreeMemoryAligned(buffer);
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
