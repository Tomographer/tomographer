
# ==============================================================================
# Add our executable
# ==============================================================================

# for <tomographer/tomographer_version.h> which was auto-generated in the binary
# dir tree, and is not yet installed.
include_directories("${CMAKE_BINARY_DIR}/cxx")

# already parallelized application, so disable Eigen's own parallelization (see
# http://eigen.tuxfamily.org/dox/TopicMultiThreading.html)
add_definitions(-DEIGEN_DONT_PARALLELIZE)

add_executable(tomorun tomorun.cxx)
add_dependencies(tomorun tomographer) # in particular tomographer_version.h

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
