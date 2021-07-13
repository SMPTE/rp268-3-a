# Remove all files created by CMake

# (c) 2013-2017 Society of Motion Picture & Television Engineers LLC and Woodman Labs, Inc.
# All rights reserved--use subject to compliance with end user license agreement.

# List of files created by CMake
set(cmake_generated_files ${CMAKE_BINARY_DIR}/CMakeCache.txt
						  ${CMAKE_BINARY_DIR}/cmake_install.cmake
						  ${CMAKE_BINARY_DIR}/Makefile
						  ${CMAKE_BINARY_DIR}/CMakeFiles
						  ${CMAKE_BINARY_DIR}/../../docs
)

foreach(file ${cmake_generated_files})
  if (EXISTS ${file})
     file(REMOVE_RECURSE ${file})
  endif()
endforeach(file)
