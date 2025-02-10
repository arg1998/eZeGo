# # Fetch all test files
# file(GLOB TEST_FILES *.cpp)

# # Loop through each test file and create an executable & test
# foreach(TEST_FILE ${TEST_FILES})
#     # Extract filename without extension
#     get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)

#     # Create test executable
#     add_executable(${TEST_NAME} ${TEST_FILE})
#     target_link_libraries(${TEST_NAME} PRIVATE MyProjectLib)

#     # Register the test
#     add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
# endforeach()

cmake_minimum_required(VERSION 3.28)


project(eZeGo VERSION 0.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# Define custom build type for Tracy Profiler
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;RelWithTracyProfiler" CACHE STRING "Available build types" FORCE)

# set the project source code directory
set(EZEGO_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src/")
message(STATUS "Source Directory: ${EZEGO_SRC_DIR}")
# collect all the .cpp files recursively
file(GLOB_RECURSE EZEGO_CPP_FILES ${EZEGO_SRC_DIR}/*.cpp)

add_executable(IOTest file_io_test.cpp ${EZEGO_SRC_DIR}/core/file/fileIO.cpp)
target_include_directories(IOTest PRIVATE ${EZEGO_SRC_DIR})
# ------------------- Download the json cpp ------------------- # This is not working for MACOS
# include(FetchContent)
# FetchContent_Declare(
#     json
#     URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp
# )
# FetchContent_MakeAvailable(json)
find_package(nlohmann_json REQUIRED)




# ------------------- Linking Libs ------------------- #
# set(BUILD_SHARED_LIBS OFF)
# add_subdirectory(libs)





target_link_libraries(IOTest PRIVATE nlohmann_json::nlohmann_json)
