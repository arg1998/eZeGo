// Platform, architecture and compiler detection (specs/base.md BA-3).
// C-compatible: the plugin SDK includes it too, so host and plugins agree on one definition.
// Every macro is defined as 0 or 1. Test with #if, never #ifdef; -Wundef catches typos.
#pragma once

// ---------------------------------------------------------------- operating system
#if defined(_WIN32)
    #define EZ_OS_WINDOWS 1
    #define EZ_OS_MACOS 0
    #define EZ_OS_LINUX 0
    #if !defined(_WIN64)
        #error "eZeGo requires a 64-bit Windows target."
    #endif
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if !TARGET_OS_OSX
        #error "Unsupported Apple platform: only macOS is supported."
    #endif
    #define EZ_OS_WINDOWS 0
    #define EZ_OS_MACOS 1
    #define EZ_OS_LINUX 0
#elif defined(__linux__)
    #define EZ_OS_WINDOWS 0
    #define EZ_OS_MACOS 0
    #define EZ_OS_LINUX 1
#else
    #error "Unsupported operating system: eZeGo targets Linux, macOS and Windows."
#endif

#define EZ_OS_POSIX (EZ_OS_LINUX || EZ_OS_MACOS)

// ---------------------------------------------------------------- architecture
#if defined(__x86_64__) || defined(_M_X64)
    #define EZ_ARCH_X64 1
    #define EZ_ARCH_ARM64 0
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define EZ_ARCH_X64 0
    #define EZ_ARCH_ARM64 1
#else
    #error "Unsupported architecture: eZeGo is 64-bit only (x86-64, arm64)."
#endif

#if EZ_OS_MACOS && !EZ_ARCH_ARM64
    #error "Only Apple Silicon Macs are supported."
#endif

// ---------------------------------------------------------------- compiler
// clang-cl defines both __clang__ and _MSC_VER; it is Clang.
#if defined(__clang__)
    #define EZ_COMPILER_CLANG 1
    #define EZ_COMPILER_MSVC 0
    #define EZ_COMPILER_GCC 0
#elif defined(_MSC_VER)
    #define EZ_COMPILER_CLANG 0
    #define EZ_COMPILER_MSVC 1
    #define EZ_COMPILER_GCC 0
#elif defined(__GNUC__) && defined(EZ_ALLOW_GCC)
    #define EZ_COMPILER_CLANG 0
    #define EZ_COMPILER_MSVC 0
    #define EZ_COMPILER_GCC 1
#else
    #error "Unsupported compiler: eZeGo builds with Clang (GCC only with -DEZ_ALLOW_GCC=ON, unsupported)."
#endif

// ---------------------------------------------------------------- hardware constants
// Destructive-interference size: per-thread data on its own line avoids false sharing.
#if EZ_OS_MACOS && EZ_ARCH_ARM64
    #define EZ_CACHE_LINE 128
#else
    #define EZ_CACHE_LINE 64
#endif

// ---------------------------------------------------------------- symbol export (plugin SDK)
#if EZ_OS_WINDOWS
    #define EZ_EXPORT __declspec(dllexport)
#else
    #define EZ_EXPORT __attribute__((visibility("default")))
#endif
