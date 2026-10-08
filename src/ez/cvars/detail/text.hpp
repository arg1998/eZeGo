// Fixed-buffer text building for the registry's messages, help and files. No allocation.
#pragma once

#include "ez/base/macros.hpp"
#include "ez/base/types.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string_view>

namespace ez::cvars::detail {

class Text {
public:
    static constexpr usize capacity = 2047;

    void append(std::string_view s) noexcept {
        const usize n = s.size() < capacity - size_ ? s.size() : capacity - size_;
        std::memcpy(data_ + size_, s.data(), n);
        size_ += n;
        data_[size_] = '\0';
    }

    EZ_PRINTF_FORMAT(2, 3) void printf(const char* fmt, ...) noexcept {
        va_list args;
        va_start(args, fmt);
        const int n = std::vsnprintf(data_ + size_, capacity + 1 - size_, fmt, args);
        va_end(args);
        if (n > 0) {
            size_ += usize(n) < capacity - size_ ? usize(n) : capacity - size_;
        }
    }

    [[nodiscard]] std::string_view view() const noexcept { return {data_, size_}; }
    [[nodiscard]] const char* c_str() const noexcept { return data_; }
    [[nodiscard]] usize size() const noexcept { return size_; }
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

private:
    char data_[capacity + 1] = {};
    usize size_ = 0;
};

constexpr bool is_space(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

constexpr std::string_view trim(std::string_view s) noexcept {
    while (!s.empty() && is_space(s.front())) {
        s.remove_prefix(1);
    }
    while (!s.empty() && is_space(s.back())) {
        s.remove_suffix(1);
    }
    return s;
}

// "text" -> text; anything else unchanged.
constexpr std::string_view unquote(std::string_view s) noexcept {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

constexpr char lower(char c) noexcept {
    return c >= 'A' && c <= 'Z' ? char(c - 'A' + 'a') : c;
}

constexpr bool iequals(std::string_view a, std::string_view b) noexcept {
    if (a.size() != b.size()) {
        return false;
    }
    for (usize i = 0; i < a.size(); ++i) {
        if (lower(a[i]) != lower(b[i])) {
            return false;
        }
    }
    return true;
}

constexpr bool icontains(std::string_view haystack, std::string_view needle) noexcept {
    if (needle.empty()) {
        return true;
    }
    for (usize i = 0; i + needle.size() <= haystack.size(); ++i) {
        if (iequals(haystack.substr(i, needle.size()), needle)) {
            return true;
        }
    }
    return false;
}

}  // namespace ez::cvars::detail
