#include "screens/screens.hpp"

#include <imgui.h>

namespace design::screens {
namespace {

constexpr Screen table[] = {
    {"tokens", "Tokens", draw_tokens},
};

}  // namespace

std::span<const Screen> all() {
    return table;
}

const Screen& current(const state::State& state) {
    for (const Screen& screen : table) {
        if (state.screen == screen.id) {
            return screen;
        }
    }
    return table[0];
}

void draw(state::State& state) {
    current(state).draw(state);
}

void begin_full_canvas(const char* id) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                                       ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                       ImGuiWindowFlags_NoDocking;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin(id, nullptr, flags);
    ImGui::PopStyleVar(2);
}

void end_full_canvas() {
    ImGui::End();
}

}  // namespace design::screens
