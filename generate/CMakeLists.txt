# CMakeLists.txt for color test pattern generator

cmake_minimum_required(VERSION 3.16)

project(generate_color_test_pattern)

# The parent directory is the root of this software project
set(ROOT ${CMAKE_SOURCE_DIR}/..)

set(CMAKE_BUILD_TYPE DEBUG)

# This project requires C++11
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED on)

add_compile_options(-D_GNU_SOURCE -Wall)
#add_compile_options(-Wall)

include_directories(${ROOT})

# List the files that comprise the project
file(GLOB headers ${ROOT}/*.h)
file(GLOB sources ${ROOT}/*.cpp)

# Exclude source files that are not included in this build
list(REMOVE_ITEM sources ${ROOT}/convert_descriptor.cpp)
list(REMOVE_ITEM sources ${ROOT}/dump_dpx.cpp)
list(REMOVE_ITEM sources ${ROOT}/hdr_dpx_error.cpp)

# Create folders for the source and header files
source_group("Headers" FILES ${headers})
source_group("Sources" FILES ${sources})

# Define the source files for building the color test pattern generator
add_executable(generate_color_test_pattern ${sources})

# Link in the standard math library
IF(NOT WIN32)
target_link_libraries(generate_color_test_pattern m)
ENDIF(NOT WIN32)

# Add target for removing all files created by CMake
add_custom_target(clean-cmake-files COMMAND ${CMAKE_COMMAND} -P clean-all.cmake)

# Add target for removing all build products and the files created by running CMake
add_custom_target(clean-all
   COMMAND ${CMAKE_BUILD_TOOL} clean
   COMMAND ${CMAKE_COMMAND} -P clean-all.cmake
)
