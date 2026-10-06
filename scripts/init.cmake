# First-time setup (B-6). Idempotent: re-running is always safe.
#
#   cmake -P scripts/init.cmake [--tools=auto|prebuilt|source|skip] [--preset=debug] [--ignore-doctor]
#
#   1. doctor   report missing system prerequisites (stops here on errors unless --ignore-doctor)
#   2. deps     shallow-fetch every dependency at its pinned commit
#   3. tools    install pinned dev tools (Tracy); failures are warnings, never blockers
#   4. vscode   generate eZeGo.code-workspace with the paths found on this machine
#   5. configure the debug preset, so the first build is one command away

include("${CMAKE_CURRENT_LIST_DIR}/lib/common.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/lib/tooling.cmake")
ez_parse_args()
if(NOT EZ_ARG_tools)
  set(EZ_ARG_tools auto)
endif()
if(NOT EZ_ARG_preset)
  set(EZ_ARG_preset debug)
endif()

function(ez_sub label)
  execute_process(COMMAND ${ARGN} WORKING_DIRECTORY "${EZ_ROOT}" RESULT_VARIABLE rc)
  set(EZ_SUB_RC ${rc} PARENT_SCOPE)
endfunction()

message("${EZ_C_BOLD}eZeGo init${EZ_C_RESET}  (${EZ_HOST}, ${EZ_ROOT})\n")

# System prerequisites only: on a fresh clone the repository checks would just say "not
# fetched", which is exactly what the next step does. Run ./ez doctor afterwards for everything.
ez_sub(doctor "${CMAKE_COMMAND}" -DEZ_DOCTOR_SYSTEM_ONLY=ON -P "${EZ_ROOT}/scripts/doctor.cmake")
if(NOT EZ_SUB_RC EQUAL 0 AND NOT EZ_ARG_ignore_doctor)
  message(FATAL_ERROR "init stopped: install the system prerequisites listed above, then re-run init "
                      "(or pass --ignore-doctor to continue anyway).")
endif()

message("")
ez_sub(deps "${CMAKE_COMMAND}" -P "${EZ_ROOT}/scripts/deps.cmake")
if(NOT EZ_SUB_RC EQUAL 0)
  message(FATAL_ERROR "init stopped: dependency fetch failed (see above).")
endif()

message("")
ez_sub(tools "${CMAKE_COMMAND}" -P "${EZ_ROOT}/scripts/tools.cmake" "--tools=${EZ_ARG_tools}" --soft)

message("")
ez_step("Configuring preset '${EZ_ARG_preset}'")
ez_sub(configure "${CMAKE_COMMAND}" --preset "${EZ_ARG_preset}")
if(NOT EZ_SUB_RC EQUAL 0)
  message(FATAL_ERROR "init stopped: configure failed (see above).")
endif()

message("")
message("${EZ_C_GREEN}init complete.${EZ_C_RESET} Next:")
message("  cmake --workflow --preset debug      build            (./ez build)")
message("  ctest --preset debug                 test             (./ez test)")
message("  cmake -P scripts/profile.cmake       profile w/ Tracy (./ez profile)")
message("  code eZeGo.code-workspace            open in VS Code")
