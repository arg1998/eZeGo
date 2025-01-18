#include "application.hpp"
#include "core/logger/logger.hpp"
#include "application/fonts/Fonts.hpp"

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>
#include <GLFW/glfw3.h>
#include <tracy/Tracy.hpp>
#include <filesystem>

namespace fs = std::filesystem;

static void glfw_error_callback(int error, const char* description) {
    EZ_LOG_TRACE();
    EZ_LOG_ERROR("GLFW[%d]: %s", error, description);
}

static ezWindow mainWindow;

void applicationEnableDarkTheme() {
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
    colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.20f, 0.20f, 0.36f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.00f, 0.00f, 0.00f, 0.52f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
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
    colors[ImGuiCol_NavHighlight] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
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

void initApplication() {
    EZ_LOG_TRACE();
    mainWindow.height = 720;
    mainWindow.width = 1280;
    mainWindow.windowTitle = "eZeGo";
    mainWindow.parentWindow = nullptr;
    mainWindow.childWindow = nullptr;
    mainWindow.state = ezWindowState::EZ_WINDOW_DYNAMIC;

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        EZ_LOG_FATAL("Failed to initialize GLFW");
    }
    const char* GLSL_VERSION = "#version 460";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

    mainWindow.window = glfwCreateWindow(mainWindow.width, mainWindow.height, mainWindow.windowTitle.c_str(), NULL, NULL);

    glfwMakeContextCurrent(mainWindow.window);
    glfwSwapInterval(0);  // Enable vsync

    IMGUI_CHECKVERSION();
    ImGuiContext* imgui_ctx = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // for docking

    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;          // for multi-viewport support (rendering outside of the window frame)
    io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;  // for

    ImGui::StyleColorsDark();
    applicationEnableDarkTheme();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(mainWindow.window, true);
    ImGui_ImplOpenGL3_Init(GLSL_VERSION);
}

void applicationBeginFrame() {
    ZoneScopedN("Begin Frame Generation");
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}
void applicationEndFrame() {
    ZoneScopedN("End Frame Generation");
    int display_w, display_h;
    const ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.00f);
    glfwGetFramebufferSize(mainWindow.window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
}

void applicationRenderFrame() {
    ZoneScopedN("Render Frame");
    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Multi-Viewport support (rendering outside the window frames)
    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
    //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
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

void applicationProcessInput() {
    ZoneScopedN("Input Processing");
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
    const f32 dpi_scale = io.DisplayFramebufferScale.y;
    f32 font_size_px = 18.0f * dpi_scale;
    const f32 icon_font_size = font_size_px;
    
    
    ImFontConfig main_font_config;
    main_font_config.OversampleH = 3;           
    main_font_config.OversampleV = 3;           
    main_font_config.RasterizerMultiply = 1.2f;
    std::string main_font_path = (fs::current_path().parent_path() / "assets" / "fonts" / "OpenSans-Regular.ttf").u8string();
    ImFont *main_font = io.Fonts->AddFontFromFileTTF(main_font_path.c_str(), font_size_px, &main_font_config, io.Fonts->GetGlyphRangesDefault());



    ImFontConfig icon_font_config;
    icon_font_config.MergeMode = true;
    icon_font_config.PixelSnapH = true;
    icon_font_config.GlyphMinAdvanceX = icon_font_size;
    icon_font_config.GlyphMaxAdvanceX = icon_font_size;
    icon_font_config.GlyphOffset.y = font_size_px * 0.25; //FIXME(Arogosta): fix this magic number later
    const ImWchar icon_glyph_range_md[] = {ICON_MIN_MD, ICON_MAX_16_MD, 0};

    std::string icon_font_path_regular = (fs::current_path().parent_path() / "assets" / "fonts" / FONT_ICON_FILE_NAME_MD).u8string();
    ImFont *icon_font_regular = io.Fonts->AddFontFromFileTTF(icon_font_path_regular.c_str(), icon_font_size, &icon_font_config, icon_glyph_range_md);
    if(icon_font_regular == nullptr){
        EZ_LOG_ERROR("Failed to load \"%s\" icon font", icon_font_path_regular.c_str());
    }


    const ImWchar icon_glyph_range_lce[] = {ICON_MIN_LC, ICON_MAX_LC, 0};
    std::string icon_font_path_solid = (fs::current_path().parent_path() / "assets" / "fonts" / FONT_ICON_FILE_NAME_LC).u8string();
    ImFont *icon_font_solid = io.Fonts->AddFontFromFileTTF(icon_font_path_solid.c_str(), icon_font_size, &icon_font_config, icon_glyph_range_lce);
    if(icon_font_solid == nullptr){
        EZ_LOG_ERROR("Failed to load \"%s\" icon font", icon_font_path_solid.c_str());
    }

    io.Fonts->Build();
}
