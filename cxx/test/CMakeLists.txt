
# ==============================================================================
# Add the tests.
# ==============================================================================

# test our spherical coordinates routines
add_executable(testsphcoords testsphcoords.cxx)


add_executable(test_lib_basics test_lib_basics.cxx)


add_executable(test_integrator_basic test_integrator_basic.cxx)


add_executable(test_multi_omp test_multi_omp.cxx)

SetOpenMPTarget(test_multi_omp)

