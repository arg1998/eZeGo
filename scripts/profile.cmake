# One-command profiling session (B-4): build `profile`, open the Tracy GUI connected to the
# app, run the app.
#
#   cmake -P scripts/profile.cmake [-- <app arguments>]
#   cmake -P scripts/profile.cmake --gui-only                  just open Tracy (VS Code preLaunchTask)
#   cmake -P scripts/profile.cmake --capture=out.tracy [--seconds=10] [-- <app arguments>]
#                                  headless: record with tracy-capture instead of the GUI
#
# Personal defaults (address, port, extra args) go in scripts/profile.user.cmake (git-ignored):
#   set(EZ_TRACY_ADDRESS 192.168.1.20)   # profile a show machine (build with -DEZ_TRACY_REMOTE=ON)
#   set(EZ_TRACY_PORT 8086)

include("${CMAKE_CURRENT_LIST_DIR}/lib/common.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/lib/tooling.cmake")
ez_parse_args()
set(EZ_TRACY_ADDRESS 127.0.0.1)
set(EZ_TRACY_PORT "")
include("${CMAKE_CURRENT_LIST_DIR}/profile.user.cmake" OPTIONAL)

# ------------------------------------------------------------------ tools present?
ez_tool_binary(tracy profiler tracy_gui)
ez_tool_binary(tracy capture tracy_capture)
if(NOT tracy_gui OR NOT EXISTS "${tracy_gui}")
  ez_step("Tracy is not installed yet; installing the pinned version")
  execute_process(COMMAND "${CMAKE_COMMAND}" -P "${EZ_ROOT}/scripts/tools.cmake" RESULT_VARIABLE rc)
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "could not install Tracy; see above")
  endif()
  ez_tool_binary(tracy profiler tracy_gui)
  ez_tool_binary(tracy capture tracy_capture)
endif()

set(conn -a "${EZ_TRACY_ADDRESS}")
if(EZ_TRACY_PORT)
  list(APPEND conn -p "${EZ_TRACY_PORT}")
endif()

function(ez_launch_detached)
  if(CMAKE_HOST_WIN32)
    # Not `cmd /c start`: its child inherits our output pipes and execute_process then blocks
    # until that program exits. Start-Process goes through the shell and inherits nothing.
    list(POP_FRONT ARGN _exe)
    set(_ps "Start-Process -FilePath '${_exe}'")
    if(ARGN)
      list(JOIN ARGN "\"','\"" _args)
      string(APPEND _ps " -ArgumentList '\"${_args}\"'")
    endif()
    execute_process(COMMAND powershell -NoProfile -Command "${_ps}")
  else()
    list(JOIN ARGN "\" \"" joined)
    execute_process(COMMAND sh -c "nohup \"${joined}\" >/dev/null 2>&1 &")
  endif()
endfunction()

if(EZ_ARG_gui_only)
  ez_step("Opening Tracy (${EZ_TRACY_ADDRESS}); it connects when the profile build starts")
  ez_launch_detached("${tracy_gui}" ${conn})
  return()
endif()

# ------------------------------------------------------------------ build
execute_process(COMMAND "${CMAKE_COMMAND}" --workflow --preset profile WORKING_DIRECTORY "${EZ_ROOT}" RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "profile build failed")
endif()
set(app "${EZ_ROOT}/build/profile/bin/ezego${CMAKE_EXECUTABLE_SUFFIX}")
if(CMAKE_HOST_WIN32)
  set(app "${app}.exe")
endif()

# ------------------------------------------------------------------ run
if(EZ_ARG_capture)
  if(NOT EZ_ARG_seconds)
    set(EZ_ARG_seconds 10)
  endif()
  get_filename_component(out "${EZ_ARG_capture}" ABSOLUTE BASE_DIR "${EZ_ROOT}")
  math(EXPR quit "${EZ_ARG_seconds} + 4")
  ez_step("Headless capture: ${EZ_ARG_seconds} s -> ${out}")
  ez_launch_detached("${app}" --quit-after ${quit} ${EZ_ARGS_POSITIONAL})
  execute_process(COMMAND "${tracy_capture}" -o "${out}" ${conn} -s ${EZ_ARG_seconds} -f RESULT_VARIABLE rc)
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "tracy-capture failed (${rc})")
  endif()
  message("Open it later with: \"${tracy_gui}\" \"${out}\"")
  return()
endif()

ez_step("Opening Tracy and starting the app (profile build, on-demand: recording starts when Tracy connects)")
ez_launch_detached("${tracy_gui}" ${conn})
execute_process(COMMAND "${app}" ${EZ_ARGS_POSITIONAL} WORKING_DIRECTORY "${EZ_ROOT}")
