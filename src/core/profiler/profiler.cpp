#include "profiler.hpp"

#include <atomic>

[[maybe_unused]] static std::atomic<bool> g_alive{false};

#if EZ_PROFILER

void startProfiler() {
    tracy::StartupProfiler();
    g_alive.store(true, std::memory_order_release);
}

void stopProfiler() {
    g_alive.store(false, std::memory_order_release);
    tracy::ShutdownProfiler();
}

b8 profilerIsAlive() { return g_alive.load(std::memory_order_acquire); }
b8 profilerIsConnected() { return profilerIsAlive() && TracyIsConnected; }

void profilerSetThreadName(const char* name) {
    if (profilerIsAlive()) tracy::SetThreadName(name);
}
void profilerMessage(const char* text, u64 length, u32 rgb) {
    if (profilerIsAlive()) TracyMessageC(text, length, rgb);
}
void profilerAlloc(const void* ptr, u64 size, const char* pool) {
    if (ptr && profilerIsAlive()) TracyAllocN(ptr, size, pool);
}
void profilerFree(const void* ptr, const char* pool) {
    if (ptr && profilerIsAlive()) TracyFreeN(ptr, pool);
}

#else

void startProfiler() {}
void stopProfiler() {}
b8 profilerIsAlive() { return false; }
b8 profilerIsConnected() { return false; }
void profilerSetThreadName(const char*) {}
void profilerMessage(const char*, u64, u32) {}
void profilerAlloc(const void*, u64, const char*) {}
void profilerFree(const void*, const char*) {}

#endif
