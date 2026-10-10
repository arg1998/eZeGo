// The placeholder shell (see detail/shell.hpp): one borderless SDL3 window with an OpenGL 4.1 core
// context, Dear ImGui through its SDL3 and OpenGL 3 backends, and eZeGo's own title bar
// (specs/windowing.md W-2, §4, §12). The OS still moves, snaps and resizes the window: the title
// bar is declared to it through SDL's hit test.
#include "ez/app/detail/shell.hpp"

#include "ez/app/detail/chrome.hpp"
#include "ez/app/detail/paths.hpp"
#include "ez/base/build.hpp"
#include "ez/base/types.hpp"
#include "ez/log/log.hpp"
#include "ez/metrics/profiler.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#include <imgui_internal.h>  // the window list: panels drawn over the title bar must keep their clicks

#include <cstdio>
#include <cstring>

namespace ez::app::detail {

namespace {

SDL_Window* g_window = nullptr;
SDL_GLContext g_gl_context = nullptr;
bool g_should_close = false;
f32 g_scale = 1.0f;  // the display's content scale; title bar sizes below are multiplied by it
ChromeMap g_chrome;  // written every frame, read by the OS hit test between frames
ImGuiWindow* g_title_bar = nullptr;
bool g_double_click_from_hit_test = false;  // X11, Wayland: the press goes to the window manager, not to us
// A caption press the window manager took arms a double-click at its position (see on_caption_press).
u64 g_armed_ns = 0;
f32 g_armed_x = 0;
f32 g_armed_y = 0;
u64 g_double_click_ns = 300'000'000;  // from ImGui's double-click time at init

// Title bar metrics in points at scale 1. The look is a placeholder until the UX designs exist.
constexpr f32 title_bar_height = 34.0f;
constexpr f32 button_width = 46.0f;
constexpr f32 resize_border = 6.0f;
constexpr f32 resize_corner = 14.0f;
constexpr int min_width = 480;
constexpr int min_height = 320;

// The few GL entry points the shell calls itself, loaded through SDL at runtime: no GL headers or
// GL development package are needed to build (ImGui's backend has its own loader).
using GlViewportFn = void (*)(int, int, int, int);
using GlClearColorFn = void (*)(float, float, float, float);
using GlClearFn = void (*)(unsigned int);
GlViewportFn g_gl_viewport = nullptr;
GlClearColorFn g_gl_clear_color = nullptr;
GlClearFn g_gl_clear = nullptr;
constexpr unsigned int gl_color_buffer_bit = 0x00004000;

bool window_has(SDL_WindowFlags flag) {
    return (SDL_GetWindowFlags(g_window) & flag) != 0;
}

HitRegion hit_region_at(f32 x, f32 y) {
    int w = 0;
    int h = 0;
    SDL_GetWindowSize(g_window, &w, &h);
    const bool resizable = !window_has(SDL_WINDOW_MAXIMIZED | SDL_WINDOW_FULLSCREEN);
    return hit_test(g_chrome, x, y, f32(w), f32(h), resizable);
}

// True while a second press at (x, y) would complete a double-click on the caption.
bool double_click_armed_at(f32 x, f32 y) {
    const f32 slop = 6.0f * g_scale;
    return g_armed_ns != 0 && SDL_GetTicksNS() - g_armed_ns < g_double_click_ns && SDL_fabsf(x - g_armed_x) < slop &&
           SDL_fabsf(y - g_armed_y) < slop;
}

// Called by SDL whenever the OS asks; reads the map the last frame wrote. No allocation.
SDL_HitTestResult SDLCALL hit_test_callback(SDL_Window*, const SDL_Point* point, void*) {
    const f32 x = f32(point->x);
    const f32 y = f32(point->y);
    switch (hit_region_at(x, y)) {
        case HitRegion::Caption:
            // The second press of a double-click stays with the application, so no move starts and
            // the maximize is not applied in the middle of one.
            return double_click_armed_at(x, y) ? SDL_HITTEST_NORMAL : SDL_HITTEST_DRAGGABLE;
        case HitRegion::ResizeTopLeft:
            return SDL_HITTEST_RESIZE_TOPLEFT;
        case HitRegion::ResizeTop:
            return SDL_HITTEST_RESIZE_TOP;
        case HitRegion::ResizeTopRight:
            return SDL_HITTEST_RESIZE_TOPRIGHT;
        case HitRegion::ResizeRight:
            return SDL_HITTEST_RESIZE_RIGHT;
        case HitRegion::ResizeBottomRight:
            return SDL_HITTEST_RESIZE_BOTTOMRIGHT;
        case HitRegion::ResizeBottom:
            return SDL_HITTEST_RESIZE_BOTTOM;
        case HitRegion::ResizeBottomLeft:
            return SDL_HITTEST_RESIZE_BOTTOMLEFT;
        case HitRegion::ResizeLeft:
            return SDL_HITTEST_RESIZE_LEFT;
        case HitRegion::Normal:
            break;
    }
    return SDL_HITTEST_NORMAL;
}

void toggle_maximized() {
    if (window_has(SDL_WINDOW_MAXIMIZED)) {
        SDL_RestoreWindow(g_window);
    } else {
        SDL_MaximizeWindow(g_window);
    }
}

// X11 and Wayland: SDL reports each press the window manager took as a hit-test event (on Wayland
// through third_party/patches/sdl3). A press on the caption arms a double-click; while armed, the
// hit test answers Normal at that spot, so the second press arrives as an ordinary click
// (on_caption_click) instead of starting another move.
void on_caption_press() {
    float x = 0;
    float y = 0;
    SDL_GetMouseState(&x, &y);
    if (hit_region_at(x, y) != HitRegion::Caption) {
        g_armed_ns = 0;
        return;
    }
    g_armed_ns = SDL_GetTicksNS();
    g_armed_x = x;
    g_armed_y = y;
}

void on_caption_click(const SDL_MouseButtonEvent& e) {
    if (e.button != SDL_BUTTON_LEFT || !double_click_armed_at(e.x, e.y)) {
        return;
    }
    g_armed_ns = 0;
    if (hit_region_at(e.x, e.y) == HitRegion::Caption) {
        toggle_maximized();
    }
}

enum class Glyph : u8 { Fullscreen, ExitFullscreen, Minimize, Maximize, Restore, Close };

// One title bar button: an invisible ImGui button with a drawn glyph. Returns true when clicked.
bool title_bar_button(const char* id, Glyph glyph, ImVec2 size) {
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const bool clicked = ImGui::InvisibleButton(id, size);
    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    g_chrome.add_exclude({min.x, min.y, max.x - min.x, max.y - min.y});
    const bool hovered = ImGui::IsItemHovered();
    const bool held = ImGui::IsItemActive();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (hovered || held) {
        const ImU32 bg = glyph == Glyph::Close
                             ? (held ? IM_COL32(170, 30, 20, 255) : IM_COL32(196, 43, 28, 255))
                             : ImGui::GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_HeaderHovered);
        dl->AddRectFilled(pos, {pos.x + size.x, pos.y + size.y}, bg);
    }
    const ImU32 fg = ImGui::GetColorU32(ImGuiCol_Text);
    const f32 g = 10.0f * g_scale;  // glyph size
    const f32 t = 1.0f * g_scale;   // stroke
    const f32 l = g * 0.35f;        // fullscreen corner length
    const ImVec2 c{pos.x + size.x * 0.5f, pos.y + size.y * 0.5f};
    const ImVec2 a{c.x - g / 2, c.y - g / 2};
    const ImVec2 b{c.x + g / 2, c.y + g / 2};
    switch (glyph) {
        case Glyph::Minimize:
            dl->AddLine({a.x, c.y}, {b.x, c.y}, fg, t);
            break;
        case Glyph::Maximize:
            dl->AddRect(a, b, fg, 0, t);
            break;
        case Glyph::Restore: {
            const f32 o = 2.5f * g_scale;
            dl->AddRect({a.x, a.y + o}, {b.x - o, b.y}, fg, 0, t);  // the front window
            const ImVec2 back[5] = {{a.x + o, a.y + o}, {a.x + o, a.y}, {b.x, a.y}, {b.x, b.y - o}, {b.x - o, b.y - o}};
            dl->AddPolyline(back, 5, fg, t);  // the window behind it
            break;
        }
        case Glyph::Close:
            dl->AddLine(a, b, fg, t);
            dl->AddLine({a.x, b.y}, {b.x, a.y}, fg, t);
            break;
        case Glyph::Fullscreen:  // four corners pointing out
            dl->AddLine(a, {a.x + l, a.y}, fg, t);
            dl->AddLine(a, {a.x, a.y + l}, fg, t);
            dl->AddLine({b.x, a.y}, {b.x - l, a.y}, fg, t);
            dl->AddLine({b.x, a.y}, {b.x, a.y + l}, fg, t);
            dl->AddLine({a.x, b.y}, {a.x + l, b.y}, fg, t);
            dl->AddLine({a.x, b.y}, {a.x, b.y - l}, fg, t);
            dl->AddLine(b, {b.x - l, b.y}, fg, t);
            dl->AddLine(b, {b.x, b.y - l}, fg, t);
            break;
        case Glyph::ExitFullscreen: {  // four corners pointing in
            const ImVec2 p[4] = {{a.x + l, a.y + l}, {b.x - l, a.y + l}, {a.x + l, b.y - l}, {b.x - l, b.y - l}};
            const f32 dx[4] = {-1, 1, -1, 1};
            const f32 dy[4] = {-1, -1, 1, 1};
            for (int i = 0; i < 4; ++i) {
                dl->AddLine(p[i], {p[i].x + dx[i] * l, p[i].y}, fg, t);
                dl->AddLine(p[i], {p[i].x, p[i].y + dy[i] * l}, fg, t);
            }
            break;
        }
    }
    return clicked;
}

// The title bar: a full-width ImGui window pinned to the top, behind every other ImGui window.
void draw_title_bar() {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const f32 height = title_bar_height * g_scale;
    const bool fullscreen = window_has(SDL_WINDOW_FULLSCREEN);
    const bool maximized = window_has(SDL_WINDOW_MAXIMIZED);

    g_chrome.clear();
    if (!fullscreen) {  // a fullscreen window is neither moved nor resized
        g_chrome.add_caption({0, 0, vp->Size.x, height});
        g_chrome.border = resize_border * g_scale;
        g_chrome.corner = resize_corner * g_scale;
    }

    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize({vp->Size.x, height});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, {0, 0});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 0});
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg));
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking |
                                       ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                                       ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                                       ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::Begin("###ezego.title_bar", nullptr, flags);
    g_title_bar = ImGui::GetCurrentWindow();

    // Title, centered.
    const char* title = "eZeGo";
    const ImVec2 text = ImGui::CalcTextSize(title);
    ImGui::GetWindowDrawList()->AddText(
        {vp->Pos.x + (vp->Size.x - text.x) * 0.5f, vp->Pos.y + (height - text.y) * 0.5f},
        ImGui::GetColorU32(ImGuiCol_Text), title);

    // Buttons, right-aligned: fullscreen, minimize, maximize or restore, close.
    const ImVec2 size{button_width * g_scale, height};
    const int count = fullscreen ? 2 : 4;
    ImGui::SetCursorPos({vp->Size.x - size.x * f32(count), 0});
    if (title_bar_button("##fullscreen", fullscreen ? Glyph::ExitFullscreen : Glyph::Fullscreen, size)) {
        SDL_SetWindowFullscreen(g_window, !fullscreen);  // borderless desktop fullscreen, never exclusive (W-10)
    }
    if (!fullscreen) {
        ImGui::SameLine();
        if (title_bar_button("##minimize", Glyph::Minimize, size)) {
            SDL_MinimizeWindow(g_window);
        }
        ImGui::SameLine();
        if (title_bar_button("##maximize", maximized ? Glyph::Restore : Glyph::Maximize, size)) {
            toggle_maximized();
        }
    }
    ImGui::SameLine();
    if (title_bar_button("##close", Glyph::Close, size)) {
        g_should_close = true;  // the same path as the OS close request
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(5);
}

// Any other ImGui window drawn over the title bar takes its own clicks instead of moving the window.
void exclude_windows_over_title_bar() {
    const f32 height = title_bar_height * g_scale;
    for (ImGuiWindow* w : ImGui::GetCurrentContext()->Windows) {
        if (w == g_title_bar || !w->Active || w->Hidden || (w->Flags & ImGuiWindowFlags_ChildWindow) != 0) {
            continue;
        }
        if (w->Pos.y < height) {
            g_chrome.add_exclude({w->Pos.x, w->Pos.y, w->Size.x, w->Size.y});
        }
    }
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
    SDL_SetAppMetadata("eZeGo", build_version, "app.ezego");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        EZ_LOG_ERROR(app, "cannot initialise SDL video: %s", SDL_GetError());
        return false;
    }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);  // required on macOS
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    g_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    if (g_scale <= 0) {
        g_scale = 1.0f;
    }
    const int width = int(1280 * g_scale);
    const int height = int(720 * g_scale);
    constexpr SDL_WindowFlags window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS |
                                             SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_HIDDEN;
    g_window = SDL_CreateWindow("eZeGo", width, height, window_flags);
    if (g_window == nullptr) {
        EZ_LOG_ERROR(app, "cannot create the window: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }
    SDL_SetWindowMinimumSize(g_window, int(min_width * g_scale), int(min_height * g_scale));
    SDL_SetWindowPosition(g_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    g_gl_context = SDL_GL_CreateContext(g_window);
    if (g_gl_context == nullptr) {
        EZ_LOG_ERROR(app, "cannot create an OpenGL 4.1 core context: %s", SDL_GetError());
        SDL_DestroyWindow(g_window);
        g_window = nullptr;
        SDL_Quit();
        return false;
    }
    SDL_GL_MakeCurrent(g_window, g_gl_context);
    SDL_GL_SetSwapInterval(1);  // vsync
    g_gl_viewport = reinterpret_cast<GlViewportFn>(SDL_GL_GetProcAddress("glViewport"));
    g_gl_clear_color = reinterpret_cast<GlClearColorFn>(SDL_GL_GetProcAddress("glClearColor"));
    g_gl_clear = reinterpret_cast<GlClearFn>(SDL_GL_GetProcAddress("glClear"));
    if (!SDL_SetWindowHitTest(g_window, &hit_test_callback, nullptr)) {
        EZ_LOG_WARN(app, "hit testing is unsupported here; the title bar cannot move or resize the window: %s",
                    SDL_GetError());
    }
    const char* driver = SDL_GetCurrentVideoDriver();
    g_double_click_from_hit_test = std::strcmp(driver, "x11") == 0 || std::strcmp(driver, "wayland") == 0;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // multi-viewport stays off (windowing.md W-6)
    io.IniFilename = nullptr;                          // TODO(Argosta): layout persistence (windowing.md W-9)
    ImGui::StyleColorsDark();
    apply_dark_theme();
    ImGui::GetStyle().ScaleAllSizes(g_scale);
    ImGui::GetStyle().FontScaleDpi = g_scale;
    g_double_click_ns = u64(f64(io.MouseDoubleClickTime) * 1.0e9);
    ImGui_ImplSDL3_InitForOpenGL(g_window, g_gl_context);
    ImGui_ImplOpenGL3_Init("#version 410");
    load_fonts();
    SDL_ShowWindow(g_window);

    int pixel_width = 0;
    int pixel_height = 0;
    SDL_GetWindowSizeInPixels(g_window, &pixel_width, &pixel_height);
    EZ_LOG_INFO(app, "SDL %d.%d.%d (%s), window %dx%d, %dx%d pixels, content scale %.2f", SDL_MAJOR_VERSION,
                SDL_MINOR_VERSION, SDL_MICRO_VERSION, SDL_GetCurrentVideoDriver(), width, height, pixel_width,
                pixel_height, f64(g_scale));
    return true;
}

void shell_shutdown() noexcept {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DestroyContext(g_gl_context);
    g_gl_context = nullptr;
    SDL_DestroyWindow(g_window);
    g_window = nullptr;
    SDL_Quit();
}

void shell_poll() noexcept {
    EZ_PROF_ZONE("Input");
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        ImGui_ImplSDL3_ProcessEvent(&e);
        switch (e.type) {
            case SDL_EVENT_QUIT:
                g_should_close = true;
                break;
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                if (e.window.windowID == SDL_GetWindowID(g_window)) {
                    g_should_close = true;
                }
                break;
            case SDL_EVENT_WINDOW_HIT_TEST:
                if (g_double_click_from_hit_test) {
                    on_caption_press();
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (g_double_click_from_hit_test && e.button.windowID == SDL_GetWindowID(g_window)) {
                    on_caption_click(e.button);
                }
                break;
            default:
                break;
        }
    }
}

bool shell_should_close() noexcept {
    return g_should_close;
}

void shell_request_close() noexcept {
    g_should_close = true;
}

void shell_begin_frame() noexcept {
    EZ_PROF_ZONE("Begin Frame");
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    draw_title_bar();
}

void shell_end_frame() noexcept {
    EZ_PROF_ZONE("Render Frame");
    exclude_windows_over_title_bar();
    ImGui::Render();
    if (window_has(SDL_WINDOW_MINIMIZED)) {
        SDL_Delay(16);  // nothing is visible; do not spin or wait on a swap that may never come
        return;
    }
    int w = 0;
    int h = 0;
    SDL_GetWindowSizeInPixels(g_window, &w, &h);
    g_gl_viewport(0, 0, w, h);
    g_gl_clear_color(0.0f, 0.0f, 0.0f, 1.0f);
    g_gl_clear(gl_color_buffer_bit);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(g_window);
}

}  // namespace ez::app::detail
