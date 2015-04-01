
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
