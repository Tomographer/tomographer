
################################################################################
################################## UTILITIES ###################################
################################################################################


macro(AppendTargetProperty tgt property value)
  set(new_property_value "${value}")
  get_target_property(existing_property_value ${tgt} ${property})
  if(existing_property_value)
    set(new_property_value "${existing_property_value} ${new_property_value}")
  endif()
  set_target_properties(${tgt} PROPERTIES ${property} "${new_property_value}")
endmacro(AppendTargetProperty)



macro(SetOpenMPTarget target)
  if(OPENMP_FOUND)
    #OpenMP_C_FLAGS   - flags to add to the C compiler for OpenMP support
    #OpenMP_CXX_FLAGS - flags to add to the CXX compiler for OpenMP support
    AppendTargetProperty(${target} COMPILE_FLAGS "${OpenMP_CXX_FLAGS} -DTOMOGRAPHER_HAVE_OMP")
    AppendTargetProperty(${target} LINK_FLAGS "${OpenMP_CXX_FLAGS}")
  else()
    message(WARNING "OpenMP not found. `${target}' will run serially with no parallelization.")
  endif()
endmacro(SetOpenMPTarget)



macro(RemoveFlag str_out str_in flag)
  # first, escape the flag to a valid regex pattern. Just backslash all chars.
  string(REGEX REPLACE "(.)" "\\\\\\1" flag_escaped "${flag}")
  # now, remove the flag from str_in.
  string(REGEX REPLACE "${flag}( |\$)" "" ${str_out} "${str_in}")

  message(STATUS "Removed flag '${flag}' from '${str_in}': flag_escaped='${flag_escaped}', str_out='${${str_out}}'")
endmacro(RemoveFlag)

macro(RemoveFlagTarget tgt prop flag)
  get_target_property(compile_flags ${tgt} ${prop})
  RemoveFlag(compile_flags_new "${compile_flags}" "-DNDEBUG")
  set_target_properties(${tgt} PROPERTIES ${prop} "${compile_flags_new}")
endmacro(RemoveFlagTarget)
