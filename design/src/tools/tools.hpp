// The design tools: a window for moving between screens and opening Dear ImGui's own inspectors.
// The ` key toggles it. It is a workbench, not part of any design.
#pragma once

#include "state/state.hpp"

namespace design::tools {

void draw(state::State& state);

}  // namespace design::tools
