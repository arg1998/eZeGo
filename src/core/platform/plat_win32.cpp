#include "platform.hpp"


#if defined(EZ_PLATFORM_WINDOWS)

    #include <windows.h>
    #include <malloc.h>                 // _aligned_malloc
    #include <stdio.h>
    #include <string.h>
    #include "core/logger/logger.hpp"

    PlatformOsType getPlatformOSType(){
        return PlatformOsType::EZ_OS_WINDOWS;
    }

    const char* getPlatformOsTypeString(){
        return "Windows";
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
        return _aligned_malloc(size, alignment);
    }
    void platformFreeMemory(void* buffer) {
        free(buffer);
    }
    void platformFreeMemoryAligned(void* buffer) {
        _aligned_free(buffer);
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
        static LARGE_INTEGER qpcFrequency = {};
        if (qpcFrequency.QuadPart == 0) {
            QueryPerformanceFrequency(&qpcFrequency);
        }
        LARGE_INTEGER qpcCounter;
        QueryPerformanceCounter(&qpcCounter);
        // split to avoid overflow: whole seconds + remainder
        u64 freq = (u64)qpcFrequency.QuadPart;
        u64 ticks = (u64)qpcCounter.QuadPart;
        return (ticks / freq) * 1000000000ull + (ticks % freq) * 1000000000ull / freq;
    }

    f64 platformGetClockTickMs() {
        return (f64)platformGetClockTickNs() / 1.0e6;
    }

    void platformSleep(const u64 ms) {
        Sleep((DWORD)ms);
    }

    const char* platformGetExecutableDir() {
        static char dir[1024] = {};
        if (dir[0] == '\0') {
            GetModuleFileNameA(nullptr, dir, sizeof(dir));
            for (char* p = dir; *p; ++p) if (*p == '\\') *p = '/';   // unix-style paths everywhere
            char* slash = strrchr(dir, '/');
            if (slash) *slash = '\0';
        }
        return dir;
    }

    void platformWriteConsoleOutput(const char* message, u8 color) {
        static u8 levels[6] = {64, 4, 6, 2, 1, 8};
        CONSOLE_SCREEN_BUFFER_INFO console_info;
        WORD original_attributes = 7;

        HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);

        // get current console styles
        if (GetConsoleScreenBufferInfo(console_handle, &console_info)) {
            original_attributes = console_info.wAttributes;
        }
        SetConsoleTextAttribute(console_handle, levels[color]);
        OutputDebugStringA(message);
        DWORD numCharsWritten = 0;
        WriteConsoleA(console_handle, message, (DWORD)strlen(message), &numCharsWritten, 0);

        // reset the console text styles
        SetConsoleTextAttribute(console_handle, original_attributes);
    }
#endif // defined(EZ_PLATFORM_WINDOWS)
