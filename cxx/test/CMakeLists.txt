
# ==============================================================================
# Add the tests.
# ==============================================================================


add_definitions(-DBOOST_TEST_DYN_LINK)

# we need to access the expected output patterns. Trailing slash needed.
set(TOMOGRAPHER_TEST_PATTERNS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/patterns/"
  CACHE STRING "path to the expected test results patterns")
add_definitions(-DTOMOGRAPHER_TEST_PATTERNS_DIR="${TOMOGRAPHER_TEST_PATTERNS_DIR}")


# test suite
set(tests_SRCS
  # list of sources containing test cases:
  test_tomographer.cxx
  testsphcoords.cxx
  test_loggers.cxx
  test_lib_basics.cxx
  test_param.cxx
  test_integrator_basic.cxx
  test_simple_find_zero.cxx
  test_multi_omp.cxx
)


# the test binary
add_executable(test_tomographer ${tests_SRCS})
SetOpenMPTarget(test_tomographer)
target_link_libraries(test_tomographer ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

