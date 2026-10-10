#include "state/state.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <cstdio>
#include <cstring>

namespace design::state {
namespace {

// The saved fields, one key per line. A new bool is one row here.
struct Flag {
    const char* key;
    bool State::*field;
};

constexpr Flag flags[] = {
    {"tools", &State::tools_open},
    {"imgui_demo", &State::show_imgui_demo},
    {"imgui_metrics", &State::show_imgui_metrics},
    {"style_editor", &State::show_style_editor},
};

void* read_open(ImGuiContext*, ImGuiSettingsHandler* handler, const char* name) {
    return std::strcmp(name, "State") == 0 ? handler->UserData : nullptr;
}

void read_line(ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line) {
    State& state = *static_cast<State*>(entry);
    char text[64];
    if (std::sscanf(line, "screen=%63s", text) == 1) {
        state.screen = text;
        return;
    }
    for (const Flag& flag : flags) {
        const std::size_t length = std::strlen(flag.key);
        if (std::strncmp(line, flag.key, length) == 0 && line[length] == '=') {
            state.*flag.field = line[length + 1] == '1';
        }
    }
}

void write_all(ImGuiContext*, ImGuiSettingsHandler* handler, ImGuiTextBuffer* out) {
    const State& state = *static_cast<const State*>(handler->UserData);
    out->appendf("[%s][State]\n", handler->TypeName);
    out->appendf("screen=%s\n", state.screen.c_str());
    for (const Flag& flag : flags) {
        out->appendf("%s=%d\n", flag.key, state.*flag.field ? 1 : 0);
    }
    out->append("\n");
}

}  // namespace

void register_settings(State& state) {
    ImGuiSettingsHandler handler;
    handler.TypeName = "Design";
    handler.TypeHash = ImHashStr("Design");
    handler.ReadOpenFn = read_open;
    handler.ReadLineFn = read_line;
    handler.WriteAllFn = write_all;
    handler.UserData = &state;
    ImGui::AddSettingsHandler(&handler);
}

}  // namespace design::state
