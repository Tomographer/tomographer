
# There is nothing to compile here because this is a header-only library.

# Install our headers in the standard header location of the system
install(DIRECTORY .
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tomographer
  FILES_MATCHING
  PATTERN "*.h"
  )
