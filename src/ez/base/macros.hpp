// Attribute and utility macros (specs/base.md §2, naming.md §7).
#pragma once

#include "ez/base/detect.h"

#if EZ_COMPILER_MSVC
    #define EZ_FORCE_INLINE __forceinline
    #define EZ_NO_INLINE __declspec(noinline)
    #define EZ_LIKELY(x) (x)
    #define EZ_UNLIKELY(x) (x)
    #define EZ_UNREACHABLE() __assume(0)
    #define EZ_ASSUME(x) __assume(x)
    #define EZ_DEBUG_BREAK() __debugbreak()
    #define EZ_PRINTF_FORMAT(fmt_index, first_arg_index)
#else
    #define EZ_FORCE_INLINE inline __attribute__((always_inline))
    #define EZ_NO_INLINE __attribute__((noinline))
    #define EZ_LIKELY(x) __builtin_expect(!!(x), 1)
    #define EZ_UNLIKELY(x) __builtin_expect(!!(x), 0)
    #define EZ_UNREACHABLE() __builtin_unreachable()
    #if EZ_COMPILER_CLANG
        #define EZ_ASSUME(x) __builtin_assume(x)
        #define EZ_DEBUG_BREAK() __builtin_debugtrap()
    #else
        #define EZ_ASSUME(x)                       \
            do {                                   \
                if (!(x)) __builtin_unreachable(); \
            } while (0)
        #define EZ_DEBUG_BREAK() __builtin_trap()
    #endif
    // Lets -Wformat check printf-style arguments; indexes are 1-based (count `this` in methods).
    #define EZ_PRINTF_FORMAT(fmt_index, first_arg_index) __attribute__((format(printf, fmt_index, first_arg_index)))
#endif

#define EZ_STRINGIFY_DETAIL(x) #x
#define EZ_STRINGIFY(x) EZ_STRINGIFY_DETAIL(x)
#define EZ_CONCAT_DETAIL(a, b) a##b
#define EZ_CONCAT(a, b) EZ_CONCAT_DETAIL(a, b)
#define EZ_UNIQUE_NAME(prefix) EZ_CONCAT(prefix, __COUNTER__)
