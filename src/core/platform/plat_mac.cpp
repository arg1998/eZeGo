#include "platform.hpp"


#if defined(EZ_PLATFORM_MACOS)

    #include <stdlib.h>                 // malloc, free, aligned_alloc
    #include <string.h>                 // memcpy, memset
    #include <stdio.h>                  // printf, etc.
    #include <unistd.h>                 // usleep
    #include <mach/mach_time.h>         // mach_absolute_time, mach_timebase_info
    #include <mach-o/dyld.h>            // _NSGetExecutablePath

    #include "core/logger/logger.hpp"

    PlatformOsType getPlatformOSType(){
        return PlatformOsType::EZ_OS_MAC;
    }

    const char* getPlatformOsTypeString(){
        return "macOS";
    }

    static PlatformState platform_state;

    PlatformState* initPlatform(const char* application_name){
        EZ_LOG_TRACE();
        (void)application_name;
        platform_state.initialized = true;
        return &platform_state;
    }

    PlatformState* getPlatformState(){
        EZ_LOG_TRACE();
        if(!platform_state.initialized) {
            EZ_LOG_WARN("Accessing platform state that is not initialized yet");
            return nullptr;
        }

        return &platform_state;
    }

    void shutdownPlatform(){
        EZ_LOG_TRACE();
        platform_state.initialized = false;
    }

    EZ_NO_DISCARD void* platformAllocateMemory(u64 size){
        return malloc(size);
    }
    EZ_NO_DISCARD void* platformAllocateMemoryAligned(u64 size, u16 alignment){
        u64 rounded = (size + alignment - 1) / alignment * alignment;
        return aligned_alloc(alignment, rounded);
    }
    void platformFreeMemory(void* buffer) {
        free(buffer);
    }
    void platformFreeMemoryAligned(void* buffer) {
        free(buffer);
    }
    void platformCopyMemory(void* source, void* dest, u64 size){
        memcpy(dest, source, size);
    }
    void platformZeroMemory(void *buffer, u64 size){
        platformSetMemory(buffer, size, 0);
    }
    void platformSetMemory(void *buffer, u64 size, s32 value){
        memset(buffer, value, size);
    }

    u64 platformGetClockTickNs() {
        // static ensures we only query the timebase once
        static mach_timebase_info_data_t timebaseInfo = {0, 0};
        if (timebaseInfo.denom == 0) {
            mach_timebase_info(&timebaseInfo);
        }

        // mach_absolute_time() gives us "ticks" based on the CPU clock
        // Convert ticks -> nanoseconds
        u64 now = mach_absolute_time();
        return (now * timebaseInfo.numer) / timebaseInfo.denom;
    }

    f64 platformGetClockTickMs() {
        return (f64)platformGetClockTickNs() / 1.0e6;
    }

    void platformSleep(const u64 ms) {
        usleep((useconds_t)(ms * 1000));
    }

    const char* platformGetExecutableDir() {
        static char dir[1024] = {};
        if (dir[0] == '\0') {
            u32 size = sizeof(dir);
            _NSGetExecutablePath(dir, &size);
            char* slash = strrchr(dir, '/');
            if (slash) *slash = '\0';
        }
        return dir;
    }

    void platformWriteConsoleOutput(const char* message, u8 color) {

        static const char* ansiColors[] = {
            "\x1b[37;41m", // white on red background
            "\x1b[31m",    // red
            "\x1b[33m",    // yellow
            "\x1b[32m",    // green
            "\x1b[34m",    // blue
            "\x1b[90m"     // gray (bright black)
        };

        // Print the color code, then the message, then reset
        FILE* out = color <= 2 ? stderr : stdout;
        fprintf(out, "%s%s\x1b[0m", ansiColors[color], message);
        fflush(out);
    }

#endif // defined(EZ_PLATFORM_MACOS)
