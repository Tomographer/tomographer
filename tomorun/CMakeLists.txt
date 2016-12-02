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
# Add our executable
# ==============================================================================

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

# dependency: Eigen
target_include_directories(tomorun SYSTEM PRIVATE ${EIGEN3_INCLUDE_DIR})
# already parallelized application, so disable Eigen's own parallelization (see
# http://eigen.tuxfamily.org/dox/TopicMultiThreading.html)
target_compile_definitions(tomorun PRIVATE -DEIGEN_DONT_PARALLELIZE)


# dependency: Boost headers, Boost program_options library
target_include_directories(tomorun SYSTEM PRIVATE ${Boost_INCLUDE_DIR})
target_link_libraries(tomorun ${Boost_PROGRAM_OPTIONS_LIBRARY})

# dependency: MatIO (and MatIO's ZLIB dependency)
target_include_directories(tomorun SYSTEM PRIVATE ${MATIO_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS})
target_link_libraries(tomorun ${MATIO_LIBRARIES} ${ZLIB_LIBRARIES})

# compile with OpenMP
if (OPENMP_FOUND)
  target_compile_options(tomorun PRIVATE ${OpenMP_CXX_FLAGS})
  target_link_libraries(tomorun ${OpenMP_CXX_FLAGS})
else()
  WarnTargetNoOpenMP("The tomorun executable")
endif()



# the installation target
install(TARGETS tomorun
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  )
