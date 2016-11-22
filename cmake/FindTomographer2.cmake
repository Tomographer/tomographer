

# Find Tomographer2 library
#
# This script defines the following variables:
#
#  TOMOGRAPHER2_FOUND - Tomographer2 was found on this system
#  TOMOGRAPHER2_INCLUDE_DIRS - include directories (for -I flags)
#  TOMOGRAPHER2_DEFINITIONS - Compiler definitions needed for Tomographer2



find_package(Eigen3 REQUIRED)
find_package(Boost 1.40 REQUIRED)

find_package(OpenMP QUIET)
find_package(MatIO QUIET)
find_package(ZLIB QUIET)



find_path(TOMOGRAPHER2_INCLUDE_DIR
  tomographer2/tomographer_version.h
  )


set(TOMOGRAPHER2_INCLUDE_DIRS
  ${TOMOGRAPHER2_INCLUDE_DIR}
  ${EIGEN3_INCLUDE_DIR}
  ${EIGEN_INCLUDE_DIR}
  ${Boost_INCLUDE_DIR}
  ${MATIO_INCLUDE_DIR}
  ${ZLIB_INCLUDE_DIRS} )


include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set TOMOGRAPHER2_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(TOMOGRAPHER2
  DEFAULT_MSG
  TOMOGRAPHER2_INCLUDE_DIR)

mark_as_advanced(TOMOGRAPHER2_INCLUDE_DIR)
