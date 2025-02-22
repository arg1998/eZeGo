#include "platform.hpp"


//TODO:(Argosta): implement macOS specific functions
#if defined(EZ_PLATFORM_LINUX)

    #include <stdlib.h>                 // malloc, free
    #include <string.h>                 // memcpy, memset
    #include <stdio.h>                  // printf, etc.
    #include <unistd.h>                 // usleep

    #include "core/logger/logger.hpp"
    
    inline PlatformOsType getPlatformOSType(){
        return PlatformOsType::EZ_OS_LINUX;
    }

    inline const char* getPlatformOsTypeString(){
        return "macOS";
    }

    // TODO(Argosta): replace main() function with Linux main function
    int main(){
        PLATFORM_MAIN();
    }

    static PlatformState platform_state;
    static b8 initialized = false;

    PlatformState* initPlatform(const char* application_name){
        EZ_LOG_TRACE();
        return &platform_state;
    }

    PlatformState* getPlatformState(){
        EZ_LOG_TRACE();
        if(!initialized) {
            EZ_LOG_WARN("Accessing platform state that is not initialized yet");
            return nullptr;
        }

        return &platform_state;
    }

    void shutdownPlatform(){
        EZ_LOG_TRACE();
    }

    EZ_NO_DISCARD void* platformAllocateMemory(u64 size){
        return malloc(size);
    }
    void platformFreeMemory(void* buffer) {
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

    f64 platformGetClockTickMs() {
        EZ_LOG_TRACE();
        // TODO(Argosta): implement this function
        EZ_LOG_FATAL("Function not implemented for linux");
        return 0.0;
    }
    
    void platformSleep(const u64 ms) {
        usleep((useconds_t)(ms * 1000));
    }

    

    void platformWriteConsoleOutput(const char* message, u8 color) {

        static const u8* ansiColors[] = {
            (const u8 *)"\x1b[37;41m", // white on red background
            (const u8 *)"\x1b[31m",    // red
            (const u8 *)"\x1b[33m",    // yellow
            (const u8 *)"\x1b[32m",    // green
            (const u8 *)"\x1b[34m",    // blue
            (const u8 *)"\x1b[90m"     // gray (bright black)
        };

        // Print the color code, then the message, then reset
        printf("%s%s\x1b[0m", ansiColors[color], message);
        fflush(stdout);
    }

#endif // defined(EZ_PLATFORM_MACOS)