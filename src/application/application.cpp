#include "application.hpp"
#include "core/logger/logger.hpp"
#include "core/platform/platform.hpp"
#include "core/profiler/profiler.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <cstdio>

static void glfw_error_callback(int error, const char* description) {
    EZ_LOG_ERROR("GLFW[%d]: %s", error, description);
}

static ezWindow mainWindow;

// The few GL entry points the application calls itself, loaded through GLFW at runtime: no GL
// headers or GL development package are needed to build (ImGui's backend has its own loader).
using PfnGlViewport   = void (*)(int, int, int, int);
using PfnGlClearColor = void (*)(float, float, float, float);
using PfnGlClear      = void (*)(unsigned int);
static PfnGlViewport   glViewportFn;
static PfnGlClearColor glClearColorFn;
static PfnGlClear      glClearFn;
static constexpr unsigned int EZ_GL_COLOR_BUFFER_BIT = 0x00004000;

static void applicationEnableDarkTheme() {
    EZ_LOG_TRACE();
    // TODO(Argosta): read this from a file
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

b8 initApplication() {
    EZ_LOG_TRACE();
    EZ_PROFILE_FUNCTION();
    mainWindow.height = 720;
    mainWindow.width = 1280;
    mainWindow.windowTitle = "eZeGo";
    mainWindow.parentWindow = nullptr;
    mainWindow.childWindow = nullptr;
    mainWindow.state = ezWindowState::EZ_WINDOW_DYNAMIC;

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        EZ_LOG_ERROR("Failed to initialize GLFW");
        return false;
    }
    const char* GLSL_VERSION = "#version 410";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);   // required on macOS
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

    mainWindow.window = glfwCreateWindow((int)mainWindow.width, (int)mainWindow.height, mainWindow.windowTitle, NULL, NULL);
    if (!mainWindow.window) {
        EZ_LOG_ERROR("Failed to create a window with an OpenGL 4.1 core context");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(mainWindow.window);
    glfwSwapInterval(1);  // Enable vsync
    glViewportFn   = (PfnGlViewport)glfwGetProcAddress("glViewport");
    glClearColorFn = (PfnGlClearColor)glfwGetProcAddress("glClearColor");
    glClearFn      = (PfnGlClear)glfwGetProcAddress("glClear");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;    // for docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;  // for multi-viewport support (rendering outside of the window frame)
    io.IniFilename = nullptr;                            // TODO(Argosta): decide where UI layout is persisted

    ImGui::StyleColorsDark();
    applicationEnableDarkTheme();
    f32 dpi_scale = 1.0f;
    glfwGetWindowContentScale(mainWindow.window, &dpi_scale, nullptr);
    ImGui::GetStyle().ScaleAllSizes(dpi_scale);
    ImGui::GetStyle().FontScaleDpi = dpi_scale;

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(mainWindow.window, true);
    ImGui_ImplOpenGL3_Init(GLSL_VERSION);

    EZ_LOG_INFO("GLFW %s, window %ux%u, DPI scale %.2f", glfwGetVersionString(), mainWindow.width, mainWindow.height, dpi_scale);
    return true;
}

void shutdownApplication() {
    EZ_LOG_TRACE();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(mainWindow.window);
    glfwTerminate();
}

void applicationBeginFrame() {
    EZ_PROFILE_ZONE("Begin Frame Generation");
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void applicationRenderFrame() {
    EZ_PROFILE_ZONE("Render Frame");
    ImGui::Render();

    int display_w, display_h;
    glfwGetFramebufferSize(mainWindow.window, &display_w, &display_h);
    glViewportFn(0, 0, display_w, display_h);
    glClearColorFn(0.0f, 0.0f, 0.0f, 1.0f);
    glClearFn(EZ_GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Multi-Viewport support (rendering outside the window frames)
    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we save/restore it.)
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }

    glfwSwapBuffers(mainWindow.window);
}

bool shouldApplicationClose() {
    return mainWindow.state == ezWindowState::EZ_WINDOW_CLOSED;
}

void applicationRequestClose() {
    glfwSetWindowShouldClose(mainWindow.window, GLFW_TRUE);
}

void applicationProcessInput() {
    EZ_PROFILE_ZONE("Input Processing");
    if (glfwWindowShouldClose(mainWindow.window)) {
        // TODO(Argosta): prompt user if they want to close the application
        // to do this, uncoment the line bellow and check the window state in the
        // rendering part to show the closing prompt

        // mainWindow.state = ezWindowState::EZ_WINDOW_USER_CLOSING;

        mainWindow.state = ezWindowState::EZ_WINDOW_CLOSED;
    }
    glfwPollEvents();
}

void applicationLoadFonts() {
    EZ_LOG_TRACE();
    ImGuiIO& io = ImGui::GetIO();

    ImFontConfig main_font_config;
    main_font_config.OversampleH = 3;
    main_font_config.OversampleV = 3;
    main_font_config.RasterizerMultiply = 1.2f;

    // Assets are copied next to the executable by the build (src/application/CMakeLists.txt).
    char main_font_path[1200];
    snprintf(main_font_path, sizeof(main_font_path), "%s/assets/fonts/OpenSans-Regular.ttf", platformGetExecutableDir());
    if (!io.Fonts->AddFontFromFileTTF(main_font_path, 18.0f, &main_font_config)) {
        EZ_LOG_WARN("Failed to load \"%s\"; using the default font", main_font_path);
    }
}
