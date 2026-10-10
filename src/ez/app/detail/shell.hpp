// The placeholder shell: one borderless SDL3 window with an OpenGL 4.1 core context, Dear ImGui and
// eZeGo's own title bar (specs/windowing.md §12). Replaced by the window, render and ui modules when
// those are built (windowing.md §3).
#pragma once

namespace ez::app::detail {

bool shell_init() noexcept;  // window, GL context, ImGui, theme, fonts
void shell_shutdown() noexcept;
void shell_poll() noexcept;  // input and window events
[[nodiscard]] bool shell_should_close() noexcept;
void shell_request_close() noexcept;
void shell_begin_frame() noexcept;  // ImGui frame start, then the title bar
void shell_end_frame() noexcept;    // render and swap

}  // namespace ez::app::detail
