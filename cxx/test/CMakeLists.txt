
# ==============================================================================
# Add the tests.
# ==============================================================================

# test our spherical coordinates routines
add_executable(testsphcoords testsphcoords.cxx)

# test the simple zero finder
add_executable(test_simple_find_zero test_simple_find_zero.cxx)

# library basics
add_executable(test_lib_basics test_lib_basics.cxx)

# parameterizations
add_executable(test_param test_param.cxx)

# basic integrator
add_executable(test_integrator_basic test_integrator_basic.cxx)

# omp support for integrator
add_executable(test_multi_omp test_multi_omp.cxx)
SetOpenMPTarget(test_multi_omp)

