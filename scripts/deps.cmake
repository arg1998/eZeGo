# Sync third_party/_src/ to dependencies.json (B-7).
#
#   cmake -P scripts/deps.cmake [--only=imgui,sdl3] [--force] [--check]
#
# Each dependency is a shallow git checkout of exactly the pinned commit: no history.
# --check   report only, change nothing (exit 1 if anything is out of date)
# --force   re-fetch even if the checkout has local modifications

include("${CMAKE_CURRENT_LIST_DIR}/lib/common.cmake")
ez_parse_args()
ez_require_git(GIT)

if(EZ_ARG_only)
  string(REPLACE "," ";" _wanted "${EZ_ARG_only}")
else()
  ez_manifest_names(_wanted)
endif()

file(MAKE_DIRECTORY "${EZ_SRC_ROOT}" "${EZ_STAMP_DIR}")

# State of one checkout: MISSING | OK | WRONG_COMMIT | DIRTY
function(ez_dep_state name commit out_state out_head)
  set(_dir "${EZ_SRC_ROOT}/${name}")
  if(NOT EXISTS "${_dir}/.git")
    set(${out_state} MISSING PARENT_SCOPE)
    return()
  endif()
  ez_run(COMMAND "${GIT}" rev-parse HEAD WORKING_DIRECTORY "${_dir}" OUTPUT _head ALLOW_FAIL RESULT _rc)
  set(${out_head} "${_head}" PARENT_SCOPE)
  if(NOT _rc EQUAL 0 OR NOT _head STREQUAL commit)
    set(${out_state} WRONG_COMMIT PARENT_SCOPE)
    return()
  endif()
  ez_run(COMMAND "${GIT}" status --porcelain WORKING_DIRECTORY "${_dir}" OUTPUT _st)
  if(NOT _st STREQUAL "")
    set(${out_state} DIRTY PARENT_SCOPE)
  else()
    set(${out_state} OK PARENT_SCOPE)
  endif()
endfunction()

function(ez_fetch name url ref commit)
  set(_dir "${EZ_SRC_ROOT}/${name}")
  if(NOT EXISTS "${_dir}/.git")
    file(REMOVE_RECURSE "${_dir}")
    file(MAKE_DIRECTORY "${_dir}")
    ez_run(COMMAND "${GIT}" init --quiet WORKING_DIRECTORY "${_dir}")
    ez_run(COMMAND "${GIT}" remote add origin "${url}" WORKING_DIRECTORY "${_dir}")
  else()
    ez_run(COMMAND "${GIT}" remote set-url origin "${url}" WORKING_DIRECTORY "${_dir}")
  endif()

  # Preferred: fetch the commit itself, depth 1.
  ez_run(COMMAND "${GIT}" fetch --quiet --depth 1 --no-tags origin "${commit}"
    WORKING_DIRECTORY "${_dir}" ALLOW_FAIL RESULT _rc OUTPUT _out)
  if(NOT _rc EQUAL 0)
    # Fallback for hosts that refuse fetch-by-hash: fetch the ref, then insist it IS the pin.
    message("     ${EZ_C_DIM}host refused fetch-by-commit; trying ref '${ref}'${EZ_C_RESET}")
    ez_run(COMMAND "${GIT}" fetch --quiet --depth 1 --no-tags origin "${ref}" WORKING_DIRECTORY "${_dir}")
  endif()
  ez_run(COMMAND "${GIT}" rev-parse FETCH_HEAD WORKING_DIRECTORY "${_dir}" OUTPUT _fetched)
  if(NOT _fetched STREQUAL commit)
    message(FATAL_ERROR "${name}: '${ref}' resolves to ${_fetched}, but the manifest pins ${commit}.\n"
                        "The ref moved. Update the pin deliberately or fix the ref.")
  endif()
  ez_run(COMMAND "${GIT}" -c advice.detachedHead=false checkout --quiet --force FETCH_HEAD WORKING_DIRECTORY "${_dir}")
  ez_run(COMMAND "${GIT}" clean -fdxq WORKING_DIRECTORY "${_dir}")

  # Patches are a last resort (B-7) and live in third_party/patches/<name>/.
  file(GLOB _patches "${EZ_ROOT}/third_party/patches/${name}/*.patch")
  list(SORT _patches)
  foreach(_p IN LISTS _patches)
    ez_run(COMMAND "${GIT}" apply "${_p}" WORKING_DIRECTORY "${_dir}")
    message("     applied ${_p}")
  endforeach()

  file(WRITE "${EZ_STAMP_DIR}/${name}" "${commit}\n")
endfunction()

set(EZ_REPORT_ERRORS 0)
set(EZ_REPORT_WARNINGS 0)
if(EZ_ARG_check)
  ez_step("Checking dependencies against dependencies.json")
else()
  ez_step("Syncing dependencies (shallow, pinned commits) into third_party/_src/")
endif()

foreach(name IN LISTS _wanted)
  ez_manifest_get(${name} url url)
  ez_manifest_get(${name} ref ref)
  ez_manifest_get(${name} commit commit)
  ez_manifest_get(${name} version version)
  string(SUBSTRING "${commit}" 0 10 _short)
  ez_dep_state(${name} ${commit} state head)
  file(GLOB _has_patches "${EZ_ROOT}/third_party/patches/${name}/*.patch")

  if(state STREQUAL "OK" OR (state STREQUAL "DIRTY" AND _has_patches))
    ez_report(OK "${name}" "${version} @ ${_short}")
    if(NOT EXISTS "${EZ_STAMP_DIR}/${name}")
      file(WRITE "${EZ_STAMP_DIR}/${name}" "${commit}\n")
    endif()
    continue()
  endif()

  if(EZ_ARG_check)
    if(state STREQUAL "MISSING")
      ez_report(ERROR "${name}" "not fetched" "cmake -P scripts/deps.cmake")
    elseif(state STREQUAL "WRONG_COMMIT")
      ez_report(ERROR "${name}" "at ${head}, manifest pins ${_short}" "cmake -P scripts/deps.cmake")
    else()
      ez_report(WARN "${name}" "has local modifications" "inspect with: git -C third_party/_src/${name} diff")
    endif()
    continue()
  endif()

  if(state STREQUAL "DIRTY" AND NOT EZ_ARG_force)
    ez_report(WARN "${name}" "has local modifications; left alone" "save them as a patch, or re-run with --force")
    continue()
  endif()

  message("  ...   ${name}  fetching ${version} @ ${_short}")
  ez_fetch(${name} "${url}" "${ref}" "${commit}")
  ez_report(OK "${name}" "${version} @ ${_short} (fetched)")
endforeach()

if(EZ_REPORT_ERRORS GREATER 0)
  message(FATAL_ERROR "${EZ_REPORT_ERRORS} dependency problem(s).")
endif()
