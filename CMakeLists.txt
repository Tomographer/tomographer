cmake_minimum_required(VERSION 2.8)

project(tomographer)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(Util)

# ==============================================================================
# Find specific features and packages
# ==============================================================================

# default to Release build
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo")
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
# Include our subdirectories
# ==============================================================================

add_subdirectory(cxx)
