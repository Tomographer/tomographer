
from setuptools import setup
from setuptools.extension import Extension

import codecs
import os.path
import sys
import re
import shlex
try:
    from shlex import quote as shlexquote
except ImportError:
    from pipes import quote as shlexquote


import numpy # numpy.get_include()
import pybind11 # pybind11.get_include()
import tomographer.include # tomographer.include.get_include()
from tomographer.include import find_include_dir, find_lib, Vars
import tomographer.version # compile_info

#
# Environment variables that the user can specify when calling this script
#

vv = Vars([
    'CXX_FLAGS',
])

#
# default values for the variables
#

# For CXX_FLAGS, use the same C++ compiler flags as those used to compile
# tomographer itself. Includes optimization options.
tomographer_cflags = tomographer.version.compile_info['cflags']
if tomographer_cflags is not None:
    vv.setDefault('CXX_FLAGS',
                  " ".join([shlexquote(x) for x in tomographer_cflags]))
else:
    # We are using a wierdly compiled tomographer module which doesn't expose
    # its cflags. Use some reasonable defaults.
    vv.setDefault('CXX_FLAGS', '-std=c++11 -UNDEBUG -O3 -g -march=native')

#
# Display variables to the user
#
print(vv.message())



#
# Set up the python package
#

setup(name="my_custom_package",
      version='0.1',
      description='A Custom Special-Purpose C++ Module with a Simple Python Interface',
      ext_modules=[
          Extension(
              # Module name. Must match whatever you gave to PYBIND11_PLUGIN(...)
              name="my_custom_module",
              # list your source C++ file(s) for this module here:
              sources=[
                  'my_custom_module.cxx'
              ],
              # Include directories (note that tomographer.include.get_include() returns a
              # list, including Boost and Eigen header locations as well as Tomographer
              # and Tomographer-Py header locations):
              include_dirs=[
                  numpy.get_include(),
                  pybind11.get_include(),
              ] + tomographer.include.get_include(),
              # Compiler flags:
              extra_compile_args=shlex.split(vv.get('CXX_FLAGS')),
              # any linker flags, if needed (do NOT use -flto, or else use it at your own risk):
              extra_link_args=[],
              # anywhere to look for libraries you need to link against? :
              library_dirs=[],
              # any libraries you need to link against? :
              libraries=[],
              # any custom header files you depend on:
              depends=[]
          ),
      ],
)
