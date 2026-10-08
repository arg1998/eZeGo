# Naming and lint enforcement inside the build (naming.md N-13).
#
#   EZ_LINT=ON    run clang-tidy on every first-party translation unit as part of compiling it
#   EZ_WERROR=ON  compiler and clang-tidy warnings are errors
#
# Both are set by the `check` preset only. Locally, clangd shows the same clang-tidy findings
# inline while typing, and the compiler reports warnings without stopping the build.

option(EZ_LINT "Run clang-tidy as part of the build (check preset)" OFF)
option(EZ_WERROR "Treat warnings as errors (check preset)" OFF)

include("${PROJECT_SOURCE_DIR}/scripts/lib/llvm.cmake")

set(EZ_CLANG_TIDY_COMMAND "")
if(EZ_LINT)
  ez_llvm_compiler_major(_ez_cmajor)
  ez_find_llvm_tool(clang-tidy _ez_tidy _ez_tidy_major COMPILER_MAJOR "${_ez_cmajor}")
  if(NOT _ez_tidy)
    message(FATAL_ERROR "EZ_LINT is ON but clang-tidy was not found. Run `cmake -P scripts/doctor.cmake` "
                        "for the install command, or set EZ_CLANG_TIDY to its path.")
  endif()
  set(EZ_CLANG_TIDY_COMMAND "${_ez_tidy}" "--quiet")
  if(EZ_WERROR)
    list(APPEND EZ_CLANG_TIDY_COMMAND "--warnings-as-errors=*")
  endif()
  message(STATUS "eZeGo lint: clang-tidy ${_ez_tidy_major} (${_ez_tidy}) on first-party targets")
endif()

# Strict warnings, clang-tidy and -Werror for one first-party target.
function(ez_strict target)
  # N-7: -Wundef turns a misspelled feature macro in #if into a warning instead of a silent 0;
  # -Wreserved-identifier (Clang) catches names the implementation owns (__X, _Capital).
  target_compile_options(${target} PRIVATE -Wundef)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${target} PRIVATE -Wreserved-identifier)
  endif()
  if(EZ_WERROR)
    if(MSVC)
      target_compile_options(${target} PRIVATE /WX)
    else()
      target_compile_options(${target} PRIVATE -Werror)
    endif()
  endif()
  if(EZ_CLANG_TIDY_COMMAND)
    set_target_properties(${target} PROPERTIES CXX_CLANG_TIDY "${EZ_CLANG_TIDY_COMMAND}"
                                               C_CLANG_TIDY "${EZ_CLANG_TIDY_COMMAND}")
  endif()
endfunction()
