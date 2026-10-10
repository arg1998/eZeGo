// Keeps Dear ImGui's settings in the browser's localStorage: window placement, docking, and the
// design state's own section (state/state.hpp). A reload after a rebuild comes back where it was.
#pragma once

namespace design::host {

// Once at startup, after every settings handler is registered and before the first frame.
void load();

// Every frame after ImGui::Render(): saves when ImGui reports a change. The page also saves on
// unload, so a reload never loses the last second.
void save_if_dirty();

}  // namespace design::host
