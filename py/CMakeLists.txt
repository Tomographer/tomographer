# This file is part of the Tomographer project, which is distributed under the
# terms of the MIT license.
# 
# The MIT License (MIT)
# 
# Copyright (c) 2015 ETH Zurich, Institute for Theoretical Physics, Philippe
# Faist
# Copyright (c) 2016 Caltech, Philippe Faist
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


set(tomographerpy_SRCS
  cxx/eigpyconv.cxx
  cxx/pyhistogram.cxx
  cxx/pymultiproc.cxx
  cxx/pymhrwtasks.cxx
  cxx/pymhrw.cxx
  #cxx/pylogger.cxx # all definitions in pylogger.h
  cxx/pydensedm.cxx
  cxx/pytomorun.cxx
  cxx/tomographerpy.cxx
  )
add_library(tomographerpy MODULE ${tomographerpy_SRCS})

# Enable C++11
set_property(TARGET tomographerpy PROPERTY CXX_STANDARD 11)

# create _tomographer_cxx.so for "import _tomographer_cxx" (internal to tomographer/__init__.py),
# not libtomographerpy.so or libtomographerpy.dylib
set_property(TARGET tomographerpy PROPERTY OUTPUT_NAME "_tomographer_cxx")
set_property(TARGET tomographerpy PROPERTY PREFIX "")
if(APPLE)
  set_property(TARGET tomographerpy PROPERTY SUFFIX ".so") # and not .dylib
  set_property(TARGET tomographerpy PROPERTY BUILD_WITH_INSTALL_RPATH 1)
  set_property(TARGET tomographerpy PROPERTY INSTALL_NAME_DIR "${CMAKE_CURRENT_BINARY_DIR}")
  target_compile_options(tomographerpy PRIVATE "-dynamic")
  target_link_libraries(tomographerpy "-undefined dynamic_lookup")
endif()

# depends in particular on tomographer_version.h
add_dependencies(tomographerpy tomographer)

# tomographer/tomographer_version.h (not yet installed)
target_include_directories(tomographerpy PRIVATE "${CMAKE_BINARY_DIR}")
# tomographer/ headers -- relative for auto dependency detection
target_include_directories(tomographerpy PUBLIC "..")

# our own tomographerpy headers
target_include_directories(tomographerpy PUBLIC "cxx")

# NOTE: no SYSTEM includes, because it messes up the system include path order if the path
# happens to be a system path such as /usr/include
#
target_include_directories(tomographerpy  PUBLIC ${EIGEN3_INCLUDE_DIR})
target_compile_definitions(tomographerpy PRIVATE -DEIGEN_DONT_PARALLELIZE)

target_include_directories(tomographerpy  PUBLIC ${PYTHON_INCLUDE_DIRS} ${NUMPY_INCLUDE_DIRS})
target_include_directories(tomographerpy  PUBLIC ${Boost_INCLUDE_DIR})
target_link_libraries(tomographerpy ${Boost_PYTHON_LIBRARY})

target_compile_options(tomographerpy PRIVATE ${OpenMP_CXX_FLAGS})
target_link_libraries(tomographerpy ${OpenMP_CXX_FLAGS})

