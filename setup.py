#setup.py

from distutils.core import setup
from distutils.extension import Extension

import os.path
import sys

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
