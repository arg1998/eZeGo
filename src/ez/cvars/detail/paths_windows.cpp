// Not yet compiled on Windows.
#include "ez/cvars/detail/paths.hpp"

#include <cstdio>
#include <cstdlib>

namespace ez::cvars::detail {

bool default_settings_path(FixedString<1023>& out) noexcept {
    const char* appdata = std::getenv("APPDATA");
    if (appdata == nullptr || appdata[0] == '\0') {
        return false;
    }
    char buf[1024];
    std::snprintf(buf, sizeof(buf), "%s\\eZeGo\\settings.cfg", appdata);
    return out.assign(buf);
}

}  // namespace ez::cvars::detail
