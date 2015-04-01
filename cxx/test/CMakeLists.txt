cmake_minimum_required(VERSION 2.8)

project(tomographer_cxx_test)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/")

include(Util)

# ==============================================================================
# Find specific features and packages
# ==============================================================================

# default to Release build
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()
message(STATUS "CMake Build Type: ${CMAKE_BUILD_TYPE}")

# Check for C++0x/C++11 features
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)

# Find Eigen3 package
find_package(Eigen3 REQUIRED)

# Find OpenMP
find_package(OpenMP)


# ==============================================================================
# Set up the build environment
# ==============================================================================

# Activate C++0x/C++11 support
if(COMPILER_SUPPORTS_CXX11)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
  message(FATAL_ERROR
    "The compiler ${CMAKE_CXX_COMPILER} has no C++0x/C++11 support. Please use a different C++ compiler."
    )
endif()

# Access <Eigen/...> include path
include_directories(${EIGEN3_INCLUDE_DIR})

# Include <tomographer/...> include path. Note that the path needs to be relative to get
# the good automatic header dependencies
include_directories(..)


# ==============================================================================
# Add the tests.
# ==============================================================================

add_executable(test_lib_basics test_lib_basics.cxx)

add_executable(test_integrator_basic test_integrator_basic.cxx)


add_executable(test_multi_omp test_multi_omp.cxx)


if(OPENMP_FOUND)
  #OpenMP_C_FLAGS   - flags to add to the C compiler for OpenMP support
  #OpenMP_CXX_FLAGS - flags to add to the CXX compiler for OpenMP support
  AppendTargetProperty(test_multi_omp COMPILE_FLAGS "${OpenMP_CXX_FLAGS}")
  AppendTargetProperty(test_multi_omp LINK_FLAGS "${OpenMP_CXX_FLAGS}")
else()
  message(WARNING "OpenMP not found. The OpenMP program will run serially with no parallelization.")
endif()
