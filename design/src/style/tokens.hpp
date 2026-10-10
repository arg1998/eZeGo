// Design tokens: the named values every widget and screen draws with. Change a value here and
// everything restyles on the next reload. The values are the application's current theme
// (apply_dark_theme in the app's shell, copied 2026-10-09); where several ImGui slots share a value,
// they share a token.
#pragma once

#include <imgui.h>

#include <span>

namespace design::style {

// Colors carry their alpha: many of the application's slots are translucent.
struct Palette {
    ImVec4 background;    // the GPU clear behind everything, and inactive title bars
    ImVec4 surface;       // window background
    ImVec4 title_active;  // the focused window's title bar
    ImVec4 raised;        // menu bar, hovered tab, dimmed selected tab
    ImVec4 popup;         // popups and tooltips
    ImVec4 text;
    ImVec4 text_muted;  // disabled text
    ImVec4 border;
    ImVec4 shadow;   // border shadow
    ImVec4 control;  // frames, buttons, scrollbar track
    ImVec4 control_hovered;
    ImVec4 control_active;  // also selected text
    ImVec4 grab;            // scrollbar and slider grabs
    ImVec4 grab_hovered;
    ImVec4 grab_active;
    ImVec4 recessed;  // headers, tabs, table header, strong table borders
    ImVec4 recessed_hovered;
    ImVec4 header_active;
    ImVec4 tab_selected;
    ImVec4 separator;  // separators, resize grips, light table borders
    ImVec4 separator_hovered;
    ImVec4 separator_active;
    ImVec4 accent;   // check marks, docking preview, drag-and-drop target
    ImVec4 row_alt;  // alternate table rows
    ImVec4 unset;    // pure red: slots the application has not chosen a color for yet
};

// Distances in logical pixels (CSS pixels in the browser).
struct Metrics {
    ImVec2 window_padding;
    ImVec2 frame_padding;
    ImVec2 cell_padding;
    ImVec2 item_spacing;
    ImVec2 item_inner_spacing;
    float indent;
    float scrollbar;
    float grab_min;
    float log_slider_deadzone;
};

struct Borders {
    float window, child, popup, frame, tab;
};

struct Radius {
    float window, child, popup, frame, grab, tab, scrollbar;
};

// Font sizes in logical pixels.
struct Type {
    float body;
};

struct Tokens {
    Palette color;
    Metrics metrics;
    Borders border;
    Radius radius;
    Type type;
};

const Tokens& tokens();

// The palette by name, for screens that list it.
struct NamedColor {
    const char* name;
    ImVec4 Palette::*value;
};

std::span<const NamedColor> named_colors();

}  // namespace design::style
