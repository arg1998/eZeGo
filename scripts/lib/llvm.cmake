# Finding the LLVM developer tools: clang-tidy, clang-format, clangd (naming.md N-13).
# Shared by scripts/doctor.cmake, scripts/lint.cmake and cmake/lint.cmake so that all three use the
# same binary. Works in script mode and inside a configure.
#
# Rules:
#   1. An explicit override wins: environment variable EZ_CLANG_TIDY / EZ_CLANG_FORMAT / EZ_CLANGD.
#   2. Otherwise prefer the tool whose major version matches the compiler, because different
#      versions disagree on edge cases: <tool>-<major>, then <tool> if its version matches.
#   3. Otherwise any version found, newest first; the caller reports the mismatch.
# Platform locations searched besides PATH: Homebrew's keg-only llvm on macOS, the LLVM installer
# and Visual Studio's bundled LLVM on Windows.

set(EZ_LLVM_VERSIONS 22 21 20 19 18 17)

function(_ez_llvm_hints out)
  set(_h "")
  if(CMAKE_HOST_APPLE)
    list(APPEND _h /opt/homebrew/opt/llvm/bin /usr/local/opt/llvm/bin)
  elseif(CMAKE_HOST_WIN32)
    list(APPEND _h "$ENV{ProgramFiles}/LLVM/bin" "$ENV{VCINSTALLDIR}/Tools/Llvm/x64/bin")
  endif()
  set(${out} "${_h}" PARENT_SCOPE)
endfunction()

# Major version of a tool, or empty.
function(ez_llvm_tool_major path out)
  set(${out} "" PARENT_SCOPE)
  if(NOT path)
    return()
  endif()
  execute_process(COMMAND "${path}" --version OUTPUT_VARIABLE _v ERROR_QUIET RESULT_VARIABLE _rc)
  if(_rc EQUAL 0 AND _v MATCHES "version ([0-9]+)\\.")
    set(${out} "${CMAKE_MATCH_1}" PARENT_SCOPE)
  endif()
endfunction()

# Major version of the Clang that builds the project, from the same search as cmake/toolchain.cmake.
function(ez_llvm_compiler_major out)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_VERSION MATCHES "^([0-9]+)")
    set(${out} "${CMAKE_MATCH_1}" PARENT_SCOPE)
    return()
  endif()
  set(_names clang)
  if(CMAKE_HOST_WIN32)
    set(_names clang-cl clang)
  endif()
  foreach(v IN LISTS EZ_LLVM_VERSIONS)
    list(APPEND _names clang-${v})
  endforeach()
  _ez_llvm_hints(_hints)
  unset(_ezllvm_cc)
  find_program(_ezllvm_cc NAMES ${_names} HINTS ${_hints} NO_CACHE)
  ez_llvm_tool_major("${_ezllvm_cc}" _m)
  set(${out} "${_m}" PARENT_SCOPE)
endfunction()

# ez_find_llvm_tool(<tool> <out_path> <out_major> [COMPILER_MAJOR <n>])
function(ez_find_llvm_tool tool out_path out_major)
  cmake_parse_arguments(A "" "COMPILER_MAJOR" "" ${ARGN})
  string(TOUPPER "${tool}" _env)
  string(REPLACE "-" "_" _env "EZ_${_env}")
  set(_found "")
  unset(_ezllvm_p)
  if(DEFINED ENV{${_env}} AND EXISTS "$ENV{${_env}}")
    set(_found "$ENV{${_env}}")
  endif()
  _ez_llvm_hints(_hints)
  if(NOT _found AND A_COMPILER_MAJOR)
    find_program(_ezllvm_p NAMES ${tool}-${A_COMPILER_MAJOR} HINTS ${_hints} NO_CACHE)
    if(_ezllvm_p)
      set(_found "${_ezllvm_p}")
    else()
      find_program(_ezllvm_p NAMES ${tool} HINTS ${_hints} NO_CACHE)
      ez_llvm_tool_major("${_ezllvm_p}" _m)
      if(_ezllvm_p AND _m STREQUAL A_COMPILER_MAJOR)
        set(_found "${_ezllvm_p}")
      endif()
    endif()
    unset(_ezllvm_p)
  endif()
  unset(_ezllvm_p)
  if(NOT _found)
    set(_names ${tool})
    foreach(v IN LISTS EZ_LLVM_VERSIONS)
      list(APPEND _names ${tool}-${v})
    endforeach()
    find_program(_ezllvm_p NAMES ${_names} HINTS ${_hints} NO_CACHE)
    set(_found "${_ezllvm_p}")
  endif()
  ez_llvm_tool_major("${_found}" _major)
  set(${out_path} "${_found}" PARENT_SCOPE)
  set(${out_major} "${_major}" PARENT_SCOPE)
endfunction()
