# This file is part of the Tomographer project, which is distributed under the
# terms of the MIT license.
# 
# The MIT License (MIT)
# 
# Copyright (c) 2015 ETH Zurich, Institute for Theoretical Physics, Philippe
# Faist
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 2.8.5)

project(tomographer)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(TomographerVersion)
include(Util)

# default to release build with debugging info
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()
message(STATUS "CMake Build Type: ${CMAKE_BUILD_TYPE}")


# ==============================================================================
# Find specific features and packages
# ==============================================================================

# Check for C++0x/C++11 features
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)

# Find Eigen3 package
find_package(Eigen3 REQUIRED)

# Find OpenMP
find_package(OpenMP)

# Find MatIO library (https://sourceforge.net/projects/matio/)
find_package(MatIO REQUIRED)

# ... which requires the libz library
find_package(ZLIB REQUIRED)

# Find boost::program_options and boost::math
find_package(Boost 1.40 COMPONENTS program_options unit_test_framework REQUIRED)


# activate warning flags on debug builds
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -Wall -Wextra")


# optimization flags for computer architecture.
# -> set TARGET_ARCHITECTURE=generic to disable optimizations
include(OptimizeForArchitecture)
OptimizeForArchitecture()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Vc_ARCHITECTURE_FLAGS}")

# ==============================================================================
# Documentation Target
# ==============================================================================

find_package(Doxygen)
if(DOXYGEN_FOUND)
  add_custom_target(doc
    COMMAND "${CMAKE_COMMAND}" "-DVersionFile=${CMAKE_BINARY_DIR}/cxx/tomographer/tomographer_VERSION" "-DRootSourceDir=${CMAKE_SOURCE_DIR}" "-DTargetDoxyfileDir=${CMAKE_BINARY_DIR}" -P "${CMAKE_SOURCE_DIR}/cmake/CreateDoxyfile.cmake"
    COMMAND "${DOXYGEN_EXECUTABLE}" "${CMAKE_BINARY_DIR}/Doxyfile"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
  # to make sure the version file is set up properly
  add_dependencies(doc tomographer)
  message(STATUS "You can build the API documentation by running `make doc`.")
else(DOXYGEN_FOUND)
  message(STATUS "Doxygen not found. Don't worry, you'd only need this if you wanted to generate API documentation.")
endif(DOXYGEN_FOUND)

# ==============================================================================
# Set up Install Targets
# ==============================================================================

include(GNUInstallDirs)


# Set up uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)
add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
    )


# ==============================================================================
# Tests?
# ==============================================================================

option(TOMOGRAPHER_ENABLE_TESTS "Build Tomographer/Tomorun tests" off)
if(TOMOGRAPHER_ENABLE_TESTS)
  message(STATUS "Will build tomographer tests")
  enable_testing()
else(TOMOGRAPHER_ENABLE_TESTS)
  message(STATUS "Will not build tests. Set TOMOGRAPHER_ENABLE_TESTS=on for that.")
endif(TOMOGRAPHER_ENABLE_TESTS)

# ==============================================================================
# Include our subdirectories
# ==============================================================================

add_subdirectory(cxx)



