# ez::log — layer 0 (specs/logging.md). Depends on base and cvars (modules.cmake).
set(_os_posix detail/os_posix.cpp)
set(_os_windows detail/os_windows.cpp)
if(WIN32)
  set(_os ${_os_windows})
else()
  set(_os ${_os_posix})
endif()

ez_module(log
  SOURCES
    log.hpp log.cpp log_cvars.cpp
    detail/ring.hpp detail/settings.hpp detail/os.hpp ${_os})

find_package(Threads REQUIRED)
target_link_libraries(ez_log PRIVATE Threads::Threads)
