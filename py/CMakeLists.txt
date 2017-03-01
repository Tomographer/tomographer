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


#
# Turn this into an independent cmake project file -- temporarily
cmake_minimum_required(VERSION 3.1)

project(tomographerpy)
set(EIGEN3_INCLUDE_DIR "/usr/local/opt/eigen/include/eigen3")
add_definitions(-DTOMOGRAPHER_VERSION="3.1" -DTOMOGRAPHER_VERSION_MAJ=3 -DTOMOGRAPHER_VERSION_MIN=1)


add_subdirectory(pybind11)



set(tomographerpy_SRCS
#  cxx/eigpyconv.cxx
#  cxx/pyhistogram.cxx
#  cxx/pymultiproc.cxx
#  cxx/pymhrwtasks.cxx
  cxx/pymhrw.cxx
#  cxx/pydensedm.cxx
#  cxx/pytomorun.cxx
  cxx/tomographerpy.cxx
  )
#add_library(tomographerpy MODULE ${tomographerpy_SRCS})
pybind11_add_module(_tomographer_cxx MODULE ${tomographerpy_SRCS})

# Enable C++11
#set_property(TARGET tomographerpy PROPERTY CXX_STANDARD 11)

# create _tomographer_cxx.so for "import _tomographer_cxx" (internal to tomographer/__init__.py),
# not libtomographerpy.so or libtomographerpy.dylib
#set_property(TARGET tomographerpy PROPERTY OUTPUT_NAME "_tomographer_cxx")
#set_property(TARGET tomographerpy PROPERTY PREFIX "")
#if(APPLE)
#  set_property(TARGET tomographerpy PROPERTY SUFFIX ".so") # and not .dylib
#  set_property(TARGET tomographerpy PROPERTY BUILD_WITH_INSTALL_RPATH 1)
#  set_property(TARGET tomographerpy PROPERTY INSTALL_NAME_DIR "${CMAKE_CURRENT_BINARY_DIR}")
#  target_compile_options(tomographerpy PRIVATE "-dynamic")
#  target_link_libraries(tomographerpy "-undefined dynamic_lookup")
#endif()

# depends in particular on tomographer_version.h
#add_dependencies(_tomographer_cxx tomographer)

# tomographer/tomographer_version.h (not yet installed)
target_include_directories(_tomographer_cxx PRIVATE "${CMAKE_BINARY_DIR}")
# tomographer/ headers -- relative for auto dependency detection
target_include_directories(_tomographer_cxx PUBLIC "..")

# our own _tomographer_cxx headers
target_include_directories(_tomographer_cxx PUBLIC "cxx")

# NOTE: no SYSTEM includes, because it messes up the system include path order if the path
# happens to be a system path such as /usr/include
#
target_include_directories(_tomographer_cxx  PUBLIC ${EIGEN3_INCLUDE_DIR})
target_compile_definitions(_tomographer_cxx PRIVATE -DEIGEN_DONT_PARALLELIZE)

#target_include_directories(_tomographer_cxx  PUBLIC ${PYTHON_INCLUDE_DIRS} ${NUMPY_INCLUDE_DIRS})
#target_include_directories(_tomographer_cxx  PUBLIC ${Boost_INCLUDE_DIR})
#target_link_libraries(_tomographer_cxx ${Boost_PYTHON_LIBRARY})

#target_compile_options(_tomographer_cxx PRIVATE ${OpenMP_CXX_FLAGS})
#target_link_libraries(_tomographer_cxx ${OpenMP_CXX_FLAGS})

