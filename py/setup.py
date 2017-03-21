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

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext

import codecs
import os
import os.path
import sys
import subprocess # for git
import re
import glob
import shutil


#
# POSSIBLE ENVIRONMENT VARIABLES WHICH CAN BE SET: -- doc printed on standard output anyway
#
# handled natively by setup.py:
# CC=/path/to/compiler/gcc
#
# handled by us: (can be found in CMake cache)
# GIT=/path/to/git
# Boost_INCLUDE_DIR=/path/to/boost/headers
# EIGEN3_INCLUDE_DIR=/path/to/eigen/headers
# PYBIND11_CPP_STANDARD=-std=c++14
#
# handled by us (not in CMake cache):
# BCP=/path/to/bcp  (Boost packaging tool)
# ARCHITECTURE_FLAGS=-march=native -O3  (compiler SIMD instruction sets & other optimizations)
#
# To read the above variables from cache:
# CMAKE_CACHE_FILE=path/to/CMakeCache.txt
#




thisdir = os.path.dirname(os.path.realpath(os.path.abspath(__file__)))
print("Tomographer py dir = {}; cwd = {}".format(thisdir, os.getcwd()))



sys.path.insert(0, os.path.join(thisdir, 'tomographer'))
# tomographer.include:
from include import find_include_dir, find_lib, Vars, ensure_str   # beurk!!

            



cmake_cache_file = os.environ.get('CMAKE_CACHE_FILE', None)
vv = Vars([
    'GIT',
    'BCP',
    'Boost_INCLUDE_DIR',
    'EIGEN3_INCLUDE_DIR',
    'PYBIND11_CPP_STANDARD',
    'ARCHITECTURE_FLAGS',
], cachefile=cmake_cache_file)

# Defaults: programs (git, bcp)
vv.setDefault('GIT', lambda : find_executable('git'))
vv.setDefault('BCP', lambda : find_executable('bcp'))

# Defaults: flags
vv.setDefault('ARCHITECTURE_FLAGS', '-march=native -O3')

# Defaults: Boost stuff
vv.setDefault('Boost_INCLUDE_DIR',
              lambda : find_include_dir('boost', pkgnames=['boost']))

# Defaults: Eigen3
vv.setDefault('EIGEN3_INCLUDE_DIR',
              lambda : find_include_dir('eigen3', pkgnames=['eigen','eigen3'],
                                        return_with_suffix='eigen3'))




#
# PREPARE THE PYTHON SOURCES
#

import setup_sources

IsTomographerSources = setup_sources.setup_sources(thisdir, vv)

print("IsTomographerSources = {!r}".format(IsTomographerSources))

#
# Read version info
#
with open(os.path.join(thisdir, 'VERSION')) as f:
    version = ensure_str(f.read()).strip()

(version_maj, version_min, version_for_pip) = setup_sources.get_version_info(version)



#
# Tell the user everything.
#

print("""
  Welcome to the setup.py script for Tomographer {version} ({version_for_pip}).
""".format(version=version, version_for_pip=version_for_pip))

print("""\
  The `tomographer` python package requires some external C++ libraries. You may
  need to specify their location with the use of environment variables. Current
  values (detected or specified manually) are:

{}""".format("\n".join([ "    {}={}".format(k,v) for k,v in vv.d.items() ])))

if IsTomographerSources and not cmake_cache_file:
    print("""
  You may also read variables from a CMakeCache.txt file with
  CMAKE_CACHE_FILE=path/to/CMakeCache.txt
""")
else:
    print("""
  (read cache file {})
""".format(cmake_cache_file))


if IsTomographerSources and sys.platform == 'darwin':
    print("""\
  To get started on Mac OS X with homebrew, you may run for instance:

    > brew install eigen3 boost pybind11
""")


#
# Set up the compilation options
#

glob_cflags = [
    # no longer needed -- now we create tomographer_version.h
    # '-DTOMOGRAPHER_VERSION=\"{}\"'.format(version),
    # '-DTOMOGRAPHER_VERSION_MAJ={}'.format(version_maj),
    # '-DTOMOGRAPHER_VERSION_MIN={}'.format(version_min),
    '-DEIGEN_DONT_PARALLELIZE',
    '-UNDEBUG',
]

cxxfiles = [ os.path.join('cxx', x) for x in [
    "tomographerpy.cxx",
    "pyhistogram.cxx",
    "pymultiproc.cxx",
    "pymhrwtasks.cxx",
    "pymhrw.cxx",
    "pydensedm.cxx",
    "pytomorun.cxx",
] ]

headers = [ os.path.join('tomographer/include', 'tomographerpy', x) for x in [
    "pyhistogram.h",
    "pymultiproc.h",
    "pymhrwtasks.h",
    "pymhrw.h",
    "pylogger.h",
    "pydensedm.h",
    "common.h"
] ]



#
# Some code taken from https://github.com/pybind/python_example
#

class get_pybind_include(object):
    """Helper class to determine the pybind11 include path
    The purpose of this class is to postpone importing pybind11
    until it is actually installed, so that the ``get_include()``
    method can be invoked. """

    def __init__(self, user=False):
        self.user = user

    def __str__(self):
        import pybind11
        return pybind11.get_include(self.user)


class get_numpy_include(object):
    """Same for numpy."""

    def __str__(self):
        import numpy
        return numpy.get_include()


# As of Python 3.6, CCompiler has a `has_flag` method.
# cf http://bugs.python.org/issue26689
def has_flag(compiler, flagname):
    """Return a boolean indicating whether a flag name is supported on
    the specified compiler.
    """
    import tempfile
    with tempfile.NamedTemporaryFile('w', suffix='.cpp') as f:
        f.write('int main (int argc, char **argv) { return 0; }')
        try:
            compiler.compile([f.name], extra_postargs=[flagname])
        except setuptools.distutils.errors.CompileError:
            return False
    return True


def cpp_flag(compiler):
    """Return the -std=c++[11/14] compiler flag.
    The c++14 is prefered over c++11 (when it is available).
    """
    if has_flag(compiler, '-std=c++14'):
        return '-std=c++14'
    elif has_flag(compiler, '-std=c++11'):
        return '-std=c++11'
    else:
        raise RuntimeError('Unsupported compiler -- at least C++11 support '
                           'is needed!')


class BuildExt(build_ext):
    """A custom build extension for adding compiler-specific options."""
    c_opts = {
        'msvc': ['/EHsc'],
        'unix': [],
    }

    if sys.platform == 'darwin':
        c_opts['unix'] += ['-stdlib=libc++', '-mmacosx-version-min=10.7']

    def build_extensions(self):
        ct = self.compiler.compiler_type
        opts = self.c_opts.get(ct, [])

        stdcpp = vv.get('PYBIND11_CPP_STANDARD')
        if stdcpp:
            opts.append(stdcpp)
        else:
            opts.append(cpp_flag(self.compiler))

        #if has_flag(self.compiler, '-fvisibility=hidden'):
        #    opts.append('-fvisibility=hidden')

        for cflg in glob_cflags:
            opts.append(cflg)

        for cflg in vv.get('ARCHITECTURE_FLAGS').split():
            opts.append(cflg)

        # create a header so that the C++ files have access to some setup.py info
        if not os.path.exists(os.path.join(thisdir, 'tmp')): os.mkdir(os.path.join(thisdir,'tmp'))
        with open(os.path.join(thisdir,'tmp','tomographerpy_compileinfo.h'), 'w') as f:
            f.write("""
#pragma once
#include <vector>
#include <string>

static inline std::vector<std::string> tomographerpy_compileinfo_get_cflags() {
    return { %(cflagslist)s } ;
}
""" % {
    'cflagslist': ", ".join([ 'std::string("' + "".join([ '\\x%02x'%(x if isinstance(x,int) else ord(x)) for x in ensure_str(opt).encode('utf-8') ]) + '")'
                              for opt in opts])
}
            )
            opts.append("-DTOMOGRAPHERPY_HAVE_COMPILEINFO")
            opts.append("-I"+os.path.join(thisdir,'tmp'))

        # don't include VERSION_INFO in the compileinfo header file
        if ct == 'unix':
            opts.append('-DVERSION_INFO="%s"' % self.distribution.get_version())
        elif ct == 'msvc':
            opts.append('/DVERSION_INFO=\\"%s\\"' % self.distribution.get_version())

        for ext in self.extensions:
            ext.extra_compile_args = opts

        build_ext.build_extensions(self)


#def make_glob_patterns(rootdir, prefixdir, patterns):
#    l = []
#    for root, dirnames, filenames in os.walk(os.path.join(rootdir, prefixdir)):
#        l += [ os.path.join(os.path.relpath(root, rootdir), d, p)
#               for p in patterns
#               for d in dirnames ]
#    return l

def find_all_files(dirname, pred, prefixfn=lambda root, filename: os.path.join(root, filename)):
    matches = []
    for root, dirnames, filenames in os.walk(dirname):
        matches += [ prefixfn(root, filename) for filename in filenames
                     if pred(filename) ]
    return matches


target_tomographer_include = os.path.join(thisdir, 'tomographer', 'include')
target_tomographer_include_deps = os.path.join(target_tomographer_include, 'deps')

tomographer_include_files = (
#    make_glob_patterns(target_tomographer_include, 'tomographerpy', ['*.h']) +
#    make_glob_patterns(target_tomographer_include, 'tomographer', ['*.h']) +
#    make_glob_patterns(target_tomographer_include, 'deps/boost', ['*']) +
#    make_glob_patterns(target_tomographer_include, 'deps/eigen3', ['*'])
    find_all_files(os.path.join(target_tomographer_include, 'tomographerpy'),
                   pred=lambda fn: fn.endswith('.h'),
                   prefixfn=lambda root, fn: os.path.relpath(os.path.join(root, fn), target_tomographer_include)) +
    find_all_files(os.path.join(target_tomographer_include, 'tomographer'),
                   pred=lambda fn: True,
                   prefixfn=lambda root, fn: os.path.relpath(os.path.join(root, fn), target_tomographer_include)) +
    find_all_files(os.path.join(target_tomographer_include_deps, 'boost', 'boost'),
                   pred=lambda fn: True,
                   prefixfn=lambda root, fn: os.path.relpath(os.path.join(root, fn), target_tomographer_include)) +
    find_all_files(os.path.join(target_tomographer_include_deps, 'eigen3'),
                   pred=lambda fn: True,
                   prefixfn=lambda root, fn: os.path.relpath(os.path.join(root, fn), target_tomographer_include))

)

#print("ALL INCLUDE FILES = {}".format(tomographer_include_files))


def readfile(x):
    with open(os.path.join(thisdir, x)) as f:
        return f.read()

#
# Set up the tomographer python package
#

setup(name="tomographer",
      version=version_for_pip,
      description=u'The Tomographer Project \u2014 Practical, Reliable Error Bars in Quantum Tomography',
      long_description=readfile('README.rst'),
      author='Philippe Faist',
      author_email='phfaist@caltech.edu',
      url='https://github.com/Tomographer/tomographer/',
      download_url='https://github.com/Tomographer/tomographer/releases/download/{version}/tomographer-{version}.tar.gz'.format(version=version),
      license='MIT',
      keywords='quantum tomography error bars',
      classifiers=[
          'Development Status :: 5 - Production/Stable',
          'Intended Audience :: Science/Research',
          'License :: OSI Approved :: MIT License',
          'Programming Language :: C++',
          'Programming Language :: Python',
          'Topic :: Scientific/Engineering :: Physics',
      ],
      packages=[
          'tomographer',
          'tomographer.tools',
          'tomographer.tools.densedm',
          'tomographer.include',
      ],
      ext_modules=[
          Extension(
              '_tomographer_cxx',
              sources=cxxfiles,
              include_dirs=[
                  # Path to pybind11 headers
                  get_pybind_include(),
                  get_pybind_include(user=True),
                  get_numpy_include(),
                  os.path.join(target_tomographer_include), # tomographer & tomographerpy
                  os.path.join(target_tomographer_include_deps, 'boost'),
                  os.path.join(target_tomographer_include_deps, 'eigen3'),
              ],
              depends=headers,
              language='c++',
          ),
      ],
      # package data - headers
      package_data={
          'tomographer.include': tomographer_include_files,
      },
      #include_package_data=True,
      install_requires=['numpy>=1.8', 'pybind11>=2.0'],
      cmdclass={ 'build_ext': BuildExt },
      zip_safe=False,
)
