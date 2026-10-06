# GLFW 3.4 through its own CMake build, options set by us (B-8).
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
set(USE_MSVC_RUNTIME_LIBRARY_DLL OFF)   # static runtime (linking.md L-3)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)   # let the variables above win over option()
add_subdirectory("${EZ_DEP_glfw_DIR}" "${CMAKE_BINARY_DIR}/third_party/glfw" EXCLUDE_FROM_ALL SYSTEM)
ez_third_party(glfw)
# Consumers never get GL headers from GLFW: GL is loaded at runtime (no GL dev package needed).
target_compile_definitions(glfw INTERFACE GLFW_INCLUDE_NONE)
