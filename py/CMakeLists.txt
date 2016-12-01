cmake_minimum_required(VERSION 3.0.2)

project(tomographerpy)


# default to release build with debugging info
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING
    "Release Type -- defaults to RelWithDebInfo" FORCE)
endif()
message(STATUS "CMake Build Type: ${CMAKE_BUILD_TYPE}")



set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

find_package(Eigen3 3.2.5 REQUIRED)

find_package(PythonLibs REQUIRED)
find_package(Boost 1.50 COMPONENTS UNIT_TEST_FRAMEWORK PYTHON REQUIRED)

find_package(Tomographer2 REQUIRED)

find_package(OpenMP REQUIRED)

# ==============================================================================
# Find specific features and packages
# ==============================================================================


# activate warning flags on DEBUG and RELWITHDEBINFO builds
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -Wall -Wextra")



# optimization flags for computer architecture.
# -> use -DTARGET_ARCHITECTURE=none to not pass "-m..." options to compiler at all
# -> use -DTARGET_ARCHITECTURE=generic to disable simd instruction sets
include(OptimizeForArchitecture)
OptimizeForArchitecture()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Vc_ARCHITECTURE_FLAGS}")




# ==============================================================================
# Include our target
# ==============================================================================


set(tomographerpy_SRCS
  cxx/eigpyconv.cxx
  cxx/pyhistogram.cxx
  cxx/pytomorun.cxx
  cxx/pylogger.cxx
  cxx/tomographerbase.cxx
  )
add_library(tomographer SHARED ${tomographerpy_SRCS})

# Enable C++11
set_property(TARGET tomographer PROPERTY CXX_STANDARD 11)

# create tomographer.so, not libtomographer.so
set_property(TARGET tomographer PROPERTY PREFIX "")
if(APPLE)
  set_property(TARGET tomographer PROPERTY SUFFIX ".so") # and not .dylib
endif()


target_include_directories(tomographer PUBLIC cxx)
target_include_directories(tomographer PUBLIC SYSTEM ${EIGEN3_INCLUDE_DIR})
target_include_directories(tomographer PUBLIC SYSTEM ${PYTHON_INCLUDE_DIRS})
target_include_directories(tomographer PUBLIC SYSTEM ${Boost_INCLUDE_DIR})
target_include_directories(tomographer PUBLIC ${TOMOGRAPHER2_INCLUDE_DIR})
target_compile_options(tomographer PRIVATE ${OpenMP_CXX_FLAGS})
target_link_libraries(tomographer  ${OpenMP_CXX_FLAGS})
target_link_libraries(tomographer ${PYTHON_LIBRARY})
target_link_libraries(tomographer ${Boost_PYTHON_LIBRARY})

