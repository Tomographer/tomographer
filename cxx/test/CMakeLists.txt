
# ==============================================================================
# Add the tests.
# ==============================================================================


add_definitions(-DBOOST_TEST_DYN_LINK)

# test cases
set(tests_SRCS
  test_tomographer.cxx
  testsphcoords.cxx
  test_lib_basics.cxx
)
add_executable(test_tomographer ${tests_SRCS})
target_link_libraries(test_tomographer ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})


# test the simple zero finder
add_executable(test_simple_find_zero test_simple_find_zero.cxx)
target_link_libraries(test_simple_find_zero ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

# library basics
#add_executable(test_lib_basics test_lib_basics.cxx)
#target_link_libraries(test_lib_basics ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

# parameterizations
add_executable(test_param test_param.cxx)
target_link_libraries(test_param ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

# basic integrator
add_executable(test_integrator_basic test_integrator_basic.cxx)
target_link_libraries(test_integrator_basic ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

# omp support for integrator
add_executable(test_multi_omp test_multi_omp.cxx)
SetOpenMPTarget(test_multi_omp)
target_link_libraries(test_multi_omp ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

