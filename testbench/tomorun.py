
from __future__ import print_function

import os
import os.path
import subprocess

from tomographerbuild import TomographerBuild, TomographerTomorunReleaseExe, timeit_call


cmake_release_opts = [
    '-DCMAKE_BUILD_TYPE=Release',
    '-DTARGET_ARCHITECTURE=auto',
    '-DCMAKE_CXX_FLAGS_RELEASE=-O3 -g -UNDEBUG',
]

build_v1_0_optim_release = TomographerBuild('v1.0', cmake_release_opts + [
    '-DCMAKE_C_COMPILER=/usr/local/opt/llvm/bin/clang',
    '-DCMAKE_CXX_COMPILER=/usr/local/opt/llvm/bin/clang++',
    # for openmp:
    '-DCMAKE_EXE_LINKER_FLAGS=-L/usr/lib -L/usr/local/opt/llvm/lib',
    ], add_environ={'LDFLAGS': '-L/usr/lib -L/usr/local/opt/llvm/lib'})

build_v4_1_optim_release = TomographerBuild('v4.1', cmake_release_opts + [
    '-DCMAKE_CXX_COMPILER=/usr/bin/clang',
    '-DCMAKE_CXX_COMPILER=/usr/bin/clang++',
    ])

build_g287a5ae_optim_release = TomographerBuild('v5.0aaa-15-g287a5ae', cmake_release_opts + [
    '-DCMAKE_CXX_COMPILER=/usr/bin/clang',
    '-DCMAKE_CXX_COMPILER=/usr/bin/clang++',
    ])

build_g287a5ae_GCC6_optim_release = TomographerBuild('v5.0aaa-15-g287a5ae', cmake_release_opts + [
    '-DCMAKE_C_COMPILER=/usr/local/opt/gcc/bin/gcc-6',
    '-DCMAKE_CXX_COMPILER=/usr/local/opt/gcc/bin/g++-6',
    '-DBoost_INCLUDE_DIR=/opt/boost-1_64_0-gcc6/include',
    '-DBoost_PROGRAM_OPTIONS_LIBRARY_RELEASE=/opt/boost-1_64_0-gcc6/lib/libboost_program_options.a',
    '-DTOMORUN_MULTIPROC=openmp',
    ])

# includes some performance enhancements to Tomographer::Logger ?
build_g18a0196_GCC6_optim_release_noasserts_tomorunconfig = TomographerBuild('v5.0-10-g18a0196', [
    # cmake_release_opts, but:
    '-DCMAKE_BUILD_TYPE=Release',
    '-DTARGET_ARCHITECTURE=auto',
    '-DCMAKE_CXX_FLAGS_RELEASE=-Ofast -DNDEBUG', # no assertions
    '-DTOMORUN_CXX_FLAGS=-DTOMORUN_MAX_LOG_LEVEL=INFO;-DTOMORUN_CUSTOM_FIXED_DIM=4;-DTOMORUN_CUSTOM_FIXED_MAX_DIM=4;-DTOMORUN_CUSTOM_MAX_POVM_EFFECTS=36',
    # ...
    '-DCMAKE_C_COMPILER=/usr/local/opt/gcc/bin/gcc-6',
    '-DCMAKE_CXX_COMPILER=/usr/local/opt/gcc/bin/g++-6',
    '-DBoost_INCLUDE_DIR=/opt/boost-1_64_0-gcc6/include',
    '-DBoost_PROGRAM_OPTIONS_LIBRARY_RELEASE=/opt/boost-1_64_0-gcc6/lib/libboost_program_options.a',
    '-DTOMORUN_MULTIPROC=openmp',
    ])
