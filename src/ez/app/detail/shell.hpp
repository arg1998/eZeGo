// The placeholder shell: one GLFW window with an OpenGL 4.1 core context and Dear ImGui (docking,
// multi-viewport). Replaced by the window, render and ui modules when they are designed.
#pragma once

namespace ez::app::detail {

bool shell_init() noexcept;  // window, GL context, ImGui, theme, fonts
void shell_shutdown() noexcept;
void shell_poll() noexcept;  // input and window events
[[nodiscard]] bool shell_should_close() noexcept;
void shell_request_close() noexcept;
void shell_begin_frame() noexcept;  // ImGui frame start
void shell_end_frame() noexcept;    // render, extra viewports, swap

}  // namespace ez::app::detail
