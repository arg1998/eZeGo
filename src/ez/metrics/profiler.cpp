#include "ez/metrics/profiler.hpp"

#include "ez/log/log.hpp"

#include <atomic>

namespace ez::metrics {

#if EZ_PROFILER

namespace {

std::atomic<bool> g_running{false};

// Every log line becomes a Tracy message at its call site, coloured by level.
void log_to_profiler(const char* text, usize length, log::Level level) {
    constexpr u32 colors[] = {
        0xACADA5,  // trace: grey
        0x90D5FF,  // debug: blue
        0x88E788,  // info: green
        0xFFFF00,  // warn: yellow
        0xFF4D00,  // error: orange
        0xFF0000,  // fatal: red
    };
    if (g_running.load(std::memory_order_acquire)) {
        TracyMessageC(text, length, colors[u8(level) < 6 ? u8(level) : 5]);
    }
}

}  // namespace

void start_profiler() noexcept {
    tracy::StartupProfiler();
    g_running.store(true, std::memory_order_release);
    log::set_profiler_hook(&log_to_profiler);
}

void stop_profiler() noexcept {
    log::set_profiler_hook(nullptr);
    g_running.store(false, std::memory_order_release);
    tracy::ShutdownProfiler();
}

bool profiler_running() noexcept {
    return g_running.load(std::memory_order_acquire);
}
bool profiler_connected() noexcept {
    return profiler_running() && TracyIsConnected;
}

void profiler_thread_name(const char* name) noexcept {
    if (profiler_running()) {
        tracy::SetThreadName(name);
    }
}

void profiler_alloc(const void* ptr, usize size, const char* pool) noexcept {
    if (ptr != nullptr && profiler_running()) {
        TracyAllocN(ptr, size, pool);
    }
}

void profiler_free(const void* ptr, const char* pool) noexcept {
    if (ptr != nullptr && profiler_running()) {
        TracyFreeN(ptr, pool);
    }
}

#else

void start_profiler() noexcept {}
void stop_profiler() noexcept {}
bool profiler_running() noexcept {
    return false;
}
bool profiler_connected() noexcept {
    return false;
}
void profiler_thread_name(const char*) noexcept {}
void profiler_alloc(const void*, usize, const char*) noexcept {}
void profiler_free(const void*, const char*) noexcept {}

#endif

}  // namespace ez::metrics
