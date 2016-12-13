

find_program(SPHINX_BUILD_COMMAND
  NAMES sphinx-build
  PATHS
    /usr/bin
    /usr/local/bin
  DOC "Sphinx build command (sphinx-build)")

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Sphinx DEFAULT_MSG SPHINX_BUILD_COMMAND)
