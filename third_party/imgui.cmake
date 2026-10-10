# Dear ImGui: upstream has no CMake build, so the target is ours (B-8). The SDL3 platform backend and
# the OpenGL 3 renderer backend serve the placeholder shell in app (windowing.md §12).
set(_d "${EZ_DEP_imgui_DIR}")
add_library(imgui STATIC
  "${_d}/imgui.cpp" "${_d}/imgui_draw.cpp" "${_d}/imgui_tables.cpp"
  "${_d}/imgui_widgets.cpp" "${_d}/imgui_demo.cpp"
  "${_d}/backends/imgui_impl_sdl3.cpp" "${_d}/backends/imgui_impl_opengl3.cpp")
target_include_directories(imgui SYSTEM PUBLIC "${_d}" "${_d}/backends")
target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)
target_link_libraries(imgui PUBLIC sdl3)
if(APPLE)
  target_link_libraries(imgui PUBLIC "-framework OpenGL")
elseif(WIN32)
  target_link_libraries(imgui PUBLIC opengl32)
endif()
# GL functions are loaded at runtime by ImGui's embedded loader: no GL dev package needed.
ez_third_party(imgui)
