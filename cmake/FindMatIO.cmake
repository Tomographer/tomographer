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


# Find MatIO library
#
# This script defines the following variables:
#
#  MATIO_FOUND - MatIO was found on this system
#  MATIO_INCLUDE_DIRS - include directories (for -I flags)
#  MATIO_LIBRARIES - libraries needed to link to MatIO
#  MATIO_DEFINITIONS - Compiler definitions needed for MatIO

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
