
# ==============================================================================
# Add our executable
# ==============================================================================

add_executable(tomorun tomorun.cxx)

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
