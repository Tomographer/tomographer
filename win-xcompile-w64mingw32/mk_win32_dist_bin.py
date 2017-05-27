#!/usr/bin/env python

# This file is part of the Tomographer project, which is distributed under the
# terms of the MIT license.
# 
# The MIT License (MIT)
# 
# Copyright (c) 2016 ETH Zurich, Institute for Theoretical Physics, Philippe
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

from __future__ import print_function

import os
import os.path
import sys
import subprocess
import shutil

sandboxdir = '/media/sf_vmshare/mingw32-w64-sandbox/local/'

# where to find stuff on my system
#C_COMPILER = '/usr/bin/i686-w64-mingw32-gcc'
#CXX_COMPILER = '/usr/bin/i686-w64-mingw32-g++'
EIGEN3_INCLUDE = '/opt/local/include/eigen3'
MATIO_INCLUDE = os.path.join(sandboxdir, 'include')
MATIO_LIB = os.path.join(sandboxdir, 'lib/libmatio.a')
ZLIB_LIB = '/usr/i686-w64-mingw32/lib/libz.a'
Boost_PROGRAM_OPTIONS_LIB = os.path.join(sandboxdir, 'lib/libboost_program_options.a')



copy_libs = [
    '/usr/lib/gcc/i686-w64-mingw32/6.2-win32/libgomp-1.dll',
    '/usr/lib/gcc/i686-w64-mingw32/6.2-win32/libgcc_s_sjlj-1.dll',
    '/usr/lib/gcc/i686-w64-mingw32/6.2-win32/libstdc++-6.dll',
    '/usr/i686-w64-mingw32/lib/libwinpthread-1.dll',
    ]


if (len(sys.argv) != 2):
    print("Usage: %s <version>\n(where <version> is a git version tag)\n"%(sys.argv[0]))
    sys.exit(1)

gitversion = sys.argv[1]
tomo_name_w_ver = "tomographer-tomorun-"+gitversion
install_name = tomo_name_w_ver+'-win-mingw32'
tomo_name_w_ver_a = {
    'zip': install_name + ".zip",
    }

if os.path.exists(tomo_name_w_ver) or any( (os.path.exists(n)
                                            for n in tomo_name_w_ver_a.values()) ):
    print("Error: some files with conflicting names exist, aborting.\n" +
          "Please remove the following files if you wish to proceed:\n" +
          "\t"+tomo_name_w_ver)
    for fn in tomo_name_w_ver_a.values():
        print("\t"+fn)
    print("")
    sys.exit(2)

print("Packaging Tomographer version", gitversion, "as", install_name)


tomographer_url = os.environ.get('TOMOGRAPHER_URL','')
if not tomographer_url:
    tomographer_url = "https://github.com/Tomographer/tomographer.git"


class MyStore: pass
e = MyStore()
e.git = os.environ.get('GIT', "git")
#e.tar = os.environ.get('TAR', "tar")
e.zip = os.environ.get('ZIP', "zip")
e.cmake = os.environ.get('CMAKE', "cmake")
e.make = os.environ.get('MAKE', "make")


def do_run(cmdargv, **opts):
    print("Running %r ..."%(cmdargv))
    subprocess.check_call(cmdargv, **opts)
    print("")

def do_rmtree(path):
    print("Removing %s ..."%(path))
    shutil.rmtree(path)

def do_copy_file(src, dst):
    print("Copying file from %s to %s ..."%(src, dst))
    shutil.copy2(src, dst)


# execute everything with lower priority
os.nice(20)

# git clone
do_run([e.git, "clone", tomographer_url, tomo_name_w_ver])

fullcwd = os.path.realpath(os.getcwd());
fullrootpath = os.path.realpath(os.path.abspath(tomo_name_w_ver));
builddirname = 'build-rel-pack'
fullbuildpath = os.path.join(fullrootpath, builddirname)
fullinstallpath = os.path.join(fullbuildpath,install_name)
print("Full root path for packaging is ", fullrootpath)
print("Full install path is ", fullinstallpath)

# git checkout <CORRECT-VERSION>
do_run([e.git, "checkout", gitversion], cwd=fullrootpath)

# mkdir build-rel-pack
os.mkdir(os.path.join(tomo_name_w_ver, builddirname))

# CMAKE COMMAND HERE
do_run([e.cmake, '..',
        # toolchain/compilers
        '-DCMAKE_TOOLCHAIN_FILE='+os.path.join(fullcwd, 'Toolchain-mingw32.cmake'),
        # build type
        '-DCMAKE_BUILD_TYPE=Release',
        # 3rd party libraries
        '-DEIGEN3_INCLUDE_DIR='+EIGEN3_INCLUDE,
        '-DMATIO_INCLUDE_DIR='+MATIO_INCLUDE,
        '-DMATIO_LIBRARY='+MATIO_LIB,
        #'-DMATIO_LIBRARY_RELEASE='+MATIO_LIB,
        #'-DZLIB_INCLUDE_DIR='+...,
        '-DZLIB_LIBRARY='+ZLIB_LIB,
        '-DZLIB_LIBRARY_RELEASE='+ZLIB_LIB,
        '-DBoost_PROGRAM_OPTIONS_LIBRARY='+Boost_PROGRAM_OPTIONS_LIB,
        '-DBoost_PROGRAM_OPTIONS_LIBRARY_RELEASE='+Boost_PROGRAM_OPTIONS_LIB,
        # Tomorun: use OpenMP not C++ threads
        '-DTOMORUN_MULTIPROC=openmp',
        # optimizations & architecture: don't include too many optimizations, so that the
        # binary can run on other machines.
        '-DTARGET_ARCHITECTURE=core',
        # additional C++ compiler flags
        '-DCMAKE_CXX_FLAGS_RELEASE=-O3 -Wall -Wextra',
        # Linker flags, if any
        # Finally, our install prefix for packaging,
        '-DCMAKE_INSTALL_PREFIX='+fullinstallpath
        ], cwd=fullbuildpath)

# compile. Since there is only 'tomorun' to make, no need for -j<#CPUs>
do_run([e.make, 'VERBOSE=1'],
       cwd=fullbuildpath)

# install/strip
do_run([e.make, 'install/strip'],
       cwd=fullbuildpath)


# copy in the destination dir it all relevant libraries
for libfn in copy_libs:
    do_copy_file(libfn, os.path.join(fullinstallpath, 'bin'))


# package
do_run([e.zip, "-r", os.path.join(fullcwd, tomo_name_w_ver_a['zip']), install_name],
       cwd=fullbuildpath)


#do_rmtree(tomo_name_w_ver)

print("Done.")
