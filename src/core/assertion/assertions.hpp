#pragma once

#include "core/definitions.hpp"
#include "core/logger/logger.hpp"   // __FILENAME__

// Compiled in when the build enables EZ_ASSERTS (debug, asan, tsan presets); compiled out otherwise.

#define ezDebugBreak() EZ_DEBUG_BREAK()

void report_assertion_failure(const char *expression, const char* message, const char *file, s32 line);


#if EZ_CONFIG_ASSERTION_ENABLED

    #define EZ_ASSERT(expression)                                                           \
        do {                                                                                \
            if (EZ_UNLIKELY(!(expression))) {                                               \
                report_assertion_failure(#expression, "", __FILENAME__, __LINE__);          \
                ezDebugBreak();                                                             \
            }                                                                               \
        } while (0)

    #define EZ_ASSERT_MSG(expression, message)                                              \
        do {                                                                                \
            if (EZ_UNLIKELY(!(expression))) {                                               \
                report_assertion_failure(#expression, message, __FILENAME__, __LINE__);     \
                ezDebugBreak();                                                             \
            }                                                                               \
        } while (0)

#else
    // The expression is not evaluated, but it still has to compile.
    #define EZ_ASSERT(expression)                   do { (void)sizeof(expression); } while (0)
    #define EZ_ASSERT_MSG(expression, message)      do { (void)sizeof(expression); } while (0)
#endif
