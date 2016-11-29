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


# ==============================================================================
# Set up the build environment
# ==============================================================================

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TOMOGRAPHER_CXX11_STD_FLAGS}")


# Access <Eigen/...> include path
include_directories(SYSTEM ${EIGEN3_INCLUDE_DIR})

# Include <tomographer2/...> include path. Note that the path needs to be relative to get
# the good automatic header dependencies
include_directories(.)




# ==============================================================================
# Common C++ stuff
# ==============================================================================

include(CheckCXXSourceCompiles)


set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${TOMOGRAPHER_CXX11_STD_FLAGS}")
CHECK_CXX_SOURCE_COMPILES(
  "#include <thread>
#include <chrono>
int main() { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }"
  tomographer_HAVE_CXX11_THREAD_SLEEP_FOR
  )
if (NOT tomographer_HAVE_CXX11_THREAD_SLEEP_FOR)
  set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${TOMOGRAPHER_CXX11_STD_FLAGS}")
  set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_REQUIRED_DEFINITIONS} -D_GLIBCXX_USE_NANOSLEEP)
  CHECK_CXX_SOURCE_COMPILES(
    "#include <thread>
#include <chrono>
int main() { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }"
    tomographer_HAVE_CXX11_THREAD_SLEEP_FOR_wdef
    )
  if (tomographer_HAVE_CXX11_THREAD_SLEEP_FOR_wdef)
    add_definitions(-D_GLIBCXX_USE_NANOSLEEP)
  else()
    message(FATAL_ERROR "Your C++ compiler doesn't seem to support std::this_thread::sleep_for(). You may need to use a different compiler, or set the required flags yourself.")
  endif()
endif()


# already parallelized application, so disable Eigen's own parallelization (see
# http://eigen.tuxfamily.org/dox/TopicMultiThreading.html)
add_definitions(-DEIGEN_DONT_PARALLELIZE)


# ==============================================================================
# Include Subdirs
# ==============================================================================



# Tomographer library
add_subdirectory(tomographer2)


# TomoRun executable
add_subdirectory(tomorun)


# Tests
add_subdirectory(test)

