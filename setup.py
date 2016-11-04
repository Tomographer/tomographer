#setup.py

from distutils.core import setup
from distutils.extension import Extension

import os.path
import sys

# I'm currently compiling with
#
# CXX=/opt/gcc4.9/bin/g++ CC=/opt/gcc4.9/bin/gcc python setup.py build
#
# and running tests with
#
# DYLD_INSERT_LIBRARIES=/opt/gcc4.9/lib/libstdc++.dylib PYTHONPATH=$PYTHONPATH:`pwd`/build/lib.macosx-10.6-x86_64-2.7 python testscript2.py

include_dirs = [ "/usr/local/include",
                 "/usr/local/include/eigen3",
                 "/opt/tomographer-install/include",
                 "." ]
libraries = [ "boost_python-mt" ]
library_dirs = ['.']
cflags = ['-std=c++11', '-fdiagnostics-color', '-DEIGEN_DEFAULT_TO_ROW_MAJOR']

files = [ "tomographerbase.cxx" ]

setup(name="tomographer",
      ext_modules=[
          Extension("tomographer", files,
                    library_dirs=library_dirs,
                    libraries=libraries,
                    include_dirs=include_dirs,
                    extra_compile_args=cflags,
                    depends=[ 'eigpyconv.h' ]),
      ]
)
