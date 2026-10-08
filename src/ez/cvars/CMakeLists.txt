# ez::cvars — layer 0 (specs/cvars.md). Depends on base (modules.cmake).
set(_paths_posix detail/paths_posix.cpp)
set(_paths_windows detail/paths_windows.cpp)
if(WIN32)
  set(_paths ${_paths_windows})
else()
  set(_paths ${_paths_posix})
endif()

ez_module(cvars
  SOURCES
    cvar.hpp registry.hpp registry.cpp
    detail/text.hpp detail/paths.hpp ${_paths})
