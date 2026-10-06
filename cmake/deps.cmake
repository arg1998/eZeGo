# Configure-time dependency check (B-6, B-7). Configure NEVER touches the network:
# it only verifies that third_party/_src/ matches dependencies.json and stops with the
# exact command to run if it does not.

set(EZ_MANIFEST "${PROJECT_SOURCE_DIR}/dependencies.json")
set(EZ_SRC_ROOT "${PROJECT_SOURCE_DIR}/third_party/_src")
file(READ "${EZ_MANIFEST}" _ez_manifest)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${EZ_MANIFEST}")

# Which manifest "used_by" groups this build tree needs.
set(_ez_needed app tests)
if(EZ_PROFILER)
  list(APPEND _ez_needed profile)
endif()

string(JSON _n LENGTH "${_ez_manifest}" dependencies)
math(EXPR _last "${_n} - 1")
set(_ez_problems "")
foreach(i RANGE 0 ${_last})
  string(JSON _name GET "${_ez_manifest}" dependencies ${i} name)
  string(JSON _commit GET "${_ez_manifest}" dependencies ${i} commit)
  string(JSON _version GET "${_ez_manifest}" dependencies ${i} version)
  string(JSON _nu LENGTH "${_ez_manifest}" dependencies ${i} used_by)
  set(_needed OFF)
  math(EXPR _ulast "${_nu} - 1")
  foreach(j RANGE 0 ${_ulast})
    string(JSON _u GET "${_ez_manifest}" dependencies ${i} used_by ${j})
    if(_u IN_LIST _ez_needed)
      set(_needed ON)
    endif()
  endforeach()

  set(EZ_DEP_${_name}_DIR "${EZ_SRC_ROOT}/${_name}")
  set(EZ_DEP_${_name}_VERSION "${_version}")
  if(NOT _needed)
    continue()
  endif()

  set(_stamp "${EZ_SRC_ROOT}/.stamps/${_name}")
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_stamp}")
  if(NOT EXISTS "${_stamp}")
    list(APPEND _ez_problems "  ${_name}: not fetched")
    continue()
  endif()
  file(STRINGS "${_stamp}" _have LIMIT_COUNT 1)
  if(NOT _have STREQUAL _commit)
    list(APPEND _ez_problems "  ${_name}: have ${_have}, manifest pins ${_commit}")
  endif()
endforeach()

if(_ez_problems)
  list(JOIN _ez_problems "\n" _msg)
  message(FATAL_ERROR
    "Dependencies do not match dependencies.json:\n${_msg}\n\n"
    "Run:  cmake -P scripts/deps.cmake      (or ./ez deps)\n")
endif()
