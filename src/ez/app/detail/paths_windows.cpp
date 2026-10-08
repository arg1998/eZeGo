// Not yet compiled on Windows.
#include "ez/app/detail/paths.hpp"

#include <cstring>
#include <windows.h>

namespace ez::app::detail {

const FixedString<1023>& executable_dir() noexcept {
    static FixedString<1023> g_dir;
    if (!g_dir.empty()) {
        return g_dir;
    }
    char path[1024] = {};
    const DWORD n = GetModuleFileNameA(nullptr, path, sizeof(path));
    if (n == 0 || n >= sizeof(path)) {
        return g_dir;
    }
    for (char* p = path; *p != '\0'; ++p) {
        if (*p == '\\') {
            *p = '/';  // paths are normalised to '/' (application-architecture.md §4)
        }
    }
    if (char* slash = std::strrchr(path, '/')) {
        *slash = '\0';
    }
    g_dir.assign(path);
    return g_dir;
}

}  // namespace ez::app::detail
