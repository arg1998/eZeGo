# eZeGo. Thin on purpose: logic lives in cmake/, one job per file.
# Configure through a preset:  cmake --workflow --preset debug   (or ./ez build debug)
cmake_minimum_required(VERSION 3.28)

project(ezego VERSION 0.1.0 LANGUAGES C CXX)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(FATAL_ERROR "In-source builds are not allowed. Use a preset: cmake --preset debug")
endif()

include(cmake/modes.cmake)
include(cmake/lint.cmake)
include(cmake/modules.cmake)
include(cmake/targets.cmake)
include(cmake/deps.cmake)
include(cmake/editor.cmake)
include(cmake/testing.cmake)

# ------------------------------------------------------------------ third party (B-8)
include(third_party/sdl3.cmake)
include(third_party/imgui.cmake)
include(third_party/tracy.cmake)
include(third_party/doctest.cmake)

# ------------------------------------------------------------------ first party (B-9)
add_subdirectory(src)

# ------------------------------------------------------------------ tests (B-12)
enable_testing()
add_subdirectory(tests)
