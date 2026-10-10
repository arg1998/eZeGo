// The design's state: one struct that screens and tools read and write, passed down from main.cpp.
// Today it holds view state; mock show data (fixtures, cues, ...) joins it when a screen needs it.
// Any change is saved with ImGui's settings under [Design][State], so it survives reloads.
#pragma once

#include <string>

namespace design::state {

struct State {
    std::string screen;               // id of the screen on the canvas; unknown or empty means the first
    bool tools_open = true;           // the design tools window (` toggles it)
    bool show_imgui_demo = false;     // Dear ImGui's demo: every stock widget, as a reference
    bool show_imgui_metrics = false;  // Dear ImGui's metrics and debugger
    bool show_style_editor = false;   // live style edits; not saved, the tokens are the source

    bool operator==(const State&) const = default;
};

// Registers the [Design][State] settings section. Call before host::load().
void register_settings(State& state);

}  // namespace design::state
