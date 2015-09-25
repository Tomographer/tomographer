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


################################################################################
################################## UTILITIES ###################################
################################################################################


# http://stackoverflow.com/a/7216542
function(ListJoin VALUES GLUE OUTPUT)
  string (REGEX REPLACE "([^\\]|^);" "\\1${GLUE}" _TMP_STR "${VALUES}")
  string (REGEX REPLACE "[\\](.)" "\\1" _TMP_STR "${_TMP_STR}") #fixes escaping
  set (${OUTPUT} "${_TMP_STR}" PARENT_SCOPE)
endfunction()


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
    AppendTargetProperty(${target} COMPILE_FLAGS "${OpenMP_CXX_FLAGS}")
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

  #message(STATUS "Removed flag '${flag}' from '${str_in}': flag_escaped='${flag_escaped}', str_out='${${str_out}}'")
endmacro(RemoveFlag)

macro(RemoveFlagTarget tgt prop flag)
  get_target_property(compile_flags ${tgt} ${prop})
  RemoveFlag(compile_flags_new "${compile_flags}" "-DNDEBUG")
  set_target_properties(${tgt} PROPERTIES ${prop} "${compile_flags_new}")
endmacro(RemoveFlagTarget)
