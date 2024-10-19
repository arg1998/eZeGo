#pragma once

#include "definitions.hpp"


#if defined(EZ_COMPILER_CLANG)
    #define ezDebugBreak() __builtin_trap() 
#elif defined(EZ_COMPILER_MSVC)
    #define ezDebugBreak() __debugbreak()
#endif

void report_assertion_failure(const char *expression, const char* message, const char *file, s32 line);


#if (EZ_CONFIG_ASSERTION_ENABLED == true)

    #define EZ_ASSERT(expression)                                                           \
        {                                                                                   \
            if (expression) {                                                               \
            } else {                                                                        \
                report_assertion_failure(#expression, "", __FILENAME__, __LINE__);          \
                ezDebugBreak();                                                             \
            }                                                                               \
        }

    #define EZ_ASSERT_MSG(expression, message)                                              \
        {                                                                                   \
            if(expression){                                                                 \
            } else {                                                                        \
                report_assertion_failure(#expression, message, __FILENAME__, __LINE__);     \
                ezDebugBreak();                                                             \
            }                                                                               \
        }


#else
    #define EZ_ASSERT(expression)                   // nothing
    #define EZ_ASSERT_MSG(expression, message)      // nothing
#endif