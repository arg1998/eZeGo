# Host toolchain selection (B-11). Used by every preset as CMAKE_TOOLCHAIN_FILE.
# Platform differences live here, not in preset names: `debug` means the same on every OS.
#
#   Linux   : clang / clang++ (newest found), linker mold -> lld -> default
#   macOS   : Apple Clang from the Xcode Command Line Tools, arm64 only
#   Windows : clang-cl + lld-link (MSVC libraries and Windows SDK from Build Tools)
#
# Override anything with CC/CXX env vars or -D on the command line / CMakeUserPresets.json.
# Escape hatch for the open GCC question (build-system.md §6 #9): -DEZ_ALLOW_GCC=ON.

if(EZ_TOOLCHAIN_LOADED)
  return()
endif()
set(EZ_TOOLCHAIN_LOADED ON)

set(_ez_versions 22 21 20 19 18 17)

function(_ez_find_versioned out base)
  set(_names "${base}")
  foreach(v IN LISTS _ez_versions)
    list(APPEND _names "${base}-${v}")
  endforeach()
  find_program(_found NAMES ${_names} NO_CACHE)
  set(${out} "${_found}" PARENT_SCOPE)
endfunction()

if(CMAKE_HOST_WIN32)
  if(NOT CMAKE_C_COMPILER AND NOT DEFINED ENV{CC})
    find_program(_clang_cl NAMES clang-cl NO_CACHE
      HINTS "$ENV{ProgramFiles}/LLVM/bin" "$ENV{VCINSTALLDIR}/Tools/Llvm/x64/bin")
    if(_clang_cl)
      set(CMAKE_C_COMPILER "${_clang_cl}")
      set(CMAKE_CXX_COMPILER "${_clang_cl}")
    endif()
  endif()
  find_program(_lld_link NAMES lld-link NO_CACHE
    HINTS "$ENV{ProgramFiles}/LLVM/bin" "$ENV{VCINSTALLDIR}/Tools/Llvm/x64/bin")
  if(_lld_link AND NOT CMAKE_LINKER)
    set(CMAKE_LINKER "${_lld_link}")
  endif()
  # Static C runtime everywhere (linking.md L-3).
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)
elseif(CMAKE_HOST_APPLE)
  set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Apple Silicon only")
  set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Minimum macOS (open question)")
  # Default cc/c++ on macOS is Apple Clang already.
else()
  if(NOT CMAKE_C_COMPILER AND NOT DEFINED ENV{CC})
    _ez_find_versioned(_cc clang)
    _ez_find_versioned(_cxx clang++)
    if(_cc AND _cxx)
      set(CMAKE_C_COMPILER "${_cc}")
      set(CMAKE_CXX_COMPILER "${_cxx}")
    elseif(NOT EZ_ALLOW_GCC)
      message(FATAL_ERROR
        "Clang was not found. eZeGo builds with Clang on every platform.\n"
        "Run `cmake -P scripts/doctor.cmake` for the install command,\n"
        "or pass -DEZ_ALLOW_GCC=ON to try GCC (unsupported).")
    endif()
  endif()
  # Fast linker: mold, else lld.
  find_program(_mold NAMES mold NO_CACHE)
  _ez_find_versioned(_lld ld.lld)
  if(_mold)
    set(EZ_LINKER_FLAG "-fuse-ld=mold")
  elseif(_lld)
    set(EZ_LINKER_FLAG "-fuse-ld=lld")
  endif()
  if(EZ_LINKER_FLAG)
    set(CMAKE_EXE_LINKER_FLAGS_INIT "${EZ_LINKER_FLAG}")
    set(CMAKE_SHARED_LINKER_FLAGS_INIT "${EZ_LINKER_FLAG}")
    set(CMAKE_MODULE_LINKER_FLAGS_INIT "${EZ_LINKER_FLAG}")
  endif()
endif()

# Compiler cache (optional, B-10). Disable with -DEZ_COMPILER_CACHE=OFF.
if(NOT DEFINED EZ_COMPILER_CACHE OR EZ_COMPILER_CACHE)
  find_program(_ccache NAMES sccache ccache NO_CACHE)
  if(_ccache)
    execute_process(COMMAND "${_ccache}" --version RESULT_VARIABLE _ccache_rc OUTPUT_QUIET ERROR_QUIET)
    if(_ccache_rc EQUAL 0)
      set(CMAKE_C_COMPILER_LAUNCHER "${_ccache}")
      set(CMAKE_CXX_COMPILER_LAUNCHER "${_ccache}")
    endif()
  endif()
endif()
