#include "platform.hpp"


//TODO:(Argosta): implement windows specific functions
#if defined(EZ_PLATFORM_WINDOWS)

    #include <Windows.h>
    #include "core/logger/logger.hpp"
    
    inline PlatformOsType getPlatformOSType(){
        return PlatformOsType::EZ_OS_WINDOWS;
    }

    inline const char* getPlatformOsTypeString(){
        return "Windows";
    }

    // TODO(Argosta): replace main() function with WinMain() 
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
        LARGE_INTEGER qpcCounter, qpcFrequency;
        QueryPerformanceCounter(&qpcCounter);
        QueryPerformanceFrequency(&qpcFrequency);

        f64  ms = (qpcCounter.QuadPart * 1000.0f) / qpcFrequency.QuadPart;

        return ms;
    }
    
    void platformSleep(const u64 ms) {
        Sleep(ms);
    }

    

    void platformWriteConsoleOutput(const char* message, u8 color) {
        static u8 levels[6] = {64, 4, 6, 2, 1, 8};
        static CONSOLE_SCREEN_BUFFER_INFO console_info;
        static WORD original_attributes;

        HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);

        // get current console styles
        if (GetConsoleScreenBufferInfo(console_handle, &console_info)) {
            original_attributes = console_info.wAttributes;  
        }
        SetConsoleTextAttribute(console_handle, levels[color]);
        OutputDebugStringA(message);
        LPDWORD numCharsWritten = 0;
        WriteConsoleA(console_handle, message, strlen(message), numCharsWritten, 0);

        // reset the console text styles
        SetConsoleTextAttribute(console_handle, original_attributes);


    }
#endif // defined(EZ_PLATFORM_WINDOWS)