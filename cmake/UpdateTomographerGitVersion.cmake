# This file is part of the Tomographer project, which is distributed under the
# terms of the MIT license.
# 
# The MIT License (MIT)
# 
# Copyright (c) 2015 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
# Copyright (c) 2017 Caltech, Institute for Quantum Information and Matter, Philippe Faist
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
# Update the version by querying GIT-DESCRIBE.
#
# Updates the file "${CMAKE_BINARY_DIR}/gitVERSION" according to the git version. If the
# version hasn't changed, the file is not touched.
#
# MUST BE DEFINED:
#  - GitVersionStampFile
#  - GitDescribeGetVersionCommand
#

#message(STATUS "UpdateTomographerGitVersion -- stamp `${GitVersionStampFile}' with command parts ${GitDescribeGetVersionCommand}")

execute_process(COMMAND ${GitDescribeGetVersionCommand}
      OUTPUT_VARIABLE version_str
      OUTPUT_STRIP_TRAILING_WHITESPACE)

if(EXISTS "${GitVersionStampFile}")

  # set TOMOGRAPHER_VERSION
  file(READ "${GitVersionStampFile}" _tomover)
  string(STRIP "${_tomover}" old_tomographer_git_version)

else(EXISTS "${GitVersionStampFile}")

  set(old_tomographer_git_version "")

endif(EXISTS "${GitVersionStampFile}")


if("${version_str}" STREQUAL "${old_tomographer_git_version}")
  # all set, don't need to update anything
else()
  # update the version in 'gitVERSION'
  file(WRITE "${GitVersionStampFile}"
    "${version_str}\n"
    )
endif()
