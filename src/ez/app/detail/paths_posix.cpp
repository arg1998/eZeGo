#include "ez/app/detail/paths.hpp"
#include "ez/base/detect.h"

#include <climits>
#include <cstring>
#include <unistd.h>

#if EZ_OS_MACOS
    #include <mach-o/dyld.h>
#endif

namespace ez::app::detail {

const FixedString<1023>& executable_dir() noexcept {
    static FixedString<1023> g_dir;
    if (!g_dir.empty()) {
        return g_dir;
    }
    char path[1024] = {};
#if EZ_OS_MACOS
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) != 0) {
        return g_dir;
    }
    char resolved[PATH_MAX] = {};
    if (realpath(path, resolved) != nullptr) {
        std::strncpy(path, resolved, sizeof(path) - 1);
    }
#else
    const ssize_t n = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (n <= 0) {
        return g_dir;
    }
    path[n] = '\0';
#endif
    if (char* slash = std::strrchr(path, '/')) {
        *slash = '\0';
    }
    g_dir.assign(path);
    return g_dir;
}

}  // namespace ez::app::detail
