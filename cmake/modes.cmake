# Mode -> flags and feature macros (B-2, B-3, observability.md §6).
#
# A preset sets exactly one project variable, EZ_MODE (debug | profile | release), plus
# optionally EZ_SANITIZE for variants. Everything else is DERIVED here as normal variables on
# every configure, so nothing mode-dependent can go stale in the cache.
#
# Source code tests features (EZ_ASSERTS, EZ_PROFILER, ...), never the mode name.

set(EZ_MODE "debug" CACHE STRING "Build mode: debug | profile | release")
set_property(CACHE EZ_MODE PROPERTY STRINGS debug profile release)
set(EZ_SANITIZE "" CACHE STRING "Sanitizers for variant presets: address;undefined | thread")
option(EZ_OPTIMIZE_THIRD_PARTY "Optimize third-party code in debug builds" ON)
option(EZ_TRACY_REMOTE "profile builds: accept Tracy connections from the network, not only localhost" OFF)

if(NOT EZ_MODE MATCHES "^(debug|profile|release)$")
  message(FATAL_ERROR "EZ_MODE must be debug, profile or release (got '${EZ_MODE}')")
endif()

# ---------------------------------------------------------------- feature table
#                     debug   profile  release
set(_asserts          1       0        0)
set(_profiler         0       1        0)
set(_memtrace         0       1        0)
set(_loglevel         0       2        3)   # 0 trace, 1 debug, 2 info, 3 warn, 4 error
set(_index_debug 0)
set(_index_profile 1)
set(_index_release 2)
set(_i ${_index_${EZ_MODE}})
list(GET _asserts  ${_i} EZ_ASSERTS)
list(GET _profiler ${_i} EZ_PROFILER)
list(GET _memtrace ${_i} EZ_MEM_TRACE)
list(GET _loglevel ${_i} EZ_LOG_LEVEL)
set(EZ_METRICS 1)  # always-on tier, every build

# Personal override, e.g. "debug with Tracy" from CMakeUserPresets.json: -DEZ_FORCE_PROFILER=ON
if(EZ_FORCE_PROFILER)
  set(EZ_PROFILER 1)
endif()

# ---------------------------------------------------------------- language & common flags
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)            # no C++20 modules (B-10); skips the scan step
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)       # host exports nothing (linking.md L-4)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

if(MSVC)  # clang-cl
  add_compile_options(/Zi /Oy- /utf-8)        # symbols + frame pointers in every mode
  add_link_options(/DEBUG)
  # profile and release share ONE optimization set (profile must measure what ships)
  set(CMAKE_C_FLAGS_RELEASE "/O2 /Ob2 /DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "/O2 /Ob2 /DNDEBUG")
else()
  add_compile_options(-g -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer)
  set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
  set(CMAKE_C_FLAGS_DEBUG "-O0")
  set(CMAKE_CXX_FLAGS_DEBUG "-O0")
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND EZ_MODE STREQUAL "release")
    # Static C++ runtime in release; glibc stays dynamic (linking.md L-3).
    add_link_options(-static-libstdc++ -static-libgcc)
  endif()
endif()

# Sanitizer variants apply to every target, third-party included.
if(EZ_SANITIZE)
  list(JOIN EZ_SANITIZE "," _san)
  if(MSVC)
    add_compile_options(/fsanitize=${_san})
  else()
    add_compile_options(-fsanitize=${_san} -fno-sanitize-recover=all)
    add_link_options(-fsanitize=${_san})
  endif()
endif()

# ---------------------------------------------------------------- the config target
# Every first-party target links ez_config; it carries the feature macros.
add_library(ez_config INTERFACE)
add_library(ez::config ALIAS ez_config)
target_compile_definitions(ez_config INTERFACE
  EZ_MODE_NAME="${EZ_MODE}"
  EZ_ASSERTS=${EZ_ASSERTS}
  EZ_PROFILER=${EZ_PROFILER}
  EZ_MEM_TRACE=${EZ_MEM_TRACE}
  EZ_METRICS=${EZ_METRICS}
  EZ_LOG_LEVEL=${EZ_LOG_LEVEL}
  EZ_VERSION_STR="${PROJECT_VERSION}"
  $<$<PLATFORM_ID:Windows>:NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS>
  $<$<PLATFORM_ID:Darwin>:GL_SILENCE_DEPRECATION>)

if(EZ_SANITIZE)
  target_compile_definitions(ez_config INTERFACE EZ_SANITIZER="${_san}")
endif()

message(STATUS "eZeGo mode: ${EZ_MODE}  asserts=${EZ_ASSERTS} profiler=${EZ_PROFILER} "
               "memtrace=${EZ_MEM_TRACE} log_level=${EZ_LOG_LEVEL} sanitize='${EZ_SANITIZE}'")
