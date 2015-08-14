
#
# Configure the tomographer_version.h.in file.
#
# Required variables:
#     - VersionFile  OR  TOMOGRAPHER_VERSION
#     - TomographerBinaryDir
#     - TomographerSourceDir
#
# those last directories must point to the 'cxx/tomographer/' directories in the binary
# and source tree, respectively.
#

#message(STATUS "VersionFile=${VersionFile}")

if(VersionFile)
  file(READ "${VersionFile}" _tomover)
  string(STRIP "${_tomover}" TOMOGRAPHER_VERSION)
endif(VersionFile)

#message(STATUS "Tomographer Version is ${TOMOGRAPHER_VERSION}")

#
# TOMOGRAPHER_VERSION is set.
#
CONFIGURE_FILE(
  "${TomographerSourceDir}/tomographer_version.h.in"
  "${TomographerBinaryDir}/tomographer_version.h"
  @ONLY
  )

