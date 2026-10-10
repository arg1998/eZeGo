// The tokens screen: every design token, drawn with itself. The style guide of the visual language.

#include "style/tokens.hpp"

#include "screens/screens.hpp"
#include "style/theme.hpp"

#include <imgui.h>

#include <algorithm>
#include <cstdio>

namespace design::screens {
namespace {

constexpr float swatch_size = 72.0f;
constexpr float swatch_column = 160.0f;

void heading(const char* text) {
    ImGui::Dummy(ImVec2(0.0f, style::tokens().metrics.item_spacing.y * 2.0f));
    ImGui::PushFont(style::fonts().bold, 0.0f);
    ImGui::TextUnformatted(text);
    ImGui::PopFont();
    ImGui::Separator();
}

// Translucent colors are drawn over a checkerboard so their alpha shows.
void checkerboard(ImDrawList* draw, ImVec2 min, ImVec2 max) {
    constexpr float cell = 9.0f;
    draw->AddRectFilled(min, max, IM_COL32(200, 200, 200, 255));
    for (int row = 0; min.y + float(row) * cell < max.y; ++row) {
        for (int col = row % 2; min.x + float(col) * cell < max.x; col += 2) {
            const ImVec2 a(min.x + float(col) * cell, min.y + float(row) * cell);
            draw->AddRectFilled(a, ImVec2(std::min(a.x + cell, max.x), std::min(a.y + cell, max.y)),
                                IM_COL32(150, 150, 150, 255));
        }
    }
}

void swatch(const char* name, const ImVec4& color) {
    const style::Tokens& t = style::tokens();
    ImGui::BeginGroup();
    const ImVec2 min = ImGui::GetCursorScreenPos();
    const ImVec2 max(min.x + swatch_size, min.y + swatch_size);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    if (color.w < 1.0f) {
        checkerboard(draw, min, max);
    }
    draw->AddRectFilled(min, max, ImGui::GetColorU32(color));
    draw->AddRect(min, max, ImGui::GetColorU32(t.color.separator));
    ImGui::Dummy(ImVec2(swatch_size, swatch_size));
    ImGui::TextUnformatted(name);
    ImGui::TextDisabled("#%02x%02x%02x  %d%%", int(color.x * 255.0f + 0.5f), int(color.y * 255.0f + 0.5f),
                        int(color.z * 255.0f + 0.5f), int(color.w * 100.0f + 0.5f));
    ImGui::EndGroup();
}

void palette() {
    const style::Tokens& t = style::tokens();
    const float start = ImGui::GetCursorPosX();
    const int per_row = std::max(1, int(ImGui::GetContentRegionAvail().x / swatch_column));
    int i = 0;
    for (const style::NamedColor& entry : style::named_colors()) {
        if (i % per_row != 0) {
            ImGui::SameLine(start + float(i % per_row) * swatch_column);
        }
        swatch(entry.name, t.color.*entry.value);
        ++i;
    }
}

void row(const char* name, const char* value) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(name);
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(value);
}

void row(const char* name, float value) {
    char text[32];
    std::snprintf(text, sizeof text, "%g", value);
    row(name, text);
}

void row(const char* name, ImVec2 value) {
    char text[48];
    std::snprintf(text, sizeof text, "%g x %g", value.x, value.y);
    row(name, text);
}

bool begin_values(const char* id) {
    return ImGui::BeginTable(id, 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit);
}

void radius_sample(const char* name, float value) {
    const style::Tokens& t = style::tokens();
    constexpr float size = 56.0f;
    ImGui::BeginGroup();
    const ImVec2 min = ImGui::GetCursorScreenPos();
    const ImVec2 max(min.x + size, min.y + size);
    ImGui::GetWindowDrawList()->AddRectFilled(min, max, ImGui::GetColorU32(t.color.raised), value);
    ImGui::GetWindowDrawList()->AddRect(min, max, ImGui::GetColorU32(t.color.separator_active), value);
    ImGui::Dummy(ImVec2(size, size));
    ImGui::Text("%s  %g", name, value);
    ImGui::EndGroup();
}

}  // namespace

void draw_tokens(state::State&) {
    const style::Tokens& t = style::tokens();
    begin_full_canvas("##tokens");

    ImGui::PushFont(style::fonts().bold, 0.0f);
    ImGui::TextUnformatted("Tokens");
    ImGui::PopFont();
    ImGui::TextDisabled("The application's current theme. src/style/tokens.cpp holds the values.");

    heading("Color");
    palette();

    heading("Type");
    ImGui::Text("Open Sans Regular %g px   The quick brown fox jumps over the lazy dog", t.type.body);
    ImGui::PushFont(style::fonts().bold, 0.0f);
    ImGui::Text("Open Sans Bold %g px   The quick brown fox jumps over the lazy dog", t.type.body);
    ImGui::PopFont();

    heading("Metrics");
    if (begin_values("##metrics")) {
        const style::Metrics& m = t.metrics;
        row("window_padding", m.window_padding);
        row("frame_padding", m.frame_padding);
        row("cell_padding", m.cell_padding);
        row("item_spacing", m.item_spacing);
        row("item_inner_spacing", m.item_inner_spacing);
        row("indent", m.indent);
        row("scrollbar", m.scrollbar);
        row("grab_min", m.grab_min);
        row("log_slider_deadzone", m.log_slider_deadzone);
        ImGui::EndTable();
    }

    heading("Borders");
    if (begin_values("##borders")) {
        row("window", t.border.window);
        row("child", t.border.child);
        row("popup", t.border.popup);
        row("frame", t.border.frame);
        row("tab", t.border.tab);
        ImGui::EndTable();
    }

    heading("Radius");
    const float gap = t.metrics.item_spacing.x * 3.0f;
    const struct {
        const char* name;
        float value;
    } radii[] = {
        {"window", t.radius.window},       {"child", t.radius.child}, {"popup", t.radius.popup},
        {"frame", t.radius.frame},         {"grab", t.radius.grab},   {"tab", t.radius.tab},
        {"scrollbar", t.radius.scrollbar},
    };
    bool first = true;
    for (const auto& radius : radii) {
        if (!first) {
            ImGui::SameLine(0.0f, gap);
        }
        first = false;
        radius_sample(radius.name, radius.value);
    }

    end_full_canvas();
}

}  // namespace design::screens
