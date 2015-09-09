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
# Add the tests.
# ==============================================================================

# 
# remove -DNDEBUG from flags!
# 
# The last thing we want here is to remove the assertion tests from the code.
#
string(TOUPPER "${CMAKE_BUILD_TYPE}" cmake_build_type_upper)
RemoveFlag(CMAKE_CXX_FLAGS_${cmake_build_type_upper} "${CMAKE_CXX_FLAGS_${cmake_build_type_upper}}" "-DNDEBUG")
RemoveFlag(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}"  "-DNDEBUG")


add_definitions(-DBOOST_TEST_DYN_LINK)


# we need to access the expected output patterns. Trailing slash needed.
set(TOMOGRAPHER_TEST_PATTERNS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/patterns/"
  CACHE STRING "path to the expected test results patterns")
add_definitions(-DTOMOGRAPHER_TEST_PATTERNS_DIR="${TOMOGRAPHER_TEST_PATTERNS_DIR}")
# we need to access some data. Trailing slash needed.
set(TOMOGRAPHER_TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data/"
  CACHE STRING "path to data files required by some tests")
add_definitions(-DTOMOGRAPHER_TEST_DATA_DIR="${TOMOGRAPHER_TEST_DATA_DIR}")

# test suite
set(tests_SRCS
  # list of sources containing test cases:
  test_tomographer.cxx
  test_sphcoords.cxx
  test_tools_util.cxx
  test_tools_conststr.cxx
  test_tools_ezmatio_1.cxx
  test_tools_ezmatio_2.cxx
  test_tools_ezmatio_3.cxx
  test_qit_util.cxx
  test_loggers.cxx
  test_lib_basics.cxx
  test_param.cxx
  test_mhrw.cxx
  test_simple_find_zero.cxx
  test_mhrw_bin_err.cxx
  test_multi_omp.cxx
  test_histogram.cxx
  #test_integrator_basic.cxx
)


# the test binary executable
add_executable(test_tomographer  EXCLUDE_FROM_ALL
  ${tests_SRCS}
  )

# compile & link with OpenMP
SetOpenMPTarget(test_tomographer)

AppendTargetProperty(test_tomographer
  COMPILE_FLAGS  "-I${MATIO_INCLUDE_DIR} -I${Boost_INCLUDE_DIR} -I${ZLIB_INCLUDE_DIRS}")

# link with Boost.Test framework
target_link_libraries(test_tomographer ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
target_link_libraries(test_tomographer ${MATIO_LIBRARIES})
target_link_libraries(test_tomographer ${ZLIB_LIBRARIES})
