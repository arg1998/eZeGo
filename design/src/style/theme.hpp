// The theme: the tokens mapped onto Dear ImGui's style, and the fonts. Stock widgets pick it up
// through ImGuiStyle; custom widgets read the tokens and fonts directly.
#pragma once

#include <imgui.h>

namespace design::style {

// Once, after ImGui::CreateContext().
void apply();

// What the GPU clears to behind everything ImGui draws.
ImVec4 clear_color();

// The application's typeface, Open Sans. Regular is the default font.
struct Fonts {
    ImFont* regular = nullptr;
    ImFont* bold = nullptr;
};

const Fonts& fonts();

}  // namespace design::style
