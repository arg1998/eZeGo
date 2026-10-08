// The few OS services the logger needs (specs/logging.md §11). A stub layer until the platform
// module exists; then these move there.
#pragma once

#include "ez/base/fixed_string.hpp"
#include "ez/base/types.hpp"

namespace ez::log::detail {

// Prepares stderr: on Windows attaches to the parent console of a GUI process and enables escape
// sequences. Returns whether stderr is an interactive terminal.
bool prepare_stderr() noexcept;
// Writes all bytes to stderr, unbuffered.
void write_stderr(const char* data, usize length) noexcept;
bool debugger_attached() noexcept;
// Windows: OutputDebugString when a debugger is attached; elsewhere nothing.
void debug_output(const char* text) noexcept;
void set_thread_name(const char* name) noexcept;
// The per-user directory for log files: $XDG_STATE_HOME/ezego/logs (Linux),
// ~/Library/Logs/eZeGo (macOS), %LOCALAPPDATA%\eZeGo\logs (Windows).
bool default_log_dir(FixedString<1023>& out) noexcept;

struct LocalTime {
    i32 year, month, day, hour, minute, second;
};
LocalTime local_time(i64 unix_seconds) noexcept;

}  // namespace ez::log::detail
