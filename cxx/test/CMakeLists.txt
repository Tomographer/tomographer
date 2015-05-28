
# ==============================================================================
# Add the tests.
# ==============================================================================


add_definitions(-DBOOST_TEST_DYN_LINK)

# test suite
set(tests_SRCS
  # list of sources containing test cases:
  test_tomographer.cxx
  testsphcoords.cxx
  test_lib_basics.cxx
  test_param.cxx
  test_integrator_basic.cxx
)

# we need to access the expected output patterns. Trailing slash needed.
set(TOMOGRAPHER_TEST_PATTERNS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/patterns/"
  CACHE STRING "path to the expected test results patterns")
add_definitions(-DTOMOGRAPHER_TEST_PATTERNS_DIR="${TOMOGRAPHER_TEST_PATTERNS_DIR}")


# the test binary
add_executable(test_tomographer ${tests_SRCS})
target_link_libraries(test_tomographer ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})



# additional, non-boost tests not yet converted to Boost/UnitTestFramework

# test the simple zero finder
add_executable(test_simple_find_zero test_simple_find_zero.cxx)
target_link_libraries(test_simple_find_zero ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

# parameterizations
#add_executable(test_param test_param.cxx)
#target_link_libraries(test_param ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

# omp support for integrator
add_executable(test_multi_omp test_multi_omp.cxx)
SetOpenMPTarget(test_multi_omp)
target_link_libraries(test_multi_omp ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

