# Tests are part of the main project (B-12). Unit tests link headless modules only: nothing here
# can see GLFW, ImGui or a GPU. Labels: unit, smoke (and gpu, excluded by the presets).

function(ez_test name)
  cmake_parse_arguments(T "" "" "SOURCES;DEPS;LABELS" ${ARGN})
  ez_executable(${name} SOURCES ${T_SOURCES} DEPS ${T_DEPS})
  set_target_properties(${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
  target_include_directories(${name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  add_test(NAME ${name} COMMAND ${name})
  set_tests_properties(${name} PROPERTIES LABELS "${T_LABELS}")
endfunction()

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
# excluded by the test presets. Run explicitly: ctest --test-dir build/debug -L gpu
add_test(NAME smoke_gui_starts COMMAND ezego --quit-after 3)
set_tests_properties(smoke_gui_starts PROPERTIES LABELS "gpu;smoke")
