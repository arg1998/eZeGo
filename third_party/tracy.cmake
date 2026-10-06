# Tracy client, profile builds only (observability.md O-4). In other modes Tracy is not added
# at all and its headers are not on any include path, so nothing can use it by accident.
if(NOT EZ_PROFILER)
  return()
endif()

set(TRACY_ENABLE ON)
set(TRACY_ON_DEMAND ON)                 # record only while a viewer is connected
set(TRACY_MANUAL_LIFETIME ON)           # start after the memory system, stop before it
set(TRACY_DELAYED_INIT ON)              # required by manual lifetime
set(TRACY_NO_BROADCAST ON)              # no LAN discovery; the GUI connects with -a
set(TRACY_IGNORE_MEMORY_FAULTS ON)      # on-demand: frees of pre-connection allocations
if(EZ_TRACY_REMOTE)
  set(TRACY_ONLY_LOCALHOST OFF)
else()
  set(TRACY_ONLY_LOCALHOST ON)
endif()
set(TRACY_STATIC ON)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
add_subdirectory("${EZ_DEP_tracy_DIR}" "${CMAKE_BINARY_DIR}/third_party/tracy" EXCLUDE_FROM_ALL SYSTEM)
ez_third_party(TracyClient)
