# Dev-tool recipe: Tracy profiler GUI + CLI tools (B-14).
# Pinned by the "tracy" entry in dependencies.json; the same pin builds the client library in
# the profile preset, so GUI and client always speak the same protocol version.
#
# A recipe defines: TOOL_DEP, and the functions tool_binaries, tool_install_prebuilt,
# tool_build_source. scripts/tools.cmake does everything else (cache dir, provenance, prompts).

set(TOOL_DEP tracy)

# Where each binary lives inside the install dir, per host OS.
function(tool_binaries install_dir out_var)
  if(EZ_HOST_OS STREQUAL "windows")
    set(_b "profiler=${install_dir}/tracy-profiler.exe"
           "capture=${install_dir}/tracy-capture.exe"
           "csvexport=${install_dir}/tracy-csvexport.exe")
  elseif(EZ_HOST_OS STREQUAL "macos")
    set(_b "profiler=${install_dir}/tracy-profiler.app/Contents/MacOS/tracy-profiler"
           "capture=${install_dir}/tracy-capture"
           "csvexport=${install_dir}/tracy-csvexport")
  else()
    set(_b "profiler=${install_dir}/tracy-profiler"
           "capture=${install_dir}/tracy-capture"
           "csvexport=${install_dir}/tracy-csvexport")
  endif()
  set(${out_var} "${_b}" PARENT_SCOPE)
endfunction()

# Unpack a verified upstream release zip into install_dir.
function(tool_install_prebuilt archive install_dir)
  file(ARCHIVE_EXTRACT INPUT "${archive}" DESTINATION "${install_dir}")
  if(EZ_HOST_OS STREQUAL "linux")
    # The Linux GUI ships as an AppImage. Extract it once so it runs without FUSE, and
    # expose a stable launcher name.
    set(_appimage "${install_dir}/tracy-profiler-x86_64.AppImage")
    file(CHMOD "${_appimage}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    execute_process(COMMAND "${_appimage}" --appimage-extract
      WORKING_DIRECTORY "${install_dir}" RESULT_VARIABLE _rc OUTPUT_QUIET ERROR_VARIABLE _err)
    if(NOT _rc EQUAL 0)
      message(FATAL_ERROR "could not extract the Tracy AppImage: ${_err}")
    endif()
    file(REMOVE "${_appimage}")
    file(WRITE "${install_dir}/tracy-profiler"
      "#!/bin/sh\nexec \"$(dirname \"$0\")/squashfs-root/AppRun\" \"$@\"\n")
  endif()
  if(NOT EZ_HOST_OS STREQUAL "windows")
    file(GLOB _bins "${install_dir}/tracy-*")
    foreach(_f IN LISTS _bins)
      if(NOT IS_DIRECTORY "${_f}")
        file(CHMOD "${_f}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
      endif()
    endforeach()
    if(EZ_HOST_OS STREQUAL "macos")
      file(CHMOD "${install_dir}/tracy-profiler.app/Contents/MacOS/tracy-profiler"
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif()
  endif()
endfunction()

# Build GUI + CLI tools from the pinned, already-fetched source (third_party/_src/tracy).
# Tracy's own build downloads its GUI dependencies the first time (CPM); that cache is kept per
# user so it happens once.
function(tool_build_source src_dir build_root install_dir)
  ez_user_cache_root(_cache)
  set(_common -G Ninja -DCMAKE_BUILD_TYPE=Release "-DCPM_SOURCE_CACHE=${_cache}/cpm")
  if(EZ_HOST_OS STREQUAL "linux")
    # Tracy prefers mold with clang; follow whatever the environment has.
    find_program(_mold mold NO_CACHE)
    if(NOT _mold)
      list(APPEND _common -DNO_MOLD_LINKER=ON)
    endif()
  endif()
  foreach(_part profiler capture csvexport)
    message("     building tracy ${_part} (Release) ...")
    set(_extra "")
    if(_part STREQUAL "profiler")
      set(_extra -DNO_FILESELECTOR=ON)  # avoids a D-Bus/GTK build dependency; open traces via argv
    endif()
    execute_process(COMMAND "${CMAKE_COMMAND}" -S "${src_dir}/${_part}" -B "${build_root}/${_part}" ${_common} ${_extra}
      RESULT_VARIABLE _rc OUTPUT_VARIABLE _out ERROR_VARIABLE _out)
    if(_rc EQUAL 0)
      execute_process(COMMAND "${CMAKE_COMMAND}" --build "${build_root}/${_part}" --parallel
        RESULT_VARIABLE _rc OUTPUT_VARIABLE _out ERROR_VARIABLE _out)
    endif()
    if(NOT _rc EQUAL 0)
      string(LENGTH "${_out}" _len)
      if(_len GREATER 3000)
        math(EXPR _start "${_len} - 3000")
        string(SUBSTRING "${_out}" ${_start} 3000 _out)
      endif()
      message(FATAL_ERROR "tracy ${_part} failed to build:\n...${_out}\n(full log: ${build_root}/${_part})")
    endif()
  endforeach()
  file(MAKE_DIRECTORY "${install_dir}")
  if(EZ_HOST_OS STREQUAL "windows")
    set(_ext ".exe")
  endif()
  file(COPY_FILE "${build_root}/profiler/tracy-profiler${_ext}" "${install_dir}/tracy-profiler${_ext}")
  file(COPY_FILE "${build_root}/capture/tracy-capture${_ext}" "${install_dir}/tracy-capture${_ext}")
  file(COPY_FILE "${build_root}/csvexport/tracy-csvexport${_ext}" "${install_dir}/tracy-csvexport${_ext}")
  if(EZ_HOST_OS STREQUAL "macos")
    # Same layout as the prebuilt: an .app bundle.
    file(MAKE_DIRECTORY "${install_dir}/tracy-profiler.app/Contents/MacOS")
    file(RENAME "${install_dir}/tracy-profiler" "${install_dir}/tracy-profiler.app/Contents/MacOS/tracy-profiler")
  endif()
endfunction()
