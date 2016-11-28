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


# ==============================================================================
# Add our executable
# ==============================================================================

# for <tomographer/tomographer_version.h> which was auto-generated in the binary
# dir tree, and is not yet installed.
include_directories("${CMAKE_BINARY_DIR}/cxx")


set(TOMORUN_SUFFIX "" CACHE STRING
  "Optional suffix to append to 'tomorun' executable name, to indicate special configuration in case you override settings in tomorun_config.h with -D... compiler flags (such as -DTOMORUN_CUSTOM_FIXED_DIM=... etc.)")


# can we use the nice() function to reduce the process priority?
CHECK_CXX_SOURCE_COMPILES(
  "#include <unistd.h>
  int main() { nice(1); }"
  tomorun_HAVE_NICE)
if(NOT tomorun_HAVE_NICE)
  add_definitions(-DTOMORUN_NOT_HAVE_NICE)
endif()


add_executable(tomorun tomorun.cxx)
add_dependencies(tomorun tomographer) # in particular tomographer_version.h

if(TOMORUN_SUFFIX)
  if(CMAKE_BUILD_TYPE)
    string(TOUPPER "${CMAKE_BUILD_TYPE}" tmp_buildtype)
    set_target_properties(tomorun PROPERTIES ${tmp_buildtype}_OUTPUT_NAME "tomorun${TOMORUN_SUFFIX}")
  else(CMAKE_BUILD_TYPE)
    set_target_properties(tomorun PROPERTIES OUTPUT_NAME "tomorun${TOMORUN_SUFFIX}")
  endif(CMAKE_BUILD_TYPE)
endif(TOMORUN_SUFFIX)

SetOpenMPTarget(tomorun)

AppendTargetProperty(tomorun COMPILE_FLAGS "-I${MATIO_INCLUDE_DIR} -I${Boost_INCLUDE_DIR} -I${ZLIB_INCLUDE_DIRS}")
target_link_libraries(tomorun ${MATIO_LIBRARIES})
target_link_libraries(tomorun ${ZLIB_LIBRARIES})
target_link_libraries(tomorun ${Boost_PROGRAM_OPTIONS_LIBRARY})

install(TARGETS tomorun
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
