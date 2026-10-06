# Editor integration (B-13): keep build/compile_commands.json pointing at the compile database of
# the most recently configured preset. clangd finds it through the tracked .clangd file, so code
# intelligence works whether VS Code opened the folder or the workspace, and follows mode switches.

set(_ez_db_dir "${PROJECT_SOURCE_DIR}/build")
set(_ez_db_link "${_ez_db_dir}/compile_commands.json")
set(_ez_db_src "${CMAKE_BINARY_DIR}/compile_commands.json")

# Only for preset trees under build/<preset>/ (not for ad-hoc build directories elsewhere).
string(FIND "${CMAKE_BINARY_DIR}/" "${_ez_db_dir}/" _ez_pos)
if(NOT CMAKE_EXPORT_COMPILE_COMMANDS OR NOT _ez_pos EQUAL 0)
  return()
endif()

file(MAKE_DIRECTORY "${_ez_db_dir}")
if(CMAKE_HOST_WIN32)
  # Symlinks need extra privileges on Windows: copy now if a database already exists (re-configure),
  # and refresh the copy on every build of this tree.
  if(EXISTS "${_ez_db_src}")
    file(COPY_FILE "${_ez_db_src}" "${_ez_db_link}" ONLY_IF_DIFFERENT)
  endif()
  add_custom_target(ez_compile_db ALL
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${_ez_db_src}" "${_ez_db_link}"
    COMMENT "Updating build/compile_commands.json for clangd"
    VERBATIM)
else()
  # A symlink to a file that CMake writes at the end of this configure: valid immediately after.
  file(REMOVE "${_ez_db_link}")
  file(CREATE_LINK "${_ez_db_src}" "${_ez_db_link}" SYMBOLIC)
endif()
