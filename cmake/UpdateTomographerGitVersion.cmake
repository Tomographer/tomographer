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
