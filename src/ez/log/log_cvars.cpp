// The logger's cvars and the --log= / EZ_LOG shorthand (specs/logging.md §6, LG-9, LG-10).
#include "ez/cvars/registry.hpp"
#include "ez/log/detail/settings.hpp"
#include "ez/log/log.hpp"

#include <string_view>

namespace ez::log::detail {

using cvars::Flags;
using cvars::Mutability;
using cvars::Tier;

constexpr const char* level_setting_names[] = {"inherit", "trace", "debug", "info", "warn", "error", "fatal", "off"};
constexpr const char* level_all_names[] = {"default", "trace", "debug", "info", "warn", "error", "fatal", "off"};
constexpr const char* toggle_names[] = {"auto", "on", "off"};

// The build mode's runtime default (LG-3): trace in debug, info in profile and release.
constexpr LevelSetting mode_default_level = LevelSetting(EZ_LOG_LEVEL + 1);

EZ_CVAR_ENUM(cv_log_level_all, "log.level.all", LevelSetting, mode_default_level, level_all_names,
             {.flags = Flags::Persist,
              .help = "Log level for every category without its own; lines below the build's floor do not exist."});

// One level cvar per module, generated from modules.cmake: log.level.<module>.
#define EZ_DETAIL_LEVEL_CVAR(name, text, layer)                                                                    \
    EZ_CVAR_ENUM(cv_log_level_##name, "log.level." text, LevelSetting, LevelSetting::Inherit, level_setting_names, \
                 {.tier = Tier::Advanced,                                                                          \
                  .flags = Flags::Persist,                                                                         \
                  .help = "Log level for the '" text "' category; inherit uses log.level.all."});
EZ_MODULES(EZ_DETAIL_LEVEL_CVAR)
#undef EZ_DETAIL_LEVEL_CVAR

cvars::CVarEnum<LevelSetting>* const g_category_levels[] = {
#define EZ_DETAIL_LEVEL_PTR(name, text, layer) &cv_log_level_##name,
    EZ_MODULES(EZ_DETAIL_LEVEL_PTR)
#undef EZ_DETAIL_LEVEL_PTR
};

EZ_CVAR_I32(cv_log_max_line, "log.max_line", 512,
            {.min = 64,
             .max = max_line_cap,
             .tier = Tier::Advanced,
             .flags = Flags::Persist,
             .help = "Longest log line in bytes; longer lines are cut and end with '...'."});
EZ_CVAR_I32(cv_log_ring_kib, "log.ring_kib", 128,
            {.min = 16,
             .max = 16384,
             .mutability = Mutability::Startup,
             .tier = Tier::Developer,
             .help = "Per-thread log buffer in KiB, rounded up to a power of two."});
EZ_CVAR_I32(cv_log_threads_max, "log.threads_max", 16,
            {.min = 2,
             .max = 256,
             .mutability = Mutability::Startup,
             .tier = Tier::Developer,
             .help = "How many threads can log at the same time."});
EZ_CVAR_I32(cv_log_history_kib, "log.history_kib", 256,
            {.min = 0,
             .max = 65536,
             .mutability = Mutability::Startup,
             .tier = Tier::Developer,
             .help = "Recent log lines kept in memory for the console and crash reports, in KiB."});
EZ_CVAR_I32(cv_log_drain_ms, "log.drain_ms", 20,
            {.min = 1,
             .max = 1000,
             .tier = Tier::Advanced,
             .flags = Flags::Persist,
             .help = "How often the log thread writes queued lines, in milliseconds."});
EZ_CVAR_BOOL(cv_log_stderr, "log.stderr", true,
             {.tier = Tier::Developer,
              .help = "Write log lines to stderr; off for benchmarks and tools that use stderr themselves."});
EZ_CVAR_BOOL(cv_log_file, "log.file", false,
             {.flags = Flags::Persist, .help = "Also write the log to a file, one per session."});
EZ_CVAR_I32(cv_log_file_flush_ms, "log.file.flush_ms", 1000,
            {.min = 10,
             .max = 60000,
             .tier = Tier::Advanced,
             .flags = Flags::Persist,
             .help = "How often the log file is flushed to disk, in ms; warnings and errors flush at once."});
EZ_CVAR_I32(cv_log_file_keep, "log.file.keep", 10,
            {.min = 1,
             .max = 1000,
             .mutability = Mutability::Startup,
             .tier = Tier::Advanced,
             .flags = Flags::Persist,
             .help = "How many session log files are kept; older ones are deleted."});
EZ_CVAR_STRING(cv_log_file_dir, "log.file.dir", "",
               {.mutability = Mutability::Startup,
                .tier = Tier::Advanced,
                .flags = Flags::Persist,
                .help = "Directory for log files; empty means the per-user state directory."});
EZ_CVAR_ENUM(cv_log_sync, "log.sync", Toggle, Toggle::Auto, toggle_names,
             {.mutability = Mutability::Startup,
              .tier = Tier::Developer,
              .help = "Write each line to stderr immediately, on the calling thread; auto means when a "
                      "debugger is attached."});
EZ_CVAR_ENUM(cv_log_color, "log.color", Toggle, Toggle::Auto, toggle_names,
             {.mutability = Mutability::Startup,
              .tier = Tier::Advanced,
              .help = "Colour on the console; auto means when stderr is a terminal and NO_COLOR is unset."});

u32 level_generation() noexcept {
    u32 sum = cv_log_level_all.generation();
    for (const auto* cv : g_category_levels) {
        sum += cv->generation();
    }
    return sum;
}

void compute_levels(u8 (&out)[category_capacity]) noexcept {
    LevelSetting all = cv_log_level_all.value();
    if (all == LevelSetting::Inherit) {  // "default" for log.level.all
        all = mode_default_level;
    }
    for (usize c = 0; c < category_capacity; ++c) {
        LevelSetting s = all;
        if (c < count_of(g_category_levels) && g_category_levels[c]->value() != LevelSetting::Inherit) {
            s = g_category_levels[c]->value();
        }
        out[c] = u8(i32(s) - 1);  // Trace..Fatal = 0..5, Off = 6
    }
}

namespace {

// --log=all:debug,net:trace  ->  log.level.all=debug, log.level.net=trace (later entries win).
bool expand_log_shorthand(cvars::Registry& r, std::string_view value, cvars::Source source,
                          cvars::Writer errors) noexcept {
    bool ok = true;
    while (!value.empty()) {
        const usize comma = value.find(',');
        const std::string_view item = value.substr(0, comma);
        value = comma == std::string_view::npos ? std::string_view{} : value.substr(comma + 1);
        const usize colon = item.find(':');
        if (colon == std::string_view::npos || colon == 0) {
            char msg[160];
            std::snprintf(msg, sizeof(msg), "log: '%.*s' is not category:level (e.g. all:debug,net:trace)",
                          int(item.size()), item.data());
            errors(msg);
            ok = false;
            continue;
        }
        char name[96];
        std::snprintf(name, sizeof(name), "log.level.%.*s", int(colon), item.data());
        if (r.find(name) == nullptr) {
            char msg[160];
            std::snprintf(msg, sizeof(msg), "log: unknown category '%.*s'", int(colon), item.data());
            errors(msg);
            ok = false;
            continue;
        }
        ok = r.stage(name, item.substr(colon + 1), source, errors) && ok;
    }
    return ok;
}

}  // namespace

}  // namespace ez::log::detail

namespace ez::log {

void register_cvars(cvars::Registry& r) noexcept {
    using namespace detail;  // NOLINT(google-build-using-namespace) the module's own detail
    r.add(cv_log_level_all);
    for (auto* cv : g_category_levels) {
        r.add(*cv);
    }
    r.add(cv_log_max_line);
    r.add(cv_log_ring_kib);
    r.add(cv_log_threads_max);
    r.add(cv_log_history_kib);
    r.add(cv_log_drain_ms);
    r.add(cv_log_stderr);
    r.add(cv_log_file);
    r.add(cv_log_file_flush_ms);
    r.add(cv_log_file_keep);
    r.add(cv_log_file_dir);
    r.add(cv_log_sync);
    r.add(cv_log_color);
    r.add_shorthand("log", &expand_log_shorthand);
}

}  // namespace ez::log
