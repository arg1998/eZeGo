#include "style/tokens.hpp"

namespace design::style {
namespace {

// The application's theme, value for value.
constexpr Tokens app_tokens = {
    .color =
        {
            .background = ImVec4(0.00f, 0.00f, 0.00f, 1.00f),
            .surface = ImVec4(0.10f, 0.10f, 0.10f, 1.00f),
            .title_active = ImVec4(0.06f, 0.06f, 0.06f, 1.00f),
            .raised = ImVec4(0.14f, 0.14f, 0.14f, 1.00f),
            .popup = ImVec4(0.19f, 0.19f, 0.19f, 0.92f),
            .text = ImVec4(1.00f, 1.00f, 1.00f, 1.00f),
            .text_muted = ImVec4(0.50f, 0.50f, 0.50f, 1.00f),
            .border = ImVec4(0.19f, 0.19f, 0.19f, 0.29f),
            .shadow = ImVec4(0.00f, 0.00f, 0.00f, 0.24f),
            .control = ImVec4(0.05f, 0.05f, 0.05f, 0.54f),
            .control_hovered = ImVec4(0.19f, 0.19f, 0.19f, 0.54f),
            .control_active = ImVec4(0.20f, 0.22f, 0.23f, 1.00f),
            .grab = ImVec4(0.34f, 0.34f, 0.34f, 0.54f),
            .grab_hovered = ImVec4(0.40f, 0.40f, 0.40f, 0.54f),
            .grab_active = ImVec4(0.56f, 0.56f, 0.56f, 0.54f),
            .recessed = ImVec4(0.00f, 0.00f, 0.00f, 0.52f),
            .recessed_hovered = ImVec4(0.00f, 0.00f, 0.00f, 0.36f),
            .header_active = ImVec4(0.20f, 0.22f, 0.23f, 0.33f),
            .tab_selected = ImVec4(0.20f, 0.20f, 0.20f, 0.36f),
            .separator = ImVec4(0.28f, 0.28f, 0.28f, 0.29f),
            .separator_hovered = ImVec4(0.44f, 0.44f, 0.44f, 0.29f),
            .separator_active = ImVec4(0.40f, 0.44f, 0.47f, 1.00f),
            .accent = ImVec4(0.33f, 0.67f, 0.86f, 1.00f),
            .row_alt = ImVec4(1.00f, 1.00f, 1.00f, 0.06f),
            .unset = ImVec4(1.00f, 0.00f, 0.00f, 1.00f),
        },
    .metrics =
        {
            .window_padding = ImVec2(8.0f, 8.0f),
            .frame_padding = ImVec2(5.0f, 2.0f),
            .cell_padding = ImVec2(6.0f, 6.0f),
            .item_spacing = ImVec2(6.0f, 6.0f),
            .item_inner_spacing = ImVec2(6.0f, 6.0f),
            .indent = 25.0f,
            .scrollbar = 15.0f,
            .grab_min = 10.0f,
            .log_slider_deadzone = 4.0f,
        },
    .border = {.window = 2.0f, .child = 1.0f, .popup = 1.0f, .frame = 1.0f, .tab = 1.0f},
    .radius =
        {.window = 7.0f, .child = 4.0f, .popup = 4.0f, .frame = 3.0f, .grab = 3.0f, .tab = 4.0f, .scrollbar = 9.0f},
    .type = {.body = 18.0f},
};

constexpr NamedColor palette_names[] = {
    {"background", &Palette::background},
    {"surface", &Palette::surface},
    {"title_active", &Palette::title_active},
    {"raised", &Palette::raised},
    {"popup", &Palette::popup},
    {"text", &Palette::text},
    {"text_muted", &Palette::text_muted},
    {"border", &Palette::border},
    {"shadow", &Palette::shadow},
    {"control", &Palette::control},
    {"control_hovered", &Palette::control_hovered},
    {"control_active", &Palette::control_active},
    {"grab", &Palette::grab},
    {"grab_hovered", &Palette::grab_hovered},
    {"grab_active", &Palette::grab_active},
    {"recessed", &Palette::recessed},
    {"recessed_hovered", &Palette::recessed_hovered},
    {"header_active", &Palette::header_active},
    {"tab_selected", &Palette::tab_selected},
    {"separator", &Palette::separator},
    {"separator_hovered", &Palette::separator_hovered},
    {"separator_active", &Palette::separator_active},
    {"accent", &Palette::accent},
    {"row_alt", &Palette::row_alt},
    {"unset", &Palette::unset},
};

}  // namespace

const Tokens& tokens() {
    return app_tokens;
}

std::span<const NamedColor> named_colors() {
    return palette_names;
}

}  // namespace design::style
