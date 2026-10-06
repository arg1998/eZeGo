# ez_core — L0 foundations: definitions, configs, assertions, logger, memory, platform, profiler.
# Headless: no window, GPU or UI library on its include path or link line.

set(_plat_linux   platform/plat_linux.cpp)
set(_plat_macos   platform/plat_mac.cpp)
set(_plat_windows platform/plat_win32.cpp)
if(WIN32)
  set(_plat ${_plat_windows})
elseif(APPLE)
  set(_plat ${_plat_macos})
else()
  set(_plat ${_plat_linux})
endif()

ez_module(ez_core
  SOURCES
    definitions.hpp configs.hpp
    assertion/assertions.hpp
    logger/logger.hpp logger/logger.cpp
    memory/memory.hpp memory/memory.cpp
    platform/platform.hpp ${_plat}
    profiler/profiler.hpp profiler/profiler.cpp)

find_package(Threads REQUIRED)
target_link_libraries(ez_core PUBLIC Threads::Threads)
if(EZ_PROFILER)
  target_link_libraries(ez_core PUBLIC Tracy::TracyClient)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_definitions(ez_core PUBLIC EZ_ALLOW_GCC)
endif()

# Sources that belong in every executable rather than in the library (linking.md L-6):
# the process entry point, and in profile builds the operator new/delete hooks.
set(EZ_CORE_EXE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/platform/entry.cpp")
if(EZ_MEM_TRACE)
  list(APPEND EZ_CORE_EXE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/memory/new_hooks.cpp")
endif()
set(EZ_CORE_EXE_SOURCES "${EZ_CORE_EXE_SOURCES}" PARENT_SCOPE)
