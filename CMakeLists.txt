cmake_minimum_required(VERSION 3.28)


project(eZeGo VERSION 0.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

# set the project source code directory
set(EZEGO_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/")

# collect all the .cpp files recursively
file(GLOB_RECURSE EZEGO_CPP_FILES ${EZEGO_SRC_DIR}/*.cpp)

add_executable(eZeGo ${EZEGO_CPP_FILES})

target_include_directories(eZeGo PRIVATE ${EZEGO_SRC_DIR})


set_target_properties(eZeGo PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")


# ------------------- Linking Libs ------------------- #
add_subdirectory(libs)

# Link the IMGUI library
target_link_libraries(eZeGo PRIVATE IMGUI)

# Link GLFW library
target_link_libraries(eZeGo PRIVATE glfw)

# link rtmidi library
target_link_libraries(eZeGo PRIVATE rtmidi)

# link fmt library
target_link_libraries(eZeGo PRIVATE fmt::fmt)