
# There is nothing to compile here because this is a header-only library.

# TOMOGRAPHER_VERSION is already set.
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/tomographer_version.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/tomographer_version.h"
  @ONLY
  )


# Install our headers in the standard header location of the system
install(DIRECTORY .
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/tomographer"
  FILES_MATCHING
  PATTERN "*.h"
  )
# Install version header
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/tomographer_version.h"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/tomographer"
  )
