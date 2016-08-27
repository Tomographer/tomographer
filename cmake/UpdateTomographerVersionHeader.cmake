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


# 
# Configure the tomographer_version.h.in file.
# 
# Also create a corresponding tomographer_VERSION file which contains the raw
# version. This is useful e.g. for the doxygen documentation.
# 
# Required variables:
#     - VersionFile  OR  TOMOGRAPHER_VERSION
#     - TomographerBinaryDir
#     - TomographerSourceDir
# 
# those last directories must point to the 'cxx/tomographer/' directories in the binary
# and source tree, respectively.


#message(STATUS "VersionFile=${VersionFile}")

if(VersionFile)
  file(READ "${VersionFile}" _tomover)
  string(STRIP "${_tomover}" TOMOGRAPHER_VERSION)
endif(VersionFile)

#message(STATUS "Tomographer Version is ${TOMOGRAPHER_VERSION}")

#
# Parse version MAJ/MIN/SUFFIX
#
if (TOMOGRAPHER_VERSION MATCHES "v?([0-9]+)[.]([0-9]+)([^0-9]|$)")
  set(TOMOGRAPHER_VERSION_MAJ ${CMAKE_MATCH_1})
  set(TOMOGRAPHER_VERSION_MIN ${CMAKE_MATCH_2})
else()
  set(TOMOGRAPHER_VERSION_MAJ -1)
  set(TOMOGRAPHER_VERSION_MIN -1)
endif()


#
# TOMOGRAPHER_VERSION is set.
#
CONFIGURE_FILE(
  "${TomographerSourceDir}/tomographer_version.h.in"
  "${TomographerBinaryDir}/tomographer_version.h"
  @ONLY
  )

file(WRITE "${TomographerBinaryDir}/tomographer_VERSION"
  "${TOMOGRAPHER_VERSION}"
  )
