#include "ez/app/detail/chrome.hpp"

namespace ez::app::detail {

bool ChromeMap::add_caption(ChromeRect rect) noexcept {
    if (caption_count >= max_captions) {
        return false;
    }
    captions[caption_count++] = rect;
    return true;
}

bool ChromeMap::add_exclude(ChromeRect rect) noexcept {
    if (exclude_count >= max_excludes) {
        return false;
    }
    excludes[exclude_count++] = rect;
    return true;
}

void ChromeMap::clear() noexcept {
    caption_count = 0;
    exclude_count = 0;
    border = 0;
    corner = 0;
}

HitRegion hit_test(const ChromeMap& map, f32 x, f32 y, f32 width, f32 height, bool resizable) noexcept {
    if (x < 0 || y < 0 || x >= width || y >= height) {
        return HitRegion::Normal;
    }
    if (resizable && map.border > 0) {
        const f32 corner = map.corner > map.border ? map.corner : map.border;
        const bool left = x < map.border;
        const bool right = x >= width - map.border;
        const bool top = y < map.border;
        const bool bottom = y >= height - map.border;
        const bool near_left = x < corner;
        const bool near_right = x >= width - corner;
        const bool near_top = y < corner;
        const bool near_bottom = y >= height - corner;
        if ((top && near_left) || (left && near_top)) {
            return HitRegion::ResizeTopLeft;
        }
        if ((top && near_right) || (right && near_top)) {
            return HitRegion::ResizeTopRight;
        }
        if ((bottom && near_left) || (left && near_bottom)) {
            return HitRegion::ResizeBottomLeft;
        }
        if ((bottom && near_right) || (right && near_bottom)) {
            return HitRegion::ResizeBottomRight;
        }
        if (top) {
            return HitRegion::ResizeTop;
        }
        if (bottom) {
            return HitRegion::ResizeBottom;
        }
        if (left) {
            return HitRegion::ResizeLeft;
        }
        if (right) {
            return HitRegion::ResizeRight;
        }
    }
    for (u32 i = 0; i < map.exclude_count; ++i) {
        if (map.excludes[i].contains(x, y)) {
            return HitRegion::Normal;
        }
    }
    for (u32 i = 0; i < map.caption_count; ++i) {
        if (map.captions[i].contains(x, y)) {
            return HitRegion::Caption;
        }
    }
    return HitRegion::Normal;
}

}  // namespace ez::app::detail
