#include "platform.hpp"


//TODO:(Argosta): implement macOS specific functions
#if defined(EZ_PLATFORM_MACOS)

    #include <stdlib.h>                 // malloc, free
    #include <string.h>                 // memcpy, memset
    #include <stdio.h>                  // printf, etc.
    #include <unistd.h>                 // usleep
    #include <mach/mach_time.h>         // mach_absolute_time, mach_timebase_info

    #include "core/logger/logger.hpp"
    
    inline PlatformOsType getPlatformOSType(){
        return PlatformOsType::EZ_OS_MAC;
    }

    inline const char* getPlatformOsTypeString(){
        return "macOS";
    }

    // TODO(Argosta): replace main() function with macOS main function
    int main(){
        PLATFORM_MAIN();
    }

    static PlatformState platform_state;
    static b8 initialized = false;

    PlatformState* initPlatform(const char* application_name){
        EZ_LOG_TRACE();


        if(!glfwInit()){
            EZ_LOG_FATAL("Faild to initialize GLFW");
            return nullptr;
        }

        platform_state.application_name = application_name;
        // TODO(Argosta): read the application settings from disk and populate glfw window width and height
        // TODO(Argosta): window creation should go in the application layer. But for now we leave it here
        platform_state.main_window = glfwCreateWindow(1000, 600, application_name, NULL, NULL);
        if(!platform_state.main_window){
            EZ_LOG_FATAL("Failed to create GLFW main window");
            return nullptr;
        }

        platform_state.os_type = PlatformOsType::EZ_OS_MAC;
        platform_state.application_version_str = EZ_APPLICATION_VERSION_STR;

        initialized = true;
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
        glfwTerminate();
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
        // static ensures we only query the timebase once
        static mach_timebase_info_data_t timebaseInfo = {0};
        if (timebaseInfo.denom == 0) {
            mach_timebase_info(&timebaseInfo);
        }

        // mach_absolute_time() gives us "ticks" based on the CPU clock
        // Convert ticks -> nanoseconds -> milliseconds
        u64 now   = mach_absolute_time();
        u64 nanos = (now * timebaseInfo.numer) / timebaseInfo.denom;
        f64 ms         = (f64)nanos / 1.0e6; 
        return ms;
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

    void platformWriteConsoleError(const char* message, u8 color) {
        //TODO(Argosta): we will remove the separate error stream! let this be here for a while.
        platformWriteConsoleOutput(message, color);
    }

#endif // defined(EZ_PLATFORM_MACOS)