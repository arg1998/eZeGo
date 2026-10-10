#include "style/theme.hpp"

#include "style/tokens.hpp"

#include <cstdio>

namespace design::style {
namespace {

Fonts g_fonts;

ImVec4 alpha(ImVec4 color, float a) {
    color.w = a;
    return color;
}

// The application's font setup: the same files, size and rasterizer settings.
ImFont* load_font(const char* path) {
    ImFontConfig config;
    config.OversampleH = 3;
    config.OversampleV = 3;
    config.RasterizerMultiply = 1.2f;
    ImFont* font = ImGui::GetIO().Fonts->AddFontFromFileTTF(path, tokens().type.body, &config);
    if (font == nullptr) {
        std::fprintf(stderr, "cannot load the font %s\n", path);
    }
    return font;
}

}  // namespace

void apply() {
    const Tokens& t = tokens();
    const Palette& p = t.color;

    // As the application does: ImGui's defaults and dark colors, then its own values on top.
    ImGuiStyle& style = ImGui::GetStyle();
    style = ImGuiStyle();
    ImGui::StyleColorsDark(&style);

    ImVec4* c = style.Colors;
    c[ImGuiCol_Text] = p.text;
    c[ImGuiCol_TextDisabled] = p.text_muted;
    c[ImGuiCol_WindowBg] = p.surface;
    c[ImGuiCol_ChildBg] = alpha(p.background, 0.0f);
    c[ImGuiCol_PopupBg] = p.popup;
    c[ImGuiCol_Border] = p.border;
    c[ImGuiCol_BorderShadow] = p.shadow;
    c[ImGuiCol_FrameBg] = p.control;
    c[ImGuiCol_FrameBgHovered] = p.control_hovered;
    c[ImGuiCol_FrameBgActive] = p.control_active;
    c[ImGuiCol_TitleBg] = p.background;
    c[ImGuiCol_TitleBgActive] = p.title_active;
    c[ImGuiCol_TitleBgCollapsed] = p.background;
    c[ImGuiCol_MenuBarBg] = p.raised;
    c[ImGuiCol_ScrollbarBg] = p.control;
    c[ImGuiCol_ScrollbarGrab] = p.grab;
    c[ImGuiCol_ScrollbarGrabHovered] = p.grab_hovered;
    c[ImGuiCol_ScrollbarGrabActive] = p.grab_active;
    c[ImGuiCol_CheckMark] = p.accent;
    c[ImGuiCol_SliderGrab] = p.grab;
    c[ImGuiCol_SliderGrabActive] = p.grab_active;
    c[ImGuiCol_Button] = p.control;
    c[ImGuiCol_ButtonHovered] = p.control_hovered;
    c[ImGuiCol_ButtonActive] = p.control_active;
    c[ImGuiCol_Header] = p.recessed;
    c[ImGuiCol_HeaderHovered] = p.recessed_hovered;
    c[ImGuiCol_HeaderActive] = p.header_active;
    c[ImGuiCol_Separator] = p.separator;
    c[ImGuiCol_SeparatorHovered] = p.separator_hovered;
    c[ImGuiCol_SeparatorActive] = p.separator_active;
    c[ImGuiCol_ResizeGrip] = p.separator;
    c[ImGuiCol_ResizeGripHovered] = p.separator_hovered;
    c[ImGuiCol_ResizeGripActive] = p.separator_active;
    c[ImGuiCol_Tab] = p.recessed;
    c[ImGuiCol_TabHovered] = p.raised;
    c[ImGuiCol_TabSelected] = p.tab_selected;
    c[ImGuiCol_TabDimmed] = p.recessed;
    c[ImGuiCol_TabDimmedSelected] = p.raised;
    c[ImGuiCol_DockingPreview] = p.accent;
    c[ImGuiCol_DockingEmptyBg] = p.unset;
    c[ImGuiCol_PlotLines] = p.unset;
    c[ImGuiCol_PlotLinesHovered] = p.unset;
    c[ImGuiCol_PlotHistogram] = p.unset;
    c[ImGuiCol_PlotHistogramHovered] = p.unset;
    c[ImGuiCol_TableHeaderBg] = p.recessed;
    c[ImGuiCol_TableBorderStrong] = p.recessed;
    c[ImGuiCol_TableBorderLight] = p.separator;
    c[ImGuiCol_TableRowBg] = alpha(p.background, 0.0f);
    c[ImGuiCol_TableRowBgAlt] = p.row_alt;
    c[ImGuiCol_TextSelectedBg] = p.control_active;
    c[ImGuiCol_DragDropTarget] = p.accent;
    c[ImGuiCol_NavCursor] = p.unset;
    c[ImGuiCol_NavWindowingHighlight] = alpha(p.unset, 0.70f);
    c[ImGuiCol_NavWindowingDimBg] = alpha(p.unset, 0.20f);
    c[ImGuiCol_ModalWindowDimBg] = alpha(p.unset, 0.35f);

    const Metrics& m = t.metrics;
    style.WindowPadding = m.window_padding;
    style.FramePadding = m.frame_padding;
    style.CellPadding = m.cell_padding;
    style.ItemSpacing = m.item_spacing;
    style.ItemInnerSpacing = m.item_inner_spacing;
    style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
    style.IndentSpacing = m.indent;
    style.ScrollbarSize = m.scrollbar;
    style.GrabMinSize = m.grab_min;
    style.LogSliderDeadzone = m.log_slider_deadzone;
    style.WindowBorderSize = t.border.window;
    style.ChildBorderSize = t.border.child;
    style.PopupBorderSize = t.border.popup;
    style.FrameBorderSize = t.border.frame;
    style.TabBorderSize = t.border.tab;
    style.WindowRounding = t.radius.window;
    style.ChildRounding = t.radius.child;
    style.PopupRounding = t.radius.popup;
    style.FrameRounding = t.radius.frame;
    style.GrabRounding = t.radius.grab;
    style.TabRounding = t.radius.tab;
    style.ScrollbarRounding = t.radius.scrollbar;

    // Embedded in the build at /fonts (CMakeLists.txt). The first font loaded is the default.
    g_fonts.regular = load_font("/fonts/OpenSans-Regular.ttf");
    g_fonts.bold = load_font("/fonts/OpenSans-Bold.ttf");
}

ImVec4 clear_color() {
    return tokens().color.background;
}

const Fonts& fonts() {
    return g_fonts;
}

}  // namespace design::style
