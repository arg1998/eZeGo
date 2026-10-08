// A string with inline storage and a fixed capacity (specs/base.md §2). No allocation; assignment
// truncates to the capacity and the buffer is always terminated.
#pragma once

#include "ez/base/types.hpp"

#include <string_view>

namespace ez {

template <usize Capacity>
class FixedString {
    static_assert(Capacity > 0);

public:
    constexpr FixedString() noexcept = default;
    constexpr FixedString(std::string_view text) noexcept { assign(text); }  // implicit on purpose: cvar defaults

    // Copies at most capacity() characters; returns false when the text was truncated.
    constexpr bool assign(std::string_view text) noexcept {
        const usize n = text.size() < Capacity ? text.size() : Capacity;
        for (usize i = 0; i < n; ++i) {
            data_[i] = text[i];
        }
        data_[n] = '\0';
        size_ = n;
        return n == text.size();
    }

    constexpr void clear() noexcept {
        data_[0] = '\0';
        size_ = 0;
    }

    [[nodiscard]] constexpr const char* c_str() const noexcept { return data_; }
    [[nodiscard]] constexpr std::string_view view() const noexcept { return {data_, size_}; }
    [[nodiscard]] constexpr usize size() const noexcept { return size_; }
    [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }
    [[nodiscard]] static constexpr usize capacity() noexcept { return Capacity; }

    friend constexpr bool operator==(const FixedString& a, std::string_view b) noexcept { return a.view() == b; }

private:
    char data_[Capacity + 1] = {};
    usize size_ = 0;
};

}  // namespace ez
