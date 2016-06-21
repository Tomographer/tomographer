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
if(MINGW)
  # On MinGW, we need to specify gnu++11 instead of c++11 standard to work around a bug
  CHECK_CXX_COMPILER_FLAG("-std=gnu++11" COMPILER_SUPPORTS_GNUXX11)
endif()

#
# Using "if(NOT mypackage_FOUND)" for the find_package() directives below allows the user
# to specify custom values to configurations (such as explicitly turning off OpenMP) and
# not being interefered with by a call to find_package().
#

# Find Eigen3 package
if(NOT EIGEN3_FOUND)
  find_package(Eigen3 REQUIRED)
endif(NOT EIGEN3_FOUND)

# Find OpenMP
if(NOT OPENMP_FOUND)
  find_package(OpenMP)
endif(NOT OPENMP_FOUND)

# Find MatIO library (https://sourceforge.net/projects/matio/)
if(NOT MATIO_FOUND)
  find_package(MatIO REQUIRED)
endif(NOT MATIO_FOUND)

# ... which requires the libz library
if(NOT ZLIB_FOUND)
  find_package(ZLIB REQUIRED)
endif(NOT ZLIB_FOUND)

# Find boost::program_options and boost::math
if(NOT BOOST_FOUND)
  find_package(Boost 1.40 COMPONENTS program_options REQUIRED)
endif(NOT BOOST_FOUND)


# more find_package stuff below for stuff we only need in tests....


# activate warning flags on DEBUG and RELWITHDEBINFO builds
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -Wall -Wextra")

# .### This is no longer needed as I have now included a function call with the GCC
# .### attribute __attribute__((force_align_arg_pointer,noinline)), which gives the code
# .### the necessary chance to realign the stack.
# 
# if(MINGW) MinGW: avoid misalignment errors with Eigen objects in OpenMP blocks. For
# this, force realignment of all stacks. Ugly, but works.  There doesn't even seem to be a
# large overhead---see http://stackoverflow.com/a/6718067/1694896
# set(TOMOGRAPHER_MINGW_STACK_REALIGN_FLAG "-mstackrealign" CACHE STRING "The flag(s) to
# pass to the MinGW C++ compiler to fix alignment issues with OpenMP")
# mark_as_advanced(TOMOGRAPHER_MINGW_STACK_REALIGN_FLAG) set(CMAKE_CXX_FLAGS
# "${CMAKE_CXX_FLAGS} ${TOMOGRAPHER_MINGW_STACK_REALIGN_FLAG}") endif()


# optimization flags for computer architecture.
# -> use -DTARGET_ARCHITECTURE=none to not pass "-m..." options to compiler at all
# -> use -DTARGET_ARCHITECTURE=generic to disable simd instruction sets
include(OptimizeForArchitecture)
OptimizeForArchitecture()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Vc_ARCHITECTURE_FLAGS}")

# ==============================================================================
# Documentation Target
# ==============================================================================

find_package(Doxygen)
if(DOXYGEN_FOUND)
  add_custom_target(doc
    COMMAND "${CMAKE_COMMAND}" "-DVersionFile=${CMAKE_BINARY_DIR}/cxx/tomographer2/tomographer_VERSION" "-DRootSourceDir=${CMAKE_SOURCE_DIR}" "-DTargetDoxyfileDir=${CMAKE_BINARY_DIR}" -P "${CMAKE_SOURCE_DIR}/cmake/CreateDoxyfile.cmake"
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

# Install README into documentation directory
install(FILES README.md README.txt LICENSE.txt
  DESTINATION "${CMAKE_INSTALL_DOCDIR}"
  )


# ==============================================================================
# Tests?
# ==============================================================================

option(TOMOGRAPHER_ENABLE_TESTS "Build Tomographer/Tomorun tests" off)

if(TOMOGRAPHER_ENABLE_TESTS)

  if(NOT Boost_UNIT_TEST_FRAMEWORK_LIBRARY)
    find_package(Boost 1.40 COMPONENTS unit_test_framework REQUIRED)
  endif()
  
  # LAPACK is only required if you want to run all the tests (but not for tomorun)
  if(NOT LAPACK_FOUND)
    find_package(LAPACK REQUIRED)
  endif(NOT LAPACK_FOUND)

  message(STATUS "Will build tomographer tests")
  enable_testing()
else(TOMOGRAPHER_ENABLE_TESTS)
  message(STATUS "Will not build tests (meant for developers). Set TOMOGRAPHER_ENABLE_TESTS=on if you wanted them.")
endif(TOMOGRAPHER_ENABLE_TESTS)


# ==============================================================================
# Include our subdirectories
# ==============================================================================

add_subdirectory(cxx)



