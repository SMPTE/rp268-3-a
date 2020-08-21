# CMakeLists.txt for color test pattern generator

cmake_minimum_required(VERSION 3.17)

# Do not select the toolchain until the system name is available
project(generate_color_test_pattern NONE)

# The system name is set by the project line above
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(CMAKE_C_COMPILER "gcc-10")
	set(CMAKE_CXX_COMPILER "g++-10")
endif ()

# Enable use of C and C++ after the toolchain has been selected
enable_language(C CXX)

# The location of the top-level directory for this project
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

IF(NOT MSVC)
  # Link in the standard math library
  target_link_libraries(generate_color_test_pattern m)
ENDIF(NOT MSVC)

# Add target for removing all build products and the files created by running CMake
add_custom_target(clean-all
   COMMAND ${CMAKE_BUILD_TOOL} clean
   COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/clean-all.cmake
)
