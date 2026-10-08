# Test targets and per-case CTest discovery (specs/testing.md T-3, T-4, T-6).
#
#   ez_test(<name> SOURCES ... [DEPS ...] [LABEL <label>])
#
# Builds ez_test_<name> into <tree>/tests/. When <name> is a module from modules.cmake, that module
# is linked. Every doctest TEST_CASE becomes its own CTest test, so VS Code's Test Explorer and
# `ctest -R` work per case, and each case runs in its own process.
#
# Label of a case: its doctest TEST_SUITE if it has one ("integration", ...), otherwise LABEL,
# which defaults to "integration" for ez_test(integration ...) and "unit" for everything else.
# Timeout per label (T-6): a unit case over 1 s or an integration case over 10 s fails.

set(EZ_TEST_TIMEOUT_unit 1)
set(EZ_TEST_TIMEOUT_integration 10)
set(EZ_TEST_TIMEOUT_default 60)

function(ez_test name)
  cmake_parse_arguments(T "" "LABEL" "SOURCES;DEPS" ${ARGN})
  set(target ez_test_${name})
  if(NOT T_LABEL)
    if(name STREQUAL "integration")
      set(T_LABEL integration)
    else()
      set(T_LABEL unit)
    endif()
  endif()
  add_executable(${target} ${T_SOURCES})
  set(_module "")
  if(name IN_LIST EZ_MODULES)
    set(_module ez::${name})
  endif()
  target_link_libraries(${target} PRIVATE ez::config ez_test_support doctest ${_module} ${T_DEPS})
  ez_warnings(${target})
  set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests" FOLDER tests)

  # Discovery runs when ctest starts, and lists the cases again only when the executable changed.
  set(_exe "${CMAKE_BINARY_DIR}/tests/${target}${CMAKE_EXECUTABLE_SUFFIX}")
  set(_list "${CMAKE_CURRENT_BINARY_DIR}/${target}_tests.cmake")
  set(_include "${CMAKE_CURRENT_BINARY_DIR}/${target}_include.cmake")
  file(WRITE "${_include}"
    "set(EZ_TEST_EXE [==[${_exe}]==])\n"
    "set(EZ_TEST_LIST [==[${_list}]==])\n"
    "set(EZ_TEST_TARGET [==[${target}]==])\n"
    "set(EZ_TEST_LABEL [==[${T_LABEL}]==])\n"
    "set(EZ_TEST_WORKDIR [==[${CMAKE_BINARY_DIR}/tests]==])\n"
    "set(EZ_TEST_TIMEOUT_unit ${EZ_TEST_TIMEOUT_unit})\n"
    "set(EZ_TEST_TIMEOUT_integration ${EZ_TEST_TIMEOUT_integration})\n"
    "set(EZ_TEST_TIMEOUT_default ${EZ_TEST_TIMEOUT_default})\n"
    "include([==[${PROJECT_SOURCE_DIR}/cmake/test_discovery.cmake]==])\n")
  set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES "${_include}")
endfunction()

# ez_bench(<module> SOURCES ... [DEPS ...]): a micro-benchmark executable ez_bench_<module> in
# <tree>/bench/ (specs/testing.md T-10). Not a CTest test: `ez bench` runs these from the release tree.
function(ez_bench name)
  cmake_parse_arguments(B "" "" "SOURCES;DEPS" ${ARGN})
  set(target ez_bench_${name})
  add_executable(${target} ${B_SOURCES})
  set(_module "")
  if(name IN_LIST EZ_MODULES)
    set(_module ez::${name})
  endif()
  target_link_libraries(${target} PRIVATE ez::config ez_bench_support ${_module} ${B_DEPS})
  ez_warnings(${target})
  set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bench" FOLDER bench)
endfunction()
