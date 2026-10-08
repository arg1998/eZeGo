#include "ez/app/app.hpp"
#include "ez/app/detail/settings.hpp"
#include "ez/cvars/registry.hpp"

namespace ez::app::detail {

using cvars::Flags;
using cvars::Mutability;
using cvars::Tier;

EZ_CVAR_F64(cv_app_quit_after_s, "app.quit_after_s", 0.0,
            {.min = 0,
             .max = 86400,
             .mutability = Mutability::Startup,
             .tier = Tier::Hidden,
             .help = "Close the application after this many seconds; 0 means never. For tests and captures."});
EZ_CVAR_I64(cv_app_quit_after_frames, "app.quit_after_frames", 0,
            {.min = 0,
             .mutability = Mutability::Startup,
             .tier = Tier::Hidden,
             .help = "Close the application after this many frames; 0 means never. The smoke test uses it."});
EZ_CVAR_BOOL(cv_app_developer, "app.developer", false,
             {.flags = Flags::Persist,
              .help = "Developer mode: shows the console, developer settings and extra diagnostics."});

}  // namespace ez::app::detail

namespace ez::app {

void register_cvars(cvars::Registry& r) noexcept {
    r.add(detail::cv_app_quit_after_s);
    r.add(detail::cv_app_quit_after_frames);
    r.add(detail::cv_app_developer);
}

}  // namespace ez::app
