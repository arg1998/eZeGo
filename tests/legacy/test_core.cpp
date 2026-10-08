// Unit tests for ez_core: definitions, memory, platform clock, assertions, logger.
#include "check.hpp"
#include "core/assertion/assertions.hpp"
#include "core/definitions.hpp"
#include "core/logger/logger.hpp"
#include "core/memory/memory.hpp"
#include "core/platform/platform.hpp"

#include <cstdint>

static void memory_roundtrip() {
    u8* a = (u8*)ezAllocate(256);
    CHECK(a != nullptr);
    ezSetMemory(a, 256, 0xAB);
    CHECK(a[0] == 0xAB && a[255] == 0xAB);
    ezZeroMemory(a, 128);
    CHECK(a[0] == 0 && a[127] == 0 && a[128] == 0xAB);

    u8* b = (u8*)ezAllocateAligned(100, 64);
    CHECK(b != nullptr);
    CHECK(((uintptr_t)b & 63) == 0);
    ezCopyMemmory(a, b, 100);
    CHECK(b[0] == 0 && b[99] == 0);
    ezFreeAligned(b);
    ezFree(a);
}

static void clock_is_monotonic_and_sleeps() {
    const u64 t0 = platformGetClockTickNs();
    platformSleep(20);
    const u64 t1 = platformGetClockTickNs();
    CHECK(t1 > t0);
    const f64 elapsed_ms = (f64)(t1 - t0) / 1.0e6;
    CHECK(elapsed_ms >= 19.0);
    CHECK(elapsed_ms < 500.0);
    CHECK(platformGetClockTickMs() > 0.0);
}

static void platform_identity() {
    const char* os = getPlatformOsTypeString();
    CHECK(os != nullptr && os[0] != '\0');
#if defined(EZ_PLATFORM_LINUX)
    CHECK(getPlatformOSType() == EZ_OS_LINUX);
#elif defined(EZ_PLATFORM_MACOS)
    CHECK(getPlatformOSType() == EZ_OS_MAC);
#elif defined(EZ_PLATFORM_WINDOWS)
    CHECK(getPlatformOSType() == EZ_OS_WINDOWS);
#endif
    CHECK(platformGetExecutableDir()[0] != '\0');
    PlatformState* state = initPlatform("tests");
    CHECK(state != nullptr && getPlatformState() == state);
    shutdownPlatform();
}

static void assertions_pass_through() {
    int evaluated = 0;
    EZ_ASSERT(1 + 1 == 2);
    EZ_ASSERT_MSG(++evaluated == 1, "assert expressions run only when assertions are on");
    CHECK(evaluated == (EZ_CONFIG_ASSERTION_ENABLED ? 1 : 0));
}

static void logger_does_not_crash() {
    CHECK(initLoggingSystem());
    EZ_LOG_INFO("info %d %s", 42, "ok");
    EZ_LOG_WARN("warning from the test suite (expected)");
    EZ_LOG_DEBUG("debug %.2f", 1.5);
    EZ_LOG_TRACE();
    shutdownLoggingSystem();
}

int main() {
    RUN(memory_roundtrip);
    RUN(clock_is_monotonic_and_sleeps);
    RUN(platform_identity);
    RUN(assertions_pass_through);
    RUN(logger_does_not_crash);
    return finish();
}
