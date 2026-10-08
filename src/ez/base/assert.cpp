#include "ez/base/assert.hpp"

#include <atomic>
#include <cstdio>

namespace ez {

namespace {

void default_assert_handler(const AssertInfo& info) {
    if (info.message[0] != '\0') {
        std::fprintf(stderr, "%s:%u: assertion failed: %s (%s)\n", info.file, info.line, info.expression, info.message);
    } else {
        std::fprintf(stderr, "%s:%u: assertion failed: %s\n", info.file, info.line, info.expression);
    }
    std::fflush(stderr);
}

std::atomic<AssertHandler> g_assert_handler{&default_assert_handler};

}  // namespace

AssertHandler set_assert_handler(AssertHandler handler) noexcept {
    return g_assert_handler.exchange(handler != nullptr ? handler : &default_assert_handler);
}

void detail::assert_failed(const char* expression, const char* message, const char* file, u32 line) noexcept {
    g_assert_handler.load(std::memory_order_acquire)(AssertInfo{expression, message, file, line});
}

}  // namespace ez
