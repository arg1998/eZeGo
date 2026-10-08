// Not yet compiled on Windows.
#include "ez/log/detail/os.hpp"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <windows.h>

namespace ez::log::detail {

bool prepare_stderr() noexcept {
    // A GUI-subsystem process has no console: attach to the terminal it was started from, if any.
    if (GetStdHandle(STD_ERROR_HANDLE) == nullptr || GetStdHandle(STD_ERROR_HANDLE) == INVALID_HANDLE_VALUE) {
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
            std::FILE* ignored = nullptr;
            freopen_s(&ignored, "CONOUT$", "w", stderr);
        }
    }
    const HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
    DWORD mode = 0;
    if (h == nullptr || h == INVALID_HANDLE_VALUE || !GetConsoleMode(h, &mode)) {
        return false;  // redirected to a file or pipe, or no console at all
    }
    SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    return true;
}

void write_stderr(const char* data, usize length) noexcept {
    const HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
    if (h == nullptr || h == INVALID_HANDLE_VALUE) {
        return;
    }
    while (length > 0) {
        DWORD n = 0;
        if (!WriteFile(h, data, DWORD(length), &n, nullptr) || n == 0) {
            return;
        }
        data += n;
        length -= n;
    }
}

bool debugger_attached() noexcept {
    return IsDebuggerPresent() != 0;
}

void debug_output(const char* text) noexcept {
    if (IsDebuggerPresent()) {
        OutputDebugStringA(text);
    }
}

void set_thread_name(const char* name) noexcept {
    // SetThreadDescription exists from Windows 10 1607; looked up so older systems still start.
    using SetDescription = HRESULT(WINAPI*)(HANDLE, PCWSTR);
    static const auto g_set = reinterpret_cast<SetDescription>(
        reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "SetThreadDescription")));
    if (g_set == nullptr) {
        return;
    }
    wchar_t wide[64];
    MultiByteToWideChar(CP_UTF8, 0, name, -1, wide, 64);
    g_set(GetCurrentThread(), wide);
}

bool default_log_dir(FixedString<1023>& out) noexcept {
    const char* local = std::getenv("LOCALAPPDATA");
    if (local == nullptr || local[0] == '\0') {
        return false;
    }
    char buf[1024];
    std::snprintf(buf, sizeof(buf), "%s\\eZeGo\\logs", local);
    return out.assign(buf);
}

LocalTime local_time(i64 unix_seconds) noexcept {
    const std::time_t t = std::time_t(unix_seconds);
    std::tm tm{};
    localtime_s(&tm, &t);
    return {tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec};
}

}  // namespace ez::log::detail
