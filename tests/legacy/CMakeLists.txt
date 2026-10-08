# Tests of the prototype under src/core (replaced, not renamed: naming.md §14). Exempt from the
# naming lint and outside the doctest setup. Removed together with src/core.
set(EZ_LEGACY ON)

ez_executable(test_core SOURCES test_core.cpp DEPS ez::core)
set_target_properties(test_core PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
target_include_directories(test_core PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME test_core COMMAND test_core)
set_tests_properties(test_core PROPERTIES LABELS unit)
