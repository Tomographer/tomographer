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


# where to find stuff on my system
C_COMPILER = '/usr/bin/gcc-6'
CXX_COMPILER = '/usr/bin/g++-6'
EIGEN3_INCLUDE = '/opt/local/include/eigen3'
MATIO_INCLUDE = '/opt/local/include'
MATIO_LIB = '/opt/local/lib/libmatio.a'
ZLIB_LIB = '/usr/lib/x86_64-linux-gnu/libz.a'
Boost_PROGRAM_OPTIONS_LIB = '/usr/lib/x86_64-linux-gnu/libboost_program_options.a'



if (len(sys.argv) != 2):
    print("Usage: %s <version>\n(where <version> is a git version tag)\n"%(sys.argv[0]))
    sys.exit(1)

gitversion = sys.argv[1]
tomo_name_w_ver = "tomographer-tomorun-"+gitversion
install_name = tomo_name_w_ver+'-linux'
tomo_name_w_ver_a = {
    'tar.gz': install_name + ".tar.gz",
    }

if os.path.exists(tomo_name_w_ver) or any((os.path.exists(n) for n in tomo_name_w_ver_a.values())):
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
e.tar = os.environ.get('TAR', "tar")
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
        # compilers
        '-DCMAKE_C_COMPILER='+C_COMPILER,
        '-DCMAKE_CXX_COMPILER='+CXX_COMPILER,

        # build type
        '-DCMAKE_BUILD_TYPE=Release',

        # 3rd party libraries
        '-DEIGEN3_INCLUDE_DIR='+EIGEN3_INCLUDE,
        '-DMATIO_INCLUDE_DIR='+MATIO_INCLUDE,
        '-DMATIO_LIBRARY='+MATIO_LIB,

        #'-DMATIO_LIBRARY_RELEASE='+MATIO_LIB,
        '-DZLIB_LIBRARY='+ZLIB_LIB,
        '-DZLIB_LIBRARY_RELEASE='+ZLIB_LIB,
        '-DBoost_PROGRAM_OPTIONS_LIBRARY='+Boost_PROGRAM_OPTIONS_LIB,
        '-DBoost_PROGRAM_OPTIONS_LIBRARY_RELEASE='+Boost_PROGRAM_OPTIONS_LIB,

        # Tomorun: use OpenMP not C++ threads
        #'-DTOMORUN_MULTIPROC=openmp',
        '-DTOMORUN_MULTIPROC=cxxthreads',

        # optimizations & architecture: don't include too many optimizations, so that the
        # binary can run on other machines.
        '-DTARGET_ARCHITECTURE=none',
        #'-DUSE_SSE2=on', # but enable sse2 which is available virtually everywhere ## DOESN'T WORK -- WHY??

        # C++ compiler flags
        '-DCMAKE_CXX_FLAGS_RELEASE=-O3 -msse2',

        # linker flags: RPath stuff etc. ### THESE ARE NOT NECESSARY, WE OVERRIDE THE LINK COMMAND ANYWAY....
#        '-DCMAKE_SKIP_BUILD_RPATH=true',
#        '-DCMAKE_BUILD_WITH_INSTALL_RPATH=false',
#        '-DCMAKE_INSTALL_RPATH=\$ORIGIN/../lib',

        # Finally, our install prefix for packaging,
        '-DCMAKE_INSTALL_PREFIX='+fullinstallpath
        ], cwd=fullbuildpath)

# compile. Since there is only 'tomorun' to make, no need for -j<#CPUs>
do_run([e.make, 'VERBOSE=1'],
       cwd=fullbuildpath)

do_run([CXX_COMPILER,
        "-O3",
        "-static", "-static-libgcc", "-static-libstdc++",
        "-fopenmp",
        "CMakeFiles/tomorun.dir/tomorun.cxx.o",
        "-o", "tomorun",
        MATIO_LIB,
        ZLIB_LIB,
        Boost_PROGRAM_OPTIONS_LIB],
       cwd=os.path.join(fullbuildpath, 'tomorun')
       );


# install/strip
do_run([e.make, 'install/strip'],
       cwd=fullbuildpath)

# package
do_run([e.tar, "cvfz", os.path.join(fullcwd, tomo_name_w_ver_a['tar.gz']), install_name],
       cwd=fullbuildpath)


#do_rmtree(tomo_name_w_ver)

print("Done.")