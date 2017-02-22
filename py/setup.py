# This file is part of the Tomographer project, which is distributed under the
# terms of the MIT license.
#
# The MIT License (MIT)
#
# Copyright (c) 2016 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
# Copyright (c) 2017 Caltech, Institute for Quantum Information and Matter, Philippe Faist
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.


from distutils.spawn import find_executable

from setuptools import setup
from setuptools.extension import Extension

import codecs
import os.path
import sys
import subprocess # for git
import re

import numpy # numpy.get_include()


#
# POSSIBLE ENVIRONMENT VARIABLES WHICH CAN BE SET: -- doc printed on standard output anyway
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
# CMAKE_CXX11_STANDARD_COMPILE_OPTION=-std=c++11
#
# To read the above variables from cache:
# CMAKE_CACHE_FILE=path/to/CMakeCache.txt
#





thisdir = os.path.dirname(__file__)
#print("This dir = {}".format(thisdir))




def find_include_dir(hname, pkgname, testfn=os.path.isdir, return_with_suffix=None):
    guesses = [
        # homebrew version
        os.path.join('/usr/local/opt', pkgname, 'include'),
        # default paths
        '/usr/local/include',
        '/usr/include',
        '/opt/include',
        os.path.expanduser('~/.local/include'),
    ]
    for i in guesses:
        dn = os.path.join(i, hname)
        if testfn(dn): # found
            if return_with_suffix:
                return os.path.join(i, return_with_suffix)
            return i

    return None

def find_lib(libname, pkgname, libnamesuffixes=[]):
    guesses = [
        # Homebrew version
        os.path.join('/usr/local/opt', pkgname, 'lib'),
        # default paths
        '/usr/local/lib',
        '/usr/lib',
        '/opt/lib',
        '/usr/lib{suffix}/x86_64-linux-gnu',
        os.path.expanduser('~/.local/lib'),
    ]
    is_64bits = sys.maxsize > 2**32 # see https://docs.python.org/2/library/platform.html
    libdirsuffixes = ['', '64' if is_64bits else '32']
    for i in guesses:
        for s in libdirsuffixes:
            if '{suffix' not in i:
                i += '{suffix}'
            #print("find_lib:TRYING i={!r}".format(i))
            basedn = i.format(suffix=s)
            #
            for lnp in ['lib', '']: # lib-name-prefixes
                for lns in libnamesuffixes + ['']: # lib-name-suffixes
                    for fns in ['.so', '.dylib', '.dll', '.a']: # file name suffixes for library
                        dn = os.path.join(basedn+s, lnp+libname+lns+fns)
                        #print("find_lib:TRYING dn={!r}".format(dn))
                        if os.path.isfile(dn):
                            return dn
    return None
            

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
    def __init__(self, cachefile=None, vars=[]):
        # self.d: values read & cached
        self.d = {}

        dc = {}
        if cachefile:
            dc = self._readcache(cachefile)

        for k in vars:
            if k in os.environ:
                self.d[k] = os.environ.get(k)
            elif k in dc:
                self.d[k] = ensure_str(dc[k])
            else:
                self.d[k] = None

    def setDefault(self, k, defvalue):
        if k not in self.d or self.d.get(k, None) is None:
            if callable(defvalue):
                self.d[k] = defvalue()
            else:
                self.d[k] = defvalue

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
vv = Vars(cmake_cache_file, [
    'GIT',
    'Boost_INCLUDE_DIR',
    'Boost_PYTHON_LIBRARY_RELEASE',
    'EIGEN3_INCLUDE_DIR',
    'OpenMP_CXX_FLAGS',
    'CMAKE_CXX11_STANDARD_COMPILE_OPTION'
])

# Defaults: GIT
vv.setDefault('GIT', lambda : find_executable('git'))

# Defaults: Boost stuff
vv.setDefault('Boost_INCLUDE_DIR', lambda : find_include_dir('boost', 'boost'))
vv.setDefault('Boost_PYTHON_LIBRARY_RELEASE', lambda :
              find_lib('boost_python', 'boost-python', [ # suffixes:
                  str(sys.version_info.major)+'-mt', str(sys.version_info.major), '-mt',
                  '-py{}{}'.format(sys.version_info.major,sys.version_info.minor),
                  ''
              ]))

# Defaults: Eigen3
vv.setDefault('EIGEN3_INCLUDE_DIR', lambda : find_include_dir('eigen3', 'eigen', return_with_suffix='eigen3'))

# Defaults: OpenMP flags
looks_like_clang = False
if 'clang' in os.environ.get('CC',''):
    looks_like_clang = True
if sys.platform == 'darwin' and not os.environ.get('CC',''):
    looks_like_clang = True
vv.setDefault('OpenMP_CXX_FLAGS', lambda : '-fopenmp=libomp' if looks_like_clang else '-fopenmp')

# Defaults: C++11 flags
vv.setDefault('CMAKE_CXX11_STANDARD_COMPILE_OPTION', '-std=c++11')



#
# Check which compiler is used, and warn user if a different one is used than
# recorded in the CMake cache file
#


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
              "cache file ("+CC+"); the former will be used (please set \"CC="+CC+"\" to override).")

CXX = ''
try:
    CXX = vv.get('CMAKE_CXX_COMPILER')
except KeyError:
    pass
if CXX:
    envCXX = os.environ.get('CXX', '')
    if envCXX and envCXX != CXX:
        # different compilers
        print("WARNING: different C++ compilers set in environment ("+envCXX+") and in CMake "
              "cache file ("+CXX+"); the former will be used (please set \"CXX="+CXX+"\" to override).")


#
# Figure out version info
#

version = None
if os.path.exists(os.path.join(thisdir, '..', 'VERSION')):
    with open('../VERSION') as f:
        version = ensure_str(f.read()).strip()
try:
    version = ensure_str(subprocess.check_output([vv.get('GIT'), 'describe', '--tags', 'HEAD'])).strip()
except Exception as e:
    print("ERROR: Can't retrieve the current code version.")
    raise

# major/minor sections of version
m = re.match(r'^v(?P<major>\d+)\.(?P<minor>\d+)', version)
version_maj = int(m.group('major'))
version_min = int(m.group('minor'))

# Normalize version string for PIP/setuptools
pip_version = version
# remove initial 'v' in 'v3.1'
if pip_version[0] == 'v':
    pip_version = pip_version[1:]
# make PEP-440 compatible if it is a specific git-describe commit number
m = re.match(r'^(?P<vtag>.*)-(?P<ncommits>\d+)-(?P<githash>g[a-fA-F0-9]+)$', pip_version)
if m:
    pip_version = "{vtag}+git{ncommits}.{githash}".format(**m.groupdict())




#
# Tell the user everything.
#

print("""
  Welcome to the setup.py script for Tomographer {version} ({pip_version}).
""".format(version=version, pip_version=pip_version))

print("""\
  The `tomographer` python package requires some external C++ libraries and
  compiler features. You may need to specify their location with the use of
  environment variables. Current values (detected or specified manually) are:

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

if sys.platform == 'darwin':
    print("""
  NOTE: Apple's default compiler on Mac OS X does not support OpenMP. Remember
  to install a custom LLVM or GCC if you want to dramatically speed up execution
  time. Specify the path to custom compilers with the environment variables "CC"
  and "CXX". To compile without OpenMP (and run tasks serially), set
  "OpenMP_CXX_FLAGS" to an empty string.


  NOTE: If you're using homebrew, the following commands will get you started
  with homebrew's python3 along with all the required dependencies:

    > brew install llvm eigen python3 boost
    > brew install boost-python --with-python3
    > CC=/usr/local/opt/llvm/bin/clang CXX=/usr/local/opt/llvm/bin/clang++ \\
        LDFLAGS='-L/usr/lib -L/usr/local/opt/llvm/lib' \\
        /usr/local/bin/python3 setup.py install

""")

#print("DEBUG VARIABLE CACHE: ")
#for (k,v) in vv.d.items():
#    print("    "+k+"="+v)





#
# Utilities for passing on the options below
#

def libbasename(x):
    if x is None:
        return None
    fn = os.path.splitext(os.path.basename(x))[0]
    if fn[:3] == 'lib':
        return fn[3:]
    return fn

def dirname_or_none(x):
    if x is None:
        return None
    return os.path.dirname(x)


#
# Set up the compilation options
#


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
    dirname_or_none(vv.get("Boost_PYTHON_LIBRARY_RELEASE"))
]
cflags = [
    vv.get('CMAKE_CXX11_STANDARD_COMPILE_OPTION'),
    vv.get("OpenMP_CXX_FLAGS"),
    '-DTOMOGRAPHER_VERSION=\"{}\"'.format(version),
    '-DTOMOGRAPHER_VERSION_MAJ={}'.format(version_maj),
    '-DTOMOGRAPHER_VERSION_MIN={}'.format(version_min),
]
ldflags = [
    vv.get("OpenMP_CXX_FLAGS"),
]

files = [ os.path.join('cxx', x) for x in [
    "tomographerpy.cxx",
    "eigpyconv.cxx",
    "pyhistogram.cxx",
    #"pylogger.cxx", # no longer used -- all definitions are in pylogger.h
    "pymultiproc.cxx",
    "pymhrwtasks.cxx",
    "pymhrw.cxx",
    "pydensedm.cxx",
    "pytomorun.cxx",
] ]

dep_headers = [ os.path.join('cxx', 'tomographerpy', x) for x in [
    "eigpyconv.h",
    "pyhistogram.h",
    "pymultiproc.h",
    "pymhrwtasks.h",
    "pymhrw.h",
    "pylogger.h",
    "pydensedm.h",
    "common.h"
] ]


#
# Set up the python package
#

setup(name="tomographer",
      version=pip_version,
      description='Tomographer Python Interface',
      author='Philippe Faist',
      author_email='phfaist@caltech.edu',
      url='https://github.com/Tomographer/tomographer/',
      packages=[
          'tomographer',
          'tomographer.tools',
          'tomographer.tools.densedm',
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
      ],
      # install headers in binary distribution
      package_data={
          'tomographer': ['cxx/tomographerpy/*.h'],
      },
      #include_package_data=True,
)
