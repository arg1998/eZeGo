#include "ez/app/app.hpp"

#include "ez/app/detail/settings.hpp"
#include "ez/app/detail/shell.hpp"
#include "ez/base/build.hpp"
#include "ez/cvars/registry.hpp"
#include "ez/log/log.hpp"
#include "ez/metrics/profiler.hpp"

#include <imgui.h>

#include <chrono>
#include <cstdio>

namespace ez::app {

namespace {

u64 now_ns() {
    return u64(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
                   .count());
}

// The placeholder content: build information and frame time.
void info_panel(f64 frame_ms, bool& show_demo) {
    ImGui::SetNextWindowSize(ImVec2(460, 0), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("eZeGo")) {
        ImGui::Text("version   %s", build_version);
        ImGui::Text("build     %s", build_mode);
        ImGui::Text("platform  %s %s", build_os, build_arch);
        ImGui::Text("compiler  %s", build_compiler);
        ImGui::Text("asserts   %s", EZ_ASSERTS ? "on" : "off");
        ImGui::Text("profiler  %s",
                    EZ_PROFILER ? (metrics::profiler_connected() ? "Tracy connected" : "waiting for Tracy") : "off");
        ImGui::Separator();
        ImGui::Text("frame     %.2f ms (%.0f FPS)", frame_ms, frame_ms > 0 ? 1000.0 / frame_ms : 0.0);
        if (detail::cv_app_developer.value()) {
            ImGui::Checkbox("Dear ImGui demo", &show_demo);
        }
    }
    ImGui::End();
    if (show_demo) {
        ImGui::ShowDemoWindow(&show_demo);
    }
}

}  // namespace

const char* version_text() noexcept {
    static char g_text[256];
    if (g_text[0] == '\0') {
        std::snprintf(g_text, sizeof(g_text), "eZeGo %s (%s, %s %s, %s)", build_version, build_mode, build_os,
                      build_arch, build_compiler);
    }
    return g_text;
}

int run(cvars::Registry& registry) noexcept {
    if (!detail::shell_init()) {
        return 1;
    }
    const f64 quit_after_s = detail::cv_app_quit_after_s.value();
    const i64 quit_after_frames = detail::cv_app_quit_after_frames.value();
    const u64 start = now_ns();
    u64 last = start;
    u32 frame = 0;
    f64 frame_ms_smoothed = 0;
    bool show_demo = false;

    while (!detail::shell_should_close()) {
        EZ_PROF_ZONE("Main Loop");
        registry.apply_pending();  // cvar writes land at the frame boundary (cvars.md CV-5)
        log::set_frame(frame);
        const u64 now = now_ns();
        const f64 frame_ms = f64(now - last) / 1.0e6;
        last = now;
        frame_ms_smoothed += (frame_ms - frame_ms_smoothed) * 0.05;
        EZ_PROF_PLOT("frame ms", frame_ms);

        if ((quit_after_s > 0 && f64(now - start) / 1.0e9 > quit_after_s) ||
            (quit_after_frames > 0 && frame >= quit_after_frames)) {
            detail::shell_request_close();
        }
        detail::shell_poll();
        detail::shell_begin_frame();
        info_panel(frame_ms_smoothed, show_demo);
        detail::shell_end_frame();
        EZ_PROF_FRAME();
        ++frame;
    }
    EZ_LOG_INFO(app, "closing after %u frames", frame);
    detail::shell_shutdown();
    return 0;
}

}  // namespace ez::app
