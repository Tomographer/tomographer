# This file is part of the Tomographer project, which is distributed under the
# terms of the MIT license.
# 
# The MIT License (MIT)
# 
# Copyright (c) 2015 ETH Zurich, Institute for Theoretical Physics, Philippe
# Faist
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.



# There is nothing to compile here because this is a header-only library.

# configure the header file
execute_process(
  COMMAND "${CMAKE_COMMAND}" "-DTOMOGRAPHER_VERSION=${TOMOGRAPHER_VERSION}" "-DTomographerBinaryDir=${CMAKE_CURRENT_BINARY_DIR}" "-DTomographerSourceDir=${CMAKE_CURRENT_SOURCE_DIR}" -P "${CMAKE_SOURCE_DIR}/cmake/UpdateTomographerVersionHeader.cmake"
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  )

# Install our headers in the standard header location of the system
install(DIRECTORY .
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/tomographer2"
  FILES_MATCHING
  PATTERN "*.h"
  )
# Install version header
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/tomographer_version.h"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/tomographer2"
  )

if(TOMOGRAPHER_SET_HOOK_GIT_UPDATE_VERSION)
  # set up a hook to update the header every time the output of `git describe --tags`
  # changes.

  message(STATUS "Tomographer Version: Setting up automatic header version update using git-describe")

  set(GitVersionStampFile "${CMAKE_BINARY_DIR}/git-describe-VERSION.stamp")

  # this needs to be run as a target, not an output
  add_custom_target(tomographer_update_git_version_stamp
    COMMAND "${CMAKE_COMMAND}" "-DGitVersionStampFile=${GitVersionStampFile}" "-DGitDescribeGetVersionCommand=${GitDescribeGetVersionCommand}" -P "${CMAKE_SOURCE_DIR}/cmake/UpdateTomographerGitVersion.cmake"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Checking that git-describe version stamp is up-to-date."
    VERBATIM
    )

  add_custom_command(OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/tomographer_version.h"
    COMMAND "${CMAKE_COMMAND}" "-DVersionFile=${GitVersionStampFile}" "-DTomographerBinaryDir=${CMAKE_CURRENT_BINARY_DIR}" "-DTomographerSourceDir=${CMAKE_CURRENT_SOURCE_DIR}" -P "${CMAKE_SOURCE_DIR}/cmake/UpdateTomographerVersionHeader.cmake"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS tomographer_update_git_version_stamp
    DEPENDS "${GitVersionStampFile}"
    COMMENT "Updating tomographer_version.h header file"
    VERBATIM
    )

endif(TOMOGRAPHER_SET_HOOK_GIT_UPDATE_VERSION)


add_custom_target(tomographer
  DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/tomographer_version.h"
  )
