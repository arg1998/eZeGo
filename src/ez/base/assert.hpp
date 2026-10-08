// Debug assertion with a replaceable failure handler (specs/base.md BA-4).
//
//   EZ_ASSERT(index < count);
//   EZ_ASSERT_MSG(ring != nullptr, "register the thread before logging");
//
// Compiled in when the build enables EZ_ASSERTS (debug and sanitizer presets). Otherwise the
// expression is not evaluated but must still compile. A failure calls the assert handler, then
// breaks into the debugger, or ends the process when none is attached.
#pragma once

#include "ez/base/build.hpp"
#include "ez/base/macros.hpp"
#include "ez/base/types.hpp"

namespace ez {

struct AssertInfo {
    const char* expression;  // the source text of the failed condition
    const char* message;     // the literal given to EZ_ASSERT_MSG, or ""
    const char* file;
    u32 line;
};

using AssertHandler = void (*)(const AssertInfo& info);

// Installs a handler and returns the previous one; nullptr restores the default, which writes
// one line to stderr. The crash reporter installs itself here. Not thread-safe: set at startup.
AssertHandler set_assert_handler(AssertHandler handler) noexcept;

}  // namespace ez

namespace ez::detail {
EZ_NO_INLINE void assert_failed(const char* expression, const char* message, const char* file, u32 line) noexcept;
}  // namespace ez::detail

#if EZ_ASSERTS
    #define EZ_ASSERT_MSG(expression, message)                                               \
        do {                                                                                 \
            if (EZ_UNLIKELY(!(expression))) {                                                \
                ::ez::detail::assert_failed(#expression, "" message "", __FILE__, __LINE__); \
                EZ_DEBUG_BREAK();                                                            \
            }                                                                                \
        } while (0)
#else
    #define EZ_ASSERT_MSG(expression, message) \
        do {                                   \
            (void)sizeof(!(expression));       \
        } while (0)
#endif

#define EZ_ASSERT(expression) EZ_ASSERT_MSG(expression, "")
