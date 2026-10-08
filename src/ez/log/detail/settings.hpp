// The logger's cvars (specs/logging.md §6), shared between log_cvars.cpp and log.cpp.
#pragma once

#include "ez/cvars/cvar.hpp"
#include "ez/log/log.hpp"

namespace ez::log::detail {

// A category's runtime level. Inherit falls back to log.level.all; Off silences everything but Fatal.
enum class LevelSetting : i32 { Inherit, Trace, Debug, Info, Warn, Error, Fatal, Off };
enum class Toggle : i32 { Auto, On, Off };

extern cvars::CVarEnum<LevelSetting> cv_log_level_all;
extern cvars::CVar<i32> cv_log_max_line;
extern cvars::CVar<i32> cv_log_ring_kib;
extern cvars::CVar<i32> cv_log_threads_max;
extern cvars::CVar<i32> cv_log_history_kib;
extern cvars::CVar<i32> cv_log_drain_ms;
extern cvars::CVar<bool> cv_log_stderr;
extern cvars::CVar<bool> cv_log_file;
extern cvars::CVar<i32> cv_log_file_flush_ms;
extern cvars::CVar<i32> cv_log_file_keep;
extern cvars::CVarString cv_log_file_dir;
extern cvars::CVarEnum<Toggle> cv_log_sync;
extern cvars::CVarEnum<Toggle> cv_log_color;

// The hard cap on a line (build time); log.max_line may be raised up to it.
inline constexpr i32 max_line_cap = 4096;

// Sum of every level cvar's generation: changes whenever any level changes.
u32 level_generation() noexcept;
// The effective level of every category, as stored in g_levels (Off = 6).
void compute_levels(u8 (&out)[category_capacity]) noexcept;

}  // namespace ez::log::detail
