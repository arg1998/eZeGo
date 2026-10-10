// The title bar's hit-test map (specs/windowing.md W-2, §4). The shell writes it every frame from
// what it drew; the OS asks which part of the window a point is, to move or resize the borderless
// window natively. Pure data and a pure function, no SDL and no ImGui.
#pragma once

#include "ez/base/types.hpp"

namespace ez::app::detail {

struct ChromeRect {
    f32 x = 0, y = 0, width = 0, height = 0;
    [[nodiscard]] constexpr bool contains(f32 px, f32 py) const noexcept {
        return px >= x && py >= y && px < x + width && py < y + height;
    }
};

enum class HitRegion : u8 {
    Normal,   // content: the application gets the click
    Caption,  // the OS moves the window
    ResizeTopLeft,
    ResizeTop,
    ResizeTopRight,
    ResizeRight,
    ResizeBottomRight,
    ResizeBottom,
    ResizeBottomLeft,
    ResizeLeft,
};

struct ChromeMap {
    static constexpr u32 max_captions = 4;
    static constexpr u32 max_excludes = 32;

    ChromeRect captions[max_captions];  // draggable areas, in window coordinates
    u32 caption_count = 0;
    ChromeRect excludes[max_excludes];  // content inside a caption: buttons, panels drawn over the bar
    u32 exclude_count = 0;
    f32 border = 0;  // width of the resize band along the window edges; 0 means no resizing
    f32 corner = 0;  // size of the corner squares; at least `border`

    bool add_caption(ChromeRect rect) noexcept;  // false when full; the rect is dropped
    bool add_exclude(ChromeRect rect) noexcept;
    void clear() noexcept;
};

// Edges first, then exclusions, then captions. `resizable` is false when the window is maximized
// or fullscreen, so its edges are plain content.
HitRegion hit_test(const ChromeMap& map, f32 x, f32 y, f32 width, f32 height, bool resizable) noexcept;

}  // namespace ez::app::detail
