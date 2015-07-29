
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

# test suite
set(tests_SRCS
  # list of sources containing test cases:
  test_tomographer.cxx
  testsphcoords.cxx
  test_misc.cxx
  test_loggers.cxx
  test_lib_basics.cxx
  test_param.cxx
  test_integrator_basic.cxx
  test_simple_find_zero.cxx
  test_mhrw_bin_err.cxx
  test_multi_omp.cxx
  test_histogram.cxx
)


# the test binary executable
add_executable(test_tomographer ${tests_SRCS})

# compile & link with OpenMP
SetOpenMPTarget(test_tomographer)

# link with Boost.Test framework
target_link_libraries(test_tomographer ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
