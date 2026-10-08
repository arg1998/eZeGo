#include "ez/base/detect.h"
#include "ez/log/detail/os.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <pthread.h>
#include <unistd.h>

#if EZ_OS_MACOS
    #include <sys/sysctl.h>
    #include <sys/types.h>
#endif

namespace ez::log::detail {

bool prepare_stderr() noexcept {
    return isatty(STDERR_FILENO) == 1;
}

void write_stderr(const char* data, usize length) noexcept {
    while (length > 0) {
        const ssize_t n = write(STDERR_FILENO, data, length);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            return;
        }
        data += n;
        length -= usize(n);
    }
}

bool debugger_attached() noexcept {
#if EZ_OS_MACOS
    int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid()};
    kinfo_proc info{};
    size_t size = sizeof(info);
    if (sysctl(mib, 4, &info, &size, nullptr, 0) != 0) {
        return false;
    }
    return (info.kp_proc.p_flag & P_TRACED) != 0;
#else
    std::FILE* f = std::fopen("/proc/self/status", "r");
    if (f == nullptr) {
        return false;
    }
    char line[256];
    bool traced = false;
    while (std::fgets(line, sizeof(line), f) != nullptr) {
        if (std::strncmp(line, "TracerPid:", 10) == 0) {
            traced = std::atoi(line + 10) != 0;
            break;
        }
    }
    std::fclose(f);
    return traced;
#endif
}

void debug_output(const char*) noexcept {}

void set_thread_name(const char* name) noexcept {
#if EZ_OS_MACOS
    pthread_setname_np(name);
#else
    char shortened[16];  // Linux limits thread names to 15 characters
    std::snprintf(shortened, sizeof(shortened), "%s", name);
    pthread_setname_np(pthread_self(), shortened);
#endif
}

bool default_log_dir(FixedString<1023>& out) noexcept {
    char buf[1024];
    const char* home = std::getenv("HOME");
#if EZ_OS_MACOS
    if (home == nullptr || home[0] == '\0') {
        return false;
    }
    std::snprintf(buf, sizeof(buf), "%s/Library/Logs/eZeGo", home);
#else
    const char* state = std::getenv("XDG_STATE_HOME");
    if (state != nullptr && state[0] == '/') {
        std::snprintf(buf, sizeof(buf), "%s/ezego/logs", state);
    } else if (home != nullptr && home[0] != '\0') {
        std::snprintf(buf, sizeof(buf), "%s/.local/state/ezego/logs", home);
    } else {
        return false;
    }
#endif
    return out.assign(buf);
}

LocalTime local_time(i64 unix_seconds) noexcept {
    const std::time_t t = std::time_t(unix_seconds);
    std::tm tm{};
    localtime_r(&t, &tm);
    return {tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec};
}

}  // namespace ez::log::detail
