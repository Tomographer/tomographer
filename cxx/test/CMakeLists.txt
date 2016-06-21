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
# Required Definitions Etc.
# ==============================================================================

if(NOT TOMOGRAPHER_ENABLE_TESTS) 
  return()
endif(NOT TOMOGRAPHER_ENABLE_TESTS) 


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


# ==============================================================================
# Enable Testing.
# ==============================================================================

enable_testing()


# ==============================================================================
# Add the tests.
# ==============================================================================

set(LIST_OF_ALL_TESTS "")

macro(addTomographerTest test_cxx NeedsList)

  # file name without extension (NAME_WE)
  get_filename_component(test_name ${test_cxx} NAME_WE)
        
  if (";${NeedsList};" MATCHES ";LAPACK;")
    # compile & link with LAPACK, but only if it is available.
    if(NOT LAPACK_FOUND)
      return()
    endif()
  endif()

  # the test binary executable
  add_executable(${test_name}
    ${test_cxx}
    test_tomographer.cxx
    )
  
  set(LIST_OF_ALL_TESTS ${LIST_OF_ALL_TESTS} ${test_name})

  if (";${NeedsList};" MATCHES ";openmp;")
    # compile & link with OpenMP
    SetOpenMPTarget(${test_name})
  endif()

  AppendTargetProperty(${test_name}
    COMPILE_FLAGS
    "-DBOOST_TEST_MODULE=${test_name} -I${MATIO_INCLUDE_DIR} -I${Boost_INCLUDE_DIR} -I${ZLIB_INCLUDE_DIRS}"
    )

  if (";${NeedsList};" MATCHES ";LAPACK;")
    AppendTargetProperty(${test_name}
      LINKER_FLAGS
      "${LAPACK_LINKER_FLAGS}"
      )
    target_link_libraries(${test_name} ${LAPACK_LIBRARIES})
  endif()

  # link with Boost.Test framework
  target_link_libraries(${test_name} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

  if (";${NeedsList};" MATCHES ";matio;")
    target_link_libraries(${test_name} ${MATIO_LIBRARIES})
    target_link_libraries(${test_name} ${ZLIB_LIBRARIES})
  endif()

  add_test(NAME ${test_name} 
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${test_name}
    )

  #message(STATUS "Added test ${test_name}")
  
endmacro(addTomographerTest)


# add tests for tomographer library.

addTomographerTest(test_densedm_dmtypes.cxx "")
addTomographerTest(test_densedm_distmeasures.cxx "")
addTomographerTest(test_densedm_param_herm_x.cxx "")
addTomographerTest(test_densedm_param_rho_a.cxx "")
addTomographerTest(test_densedm_indepmeasllh.cxx "")
addTomographerTest(test_densedm_tspacellhwalker.cxx "")
addTomographerTest(test_densedm_tspacefigofmerit.cxx "")
addTomographerTest(test_tools_loggers.cxx  "")
addTomographerTest(test_tools_cxxutil.cxx  "")
addTomographerTest(test_tools_needownoperatornew.cxx  "")
addTomographerTest(test_tools_eigenutil.cxx  "")
addTomographerTest(test_tools_fmt.cxx  "")
addTomographerTest(test_tools_conststr.cxx  "")
addTomographerTest(test_tools_ezmatio_1.cxx  "matio")
addTomographerTest(test_tools_ezmatio_2.cxx  "matio")
addTomographerTest(test_tools_ezmatio_3.cxx  "matio")
addTomographerTest(test_tools_eigen_assert_exception.cxx "")
addTomographerTest(test_mathtools_simple_find_zero.cxx  "")
addTomographerTest(test_mathtools_random_unitary.cxx  "")
addTomographerTest(test_mathtools_pos_semidef_util.cxx  "")
addTomographerTest(test_mathtools_check_derivatives.cxx  "")
addTomographerTest(test_mathtools_sphcoords.cxx  "")
addTomographerTest(test_mathtools_solveclyap.cxx "LAPACK")
#
addTomographerTest(test_histogram.cxx  "")
addTomographerTest(test_mhrw.cxx  "")
addTomographerTest(test_mhrwstatscollectors.cxx  "")
addTomographerTest(test_valuecalculator.cxx  "")
addTomographerTest(test_mhrw_bin_err.cxx  "")
addTomographerTest(test_mhrw_valuehist_tasks.cxx  "openmp")
addTomographerTest(test_multiprocomp.cxx  "openmp")
addTomographerTest(test_zzzcombinations.cxx "openmp")


# check "minimal tomorun" program compiles and runs
add_executable(minimal_tomorun
  minimal_tomorun.cxx
  )
SetOpenMPTarget(minimal_tomorun)

add_test(NAME minimal_tomorun
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMAND "$<TARGET_FILE:minimal_tomorun>"
  )



add_executable(test_tomorun_1qubit_analytic_solution_check
  "tomorun/test_tomorun_1qubit_analytic_solution_check.cxx"
  )

# version without binning analysis
add_test(NAME test_tomorun_1qubit_analytic_solution_run_simple
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMAND "$<TARGET_FILE:tomorun>" --data-file-name "${CMAKE_CURRENT_SOURCE_DIR}/tomorun/test_tomorun_1qubit_analytic_solution.mat"  --n-repeats=64  --n-sweep=25  --n-run=4096  --step-size=0.04  --value-type=fidelity:rho_ref  --value-hist=0.985:1/200  --write-histogram=test_tomorun_1qubit_testrun_simple --no-binning-analysis-error-bars --verbose
  )
add_test(NAME test_tomorun_1qubit_analytic_solution_runcheck_simple
  COMMAND "$<TARGET_FILE:test_tomorun_1qubit_analytic_solution_check>"
  test_tomorun_1qubit_testrun_simple-histogram.csv
  500
  )
set_tests_properties(test_tomorun_1qubit_analytic_solution_runcheck_simple
  PROPERTIES DEPENDS "test_tomorun_1qubit_analytic_solution_check;test_tomorun_1qubit_analytic_solution_run_simple")
  

# version with binning analysis
add_test(NAME test_tomorun_1qubit_analytic_solution_run_binerr
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMAND "$<TARGET_FILE:tomorun>" --data-file-name "${CMAKE_CURRENT_SOURCE_DIR}/tomorun/test_tomorun_1qubit_analytic_solution.mat"  --n-repeats=4  --n-sweep=25  --n-run=32768  --step-size=0.04  --value-type=fidelity:rho_ref  --value-hist=0.985:1/200  --write-histogram=test_tomorun_1qubit_testrun_binerr --verbose
  )
add_test(NAME test_tomorun_1qubit_analytic_solution_runcheck_binerr
  COMMAND "$<TARGET_FILE:test_tomorun_1qubit_analytic_solution_check>"
  test_tomorun_1qubit_testrun_binerr-histogram.csv
  500
  )
set_tests_properties(test_tomorun_1qubit_analytic_solution_runcheck_binerr
  PROPERTIES DEPENDS "test_tomorun_1qubit_analytic_solution_check;test_tomorun_1qubit_analytic_solution_run_binerr")


message(STATUS "Added test targets")

