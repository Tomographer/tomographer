
# ==============================================================================
# Add our executable
# ==============================================================================

add_executable(tomorun tomorun.cxx)

SetOpenMPTarget(tomorun)

AppendTargetProperty(tomorun COMPILE_FLAGS "-I${MATIO_INCLUDE_DIR} -I${Boost_INCLUDE_DIR}")
target_link_libraries(tomorun ${MATIO_LIBRARIES} ${Boost_LIBRARIES})
