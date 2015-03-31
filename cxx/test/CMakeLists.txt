cmake_minimum_required(VERSION 2.6)

project(tomographer_cxx_test)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/")

# Activate C++0x/C++11 features
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
  message(FATAL_ERROR
    "The compiler ${CMAKE_CXX_COMPILER} has no C++0x/C++11 support. Please use a different C++ compiler."
    )
endif()

# path needs to be relative to get the good header dependencies
include_directories(..)

find_package(Eigen3 REQUIRED)
include_directories(${EIGEN3_INCLUDE_DIR})

add_executable(test_integrator_basic test_integrator_basic.cxx)

add_executable(test_lib_basics test_lib_basics.cxx)
