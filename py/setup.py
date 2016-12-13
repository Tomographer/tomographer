#setup.py

from distutils.core import setup
from distutils.extension import Extension

import codecs
import os.path
import sys
import subprocess # for git
import re

import numpy # numpy.get_include()

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


def ensure_str(s):
    """
    Convert value `s` to a `str`, that is, a byte string on Python 2 and a unicode string
    on Python 3.

    Input is expected to be a byte-string in both Py2 or Py3.
    """
    if not isinstance(s, str):
        return s.decode('utf-8')
    return s


#
# Looks up the variables:
#   - in the environment
#   - in the CMake cache, if any
#   - default value
#
class Vars(object):
    def __init__(self, cachefile=None, defaults={}):
        # self.d: values read & cached
        self.d = {}

        dc = {}
        if cachefile:
            dc = self._readcache(cachefile)

        for k, dfltval in defaults.items():
            if k in os.environ:
                self.d[k] = os.environ.get(k)
            elif k in dc:
                self.d[k] = ensure_str(dc[k])
            else:
                self.d[k] = ensure_str(dfltval)

    def _readcache(self, cachefile):
        dc = {}
        rx = re.compile(r'^(?P<name>[A-Za-z0-9_]+)(:(?P<type>[A-Za-z_]+))=(?P<value>.*)$')
        with open(cachefile) as f:
            for line in f:
                m = rx.match(line)
                if not m:
                    continue
                dc[m.group('name')] = ensure_str(m.group('value'))
        return dc

    def get(self, name):
        return self.d.get(name)


cmake_cache_file = os.environ.get('CMAKE_CACHE_FILE', None)
vv = Vars(cmake_cache_file, {
    'GIT': '/usr/bin/git',
    'Boost_INCLUDE_DIR': '/usr/include',
    'Boost_PYTHON_LIBRARY_RELEASE': '/usr/lib/libboost_python.so',
    'EIGEN3_INCLUDE_DIR': '/usr/include',
    'OpenMP_CXX_FLAGS': '-fopenmp',
    'CXX11_STD_FLAGS': '-std=c++11',
})


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



print("""
  The `tomographer` python package requires some external C++ libraries and
  compiler features. You may need to specify their location with the use of
  environment variables. Current detected values are:

{}""".format("\n".join([ "    {}={}".format(k,v) for k,v in vv.d.items() ])))

if not cmake_cache_file:
    print("""
  You may also read variables from a CMakeCache.txt file with
  CMAKE_CACHE_FILE=path/to/CMakeCache.txt
""")
else:
    print("""
  (read cache file {})
""".format(cmake_cache_file))

#print("VARIABLE CACHE: ")
#for (k,v) in vv.d.items():
#    print("    "+k+"="+v)




# figure out version info
version = None
if os.path.exists(os.path.join(thisdir, '..', 'VERSION')):
    with open('../VERSION') as f:
        version = ensure_str(f.read()).strip()
try:
    version = ensure_str(subprocess.check_output([vv.get('GIT'), 'describe', '--tags', 'HEAD'])).strip()
except Exception as e:
    print("ERROR: Can't retrieve the current code version.")
    raise


def libbasename(x):
    fn = os.path.splitext(os.path.basename(x))[0]
    if fn[:3] == 'lib':
        return fn[3:]
    return fn

include_dirs = [
    numpy.get_include(),
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
    '-DTOMOGRAPHER_VERSION=\"{}\"'.format(version),
]
ldflags = [
    vv.get("OpenMP_CXX_FLAGS"),
]

files = [ os.path.join('cxx', x) for x in [
    "tomographerpy.cxx",
    "eigpyconv.cxx",
    "pyhistogram.cxx",
    "pylogger.cxx",
    "pymultiproc.cxx",
    "pymhrwtasks.cxx",
    "pymhrw.cxx",
    "pytomorun.cxx",
] ]

dep_headers = [ os.path.join('cxx', 'tomographerpy', x) for x in [
    "eigpyconv.h",
    "pyhistogram.h",
    "pymultiproc.h",
    "pymhrwtasks.h",
    "pymhrw.h",
    "pylogger.h",
    "common.h"
] ]

setup(name="tomographer",
      version=version,
      description='Tomographer Python Interface',
      author='Philippe Faist',
      author_email='phfaist@caltech.edu',
      url='https://github.com/Tomographer/tomographer/',
      packages=[
          'tomographer',
      ],
      ext_modules=[
          Extension(name="_tomographer_cxx",
                    sources=files,
                    library_dirs=library_dirs,
                    libraries=libraries,
                    include_dirs=include_dirs,
                    extra_compile_args=cflags,
                    extra_link_args=ldflags,
                    depends=dep_headers),
      ]
)
