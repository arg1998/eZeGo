// A stable 64-bit hash (specs/base.md §2): FNV-1a. Identical on every platform and standard
// library, so a hash can be stored in a file or compared between machines. Not for security.
#pragma once

#include "ez/base/types.hpp"

#include <string_view>

namespace ez {

constexpr u64 hash64(std::string_view text) noexcept {
    u64 h = 0xcbf29ce484222325ull;
    for (const char c : text) {
        h ^= static_cast<u8>(c);
        h *= 0x100000001b3ull;
    }
    return h;
}

}  // namespace ez
