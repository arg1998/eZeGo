// The host: an SDL3 window on the page's <canvas>, a WebGL2 context, Dear ImGui, and the frame loop
// the browser drives. What the canvas shows is drawn by screens/ and tools/.

#include "host/persist.hpp"
#include "screens/screens.hpp"
#include "state/state.hpp"
#include "style/theme.hpp"
#include "tools/tools.hpp"

#include <GLES3/gl3.h>
#include <SDL3/SDL.h>
#include <emscripten/emscripten.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#include <imgui_internal.h>

#include <cstdio>

namespace {

SDL_Window* g_window = nullptr;
design::state::State g_state;

void frame(void*) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    const design::state::State before = g_state;
    design::screens::draw(g_state);
    design::tools::draw(g_state);
    if (g_state != before) {
        ImGui::MarkIniSettingsDirty();
    }

    ImGui::Render();
    design::host::save_if_dirty();

    const ImGuiIO& io = ImGui::GetIO();
    const ImVec4 clear = design::style::clear_color();
    glViewport(0, 0, int(io.DisplaySize.x * io.DisplayFramebufferScale.x),
               int(io.DisplaySize.y * io.DisplayFramebufferScale.y));
    glClearColor(clear.x, clear.y, clear.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(g_window);
}

}  // namespace

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    // WebGL2 is OpenGL ES 3.0.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    // The page sizes the canvas with CSS; SDL follows that size and keeps the pixel buffer at the
    // display's density.
    g_window = SDL_CreateWindow("eZeGo design", 1280, 800,
                                SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (g_window == nullptr) {
        std::fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        return 1;
    }
    SDL_GLContext gl = SDL_GL_CreateContext(g_window);
    if (gl == nullptr) {
        std::fprintf(stderr, "SDL_GL_CreateContext: %s\n", SDL_GetError());
        return 1;
    }
    SDL_GL_MakeCurrent(g_window, gl);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
    io.IniFilename = nullptr;  // kept in the browser's storage by host/persist instead of a file
    io.IniSavingRate = 1.0f;

    design::style::apply();
    design::state::register_settings(g_state);
    design::host::load();

    ImGui_ImplSDL3_InitForOpenGL(g_window, gl);
    ImGui_ImplOpenGL3_Init("#version 300 es");

    // requestAnimationFrame drives frame(); main() never returns.
    emscripten_set_main_loop_arg(frame, nullptr, 0, true);
    return 0;
}
