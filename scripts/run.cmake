# Build a preset, then run the app from it.
#
#   cmake -P scripts/run.cmake [--preset=debug] [--target=ezego] [-- <app arguments>]

include("${CMAKE_CURRENT_LIST_DIR}/lib/common.cmake")
ez_parse_args()
if(NOT EZ_ARG_preset)
  set(EZ_ARG_preset debug)
endif()
if(NOT EZ_ARG_target)
  set(EZ_ARG_target ezego)
endif()

execute_process(COMMAND "${CMAKE_COMMAND}" --workflow --preset "${EZ_ARG_preset}"
  WORKING_DIRECTORY "${EZ_ROOT}" RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "build failed")
endif()
set(exe "${EZ_ROOT}/build/${EZ_ARG_preset}/bin/${EZ_ARG_target}${CMAKE_EXECUTABLE_SUFFIX}")
if(CMAKE_HOST_WIN32)
  set(exe "${exe}.exe")
endif()
ez_step("Running ${exe}")
execute_process(COMMAND "${exe}" ${EZ_ARGS_POSITIONAL} WORKING_DIRECTORY "${EZ_ROOT}" RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "${EZ_ARG_target} exited with ${rc}")
endif()
