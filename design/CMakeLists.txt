# The eZeGo design sandbox: Dear ImGui in the browser through Emscripten. Standalone; README.md
# explains the layout. `bun run dev` configures and builds it; there is no native build.
cmake_minimum_required(VERSION 3.28)
project(ezego_design LANGUAGES C CXX)

if(NOT EMSCRIPTEN)
  message(FATAL_ERROR "The design sandbox builds for the browser only. Run `bun run dev`; it configures with Emscripten.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# No debug info in development builds: it multiplies the size of the .wasm and the reload time.
set(CMAKE_C_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")

# ---------------------------------------------------------------- dependencies (bun run setup)
set(deps "${CMAKE_CURRENT_SOURCE_DIR}/.deps")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/versions.json" versions)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS versions.json)
foreach(dep imgui sdl3)
  string(JSON want GET "${versions}" ${dep} sha256)
  set(have "")
  if(EXISTS "${deps}/${dep}/.sha256")
    file(READ "${deps}/${dep}/.sha256" have)
    string(STRIP "${have}" have)
  endif()
  if(NOT have STREQUAL want)
    message(FATAL_ERROR "${dep} in .deps/ does not match versions.json. Run: bun run setup")
  endif()
endforeach()

# SDL3: the window on the page's canvas, input, and the WebGL2 context. Video and events only.
set(SDL_SHARED OFF)
set(SDL_STATIC ON)
set(SDL_TEST_LIBRARY OFF)
set(SDL_TESTS OFF)
set(SDL_EXAMPLES OFF)
set(SDL_INSTALL OFF)
set(SDL_AUDIO OFF)
set(SDL_GPU OFF)
set(SDL_RENDER OFF)
set(SDL_CAMERA OFF)
set(SDL_JOYSTICK OFF)
set(SDL_HAPTIC OFF)
set(SDL_HIDAPI OFF)
set(SDL_POWER OFF)
set(SDL_SENSOR OFF)
set(SDL_DIALOG OFF)
set(SDL_TRAY OFF)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)  # let the variables above win over option()
add_subdirectory("${deps}/sdl3" "${CMAKE_BINARY_DIR}/sdl3" EXCLUDE_FROM_ALL SYSTEM)
target_compile_options(SDL3-static PRIVATE -O2)  # optimized even in development builds

# Dear ImGui with the upstream SDL3 and OpenGL3 (WebGL2) backends.
set(imgui_dir "${deps}/imgui")
add_library(imgui STATIC
  "${imgui_dir}/imgui.cpp"
  "${imgui_dir}/imgui_demo.cpp"
  "${imgui_dir}/imgui_draw.cpp"
  "${imgui_dir}/imgui_tables.cpp"
  "${imgui_dir}/imgui_widgets.cpp"
  "${imgui_dir}/backends/imgui_impl_sdl3.cpp"
  "${imgui_dir}/backends/imgui_impl_opengl3.cpp")
target_include_directories(imgui SYSTEM PUBLIC "${imgui_dir}" "${imgui_dir}/backends")
target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)
target_compile_options(imgui PRIVATE -O2)
target_link_libraries(imgui PUBLIC SDL3::SDL3-static)

# ---------------------------------------------------------------- the design
# New files under src/ are picked up by the next build without touching this file.
file(GLOB_RECURSE design_sources CONFIGURE_DEPENDS src/*.cpp src/*.hpp)
add_executable(design ${design_sources})
target_include_directories(design PRIVATE src)
target_link_libraries(design PRIVATE imgui)
target_compile_options(design PRIVATE -Wall -Wextra -Wno-unused-parameter -fdiagnostics-color=always)
set_target_properties(design PROPERTIES
  SUFFIX ".js"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out")
target_link_options(design PRIVATE
  -sMODULARIZE=1
  -sEXPORT_ES6=1
  -sEXPORT_NAME=createDesign
  -sENVIRONMENT=web
  -sMIN_WEBGL_VERSION=2
  -sMAX_WEBGL_VERSION=2
  -sALLOW_MEMORY_GROWTH=1
  -sSTACK_SIZE=1MB
  # SDL3 replaces Module.requestFullscreen; debug builds guard unexported runtime names as read-only.
  -sEXPORTED_RUNTIME_METHODS=requestFullscreen
  # assets/fonts is readable at /fonts from C++; editing a font relinks.
  "--embed-file=${CMAKE_CURRENT_SOURCE_DIR}/assets/fonts@/fonts"
  $<$<CONFIG:Debug>:-sASSERTIONS=1>)
file(GLOB design_fonts CONFIGURE_DEPENDS assets/fonts/*)
set_property(TARGET design APPEND PROPERTY LINK_DEPENDS ${design_fonts})
