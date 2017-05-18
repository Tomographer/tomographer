# This file is part of the Tomographer project, which is distributed under the
# terms of the MIT license.
# 
# The MIT License (MIT)
# 
# Copyright (c) 2015 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
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


# ==============================================================================
# Add our executable
# ==============================================================================

set(TOMORUN_CONFIG_DO_SLOW_POVM_CONSISTENCY_CHECKS "true" CACHE BOOL
  "Whether to perform (slow) POVM consistency checks")

add_executable(tomorun tomorun.cxx)

# depends in particular on tomographer_version.h
add_dependencies(tomorun tomographer)

# C++11 target
set_property(TARGET tomorun PROPERTY CXX_STANDARD 11)

# include our tomographer headers
target_include_directories(tomorun PRIVATE "..")  # relative path for automatic dependency detection

# for <tomographer/tomographer_version.h> which was auto-generated in the binary
# dir tree, and is not yet installed.
target_include_directories(tomorun PRIVATE "${CMAKE_BINARY_DIR}")

if(NOT tomographer_HAVE_NICE)
  target_compile_definitions(tomorun PRIVATE "-DTOMORUN_NOT_HAVE_NICE")
endif()

if(TOMORUN_SUFFIX)
  if(CMAKE_BUILD_TYPE)
    string(TOUPPER "${CMAKE_BUILD_TYPE}" tmp_buildtype)
    set_target_properties(tomorun PROPERTIES OUTPUT_NAME_${tmp_buildtype} "tomorun${TOMORUN_SUFFIX}")
  else(CMAKE_BUILD_TYPE)
    message(FATAL_ERROR "Internal error: CMAKE_BUILD_TYPE is not set.")
  endif(CMAKE_BUILD_TYPE)
endif(TOMORUN_SUFFIX)

# NOTE: no SYSTEM includes because if the path happens to be e.g. /usr/include, it messes
# up the system include paths (and gcc 6 e.g. can't find "math.h" or "stdlib.h")
#
# dependency: Eigen
target_include_directories(tomorun  PRIVATE ${EIGEN3_INCLUDE_DIR})
# already parallelized application, so disable Eigen's own parallelization (see
# http://eigen.tuxfamily.org/dox/TopicMultiThreading.html)
target_compile_definitions(tomorun PRIVATE -DEIGEN_DONT_PARALLELIZE)


# dependency: Boost headers, Boost program_options library
target_include_directories(tomorun  PRIVATE ${Boost_INCLUDE_DIR})
target_link_libraries(tomorun PRIVATE ${Boost_PROGRAM_OPTIONS_LIBRARY})

# dependency: MatIO (and MatIO's ZLIB dependency)
target_include_directories(tomorun  PRIVATE ${MATIO_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS})
target_link_libraries(tomorun PRIVATE ${MATIO_LIBRARIES} ${ZLIB_LIBRARIES})

set(TOMORUN_MULTIPROC "cxxthreads" CACHE STRING
  "MultiProcessing scheme to use for `tomorun`: 'cxxthreads', 'openmp' or 'sequential'")

if(TOMORUN_MULTIPROC STREQUAL "openmp")
  # compile with OpenMP
  message(STATUS "tomorun will be compiled with OpenMP multiprocessing")
  target_compile_options(tomorun PRIVATE ${OpenMP_CXX_FLAGS})
  target_link_libraries(tomorun PRIVATE ${OpenMP_CXX_FLAGS})
  target_compile_definitions(tomorun PRIVATE -DTOMORUN_MULTIPROC_OPENMP)
elseif(TOMORUN_MULTIPROC STREQUAL "cxxthreads")
  message(STATUS "tomorun will be compiled with C++11 threads multiprocessing")
  target_link_libraries(tomorun PRIVATE Threads::Threads)
  target_compile_definitions(tomorun PRIVATE -DTOMORUN_MULTIPROC_CXXTHREADS)
elseif(TOMORUN_MULTIPROC STREQUAL "sequential")
  message(WARNING "tomorun will be compiled with sequential execution only (no parallelization) -- this will be really slow!")
  target_compile_definitions(tomorun PRIVATE -DTOMORUN_MULTIPROC_SEQUENTIAL)
else()
  message(FATAL_ERROR "Invalid value for TOMORUN_MULTIPROC: expected 'cxxthreads', 'openmp' or 'sequential'")
endif()



# the installation target
install(TARGETS tomorun
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  )
