#include "ez/base/detect.h"
#include "ez/cvars/detail/paths.hpp"

#include <cstdio>
#include <cstdlib>

namespace ez::cvars::detail {

bool default_settings_path(FixedString<1023>& out) noexcept {
    char buf[1024];
    const char* home = std::getenv("HOME");
#if EZ_OS_MACOS
    if (home == nullptr || home[0] == '\0') {
        return false;
    }
    std::snprintf(buf, sizeof(buf), "%s/Library/Application Support/eZeGo/settings.cfg", home);
#else
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg != nullptr && xdg[0] == '/') {
        std::snprintf(buf, sizeof(buf), "%s/ezego/settings.cfg", xdg);
    } else if (home != nullptr && home[0] != '\0') {
        std::snprintf(buf, sizeof(buf), "%s/.config/ezego/settings.cfg", home);
    } else {
        return false;
    }
#endif
    return out.assign(buf);
}

}  // namespace ez::cvars::detail
