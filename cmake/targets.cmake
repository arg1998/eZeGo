# Target helpers: one static library per module (B-9), our own build description of every
# third-party library (B-8), uniform warnings.

# Third-party targets: no warnings, includes as SYSTEM, optimized even in debug (B-10).
function(ez_third_party target)
  get_target_property(_type ${target} TYPE)
  if(_type STREQUAL "INTERFACE_LIBRARY")
    get_target_property(_inc ${target} INTERFACE_INCLUDE_DIRECTORIES)
    if(_inc)
      set_target_properties(${target} PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_inc}")
    endif()
    return()
  endif()
  set_target_properties(${target} PROPERTIES SYSTEM ON FOLDER third_party)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W0)
  else()
    target_compile_options(${target} PRIVATE -w)
  endif()
  if(EZ_MODE STREQUAL "debug" AND EZ_OPTIMIZE_THIRD_PARTY AND NOT EZ_SANITIZE)
    if(MSVC)
      target_compile_options(${target} PRIVATE /O2 /Ob2)
    else()
      target_compile_options(${target} PRIVATE -O2)
    endif()
  endif()
endfunction()

function(ez_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive-)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow
      -Wno-missing-field-initializers -Wno-gnu-zero-variadic-macro-arguments)
  endif()
  ez_strict(${target})   # cmake/lint.cmake: stricter flags, clang-tidy, -Werror; skipped for legacy
endfunction()

# ez_module(<name> SOURCES ... [PRIVATE_DEPS ...])
# Explicit source lists, no globbing: the build always knows every file.
#
# For a module in modules.cmake, <name> is the table name (`log`): the target is ez_log, its
# first-party dependencies and layer come from the table, and PRIVATE_DEPS is only for third-party
# libraries. The prototype under src/core still passes a full target name and PUBLIC_DEPS.
function(ez_module name)
  cmake_parse_arguments(M "" "" "SOURCES;PUBLIC_DEPS;PRIVATE_DEPS" ${ARGN})
  if(name IN_LIST EZ_MODULES)
    set(_table_name "${name}")
    set(name "ez_${name}")
    foreach(d IN LISTS EZ_MODULE_${_table_name}_DEPS)
      list(APPEND M_PUBLIC_DEPS ez::${d})
    endforeach()
  endif()
  add_library(${name} STATIC ${M_SOURCES})
  if(_table_name)
    set_target_properties(${name} PROPERTIES EZ_LAYER "${EZ_MODULE_${_table_name}_LAYER}")
    if(_table_name STREQUAL "base")
      target_include_directories(${name} PUBLIC "${EZ_GENERATED_DIR}")   # ez/base/modules.gen.hpp
    endif()
  endif()
  string(REGEX REPLACE "^ez_" "" _short "${name}")
  add_library(ez::${_short} ALIAS ${name})   # ez_base -> ez::base
  target_include_directories(${name} PUBLIC "${PROJECT_SOURCE_DIR}/src")
  target_link_libraries(${name} PUBLIC ez::config ${M_PUBLIC_DEPS} PRIVATE ${M_PRIVATE_DEPS})
  ez_warnings(${name})
  set_target_properties(${name} PROPERTIES FOLDER modules)
endfunction()

function(ez_executable name)
  cmake_parse_arguments(M "" "" "SOURCES;DEPS" ${ARGN})
  add_executable(${name} ${M_SOURCES})
  target_link_libraries(${name} PRIVATE ez::config ${M_DEPS})
  ez_warnings(${name})
  set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endfunction()
