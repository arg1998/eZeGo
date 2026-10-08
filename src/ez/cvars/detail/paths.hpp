// The per-user settings location (specs/cvars.md §6.4). A stub until the platform module exists.
#pragma once

#include "ez/base/fixed_string.hpp"

namespace ez::cvars::detail {

// <config dir>/settings.cfg: $XDG_CONFIG_HOME/ezego or ~/.config/ezego on Linux,
// ~/Library/Application Support/eZeGo on macOS, %APPDATA%\eZeGo on Windows.
// Returns false when no home directory can be determined.
bool default_settings_path(FixedString<1023>& out) noexcept;

}  // namespace ez::cvars::detail
