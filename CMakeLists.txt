# This file is part of the Tomographer project, which is distributed under the
# terms of the MIT license.
# 
# The MIT License (MIT)
# 
# Copyright (c) 2016 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
# Copyright (c) 2017 Caltech, Institute for Quantum Information and Matter, Philippe Faist
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

cmake_minimum_required(VERSION 3.1)

project(tomographer)

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/vc")

include(TomographerVersion)
include(Util)

# default to release build with debugging info
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
    "Release Type -- defaults to RelWithDebInfo" FORCE)
endif()
message(STATUS "CMake Build Type: ${CMAKE_BUILD_TYPE}")


# ==============================================================================
# Find specific features and packages
# ==============================================================================

#
# Using "if(NOT mypackage_FOUND)" for the find_package() directives below allows the user
# to specify custom values to configurations (such as explicitly turning off OpenMP) and
# not being interfered with by a call to find_package().
#

# Find Eigen3 package
#if(NOT EIGEN3_FOUND)
find_package(Eigen3 3.3.0 REQUIRED)
#endif(NOT EIGEN3_FOUND)

# Find OpenMP
#if(NOT OPENMP_FOUND)
find_package(OpenMP)
#endif(NOT OPENMP_FOUND)

# Find boost headers (math, ...) 
#if(NOT BOOST_FOUND)
find_package(Boost 1.40 REQUIRED)
#endif(NOT BOOST_FOUND)

#
# More find_package calls appear below, depending on which targets are being compiled
#



# activate warning flags on DEBUG and RELWITHDEBINFO builds
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -Wall -Wextra")



# NOTE: By default GCC/G++ uses a LOT of RAM (g++ 6.2 with 4GB was not working well).
# [Clang-3.9 was very good by comparison.]  You can instruct gcc to use less RAM by
# fiddling around with the options "--param ggc-min-expand=10 --param
# ggc-min-heapsize=32768", or "--param ggc-min-expand=0 --param ggc-min-heapsize=8192" if
# you want to be even more conservative.



# optimization flags for computer architecture.
# -> use -DTARGET_ARCHITECTURE=none to not pass "-m..." options to compiler at all
# -> use -DTARGET_ARCHITECTURE=generic to disable simd instruction sets
include(OptimizeForArchitecture)
OptimizeForArchitecture()
string(REPLACE ";" " " _vc_arch_flags_joined "${Vc_ARCHITECTURE_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${_vc_arch_flags_joined}")



# ==============================================================================
# Documentation Target
# ==============================================================================

find_package(Doxygen)
find_package(Sphinx)

# the master documentation target
add_custom_target(doc)

if(DOXYGEN_FOUND)
  # C++ documentation target
  add_custom_target(doxdoc
    COMMAND "${CMAKE_COMMAND}" "-DVersionFile=${CMAKE_BINARY_DIR}/tomographer/tomographer_VERSION" "-DRootSourceDir=${CMAKE_SOURCE_DIR}" "-DTargetDoxyfileDir=${CMAKE_BINARY_DIR}" -P "${CMAKE_SOURCE_DIR}/cmake/CreateDoxyfile.cmake"
    COMMAND "${DOXYGEN_EXECUTABLE}" "${CMAKE_BINARY_DIR}/Doxyfile"
    COMMENT "Generating C++ API documentation with Doxygen" VERBATIM
    )
  # to make sure the version file is set up properly
  add_dependencies(doxdoc tomographer)
  add_dependencies(doc doxdoc)
  message(STATUS "C++ API documentation will be built when running `make doc`.")
else()
  message(STATUS "Doxygen not found. Don't worry, you'd only need this if you wanted to generate C++ API documentation.")
endif()
if(SPHINX_FOUND AND BUILD_TOMOPY)
  # Python documentation target
  add_custom_target(pydoc
    COMMAND "${CMAKE_COMMAND}" -E env "PYTHONPATH=${CMAKE_BINARY_DIR}/py:${CMAKE_SOURCE_DIR}/py:\$PYTHONPATH" "${SPHINX_BUILD_COMMAND}" -a -E -n -b html "${CMAKE_SOURCE_DIR}/doc/py" "html/py"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Generating Python API documentation with Sphinx" VERBATIM
    )
  add_dependencies(pydoc tomographerpy)
  add_dependencies(doc pydoc)
  message(STATUS "Python API documentation will be built when running `make doc`.")
else()
  if (BUILD_TOMOPY)
    message(STATUS "Sphinx not found. Don't worry, you'd only need this if you wanted to generate Python API documentation.")
  endif()
endif()


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
# The basic tomographer header-only C++ template library
# ==============================================================================

# there are essentially two things to do: set up tomographer_version.h (obtaining the
# current version from git-describe or explicit VERSION file), and define the install
# target.
add_subdirectory(tomographer)


# ==============================================================================
# Build Tomorun?
# ==============================================================================

option(BUILD_TOMORUN "Build the tomorun executable" on)
if (BUILD_TOMORUN)

  # Find MatIO library (https://sourceforge.net/projects/matio/)
  #if(NOT MATIO_FOUND)
  find_package(MatIO REQUIRED)
  #endif(NOT MATIO_FOUND)
  # ... which requires the libz library
  #if(NOT ZLIB_FOUND)
  find_package(ZLIB REQUIRED)
  #endif(NOT ZLIB_FOUND)

  # Boost program_options
  find_package(Boost 1.40 COMPONENTS program_options REQUIRED)

  EnsureCXX11StdThisThreadSleepForAvailable()
  
  # can we use the nice() function to reduce the process priority?
  CHECK_CXX_SOURCE_COMPILES(
    "#include <unistd.h>
int main() { nice(1); }"
    tomographer_HAVE_NICE)

  set(TOMORUN_SUFFIX "" CACHE STRING
    "Optional suffix to append to 'tomorun' executable name, to indicate special configuration in case you override settings in tomorun_config.h with -D... compiler flags (such as -DTOMORUN_CUSTOM_FIXED_DIM=... etc.)")

  message(STATUS "Will build tomorun executable (BUILD_TOMORUN=on)")
  add_subdirectory(tomorun)

else()
  message(STATUS "Will not build tomorun executable (BUILD_TOMORUN=off)")
endif()

# ==============================================================================
# Build Python interface library module?
# ==============================================================================

option(BUILD_TOMOPY "Build the tomographer Python interface" off)
if (BUILD_TOMOPY)

  # We need python and boost::python
  find_package(PythonLibs REQUIRED)
  find_package(Boost 1.40 COMPONENTS python REQUIRED)
  
  # We need NumPy also
  find_package(NumPy 1.7 REQUIRED)

  EnsureCXX11StdThisThreadSleepForAvailable()

  message(STATUS "Will build tomographer Python interface (BUILD_TOMOPY=on)")
  message("
    *** NOTE ***
    Building the Python interface with CMake is meant for development (in
    particular for running the tests and generating API documentation).  Please
    note that 'make install' will NOT install any Python interface related
    files.

    Please use 'python setup.py install' inside the py/ directory to properly
    build and install the python module.

")
  add_subdirectory(py)

else()
  message(STATUS "Will not build tomographer Python interface (BUILD_TOMOPY=off)")
endif()

# ==============================================================================
# Tests?
# ==============================================================================

option(TOMOGRAPHER_ENABLE_TESTS "Build Tomographer/Tomorun tests" off)

if(TOMOGRAPHER_ENABLE_TESTS)

# we will selectively only enable those tests for which we have built all
# components (but we will add a failing test if a component is not built, to
# make sure we don't miss anything)
#
#  if(NOT BUILD_TOMORUN OR NOT BUILD_TOMOPY)
#    message(FATAL_ERROR "You can only build the tests if you are building both tomorun and the Python interface library.")
#  endif()

  #if(NOT Boost_UNIT_TEST_FRAMEWORK_LIBRARY)
  find_package(Boost 1.40 COMPONENTS unit_test_framework REQUIRED)
  #endif()
  
  # LAPACK is required in some tests (but not for tomorun)
  #if(NOT LAPACK_FOUND)
  find_package(LAPACK REQUIRED)
  #endif(NOT LAPACK_FOUND)


  option(TOMOGRAPHER_TEST_SETUPPY_BUILD "Add tests for python module built separately with setup.py bdist_egg" off)


  EnsureCXX11StdThisThreadSleepForAvailable()

  # find the python executable -- used to run the tests for the python interface
  if(BUILD_TOMOPY OR TOMOGRAPHER_TEST_SETUPPY_BUILD)
    find_package(PythonInterp REQUIRED)
  endif()

  # Enable testing for the project
  enable_testing()

  message(STATUS "Will build tomographer tests. Use `make test' or `ctest' to run the tests.")
  add_subdirectory(test)
  
else()
  message(STATUS "Will not build tests (meant for developers). Set TOMOGRAPHER_ENABLE_TESTS=on if you wanted them.")
endif()

