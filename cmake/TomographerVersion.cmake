
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
