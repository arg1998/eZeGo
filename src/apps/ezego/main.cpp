// The eZeGo executable: wiring only (specs/code-organization.md CO-6). Startup order follows
// observability.md §3: settings, then the profiler, then the logger, then everything else; the
// reverse on shutdown.
#include "ez/app/app.hpp"
#include "ez/cvars/registry.hpp"
#include "ez/log/log.hpp"
#include "ez/metrics/profiler.hpp"

#include <cstdio>

int main(int argc, char** argv) {
    ez::cvars::Registry& registry = ez::cvars::registry();
    ez::log::register_cvars(registry);  // one visible list, in module order (cvars.md CV-11)
    ez::app::register_cvars(registry);

    const ez::cvars::StartupResult start =
        registry.init({.argc = argc, .argv = argv, .version_text = ez::app::version_text()});
    if (start.exit_now) {  // --help, --version, or invalid settings (fail fast)
        std::fputs(start.report.c_str(), stderr);
        return start.exit_code;
    }

    ez::metrics::start_profiler();
    ez::log::init(registry);
    EZ_LOG_INFO(app, "%s starting; settings: %s", ez::app::version_text(),
                registry.settings_path()[0] != '\0' ? registry.settings_path() : "none");

    const int exit_code = ez::app::run(registry);

    registry.save();
    ez::log::shutdown();
    ez::metrics::stop_profiler();
    return exit_code;
}
