#setup.py

from distutils.core import setup
from distutils.extension import Extension

import os.path
import sys
import subprocess # for git
import re

#
# POSSIBLE ENVIRONMENT VARIABLES WHICH CAN BE SET:
#
# handled natively by setup.py:
# CC=/path/to/compiler/gcc
#
# handled by us: (can be found in CMake cache)
# GIT=/path/to/git
# Boost_INCLUDE_DIR=/path/to/boost/headers
# Boost_PYTHON_LIBRARY_RELEASE=/path/to/libboost_python-py35.so
# EIGEN3_INCLUDE_DIR=/path/to/eigen/headers
# OpenMP_CXX_FLAGS=-fopenmp=libomp
#
# also handled by us: (cannot be found in cmake cache)
# CXX11_STD_FLAGS=-std=c++11
#
# CMAKE_CACHE_FILE=path/to/CMakeCache.txt -- read the above variables from cache
#


thisdir = os.path.dirname(__file__)
#print("This dir = {}".format(thisdir))


#
# Looks up the variables:
#   - in the environment
#   - in the CMake cache, if any
#   - default value
#
class Vars(object):
    def __init__(self, cachefile=None):
        self.d = {} # values read & cached
        if cachefile:
            rx = re.compile(r'^(?P<name>[A-Za-z0-9_]+)(:(?P<type>[A-Za-z_]+))=(?P<value>.*)$')
            with open(cachefile) as f:
                for line in f:
                    m = rx.match(line)
                    if not m:
                        continue
                    self.d[m.group('name')] = m.group('value')

    def setDefault(self, name, default):
        try:
            self.get(name)
        except KeyError:
            self.d[name] = default
            
    def get(self, name):
        if name in os.environ:
            return os.environ[name]
        if name in self.d:
            return self.d[name]
        raise KeyError("Variable "+name+" not found")

vv = Vars(os.environ.get('CMAKE_CACHE_FILE', None))

vv.setDefault('GIT', '/usr/bin/git')
vv.setDefault('Boost_INCLUDE_DIR', '/usr/include')
vv.setDefault('Boost_PYTHON_LIBRARY_RELEASE', '/usr/lib/libboost_python.so')
vv.setDefault('EIGEN3_INCLUDE_DIR', '/usr/include')
vv.setDefault('OpenMP_CXX_FLAGS', '-fopenmp')
vv.setDefault('CXX11_STD_FLAGS', '-std=c++11')

CC = ''
try:
    CC = vv.get('CMAKE_C_COMPILER')
except KeyError:
    pass
if CC:
    envCC = os.environ.get('CC', '')
    if envCC and envCC != CC:
        # different compilers specified.
        print("WARNING: different compilers set in environment ("+envCC+") and in CMake "
              "cache file ("+CC+"); the former will be used.")


#print("VARIABLE CACHE: ")
#for (k,v) in vv.d.items():
#    print("    "+k+"="+v)


# figure out version info
version = None
if os.path.exists(os.path.join(thisdir, '..', 'VERSION')):
    with open('../VERSION') as f:
        version = f.read().strip()
try:
    version = subprocess.check_output([vv.get('GIT'), 'describe', '--tags', 'HEAD']).strip()
except Exception as e:
    print("ERROR: Can't retrieve the current code version.")
    raise


def libbasename(x):
    fn = os.path.splitext(os.path.basename(x))[0]
    if fn[:3] == 'lib':
        return fn[3:]
    return fn

include_dirs = [
    vv.get("Boost_INCLUDE_DIR"),
    vv.get("EIGEN3_INCLUDE_DIR"),
    os.path.join(thisdir, ".."), # tomographer
    os.path.join(thisdir, "cxx"), # tomographerpy
]
libraries = [
    libbasename(vv.get("Boost_PYTHON_LIBRARY_RELEASE"))
]
library_dirs = [
    os.path.dirname(vv.get("Boost_PYTHON_LIBRARY_RELEASE"))
]
cflags = [
    vv.get('CXX11_STD_FLAGS'),
    vv.get("OpenMP_CXX_FLAGS"),
    '-DTOMOGRAPHER_VERSION=\"{}\"'.format(version.decode("utf-8")),
]
ldflags = [
    vv.get("OpenMP_CXX_FLAGS"),
]

files = [ os.path.join('cxx', x) for x in [
    "tomographerbase.cxx",
    "eigpyconv.cxx",
    "pyhistogram.cxx",
    "pylogger.cxx",
    "pymultiproc.cxx",
    "pytomorun.cxx",
] ]

dep_headers = [ os.path.join('cxx', 'tomographerpy', x) for x in [
    "eigpyconv.h",
    "pyhistogram.h",
    "pymultiproc.h",
    "pylogger.h",
    "common.h"
] ]

setup(name="tomographer",
      version=version,
      description='Tomographer Python Interface',
      author='Philippe Faist',
      author_email='phfaist@caltech.edu',
      url='https://github.com/Tomographer/tomographer/',
      ext_modules=[
          Extension("tomographer",
                    files,
                    library_dirs=library_dirs,
                    libraries=libraries,
                    include_dirs=include_dirs,
                    extra_compile_args=cflags,
                    extra_link_args=ldflags,
                    depends=dep_headers),
      ]
)
