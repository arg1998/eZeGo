# Regenerate eZeGo.code-workspace from what is installed on this machine (B-13).
# init and tools run this automatically; run it yourself after installing clangd, etc.
#
#   cmake -P scripts/vscode.cmake
include("${CMAKE_CURRENT_LIST_DIR}/lib/common.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/lib/tooling.cmake")
ez_write_vscode_workspace()
