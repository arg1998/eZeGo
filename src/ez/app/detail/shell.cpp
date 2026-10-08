// The placeholder shell (see detail/shell.hpp), moved from the prototype's src/application.
#include "ez/app/detail/shell.hpp"

#include "ez/app/detail/paths.hpp"
#include "ez/base/types.hpp"
#include "ez/log/log.hpp"
#include "ez/metrics/profiler.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <cstdio>

namespace ez::app::detail {

namespace {

GLFWwindow* g_window = nullptr;

// The few GL entry points the shell calls itself, loaded through GLFW at runtime: no GL headers or
// GL development package are needed to build (ImGui's backend has its own loader).
using GlViewportFn = void (*)(int, int, int, int);
using GlClearColorFn = void (*)(float, float, float, float);
using GlClearFn = void (*)(unsigned int);
GlViewportFn g_gl_viewport = nullptr;
GlClearColorFn g_gl_clear_color = nullptr;
GlClearFn g_gl_clear = nullptr;
constexpr unsigned int gl_color_buffer_bit = 0x00004000;

void glfw_error(int error, const char* description) {
    EZ_LOG_ERROR(app, "GLFW %d: %s", error, description);
}

void apply_dark_theme() {
    // TODO(Argosta): load from the theme file (application-architecture.md §6).
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.19f, 0.19f, 0.19f, 0.92f);
    colors[ImGuiCol_Border] = ImVec4(0.19f, 0.19f, 0.19f, 0.29f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.24f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);
    colors[ImGuiCol_Button] = ImVec4(0.05f, 0.05f, 0.05f, 0.54f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.19f, 0.19f, 0.19f, 0.54f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 0.00f, 0.00f, 0.36f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.22f, 0.23f, 0.33f);
    colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.44f, 0.44f, 0.44f, 0.29f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.44f, 0.47f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_TabSelected] = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
    colors[ImGuiCol_TabDimmed] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.28f, 0.28f, 0.28f, 0.29f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.20f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.33f, 0.67f, 0.86f, 1.00f);
    colors[ImGuiCol_NavCursor] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.00f, 0.00f, 0.00f, 0.35f);

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(8.00f, 8.00f);
    style.FramePadding = ImVec2(5.00f, 2.00f);
    style.CellPadding = ImVec2(6.00f, 6.00f);
    style.ItemSpacing = ImVec2(6.00f, 6.00f);
    style.ItemInnerSpacing = ImVec2(6.00f, 6.00f);
    style.TouchExtraPadding = ImVec2(0.00f, 0.00f);
    style.IndentSpacing = 25;
    style.ScrollbarSize = 15;
    style.GrabMinSize = 10;
    style.WindowBorderSize = 2;
    style.ChildBorderSize = 1;
    style.PopupBorderSize = 1;
    style.FrameBorderSize = 1;
    style.TabBorderSize = 1;
    style.WindowRounding = 7;
    style.ChildRounding = 4;
    style.FrameRounding = 3;
    style.PopupRounding = 4;
    style.ScrollbarRounding = 9;
    style.GrabRounding = 3;
    style.LogSliderDeadzone = 4;
    style.TabRounding = 4;
}

void load_fonts() {
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig config;
    config.OversampleH = 3;
    config.OversampleV = 3;
    config.RasterizerMultiply = 1.2f;
    // Assets are copied next to the executable by the build (src/apps/ezego/CMakeLists.txt).
    char path[1200];
    std::snprintf(path, sizeof(path), "%s/assets/fonts/OpenSans-Regular.ttf", executable_dir().c_str());
    if (io.Fonts->AddFontFromFileTTF(path, 18.0f, &config) == nullptr) {
        EZ_LOG_WARN(app, "cannot load the font %s; using the default font", path);
    }
}

}  // namespace

bool shell_init() noexcept {
    EZ_PROF_FUNCTION();
    glfwSetErrorCallback(&glfw_error);
    if (glfwInit() == GLFW_FALSE) {
        EZ_LOG_ERROR(app, "cannot initialise GLFW");
        return false;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);  // required on macOS
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
    constexpr int width = 1280;
    constexpr int height = 720;
    g_window = glfwCreateWindow(width, height, "eZeGo", nullptr, nullptr);
    if (g_window == nullptr) {
        EZ_LOG_ERROR(app, "cannot create a window with an OpenGL 4.1 core context");
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(g_window);
    glfwSwapInterval(1);  // vsync
    g_gl_viewport = reinterpret_cast<GlViewportFn>(glfwGetProcAddress("glViewport"));
    g_gl_clear_color = reinterpret_cast<GlClearColorFn>(glfwGetProcAddress("glClearColor"));
    g_gl_clear = reinterpret_cast<GlClearFn>(glfwGetProcAddress("glClear"));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;  // windows outside the main window
    io.IniFilename = nullptr;                            // TODO(Argosta): decide where the UI layout is persisted
    ImGui::StyleColorsDark();
    apply_dark_theme();
    float dpi_scale = 1.0f;
    glfwGetWindowContentScale(g_window, &dpi_scale, nullptr);
    ImGui::GetStyle().ScaleAllSizes(dpi_scale);
    ImGui::GetStyle().FontScaleDpi = dpi_scale;
    ImGui_ImplGlfw_InitForOpenGL(g_window, true);
    ImGui_ImplOpenGL3_Init("#version 410");
    load_fonts();
    EZ_LOG_INFO(app, "GLFW %s, window %dx%d, DPI scale %.2f", glfwGetVersionString(), width, height, double(dpi_scale));
    return true;
}

void shell_shutdown() noexcept {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(g_window);
    g_window = nullptr;
    glfwTerminate();
}

void shell_poll() noexcept {
    EZ_PROF_ZONE("Input");
    glfwPollEvents();
}

bool shell_should_close() noexcept {
    return glfwWindowShouldClose(g_window) == GLFW_TRUE;
}

void shell_request_close() noexcept {
    glfwSetWindowShouldClose(g_window, GLFW_TRUE);
}

void shell_begin_frame() noexcept {
    EZ_PROF_ZONE("Begin Frame");
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void shell_end_frame() noexcept {
    EZ_PROF_ZONE("Render Frame");
    ImGui::Render();
    int w = 0;
    int h = 0;
    glfwGetFramebufferSize(g_window, &w, &h);
    g_gl_viewport(0, 0, w, h);
    g_gl_clear_color(0.0f, 0.0f, 0.0f, 1.0f);
    g_gl_clear(gl_color_buffer_bit);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    // Extra viewports may switch the GL context; restore it.
    if ((ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) != 0) {
        GLFWwindow* context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(context);
    }
    glfwSwapBuffers(g_window);
}

}  // namespace ez::app::detail
