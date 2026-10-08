# ez_test_support (specs/testing.md §4, §5): the one main(), doctest's implementation, the
# subprocess runner and the temp directory. An object library, so every object reaches every test
# executable, including doctest listener registration.
set(_proc_posix subprocess_posix.cpp)
set(_proc_windows subprocess_windows.cpp)
if(WIN32)
  set(_proc ${_proc_windows})
else()
  set(_proc ${_proc_posix})
endif()

add_library(ez_test_support OBJECT main.cpp subprocess.hpp subprocess.cpp temp_dir.hpp temp_dir.cpp ${_proc})
target_include_directories(ez_test_support PUBLIC "${PROJECT_SOURCE_DIR}/tests")
target_link_libraries(ez_test_support PUBLIC ez::config ez::base doctest)
ez_warnings(ez_test_support)
# doctest's implementation is compiled once, here. Set by the build so no first-party file defines
# a non-EZ_ macro (naming.md N-7).
set_source_files_properties(main.cpp PROPERTIES COMPILE_DEFINITIONS DOCTEST_CONFIG_IMPLEMENT)

# ez_bench_support: the runner main of every bench executable (specs/testing.md §9).
add_library(ez_bench_support OBJECT bench.hpp bench_main.cpp)
target_include_directories(ez_bench_support PUBLIC "${PROJECT_SOURCE_DIR}/tests")
target_link_libraries(ez_bench_support PUBLIC ez::config ez::base)
ez_warnings(ez_bench_support)
