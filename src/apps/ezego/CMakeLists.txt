# The eZeGo application executable: main() only; everything else is in modules (CO-6).
ez_executable(ezego SOURCES main.cpp DEPS ez::app)
if(TARGET ez_metrics_new_hooks)
  target_link_libraries(ezego PRIVATE ez_metrics_new_hooks)  # profile builds: operator new hooks
endif()

# Assets next to the binary, refreshed on every build.
add_custom_command(TARGET ezego POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
          "${PROJECT_SOURCE_DIR}/assets" "$<TARGET_FILE_DIR:ezego>/assets"
  COMMENT "Copying assets")

# Release: keep symbols in a separate file beside the binary (B-3). Linux only for now.
if(EZ_MODE STREQUAL "release" AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_OBJCOPY)
  add_custom_command(TARGET ezego POST_BUILD
    COMMAND "${CMAKE_OBJCOPY}" --only-keep-debug "$<TARGET_FILE:ezego>" "$<TARGET_FILE:ezego>.debug"
    COMMAND "${CMAKE_OBJCOPY}" --strip-debug --add-gnu-debuglink=ezego.debug "$<TARGET_FILE:ezego>"
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:ezego>"
    COMMENT "Splitting debug symbols into ezego.debug")
endif()
