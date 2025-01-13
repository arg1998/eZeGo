cmake_minimum_required(VERSION 3.28)


project(eZeGo VERSION 0.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# Define custom build type for Tracy Profiler
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;RelWithTracyProfiler" CACHE STRING "Available build types" FORCE)

# set the project source code directory
set(EZEGO_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/")

# collect all the .cpp files recursively
file(GLOB_RECURSE EZEGO_CPP_FILES ${EZEGO_SRC_DIR}/*.cpp)

add_executable(eZeGo ${EZEGO_CPP_FILES})
target_include_directories(eZeGo PRIVATE ${EZEGO_SRC_DIR})
target_compile_definitions(eZeGo PRIVATE EZ_APPLICATION_VERSION_STR="${CMAKE_PROJECT_VERSION}")
set_target_properties(eZeGo PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")


# ------------------- Linking Libs ------------------- #
set(BUILD_SHARED_LIBS OFF)
add_subdirectory(libs)

# Link the IMGUI library
target_link_libraries(eZeGo PRIVATE IMGUI)

# Link GLFW library
target_link_libraries(eZeGo PRIVATE glfw)

# link rtmidi library
target_link_libraries(eZeGo PRIVATE rtmidi)


# ------------------------ Conditional Build for Profiling ---------------------------

# Set RelWithTracyProfiler build type settings
if(CMAKE_BUILD_TYPE STREQUAL "RelWithTracyProfiler")
    set(CMAKE_CXX_FLAGS_RELWITHTRACYPROFILER "${CMAKE_CXX_FLAGS_RELEASE}")
    set(CMAKE_EXE_LINKER_FLAGS_RELWITHTRACYPROFILER "${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
    set(CMAKE_SHARED_LINKER_FLAGS_RELWITHTRACYPROFILER "${CMAKE_SHARED_LINKER_FLAGS_RELEASE}")

    # Enable Tracy options and add Tracy dependency
    option(TRACY_ENABLE "Enable Tracy Profiler" ON)
    option(TRACY_ON_DEMAND "Enable Tracy On-Demand Mode" ON)
else()
    # Disable Tracy options
    option(TRACY_ENABLE "Enable Tracy Profiler" OFF)
    option(TRACY_ON_DEMAND "Enable Tracy On-Demand Mode" OFF)
endif()

add_subdirectory(libs/tracy)

# Disable Werror for TracyClient
target_compile_options(TracyClient PRIVATE -Wno-error)
target_link_libraries(eZeGo PRIVATE Tracy::TracyClient)

