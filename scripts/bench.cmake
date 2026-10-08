# Micro-benchmarks (specs/testing.md T-10): builds the release tree, runs every bench executable,
# prints min / median / p99 per operation and the change against this machine's previous run.
# Results are appended to build/bench/history.jsonl. No gate: numbers inform, they do not fail.
#
#   cmake -P scripts/bench.cmake [--filter=<text>] [--min-ms=<n>] [--no-build]

include("${CMAKE_CURRENT_LIST_DIR}/lib/common.cmake")
ez_parse_args()

if(NOT EZ_ARG_no_build)
  ez_step("building the release tree")
  execute_process(COMMAND "${CMAKE_COMMAND}" --workflow --preset release WORKING_DIRECTORY "${EZ_ROOT}"
    RESULT_VARIABLE _rc OUTPUT_QUIET)
  if(NOT _rc EQUAL 0)
    message(FATAL_ERROR "the release build failed; run `cmake --workflow --preset release` to see why")
  endif()
endif()

execute_process(COMMAND git -C "${EZ_ROOT}" rev-parse --short HEAD OUTPUT_VARIABLE _commit
  OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
execute_process(COMMAND git -C "${EZ_ROOT}" status --porcelain --untracked-files=no OUTPUT_VARIABLE _dirty ERROR_QUIET)
if(_dirty)
  set(_commit "${_commit}+dirty")
endif()
file(MAKE_DIRECTORY "${EZ_ROOT}/build/bench")
set(_args "--history=${EZ_ROOT}/build/bench/history.jsonl" "--commit=${_commit}")
if(EZ_ARG_filter)
  list(APPEND _args "--filter=${EZ_ARG_filter}")
endif()
if(EZ_ARG_min_ms)
  list(APPEND _args "--min-ms=${EZ_ARG_min_ms}")
endif()

file(GLOB _benches "${EZ_ROOT}/build/release/bench/ez_bench_*")
list(FILTER _benches EXCLUDE REGEX "\\.(pdb|ilk)$")
if(NOT _benches)
  message(FATAL_ERROR "no bench executables in build/release/bench")
endif()
foreach(_b IN LISTS _benches)
  get_filename_component(_n "${_b}" NAME_WE)
  ez_step("${_n}")
  execute_process(COMMAND "${_b}" ${_args} WORKING_DIRECTORY "${EZ_ROOT}")
endforeach()
message("\nhistory: build/bench/history.jsonl")
