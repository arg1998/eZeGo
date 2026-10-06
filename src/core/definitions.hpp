#pragma once

/*

This file contains the type definitions, platform flags and other necessary flags
to properly operate and compile for multiple platforms

*/


#include "configs.hpp"


//----------------------------------------------------------------
#pragma region "Primitive type defintions"
//----------------------------------------------------------------


// Unsigned integer
using u8    = unsigned char;
using u16   = unsigned short;
using u32   = unsigned int;
using u64   = unsigned long long;

// Signed integer
using s8    = signed char;
using s16   = signed short;
using s32   = signed int;
using s64   = signed long long;

// Floating-point
using f32   = float;
using f64   = double;

// Boolean
using b8    = bool;

#pragma endregion


//----------------------------------------------------------------
#pragma region "mandatory type assertions"
//----------------------------------------------------------------
#define STATIC_ASSERT static_assert

// Ensure all types are of the correct size.
STATIC_ASSERT(sizeof(u8)    == 1, "Expected u8  to be 1 byte.");
STATIC_ASSERT(sizeof(u16)   == 2, "Expected u16 to be 2 bytes.");
STATIC_ASSERT(sizeof(u32)   == 4, "Expected u32 to be 4 bytes.");
STATIC_ASSERT(sizeof(u64)   == 8, "Expected u64 to be 8 bytes.");

STATIC_ASSERT(sizeof(s8)    == 1, "Expected i8  to be 1 byte.");
STATIC_ASSERT(sizeof(s16)   == 2, "Expected i16 to be 2 bytes.");
STATIC_ASSERT(sizeof(s32)   == 4, "Expected i32 to be 4 bytes.");
STATIC_ASSERT(sizeof(s64)   == 8, "Expected i64 to be 8 bytes.");

STATIC_ASSERT(sizeof(f32)   == 4, "Expected f32 to be 4 bytes.");
STATIC_ASSERT(sizeof(f64)   == 8, "Expected f64 to be 8 bytes.");
STATIC_ASSERT(sizeof(void*) == 8, "Expected a 64-bit target.");
#pragma endregion // mandatory type assertions


//----------------------------------------------------------------
#pragma region "Detect Build type (debug/release)"
//----------------------------------------------------------------
// Build *modes* (debug/profile/release) are decided by the CMake preset; code should test the
// feature macros from configs.hpp. These two remain as the coarse optimized/unoptimized split.
#if defined(NDEBUG)
    // Anything optimized: the `profile` and `release` presets
    #define EZ_RELEASE_BUILD
#else
    #define EZ_DEBUG_BUILD
#endif
#pragma endregion


//----------------------------------------------------------------
#pragma region "Architecture detection"
//----------------------------------------------------------------
#if defined(__x86_64__) || defined(_M_X64)
    #define EZ_PLATFORM_64BIT 1
    #define EZ_ARCH_X64 1
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define EZ_PLATFORM_64BIT 1
    #define EZ_ARCH_ARM64 1
#else
    #error "Only 64-bit architectures are supported."
#endif
#pragma endregion


//----------------------------------------------------------------
#pragma region "Compiler detection"
//----------------------------------------------------------------
// clang-cl defines both __clang__ and _MSC_VER: it is treated as Clang.
#if defined(__clang__)
    #define EZ_COMPILER_CLANG
#elif defined(_MSC_VER)
    #define EZ_COMPILER_MSVC
#elif defined(__GNUC__) && defined(EZ_ALLOW_GCC)
    #define EZ_COMPILER_GCC   // unsupported, unblocked on request (-DEZ_ALLOW_GCC=ON)
#else
    #error "Unsupported compiler. Only Clang and MSVC are supported (GCC: configure with -DEZ_ALLOW_GCC=ON)."
#endif
#pragma endregion

//----------------------------------------------------------------
#pragma region "Platform/OS detection"
//----------------------------------------------------------------
// Apple is checked before the generic UNIX/POSIX fallbacks, which macOS would also match.
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64)
    #define EZ_PLATFORM_WINDOWS 1

    #ifndef _WIN64
        #error "64-bit is required on Windows!"
    #endif

#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_MAC
        #define EZ_PLATFORM_MACOS 1

        #if defined(__arm64__) || defined(__aarch64__) || (defined(TARGET_CPU_ARM64) && TARGET_CPU_ARM64)
            // Apple Silicon (M Series)
            #define EZ_PLATFORM_APPLE_SILICON 1
        #else
            #error "Only Apple Silicon Macs are supported."
        #endif

    #else
        #error "Unsupported Apple platform."
    #endif

#elif defined(__linux__) || defined(__gnu_linux__)
    // Linux OS
    #define EZ_PLATFORM_LINUX 1

#else
    #error "Unknown platform!"
#endif

#pragma endregion

//----------------------------------------------------------------
#pragma region "Function attributes"
//----------------------------------------------------------------

#define EZ_NO_DISCARD [[nodiscard]]

#if defined(EZ_COMPILER_MSVC)
    #define EZ_LIKELY(x)   (x)
    #define EZ_UNLIKELY(x) (x)
    #define EZ_FORCE_INLINE __forceinline
#else
    #define EZ_LIKELY(x)   __builtin_expect(!!(x), true)
    #define EZ_UNLIKELY(x) __builtin_expect(!!(x), false)
    #define EZ_FORCE_INLINE inline __attribute__((always_inline))
#endif

#pragma endregion

//----------------------------------------------------------------
#pragma region "Utility Macros"
//----------------------------------------------------------------

#if defined(EZ_COMPILER_MSVC)
    #define EZ_DEBUG_BREAK() __debugbreak()
#else
    #define EZ_DEBUG_BREAK() __builtin_trap()
#endif
#define EZ_STRINGIFY(x) #x
#define EZ_UNIQUE_STR(str) str "_" EZ_STRINGIFY(__COUNTER__)
#define EZ_ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))

#pragma endregion
