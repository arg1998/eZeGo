// The screens. Each one is a complete design of the window's interior, drawn over the whole canvas.
// To add one: write its draw function in its own file, declare it below, add a row to the table in
// screens.cpp. The design tools list the table.
#pragma once

#include "state/state.hpp"

#include <span>

namespace design::screens {

struct Screen {
    const char* id;     // stable: saved in the state
    const char* title;  // shown in the design tools
    void (*draw)(state::State& state);
};

std::span<const Screen> all();

// The screen state.screen names, or the first one.
const Screen& current(const state::State& state);

// Draws the current screen.
void draw(state::State& state);

// Opens a borderless window covering the canvas, for screens that own the whole interior. Always
// pair with end_full_canvas().
void begin_full_canvas(const char* id);
void end_full_canvas();

void draw_tokens(state::State& state);

}  // namespace design::screens
