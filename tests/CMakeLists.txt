# Tests are part of the main project (B-12, specs/testing.md). They link headless modules only:
# nothing here can see GLFW, ImGui or a GPU. One executable per module (ez_test, cmake/testing.cmake),
# one CTest test per doctest case. Labels: unit, integration, smoke, lint; gpu is excluded by the
# presets and runs with `ctest --preset debug -L gpu`.

add_subdirectory(support)

# One directory per module, mirroring src/ez/ (CO-5), in module-table order.
foreach(_m IN LISTS EZ_MODULES)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_m}/CMakeLists.txt")
    add_subdirectory(${_m})
  endif()
endforeach()

add_subdirectory(bench)
add_subdirectory(legacy)

# Lint as a test in the check tree only (naming.md N-13): layout rules and formatting. clang-tidy
# already ran as part of this tree's build, so it is skipped here.
if(EZ_LINT)
  add_test(NAME lint COMMAND "${CMAKE_COMMAND}" -P "${PROJECT_SOURCE_DIR}/scripts/lint.cmake" --tidy=off)
  set_tests_properties(lint PROPERTIES LABELS lint)
endif()

# Smoke: the real application binary starts and answers --version without opening a window.
add_test(NAME smoke_app_version COMMAND ezego --version)
set_tests_properties(smoke_app_version PROPERTIES LABELS smoke PASS_REGULAR_EXPRESSION "eZeGo [0-9]+\\.[0-9]+\\.[0-9]+")

# GUI smoke: opens a window for 3 s. Needs a display and GPU, so it is labelled "gpu" and
# excluded by the test presets. Run explicitly: ctest --preset debug -L gpu
add_test(NAME smoke_gui_starts COMMAND ezego --quit-after 3)
set_tests_properties(smoke_gui_starts PROPERTIES LABELS "gpu;smoke")
