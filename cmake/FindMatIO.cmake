# - Try to find MatIO
# Once done this will define
#  MATIO_FOUND - System has MatIO
#  MATIO_INCLUDE_DIRS - The MatIO include directories
#  MATIO_LIBRARIES - The libraries needed to use MatIO
#  MATIO_DEFINITIONS - Compiler switches required for using MatIO


find_path(MATIO_INCLUDE_DIR
  matio.h
  )

find_library(MATIO_LIBRARY
  NAMES matio
  )

set(MATIO_LIBRARIES ${MATIO_LIBRARY} )
set(MATIO_INCLUDE_DIRS ${MATIO_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set MATIO_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(MatIO  DEFAULT_MSG
                                  MATIO_LIBRARY MATIO_INCLUDE_DIR)

mark_as_advanced(MATIO_INCLUDE_DIR MATIO_LIBRARY )
