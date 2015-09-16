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
# Set up the Doxygen configuration file with correct TOMOGRAPHER Version.
# 
# Also create a corresponding tomographer_VERSION file which contains the raw
# version. This is useful, e.g. for the doxygen documentation.
# 
# Required variables:
#  - VersionFile
#  - RootSourceDir
#  - TargetDoxyfileDir
# 


file(READ "${VersionFile}" _tomover)
string(STRIP "${_tomover}" TOMOGRAPHER_VERSION)

file(RELATIVE_PATH RELPATH_SRCROOT "${TargetDoxyfileDir}" "${RootSourceDir}")
file(RELATIVE_PATH DOCROOT "${TargetDoxyfileDir}" "${RootSourceDir}/doc")

message(STATUS "Tomographer Version is ${TOMOGRAPHER_VERSION}")
message(STATUS "    RELPATH_SRCROOT=${RELPATH_SRCROOT}")
message(STATUS "    DOCROOT=${DOCROOT}")

#
# TOMOGRAPHER_VERSION is set.
#
CONFIGURE_FILE(
  "${RootSourceDir}/doc/Doxyfile.in"
  "${TargetDoxyfileDir}/Doxyfile"
  @ONLY
  )
