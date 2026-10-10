// The application module (specs/code-organization.md §4, layer 3). Today it holds the main loop
// and a placeholder shell: one SDL3 window with eZeGo's own title bar, Dear ImGui and an information
// panel. The shell is replaced by the window, render and ui modules when those are built
// (windowing.md §3); until then it is the one place that calls SDL3 and ImGui directly.
#pragma once

namespace ez::cvars {  // NOLINT(ez-namespace) forward declaration of a dependency's type
class Registry;
}  // namespace ez::cvars

namespace ez::app {

// app.quit_after_s, app.quit_after_frames, app.developer.
void register_cvars(cvars::Registry& registry) noexcept;

// Opens the window and runs the frame loop until the window closes or a quit cvar fires. Applies
// pending cvar writes at the start of every frame. Returns the process exit code.
int run(cvars::Registry& registry) noexcept;

// The line --version prints: "eZeGo <version> (<mode>, <os> <arch>, <compiler>)".
const char* version_text() noexcept;

}  // namespace ez::app
