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
from distutils.version import LooseVersion

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
import shlex

# build-time requirements -- Apparently, PIP cannot install these automatically -- so make
# sure we get a hard failure if they aren't present
import numpy
import pybind11

def require_mod_version(mod, modver, minver, fix=None):
    if LooseVersion(modver) < LooseVersion(minver):
        msg = "Minimum required {mod} version is {minver}".format(mod=mod,minver=minver)
        if fix:
            msg += "; try {}".format(fix)
        raise RuntimeError(msg)


# Not too old NumPy
require_mod_version("NumPy", numpy.__version__, "1.8",
                    fix='running "pip install numpy --upgrade"')

# Very recent Pybind11, need >= 2.1 for automatic py::metaclass()
require_mod_version("PyBind11", pybind11.__version__, "2.1",
                    fix='running "pip install pybind11 --upgrade"')



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
# OPTIMIZATION_CXX_FLAGS=-UNDEBUG -march=native -O3
#                        (compiler SIMD instruction sets & other optimizations)
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
    'OPTIMIZATION_CXX_FLAGS',
#    'MACOSX_CXX_FLAGS',
], cachefile=cmake_cache_file)

# Defaults: programs (git, bcp)
vv.setDefault('GIT', lambda : find_executable('git'))
vv.setDefault('BCP', lambda : find_executable('bcp'))

# Defaults: flags
#
# DO NOT USE -fvisibility=hidden OR -flto. (Event with visibility attributes set I
# couldn't get other modules which use the python tomographer API to load properly)
vv.setDefault('OPTIMIZATION_CXX_FLAGS', '-UNDEBUG -march=native -O3')

# Defaults: Boost stuff
vv.setDefault('Boost_INCLUDE_DIR',
              lambda : find_include_dir('boost', pkgnames=['boost']))

# Defaults: Eigen3
vv.setDefault('EIGEN3_INCLUDE_DIR',
              lambda : find_include_dir('eigen3', pkgnames=['eigen','eigen3'],
                                        return_with_suffix='eigen3'))

# Defaults: Mac OS X C++ flags
#
# ### Wait... since each client compiles the module locally, there is no need for these.
#vv.setDefault('MACOSX_CXX_FLAGS', '-stdlib=libc++ -mmacosx-version-min=10.7')



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

pybind_includes = [
    pybind11.get_include(False),
    pybind11.get_include(True),
]
numpy_includes = [
    numpy.get_include()
]




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


def escape_c_utf8_str(s):
    _rx_notok = re.compile(r"""[^a-zA-Z_0-9.-/<>?~!@#$%^&*\[\]()_+={};':" -]""")
    def esc_char(x):
        if not isinstance(x, int):
            x = ord(x)
        if not _rx_notok.match(chr(x)):
            return chr(x)
        return '\\x%02x'%(x)
    return ('"' +
            "".join([ esc_char(x) for x in ensure_str(s).encode('utf-8') ])
            + '"')

class BuildExt(build_ext):
    """A custom build extension for adding compiler-specific options."""
    c_opts = {
        'msvc': ['/EHsc'],
        'unix': [],
    }

    #if sys.platform == 'darwin':
    #    c_opts['unix'] += shlex.split(vv.get('MACOSX_CXX_FLAGS'))

    def build_extensions(self):
        ct = self.compiler.compiler_type
        opts = self.c_opts.get(ct, [])

        def defpp(sym, val=None):
            s = sym
            if val is not None:
                s += '='+str(val)
            if ct == 'msvc':
                return '/D'+s
            # -DSYMBOL=Value
            return '-D'+s

        stdcpp = vv.get('PYBIND11_CPP_STANDARD')
        if stdcpp:
            opts.append(stdcpp)
        else:
            opts.append(cpp_flag(self.compiler))

        for cflg in glob_cflags:
            opts.append(cflg)

        for cflg in shlex.split(vv.get('OPTIMIZATION_CXX_FLAGS')):
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

static inline py::dict tomographerpy_compileinfo_get_compiler() {
    py::dict d;
    d["type"_s] = std::string(%(compiler_type)s);
    d["path"_s] = std::string(%(compiler_path)s);
    d["link_path"_s] = std::string(%(compiler_link_path)s);
    return d;
}
""" % {
    'cflagslist': ", ".join([ 
        'std::string(' + escape_c_utf8_str(opt) + ')'
        for opt in opts
    ]),
    'compiler_type': escape_c_utf8_str(self.compiler.compiler_type),
    'compiler_path': escape_c_utf8_str(self.compiler.compiler_so[0]),
    'compiler_link_path': escape_c_utf8_str(self.compiler.linker_so[0]),
}
            )
            opts.append(defpp("TOMOGRAPHERPY_HAVE_COMPILEINFO"))
            opts.append("-I"+os.path.join(thisdir,'tmp'))

        # don't include VERSION_INFO in the compileinfo header file
        opts.append(defpp('VERSION_INFO', '"%s"'%self.distribution.get_version()))
        opts.append(defpp('_tomographer_cxx_EXPORTS'))

        for ext in self.extensions:
            ext.extra_compile_args = opts

        build_ext.build_extensions(self)


def find_all_files(dirname, pred, prefixfn=lambda root, filename: os.path.join(root, filename)):
    matches = []
    for root, dirnames, filenames in os.walk(dirname):
        matches += [ prefixfn(root, filename) for filename in filenames
                     if pred(filename) ]
    return matches


target_tomographer_include = os.path.join(thisdir, 'tomographer', 'include')
target_tomographer_include_deps = os.path.join(target_tomographer_include, 'deps')

tomographer_include_files = (
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
# Build & installation requirements
#
REQUIREMENTS = [
    # recent PIP is required (get errors otherwise)
    'pip >= 7.1',
    'numpy >= 1.8',
    'pybind11 >= 2.1',
],


#
# Set up the tomographer python package
#

url = 'https://github.com/Tomographer/tomographer/'
download_url = ('https://github.com/Tomographer/tomographer/releases/download/'
                '{version}/tomographer-{version}.tar.gz').format(version=version)

setup(
    #
    # Essential meta-information
    #
    name="tomographer",
    version=version_for_pip,
    description=u'The Tomographer Project \u2014 Practical, Reliable Error Bars in Quantum Tomography',
    long_description=readfile('README.rst'),
    author='Philippe Faist',
    author_email='phfaist@caltech.edu',
    license='MIT',
    url=url,
    download_url=download_url,
    keywords='quantum tomography error bars',
    classifiers=[
        'Development Status :: 5 - Production/Stable',
        'Intended Audience :: Science/Research',
        'License :: OSI Approved :: MIT License',
        'Programming Language :: C++',
        'Programming Language :: Python',
        'Topic :: Scientific/Engineering :: Physics',
    ],

    #
    # Included pure-Python tomographer packages
    #
    packages=[
        'tomographer',
        'tomographer.tools',
        'tomographer.tools.densedm',
        'tomographer.include',
    ],

    #
    # Our C++/PyBind11 extension module.
    #
    ext_modules=[
        Extension(
            '_tomographer_cxx',
            sources=cxxfiles,
            include_dirs=(
                # Path to pybind11 headers
                pybind_includes + 
                numpy_includes +
                # our own internal header dependencies
                [
                os.path.join(target_tomographer_include), # tomographer & tomographerpy
                os.path.join(target_tomographer_include_deps, 'boost'),
                os.path.join(target_tomographer_include_deps, 'eigen3'),
                ]),
            depends=headers,
            language='c++',
        ),
    ],
    
    #
    # package data - include our headers inside the package, so that other extensions
    # can depend easily on the tomographer header library (along with Eigen & Boost
    # dependencies)
    #
    package_data={
        'tomographer.include': tomographer_include_files,
    },
    
    #
    # Installation requirements
    #
    install_requires = REQUIREMENTS,
    #
    # Beware of setup_requires --
    # https://pip.pypa.io/en/stable/reference/pip_install/#controlling-setup-requires
    #
    # But setup_requires would apparently be the only solution for us, as PIP won't
    # install the install_requires dependencies before building the wheel :( -- but still
    # doesn't work, not sure why :( :(  -- Arrhgggh!!!
    #
    # Just document dependencies.  Apparently PIP doesn't support doing this
    # automatically.
    #
    #setup_requires = REQUIREMENTS,
    
    #
    # Our customized build procedure
    #
    cmdclass={ 'build_ext': BuildExt },
    
    #
    # Not safe for keeping in a ZIP file (because of our extension)
    #
    zip_safe=False,

)
