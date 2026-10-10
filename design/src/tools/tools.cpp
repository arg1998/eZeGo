#include "tools/tools.hpp"

#include "screens/screens.hpp"

#include <imgui.h>

namespace design::tools {
namespace {

// While the tools window is closed, a small button in the top-right corner brings it back, so
// closing it never depends on knowing the shortcut.
void draw_reopen_button(state::State& state) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - 8.0f, viewport->WorkPos.y + 8.0f),
                            ImGuiCond_Always, ImVec2(1.0f, 0.0f));
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
                                       ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                                       ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoFocusOnAppearing |
                                       ImGuiWindowFlags_NoNav;
    if (ImGui::Begin("##design_tools_reopen", nullptr, flags)) {
        if (ImGui::SmallButton("Design tools")) {
            state.tools_open = true;
        }
        ImGui::SetItemTooltip("Or press ` (the key left of 1)");
    }
    ImGui::End();
}

}  // namespace

void draw(state::State& state) {
    const ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsKeyPressed(ImGuiKey_GraveAccent, false) && !io.WantTextInput) {
        state.tools_open = !state.tools_open;
    }

    if (state.tools_open) {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - 16.0f, viewport->WorkPos.y + 16.0f),
                                ImGuiCond_FirstUseEver, ImVec2(1.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(260.0f, 0.0f), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Design tools", &state.tools_open, ImGuiWindowFlags_NoDocking)) {
            ImGui::SeparatorText("Screens");
            const screens::Screen& shown = screens::current(state);
            for (const screens::Screen& screen : screens::all()) {
                if (ImGui::Selectable(screen.title, &screen == &shown)) {
                    state.screen = screen.id;
                }
            }

            ImGui::SeparatorText("Dear ImGui");
            ImGui::Checkbox("Demo window", &state.show_imgui_demo);
            ImGui::Checkbox("Metrics and debugger", &state.show_imgui_metrics);
            ImGui::Checkbox("Style editor", &state.show_style_editor);

            ImGui::Spacing();
            ImGui::TextDisabled("%.1f ms per frame", 1000.0f / io.Framerate);
            ImGui::TextDisabled("` shows and hides this window");
        }
        ImGui::End();
    } else {
        draw_reopen_button(state);
    }

    if (state.show_imgui_demo) {
        ImGui::ShowDemoWindow(&state.show_imgui_demo);
    }
    if (state.show_imgui_metrics) {
        ImGui::ShowMetricsWindow(&state.show_imgui_metrics);
    }
    if (state.show_style_editor) {
        ImGui::Begin("Style editor", &state.show_style_editor);
        ImGui::TextDisabled("Edits last until the next reload; the tokens are the source.");
        ImGui::ShowStyleEditor();
        ImGui::End();
    }
}

}  // namespace design::tools
