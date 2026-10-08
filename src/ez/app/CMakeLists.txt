# ez::app — layer 3. The main loop and, for now, the placeholder shell: the only module that sees
# GLFW and Dear ImGui until the window, render and ui modules exist.
set(_paths_posix detail/paths_posix.cpp)
set(_paths_windows detail/paths_windows.cpp)
if(WIN32)
  set(_paths ${_paths_windows})
else()
  set(_paths ${_paths_posix})
endif()

ez_module(app
  SOURCES
    app.hpp app.cpp app_cvars.cpp
    detail/settings.hpp detail/shell.hpp detail/shell.cpp detail/paths.hpp ${_paths}
  PRIVATE_DEPS imgui glfw)
