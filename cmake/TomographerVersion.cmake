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



set(TomographerVersionFile "${CMAKE_SOURCE_DIR}/VERSION")

################################################################################

set(GitNames git git.exe)

macro(DoFindGit)
  find_program(GIT
    NAMES ${GitNames}
    DOC "GIT command-line executable"
    )
  set(GitDescribeGetVersionCommand
    "${GIT}" describe --tags HEAD
    )
  macro(GetTomographerGitVersion version_str)
    execute_process(COMMAND ${GitDescribeGetVersionCommand}
      OUTPUT_VARIABLE "${version_str}"
      OUTPUT_STRIP_TRAILING_WHITESPACE)
  endmacro(GetTomographerGitVersion)
endmacro()


################################################################################

if(EXISTS "${TomographerVersionFile}")

  # set TOMOGRAPHER_VERSION
  file(READ "${CMAKE_SOURCE_DIR}/VERSION" _tomover)
  string(STRIP "${_tomover}" TOMOGRAPHER_VERSION)
  
  # not sure we got GIT, don't bother end users because we've got our version from the
  # text file
  set(_set_git_version_update_hook false)

else(EXISTS "${TomographerVersionFile}")

  set(_git_all_ok false)
  if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    DoFindGit()
    if(GIT)
      set(_git_all_ok true)
    endif(GIT)
  else(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    set(GIT "")
  endif(EXISTS "${CMAKE_SOURCE_DIR}/.git")

  if(_git_all_ok)
    
    # read version number from ``git describe --tags`` automatically, etc.
    GetTomographerGitVersion(TOMOGRAPHER_VERSION)
    
    # Also, add compilation hook that will update the tomographer_version.h whenever the git
    # description changes.
    set(_set_git_version_update_hook true)
    
  else(_git_all_ok)

    message(WARNING, "Can't determine Tomographer GIT version. Looks like you didn't clone the GIT repo and didn't download an official release; or CMake couldn't find the `git' executable.")
    
    set(TOMOGRAPHER_VERSION "<unknown>")
    # don't try to update, won't find git repo
    set(_set_git_version_update_hook false)

  endif(_git_all_ok)

endif(EXISTS "${TomographerVersionFile}")


set(TOMOGRAPHER_SET_HOOK_GIT_UPDATE_VERSION "${_set_git_version_update_hook}"
  CACHE BOOL "[For Developers] Whether to automatically update version information from git-describe at each compilation or not.")


################################################################################

# parse TOMOGRAPHER_VERSION

if(TOMOGRAPHER_VERSION MATCHES "^v([0-9]+)[.]([0-9]+)(.*)$")
  set(TOMOGRAPHER_VERSION_MAJOR "${CMAKE_MATCH_1}")
  set(TOMOGRAPHER_VERSION_MINOR "${CMAKE_MATCH_2}")
  set(TOMOGRAPHER_VERSION_SUFFIX "${CMAKE_MATCH_3}")
else()
  message(FATAL_ERROR "Invalid TOMOGRAPHER_VERSION: ${TOMOGRAPHER_VERSION}")
endif()

set(PROJECT_VERSION "${TOMOGRAPHER_VERSION_MAJOR}.${TOMOGRAPHER_VERSION_MINOR}")
