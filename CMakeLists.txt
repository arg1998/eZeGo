cmake_minimum_required(VERSION 3.28)


project(eZeGo VERSION 0.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# Find Boost libraries
find_package(Boost REQUIRED)
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

target_include_directories(eZeGo PRIVATE ${Boost_INCLUDE_DIRS})