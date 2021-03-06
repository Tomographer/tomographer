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
# need Eigen assertions to be live, remove any -NDEBUG flag
#
string(TOUPPER "${CMAKE_BUILD_TYPE}" cmake_build_type_upper)
RemoveFlag(CMAKE_CXX_FLAGS_${cmake_build_type_upper} "${CMAKE_CXX_FLAGS_${cmake_build_type_upper}}" "-DNDEBUG")
RemoveFlag(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" "-DNDEBUG")


set(tomographerpy_SRCS
  cxx/pyhistogram.cxx
  cxx/pymultiproc.cxx
  cxx/pymhrwtasks.cxx
  cxx/pymhrw.cxx
  cxx/pydensedm.cxx
  cxx/pytomorun.cxx
  cxx/tomographerpy.cxx
  )
pybind11_add_module(_tomographer_cxx MODULE NO_EXTRAS ${tomographerpy_SRCS})
# needs to export symbols in binary lib
#target_compile_options(_tomographer_cxx PRIVATE "-fvisibility=default") # ok, try with hidden visibility as pybind11 recommends so strongly


# depends in particular on tomographer_version.h
add_dependencies(_tomographer_cxx tomographer)

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
target_include_directories(_tomographer_cxx PUBLIC ${Boost_INCLUDE_DIR})

# Uses C++ threads (need -pthread on gcc)
target_link_libraries(_tomographer_cxx PRIVATE Threads::Threads)
TargetMaybeUseMingwStdThread(_tomographer_cxx)

add_custom_target(tomographerpy)
add_dependencies(tomographerpy _tomographer_cxx)
