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
# Required Definitions Etc.
# ==============================================================================


# 
# Make sure we don't have -DNDEBUG in flags.  The last thing we want here is to remove the
# assertion tests from the code.
#
# NOTE: overriding the variables CMAKE_CXX_FLAGS[_XXX] only has effect in this directory
# (CHECK THIS?? the docs are terrible)
#
string(TOUPPER "${CMAKE_BUILD_TYPE}" cmake_build_type_upper)
RemoveFlag(CMAKE_CXX_FLAGS_${cmake_build_type_upper} "${CMAKE_CXX_FLAGS_${cmake_build_type_upper}}" "-DNDEBUG")
RemoveFlag(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" "-DNDEBUG")

if (Boost_UNIT_TEST_FRAMEWORK_LIBRARY MATCHES "\\.a\$")
  set(tomotests_set_BOOST_TEST_DYN_LINK false)
else()
  set(tomotests_set_BOOST_TEST_DYN_LINK true)
endif()

# we need to access the expected output patterns. Trailing slash needed.
file(RELATIVE_PATH REL_TEST_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/" "${CMAKE_CURRENT_SOURCE_DIR}/")
set(TOMOGRAPHER_TEST_PATTERNS_DIR "${REL_TEST_SOURCE_DIR}/patterns/"
  CACHE STRING "path to the expected test results patterns")
add_definitions(-DTOMOGRAPHER_TEST_PATTERNS_DIR="${TOMOGRAPHER_TEST_PATTERNS_DIR}")
# we need to access some data. Trailing slash needed.
set(TOMOGRAPHER_TEST_DATA_DIR "${REL_TEST_SOURCE_DIR}/data/"
  CACHE STRING "path to data files required by some tests")
add_definitions(-DTOMOGRAPHER_TEST_DATA_DIR="${TOMOGRAPHER_TEST_DATA_DIR}")


# ==============================================================================
# Add the tests.
# ==============================================================================

#set(LIST_OF_ALL_TESTS "")

macro(generateDummyFailTest cmd_argv_var test_name_ msg_)
  # replace cmd with dummy command
  if (";${test_name_};" MATCHES ";${TOMOGRAPHER_TESTS_EXPECTED_TO_BE_SKIPPED_AND_FAIL};")
    set(${cmd_argv_var} bash -c "echo '${msg_} -- AS EXPECTED' >&2 || true")
  else()
    set(${cmd_argv_var} bash -c "echo '${msg_}' >&2 && false")
  endif()
endmacro()

macro(addTomographerTest test_cxx NeedsList)

  # file name without extension (NAME_WE)
  get_filename_component(test_name ${test_cxx} NAME_WE)
        
#  set(LIST_OF_ALL_TESTS ${LIST_OF_ALL_TESTS} ${test_name})

  # NEED lapack in order to run test suite---don't want tests silently not to run because
  # we didn't realize lapack wasn't found
  #
  #  if (";${NeedsList};" MATCHES ";LAPACK;")
  #    # compile & link with LAPACK, but only if it is available.
  #    if(NOT LAPACK_FOUND)
  #      return()
  #    endif()
  #  endif()

  if (NOT HAVE_CXX11_THREAD_CLASSES AND ";${NeedsList};" MATCHES ";cxxthreads;")
    # Can't compile, we don't have std::thread & friends -- skip test
    message(WARNING "Test ${test_name} will automatically fail because this system doesn't support C++11 threads")
    add_test(NAME ${test_name} COMMAND false)
    return()
  endif()

  # the test binary executable
  add_executable(${test_name}
    ${test_cxx}
    test_tomographer.cxx
    )

  # Enable C++11
  set_property(TARGET ${test_name} PROPERTY CXX_STANDARD 11)

  # tomographer headers
  target_include_directories(${test_name} PRIVATE "..")

  set(_generate_dummy_fail_test false)


  # NOTE: no SYSTEM includes because if the path happens to be e.g. /usr/include, it messes
  # up the system include paths (and gcc 6 e.g. can't find "math.h" or "stdlib.h")
  #
  # dependency: Eigen
  target_include_directories(${test_name}  PRIVATE ${EIGEN3_INCLUDE_DIR})
  target_compile_definitions(${test_name} PRIVATE -DEIGEN_DONT_PARALLELIZE)

  if (tomotests_set_BOOST_TEST_DYN_LINK)
    target_compile_definitions(${test_name} PRIVATE "-DBOOST_TEST_DYN_LINK")
  endif()
  
  if (";${NeedsList};" MATCHES ";openmp;")
    # compile & link with OpenMP
    if (OPENMP_FOUND)
      target_compile_options(${test_name} PRIVATE ${OpenMP_CXX_FLAGS})
      target_link_libraries(${test_name} PRIVATE ${OpenMP_CXX_FLAGS})
    else()
      WarnTargetNoOpenMP("Test ${test_name}")
    endif()
  endif()

  if (";${NeedsList};" MATCHES ";cxxthreads;")
    target_link_libraries(${test_name} PRIVATE Threads::Threads)
    if (TOMOGRAPHER_USE_MINGW_STD_THREAD)
      target_compile_definitions(${test_name} PRIVATE -DTOMOGRAPHER_USE_MINGW_STD_THREAD)
      target_include_directories(${test_name} PRIVATE ${TOMOGRAPHER_USE_MINGW_STD_THREAD})
    endif()
  endif()

  if (";${NeedsList};" MATCHES ";serialization;")
    target_link_libraries(${test_name} PRIVATE ${Boost_SERIALIZATION_LIBRARIES})
  endif()

  if (";${NeedsList};" MATCHES ";mpi;")
    if (MPI_FOUND)
      target_include_directories(${test_name} PRIVATE ${MPI_INCLUDE_PATH})
      target_link_libraries(${test_name} PRIVATE
        ${Boost_MPI_LIBRARIES} ${Boost_SERIALIZATION_LIBRARIES} ${MPI_LIBRARIES})
    else()
      set(_generate_dummy_fail_test "MPI not found on this system -- test case not compiled")
    endif()
  endif()

  target_compile_definitions(${test_name} PRIVATE "-DBOOST_TEST_MODULE=${test_name}")
  target_include_directories(${test_name} PRIVATE ${MATIO_INCLUDE_DIR} ${Boost_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS})

  if (";${NeedsList};" MATCHES ";LAPACK;")
    target_link_libraries(${test_name}
      ${LAPACK_LINKER_FLAGS}
      )
    target_link_libraries(${test_name} PRIVATE ${LAPACK_LIBRARIES})
  endif()

  # link with Boost.Test framework
  target_link_libraries(${test_name} PRIVATE ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

  if (";${NeedsList};" MATCHES ";matio;")
    target_link_libraries(${test_name} PRIVATE ${MATIO_LIBRARIES})
    target_link_libraries(${test_name} PRIVATE ${ZLIB_LIBRARIES})
  endif()

  set(cmd_argv )
  if (";${NeedsList};" MATCHES ";mpi;")
    set(cmd_argv ${cmd_argv} "${MPIEXEC}")
  endif()
  set(cmd_argv ${cmd_argv} ${CMAKE_CURRENT_BINARY_DIR}/${test_name})

  if(_generate_dummy_fail_test)
    set_target_properties(${test_name} PROPERTIES EXCLUDE_FROM_ALL 1)
    generateDummyFailTest(cmd_argv "${test_name}" "${_generate_dummy_fail_test}")
  endif()

  add_test(NAME ${test_name} 
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${cmd_argv}
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
addTomographerTest(test_tools_statusprovider.cxx "")
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
addTomographerTest(test_mhrwacceptratiowalkerparamscontroller.cxx  "")
addTomographerTest(test_mhrwstepsizecontroller.cxx  "")
addTomographerTest(test_mhrwvalueerrorbinsconvergedcontroller.cxx  "")
addTomographerTest(test_mhrwtasks.cxx  "")
addTomographerTest(test_valuecalculator.cxx  "")
addTomographerTest(test_mhrw_bin_err.cxx  "")
#addTomographerTest(test_mhrw_valuehist_tasks.cxx  "") # DELETE THIS
addTomographerTest(test_mhrw_valuehist_tools.cxx  "")
addTomographerTest(test_multiprocthreads.cxx  "cxxthreads")
addTomographerTest(test_multiproc.cxx  "openmp") # openmp needed for testing the status report feature
addTomographerTest(test_multiprocomp.cxx  "openmp")
addTomographerTest(test_mpi_multiprocmpi.cxx  "mpi")
# only works with g++ because we do exact comparison of the output histogram data, and
# other compilers may have small differences:
#addTomographerTest(test_zzzcombinations.cxx "openmp")
addTomographerTest(test_z_serialization.cxx "serialization")


#
# check "minimal tomorun" example program compiles and runs
#
add_executable(minimal_tomorun
  minimal_tomorun.cxx
  )
# Enable C++11
set_property(TARGET minimal_tomorun PROPERTY CXX_STANDARD 11)
# tomographer headers
target_include_directories(minimal_tomorun PRIVATE "..")
# dependency: Eigen
target_include_directories(minimal_tomorun  PRIVATE ${EIGEN3_INCLUDE_DIR})
target_compile_definitions(minimal_tomorun PRIVATE -DEIGEN_DONT_PARALLELIZE)
target_link_libraries(minimal_tomorun PRIVATE Threads::Threads)
TargetMaybeUseMingwStdThread(minimal_tomorun)
target_include_directories(minimal_tomorun PRIVATE ${Boost_INCLUDE_DIR})

add_test(NAME minimal_tomorun
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMAND "$<TARGET_FILE:minimal_tomorun>"
  )

#
# check "minimal tomorun with dyn controller" example program compiles and runs
#
add_executable(minimal_tomorun_controlled
 minimal_tomorun_controlled.cxx
 )
# Enable C++11
set_property(TARGET minimal_tomorun_controlled PROPERTY CXX_STANDARD 11)
# tomographer headers
target_include_directories(minimal_tomorun_controlled PRIVATE "..")
# dependency: Eigen
target_include_directories(minimal_tomorun_controlled  PRIVATE ${EIGEN3_INCLUDE_DIR})
target_compile_definitions(minimal_tomorun_controlled  PRIVATE -DEIGEN_DONT_PARALLELIZE)
target_link_libraries(minimal_tomorun_controlled  PRIVATE Threads::Threads)
TargetMaybeUseMingwStdThread(minimal_tomorun_controlled)
target_include_directories(minimal_tomorun_controlled  PRIVATE ${Boost_INCLUDE_DIR})

add_test(NAME minimal_tomorun_controlled
 WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
 COMMAND "$<TARGET_FILE:minimal_tomorun_controlled>"
 )

#
# check "minimal single random walk" example program compiles and runs
#
add_executable(minimal_single_random_walk
  minimal_single_random_walk.cxx
  )
# Enable C++11
set_property(TARGET minimal_single_random_walk PROPERTY CXX_STANDARD 11)
# tomographer headers
target_include_directories(minimal_single_random_walk PRIVATE "..")
# dependency: Eigen
target_include_directories(minimal_single_random_walk  PRIVATE ${EIGEN3_INCLUDE_DIR})
target_compile_definitions(minimal_single_random_walk  PRIVATE -DEIGEN_DONT_PARALLELIZE)
target_include_directories(minimal_single_random_walk  PRIVATE ${Boost_INCLUDE_DIR})

add_test(NAME minimal_single_random_walk
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMAND "$<TARGET_FILE:minimal_tomorun>"
  )

#
# check "minimal tomorun" with MPI example program compiles and runs
#
if(MPI_FOUND)
  add_executable(minimal_mpi_tomorun
    minimal_mpi_tomorun.cxx
    )
  # Enable C++11
  set_property(TARGET minimal_mpi_tomorun PROPERTY CXX_STANDARD 11)
  # tomographer headers
  target_include_directories(minimal_mpi_tomorun PRIVATE "..")
  # dependency: Eigen
  target_include_directories(minimal_mpi_tomorun  PRIVATE ${EIGEN3_INCLUDE_DIR})
  target_compile_definitions(minimal_mpi_tomorun PRIVATE -DEIGEN_DONT_PARALLELIZE)
  # Boost and MPI stuff
  target_include_directories(minimal_mpi_tomorun PRIVATE ${Boost_INCLUDE_DIR} ${MPI_INCLUDE_PATH})
  target_link_libraries(minimal_mpi_tomorun PRIVATE
    ${Boost_MPI_LIBRARIES} ${Boost_SERIALIZATION_LIBRARIES} ${MPI_LIBRARIES})
  add_test(NAME minimal_mpi_tomorun
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMAND "$<TARGET_FILE:minimal_mpi_tomorun>"
    )
else()
  generateDummyFailTest(cmd_argv "minimal_mpi_tomorun" "MPI not present -- test case not compiled")
  add_test(NAME minimal_mpi_tomorun
    COMMAND ${cmd_argv}
    )
endif()


if (BUILD_TOMORUN)
  #
  # Add tests to check for tomorun's overall behavior
  #
  add_executable(test_tomorun_1qubit_analytic_solution_check
    "tomorun/test_tomorun_1qubit_analytic_solution_check.cxx"
    )
  # Enable C++11
  set_property(TARGET test_tomorun_1qubit_analytic_solution_check PROPERTY CXX_STANDARD 11)

  # # query help text
  # add_test(NAME test_tomorun_help
  #   WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  #   COMMAND "$<TARGET_FILE:tomorun>" --help
  #   )

  # version without binning analysis
  add_test(NAME test_tomorun_1qubit_analytic_solution_run_simple
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMAND "$<TARGET_FILE:tomorun>" --data-file-name "${CMAKE_CURRENT_SOURCE_DIR}/tomorun/test_tomorun_1qubit_analytic_solution.mat"  --n-repeats=64  --n-sweep=25  --n-run=4096  --step-size=0.04  --value-type=fidelity:rho_ref  --value-hist=0.985:1/200  --write-histogram=test_tomorun_1qubit_testrun_simple --no-binning-analysis-error-bars --no-light-jumps --verbose
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
    COMMAND "$<TARGET_FILE:tomorun>" --data-file-name "${CMAKE_CURRENT_SOURCE_DIR}/tomorun/test_tomorun_1qubit_analytic_solution.mat"  --n-repeats=4  --n-sweep=25  --n-run=32768  --step-size=0.04  --value-type=fidelity:rho_ref  --value-hist=0.985:1/200  --write-histogram=test_tomorun_1qubit_testrun_binerr --control-binning-converged-max-unknown=12 --control-binning-converged-max-unknown-notisolated=4 --control-binning-converged-max-not-converged=2 --no-light-jumps --verbose
    )
  add_test(NAME test_tomorun_1qubit_analytic_solution_runcheck_binerr
    COMMAND "$<TARGET_FILE:test_tomorun_1qubit_analytic_solution_check>"
    test_tomorun_1qubit_testrun_binerr-histogram.csv
    500
    )
  set_tests_properties(test_tomorun_1qubit_analytic_solution_runcheck_binerr
    PROPERTIES DEPENDS "test_tomorun_1qubit_analytic_solution_check;test_tomorun_1qubit_analytic_solution_run_binerr")

  # version with binning analysis, light jumps
  add_test(NAME test_tomorun_1qubit_analytic_solution_run_light
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMAND "$<TARGET_FILE:tomorun>" --data-file-name "${CMAKE_CURRENT_SOURCE_DIR}/tomorun/test_tomorun_1qubit_analytic_solution.mat"  --n-repeats=4  --n-sweep=25  --n-run=32768  --step-size=0.04  --value-type=fidelity:rho_ref  --value-hist=0.985:1/200  --write-histogram=test_tomorun_1qubit_testrun_light --control-binning-converged-max-unknown=12 --control-binning-converged-max-unknown-notisolated=4 --control-binning-converged-max-not-converged=2 --light-jumps --verbose
    )
  add_test(NAME test_tomorun_1qubit_analytic_solution_runcheck_light
    COMMAND "$<TARGET_FILE:test_tomorun_1qubit_analytic_solution_check>"
    test_tomorun_1qubit_testrun_light-histogram.csv
    500
    )
  set_tests_properties(test_tomorun_1qubit_analytic_solution_runcheck_light
    PROPERTIES DEPENDS "test_tomorun_1qubit_analytic_solution_check;test_tomorun_1qubit_analytic_solution_run_light")

  if (BUILD_TOMOPY OR TOMOGRAPHER_TEST_SETUPPY_BUILD)

    # Additional tests
    add_test(NAME test_tomorun_case_help
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
      COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/test/tomorun/test_tomorun_run.py"
      "--setpath=${CMAKE_BINARY_DIR}/py:${CMAKE_SOURCE_DIR}/py:__TOMOGRAPHER_PYTHONPATH__"
      --expect-exit-code=1 --no-further-checks
      --
      "$<TARGET_FILE:tomorun>" --help
      )
    add_test(NAME test_tomorun_case_version
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
      COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/test/tomorun/test_tomorun_run.py"
      "--setpath=${CMAKE_BINARY_DIR}/py:${CMAKE_SOURCE_DIR}/py:__TOMOGRAPHER_PYTHONPATH__"
      --expect-exit-code=2 --no-further-checks
      --
      "$<TARGET_FILE:tomorun>" --version
      )
    add_test(NAME test_tomorun_case_1
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
      COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/test/tomorun/test_tomorun_run.py"
      "--setpath=${CMAKE_BINARY_DIR}/py:${CMAKE_SOURCE_DIR}/py:__TOMOGRAPHER_PYTHONPATH__"
      --check-histogram-file=out-histogram.csv "--check-qeb=(0.958,0.0090,6.1e-4),10%" "--ftox=(1,-1)"
      --
      "$<TARGET_FILE:tomorun>" "--data-file-name=${CMAKE_SOURCE_DIR}/examples/two-qubits-Bell/thedata.mat"
      --write-histogram=out --n-run=32768 --n-repeats=8 "--value-type=obs-value:rho_ref" "--value-hist=0.9:1/50"
      --light-jumps
      --periodic-status-report-ms=2000
      --verbose --verbose-log-info
      )
    add_test(NAME test_tomorun_case_1b
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
      COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/test/tomorun/test_tomorun_run.py"
      "--setpath=${CMAKE_BINARY_DIR}/py:${CMAKE_SOURCE_DIR}/py:__TOMOGRAPHER_PYTHONPATH__"
      --check-histogram-file=out1b-histogram.csv --check-binning-vs-naive
      --
      "$<TARGET_FILE:tomorun>" "--data-file-name=${CMAKE_SOURCE_DIR}/examples/two-qubits-Bell/thedata.mat"
      --write-histogram=out1b --n-run=8192 --n-repeats=64 "--value-type=obs-value:rho_ref" "--value-hist=0.93:.980/20"
      --binning-analysis-error-bars --binning-analysis-num-levels=7
      --periodic-status-report-ms=2000
      --verbose --verbose-log-info
      )
    add_test(NAME test_tomorun_case_2
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
      COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/test/tomorun/test_tomorun_run.py"
      "--setpath=${CMAKE_BINARY_DIR}/py:${CMAKE_SOURCE_DIR}/py:__TOMOGRAPHER_PYTHONPATH__"
      --check-histogram-file=out2-histogram.csv "--check-qeb=(0.0377,0.013,0.0014),10%" "--ftox=(0,1)"
      --
      "$<TARGET_FILE:tomorun>" "--data-file-name=${CMAKE_SOURCE_DIR}/examples/two-qubits-Bell/thedata.mat"
      --write-histogram=out2 --n-run=32768 --n-repeats=8 "--value-type=tr-dist:rho_MLE" "--value-hist=0.:0.2/50"
      --no-light-jumps
      --periodic-status-report-ms=2000 --no-control-binning-converged
      --verbose --verbose-log-info
      )
    add_test(NAME test_tomorun_case_3
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
      COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/test/tomorun/test_tomorun_run.py"
      "--setpath=${CMAKE_BINARY_DIR}/py:${CMAKE_SOURCE_DIR}/py:__TOMOGRAPHER_PYTHONPATH__"
      --check-histogram-file=out3-histogram.csv "--check-qeb=(1.83,0.036,0.0025),30%" "--ftox=(2,-1)"
      --
      "$<TARGET_FILE:tomorun>" "--data-file-name=${CMAKE_SOURCE_DIR}/examples/two-qubits-Bell/thedata.mat"
      --write-histogram=out3 --n-run=32768 --n-repeats=8 "--value-type=obs-value:EntglWitness" "--value-hist=1.6:2.0/50"
      --no-light-jumps
      --periodic-status-report-ms=2000
      --verbose --verbose-log-info
      )

    add_test(NAME test_tomorun_case_4
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
      COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/test/tomorun/test_tomorun_run.py"
      "--setpath=${CMAKE_BINARY_DIR}/py:${CMAKE_SOURCE_DIR}/py:__TOMOGRAPHER_PYTHONPATH__"
      # not sure the qeb's are really relevant here (for this figure of merit), but it's good enough for the test case
      --check-histogram-file=out4-histogram.csv "--check-percent-within-range=95%(0.156,0.260)"
      --
      "$<TARGET_FILE:tomorun>" "--data-file-name=${CMAKE_SOURCE_DIR}/examples/two-qubits-Bell/thedata.mat"
      --write-histogram=out4 --n-run=32768 --n-repeats=8 "--value-type=purif-dist:rho_ref" "--value-hist=0.0:0.4/50"
      --no-light-jumps
      --periodic-status-report-ms=2000
      --verbose --verbose-log-info
      )
    
  else()

    add_test(NAME tomorun_specific_checks_NOT_BUILT_NEEDS_TOMOPY COMMAND "false")

  endif()

else() # BUILD_TOMORUN

  add_test(NAME tomorun_NOT_BUILT
    COMMAND "false"
    )

endif()


# Python tests.  Potentially prepare two versions -- one for cmake-built python module and
# one for setup.py-built python module.

if (BUILD_TOMOPY)
  macro(addTomographerPyTest_cmake pymodule)
    add_test(NAME ${pymodule}
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
      COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/pyruntest.py" "--test=${pymodule}" "--tomographer-cxx-lib=$<TARGET_FILE:_tomographer_cxx>" "--tomographer-source=${CMAKE_SOURCE_DIR}"
      )
  endmacro()
else()
  macro(addTomographerPyTest_cmake pymodule)
  endmacro()
endif() # BUILD_TOMOPY

if(TOMOGRAPHER_TEST_SETUPPY_BUILD)
  message(STATUS "Adding python tests for setup.py-built module; make sure you set \$TOMOGRAPHER_PYTHONPATH at runtime.")
  macro(addTomographerPyTest_setuppybuilt pymodule)
    add_test(NAME ${pymodule}_setuppybuilt
      WORKING_DIRECTORY "${CMAKE_BINARY_DIR}" # other dir, make sure we don't pick up a cmake-built _tomographer_cxx.so
      COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/pyruntest.py" "--test=${pymodule}" "--mode=setup-py-built" "--tomographer-source=${CMAKE_SOURCE_DIR}"
      )
  endmacro()
else()
  macro(addTomographerPyTest_setuppybuilt pymodule)
  endmacro()
endif() # TOMOGRAPHER_TEST_WITH_SEPARATE_PYTHON_BUILD

macro(addTomographerPyTest pymodule)
  addTomographerPyTest_cmake(${pymodule})
  addTomographerPyTest_setuppybuilt(${pymodule})
endmacro()

# add failing test if neither cmake nor setup.py python is being tested
if (NOT BUILD_TOMOPY AND NOT TOMOGRAPHER_TEST_WITH_SEPARATE_PYTHON_BUILD)
  add_test(NAME tomographerpy_NOT_BUILT
    COMMAND "false"
    )
endif()


addTomographerPyTest(pytest_tomographer)
addTomographerPyTest(pytest_t_multiproc)
addTomographerPyTest(pytest_t_densedm)
addTomographerPyTest(pytest_t_mhrwtasks)
addTomographerPyTest(pytest_t_tomorun)
addTomographerPyTest(pytest_t_querrorbars)
addTomographerPyTest(pytest_t_jpyutil)
addTomographerPyTest(pytest_t_tools_densedm)
addTomographerPyTest(pytest_pickle)



# ------------------------------------------------


message(STATUS "Added test targets")

