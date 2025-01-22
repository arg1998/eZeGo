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
// defining static assertion
#if defined(__clang__) || defined(__gcc__)
    #define STATIC_ASSERT _Static_assert
#else
    #define STATIC_ASSERT static_assert
#endif




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
#pragma endregion // mandatory type assertions


//----------------------------------------------------------------
#pragma region "Detect Build type (debug/release)"
//----------------------------------------------------------------
#if defined(NDEBUG)
    // Anything that is not a DEBUG build: Release, MinSizeRel, RelWithDebInfo
    #define EZ_RELEASE_BUILD
#else
    #define EZ_DEBUG_BUILD
#endif
#pragma endregion


//----------------------------------------------------------------
#pragma region "Architecture detection"
//----------------------------------------------------------------
#if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__)
    #define EZ_PLATFORM_64BIT 1
#else
    #error "Only 64-bit architectures are supported."
#endif
#pragma endregion


//----------------------------------------------------------------
#pragma region "Compiler detection"
//----------------------------------------------------------------
#if defined(__clang__)
    #define EZ_COMPILER_CLANG
#elif defined(_MSC_VER)
    #define EZ_COMPILER_MSVC
#else
    #error "Unsupported compiler. Only Clang and MSVC are supported."
#endif
#pragma endregion

//----------------------------------------------------------------
#pragma region "Platform/OS detection"
//----------------------------------------------------------------
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(_WIN64)
    #define EZ_PLATFORM_WINDOWS 1

    #ifndef _WIN64
        #error "64-bit is required on Windows!"
    #endif

#elif defined(__linux__) || defined(__gnu_linux__)
    // Linux OS
    #define EZ_PLATFORM_LINUX 1

#elif defined(__unix__)
    // Catch any UNIX-like that is not caught by the above checks.
    #define EZ_PLATFORM_UNIX 1

#elif defined(_POSIX_VERSION)
    // see if the platform is POSIX compliant
    #define EZ_PLATFORM_POSIX 1

#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_MAC
        #define EZ_PLATFORM_MACOS 1

        #if defined(__arm64__) || defined(__aarch64__) || (defined(TARGET_CPU_ARM64) && TARGET_CPU_ARM64)
            // Apple Silicon (M Series)
            #define EZ_PLATFORM_APPLE_SILICON 1
        #else
            // Intel-based Mac
            #define EZ_PLATFORM_APPLE_INTEL 1
        #endif

    #else
        #error "Unsupported Apple platform."
    #endif
#else
    #error "Unknown platform!"
#endif

#pragma endregion

//----------------------------------------------------------------
#pragma region "Function attributes"
//----------------------------------------------------------------

#define EZ_NO_DISCARD [[nodiscard]]
#define EZ_LIKELY(x)   __builtin_expect(!!(x), true)
#define EZ_UNLIKELY(x) __builtin_expect(!!(x), false)

#pragma endregion

//----------------------------------------------------------------
#pragma region "Utility Macros"
//----------------------------------------------------------------

#define EZ_STRINGIFY(x) #x
#define EZ_UNIQUE_STR(str) str "_" EZ_STRINGIFY(__COUNTER__)

#pragma endregion