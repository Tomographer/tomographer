
THE TOMOGRAPHER PROJECT
=======================

[![DOI](https://zenodo.org/badge/24211/Tomographer/tomographer.svg)](https://zenodo.org/badge/latestdoi/24211/Tomographer/tomographer)

A toolbox for error analysis in quantum tomography.


Overview
--------

This project comprises three parts:

 * The `tomorun` program — use this to analyze data from a typical tomography
   experiment;

 * The generic, `Tomographer` C++ framework — a flexible set of tools to run
   Metropolis-Hastings random walks, deal with density matrices, and other
   utilities;

 * A python interface — analyze data from a typical tomography experiment in a
   python workflow.

The `tomorun` executable produces a histogram of a figure of merit under the
distribution relevant for a reliable error analysis as described in [Faist &
Renner, Practical and Reliable Error Bars in Quantum Tomography, Phys. Rev.
Lett. 117, 010404 (2016)](https://doi.org/10.1103/PhysRevLett.117.010404)
([arXiv:1509.06763](http://arxiv.org/abs/1509.06763)).  The python interface
provides an interface to the essentially the same functionality as `tomorun`
from python/numpy code (plus the possibility for calculating a custom figure
of merit).

The C++ framework is a set of abstract and generic classes which you can combine
in your preferred way to implement this random walk for even more general
settings.


Download
--------

Tomographer can be downloaded from our releases page:

  https://github.com/Tomographer/tomographer/releases

There are precompiled binary releases for Mac OS X, Linux, and Windows.  If a
binary isn't available for your platform or if it doesn't run, you may have to
compile from source. Don't worry, that's not complicated.  If you haven't
already done so, install all the usual development tools
(`gcc`/`g++`/`make`/etc.) and follow the *Installation* instructions below.

If you compile from source, make sure you download the offical source package
from our releases page (or clone the git repo). If you use github's automatic
`Download ZIP' links from the source code, you'll end up with missing files.

If you wish to contribute to development, don't hesitate to fork the repo on
github and send me pull requests, or to contact me if you have questions.

[![Build Status](https://travis-ci.org/Tomographer/tomographer.svg?branch=master)](https://travis-ci.org/Tomographer/tomographer)


Prerequisites
-------------

To run `tomorun`: If you're lucky and there's a binary release for your system,
you won't need anything else.

To compile `tomorun` from source, you'll need:

  - a recent C++ compiler (g++ ≥ 4.8, Intel ICC ≥ 14, LLVM/Clang++ ≥ 3.3)
  - [CMake ≥ 3.1](http://www.cmake.org/)
  - [Boost libraries ≥ 1.40](http://www.boost.org/)
  - [Eigen3 library ≥ 3.3](http://eigen.tuxfamily.org/)
  - [MatIO library](https://sourceforge.net/projects/matio/)

To install the Python `tomographer` package, you'll need:

  - `pip`, the Python package manager — you probably already have it (≥ 7.1)
  - a recent C++ compiler (g++ ≥ 4.8, Intel ICC ≥ 14, LLVM/Clang++ ≥ 3.3)
  - *Optional: you can also install `tomographer` in
    a [conda](https://www.anaconda.com/) environment, which can
    automatically provide both above requirements*
  
OR: you can compile from source, in which case you'll need:

  - a recent C++ compiler (g++ ≥ 4.8, Intel ICC ≥ 14, LLVM/Clang++ ≥ 3.3)
  - [Boost headers ≥ 1.40](http://www.boost.org/) and
    [BCP](http://www.boost.org/doc/libs/1_65_0/tools/bcp/doc/html/index.html)
  - [PyBind11 ≥ 2.2](http://github.com/pybind/pybind11/)
  - [Python 2.7 or Python 3](http://www.python.org/)
  - [Eigen3 library ≥ 3.3](http://eigen.tuxfamily.org/)

In any case a recent C++ compiler is required as some C++11 features and
elements of its standard library are used.

Tomorun is tested on Linux/Ubuntu, Mac OS X and Windows (MinGW32).  The python
package is tested on Linux and Mac OS X.

Note: OpenMP is now no longer needed if your compiler supports C++ threads
(`std::thread`).  This is the case for many compilers, including Apple's
`clang`.

Tip: On Apple Mac OS X with [homebrew](https://brew.sh), the following commands
will get you started with all the prerequisites for compiling both `tomorun` and
the Python `tomographer` package, using Homebrew's python3.  For python, you
should also consider using [*conda*](https://www.anaconda.com/).

    > brew tap homebrew/science
    > brew install python3 boost boost-bcp eigen libmatio pybind11


Installing and Running `tomorun`
--------------------------------

If you found a binary release for your system, simply unpack it wherever you like.
It is then ready for use—skip down to "Running `tomorun`".


### Compiling Tomorun from source

The configuration, compilation and installation process is done using CMake.
(You'll need CMake ≥ 3.1. Don't worry, it's easy to [install a binary
release](https://cmake.org/download/).)  Download an [official
release](https://github.com/Tomographer/tomographer/releases/) of Tomographer,
unpack it, and enter the unpacked directory.  Then, issue the commands:

    tomographer-X.X> mkdir build
    tomographer-X.X> cd build
    tomographer-X.X/build> cmake ..
    tomographer-X.X/build> make
    tomographer-X.X/build> make install/strip

And you'll have the `tomorun` installed on your system.

You may also run CMake multiple times to adjust all the relevant options.  You
can specify some standard CMake variables, such as `CMAKE_INSTALL_PREFIX`.  If you
installed a recent compiler manually, you'll need to point CMake to that
compiler, e.g. with

    > cmake .. -DCMAKE_C_COMPILER=/path/to/gcc -DCMAKE_CXX_COMPILER=/path/to/g++ 

To specify paths to the Boost, Eigen3 and MatIO libraries, use the CMake
switches:

    -DEIGEN3_INCLUDE_DIR=/path/to/include/eigen3
    -DBOOST_ROOT=/path/to/boost
    -DMATIO_LIBRARY=/path/to/libmatio.a
    -DMATIO_INCLUDE_DIR=/path/to/include

You may of course also alternatively use CMake's graphical interface, CMake-GUI.

Note the compilation step (`make`) is quite computation-heavy because of the
extensive C++11 template metaprogramming. It might take a minute or two to
complete depending on your hardware, and might be pretty greedy on RAM.

*GCC/G++ and RAM usage:*
The heavy template meta-programming can cause GCC/G++ to use a LOT of memory
while compiling.  If your system is limited on memory, you should consider
tuning the [gcc flags](https://gcc.gnu.org/onlinedocs/gcc-3.3/gcc/Optimize-Options.html)
`--param ggc-min-expand` and `--param ggc-min-heapsize`: I have found
`--param ggc-min-expand=10 --param ggc-min-heapsize=32768` to work all right
for 4GB of memory; if you want to be really conservative use
 `--param ggc-min-expand=0 --param ggc-min-heapsize=8192`.  These options
should be specified to CMake as `-DCMAKE_CXX_FLAGS="--param ..."`.


### Running `tomorun`

Check out the [online quick start
page](https://tomographer.github.io/tomographer/get-started/#command-line-tomorun-matlab).
In the binary distributions, `tomorun` is located in the `bin/` subdirectory.
Detailed information about how to use & run `tomorun` is obtained by querying
its help text:

    > tomorun --help

Data is read from a file in MATLAB format (see option `--data-file-name`), and
several options control which figure of merit to calculate as well as the
parameters and behavior of the random walk.

As `tomorun` is running, you may query its progress by hitting CTRL-C (Linux/Mac
OS X). If you want to interrupt `tomorun` and quit the current task, hit CTRL-C
twice in short succession.

Also, it is often more convenient to make `tomorun` read its options from a
configuration file:

    > tomorun --config myconfigfile

An example config file would be:

    # Configuration file for an execution of `tomorun`.  Lines starting with '#'
    # are comments and are ignored.
    
    # The data file which contains the POVM effects and frequencies etc.
    data-file-name=the_data.mat
    
    # Write the histogram to the file named "<myconfigfile>-histogram.csv"
    write-histogram-from-config-file-name = 1

    # Number of random walk instances. Tip: Use all your CPU's.
    n-repeats = 12

    # Step size of the random walk. Adjust so that the acceptance ratio is
    # around 0.25-0.4.
    step-size = 0.01

    # Sweep size. Keep only one in so many samples, to decorrelate them.
    # Choose of the order of 1/<step-size>
    n-sweep = 100

    # Run this many sweeps before recording any samples (to thermalize the
    # random walk)
    n-therm = 1024

    # Run until we have recorded this number of samples. Power of two
    # recommended for binning analysis (the default)
    n-run = 32768

    # Choice of figure of merit. Here for example, tr(rho*rho_ref). If rho_ref
    # is pure, this is exactly the (squared) fidelity to rho_ref. "rho_ref" is
    # the name of a variable defined in 'the_data.mat'
    value-type=obs-value:rho_ref

    # The parameters of the histogram. MIN:MAX/NUM_BINS
    value-hist=0.9:1/50


### MATLAB Tools

Some tools are provided for fitting the resulting histogram to our theoretical
model. These are MATLAB scripts located in the `tools` subdirectory.

For more information on how to run the histogram analyzer/fitter, run:

    >> analyze_tomorun_histogram('Help')


Installing and Using the Python Package
---------------------------------------

For an overview, you may want to check out the [online quick start
page](https://tomographer.github.io/tomographer/get-started/#python-version).

### Install using PIP — the easy way

First, make sure you install the prerequisites.  (You do have to install the
`numpy` and `pybind11` dependencies manually before installing `tomographer`.
This is because `pip` apparently doesn't know how to correctly handle build-time
dependencies.)

    # If using *conda*:
    
    > conda install numpy gcc libgcc
    > conda install -c conda-forge pybind11
    
    # OR: if using *pip* by itself:
    
    > pip install numpy pybind11
    
All `pip install` commands (e.g. `pip install SOMEPACKAGE`) might have to be
prefixed by `sudo -H` (e.g. `sudo -H pip install SOMEPACKAGE`) if you need
administrator priviledges, or you can use the option `--user` to install into a
user directory (e.g. `pip install SOMEPACKAGE --user`).

Now, install `tomographer` itself, using `pip` (this should also work within a
*conda* environment):

    # for both *conda* and *pip* users:
    > pip install tomographer

When installing `tomographer` using `pip`, you don't need the Eigen and Boost
libraries (which are otherwise prerequisites), as they are bundled along with
the `tomographer` PyPI python package.

If the installation fails because of a compilation error, then run `pip` with
the `--verbose` option, and look at which settings are detected by the setup
script for variables such as `CXX_FLAGS`. You may override these using
environment variables, for instance: `CXX_FLAGS="-stdlib=libc++"
OPTIMIZATION_CXX_FLAGS="" pip install tomographer`. Consider submitting
a [bug report](https://github.com/Tomographer/tomographer/issues) if you think
it's an issue.

**Important note for conda users: You must use the same C/C++ compiler as for
the python libraries bundled in conda itself.** On Linux, you should use the
version of gcc which *conda* provides (`conda install gcc`), and which you might
have to specify explicitly:

    > CC=path-to-conda/bin/gcc CXX=path-to-conda/bin/g++ pip install tomographer
    
I'm not too sure of the exact situation with *conda* on Mac OS X. It appears
that *conda*'s python is built using Apple's *clang*, but after some tests it
seems that `tomographer` will also happily compile with *conda*'s `gcc`.


### Install from source — advanced

The python `tomographer` package can be installed by entering the `py/`
directory and running the `setup.py` script as for usual manual installation of
python packages:

    tomographer/py> python setup.py install

If tomographer's C++ library dependencies aren't installed in standard paths,
you may need to specify them directly to the `setup.py` script in the form of
environment variables (they can also be read from a CMake cache file). Check the
output of `setup.py` for more info.  Compilers may be specified using the
environment variables `CC=/path/to/gcc` and `CXX=/path/to/g++`.


### Using the Python Interface

The python interface provides a convenient entry point to the C++ routines, with data
provided as `NumPy` arrays.  There are also some additional tools, for example to simulate
measurement outcomes, or to calculate the maximum likelihood estimate given some data.

A couple examples to get started are provided in the `examples/` subfolder.  We suggest
the use of [jupyter notebooks](http://jupyter.org), as they provide a simple and
convenient environment to run python code interactively.

The full documentation of the Python interface can be generated via Sphinx, and
is available [online
here](https://tomographer.github.io/tomographer/api-doc/current/html/py/).


C++ Framework: API Documentation
--------------------------------

Tomographer, at its core, is a set of C++ template classes which can be put
together in the way that fits your needs.  C++ template metaprogramming is very
advantageous performancewise, as most optimizations and specializations are
performed during compilation and thus avoiding runtime look-ups.

In most cases, the `tomorun` executable or the python interface is probably what
you're looking for.  However, if you would like to consider a more general
setting (such as a different random walk method), then you can do so by reusing
existing code as much as possible.

The API documentation of the Tomographer C++ Framework can be found
[here][tomographer_apidoc].  You can also build the API documentation of the
Tomographer C++ Framework using [Doxygen ≥ 1.8][doxygen]. You'll also need
`dot` (from the `graphviz` suite). To build the documentation, simply run

    tomographer-X.X/build> make doc

This will create API documentation in both HTML and LaTeX format. The HTML
output is located in `build/html` and the latex files are in `build/latex`.

If `doxygen` was not found by CMake, then re-run `cmake` specifying the location
of Doxygen using the `-DDOXYGEN_EXECUTABLE=/location/of/doxygen` switch.

To build the latex documentation, enter the `latex` directory and run `make`.
The output file is called `refman.pdf`.

[tomographer_apidoc]: https://tomographer.github.io/tomographer/api-doc/
[doxygen]: http://www.doxygen.org/


Test Suite (for developers)
---------------------------

There is a test suite which checks that the tomographer C++ framework works
correctly, and that no bugs or regressions are being introduced as the code is
being changed and maintained.  The tests cover `tomorun` and the python
interface as well.

To compile and run the test suite, you need to specify to cmake that you want to
build it:

    tomographer-X.X/build> cmake .. -DTOMOGRAPHER_ENABLE_TESTS=on

The test suite will then be compiled normally when you run `make`. Use the `-j`
option with the number of CPU cores on your system to speed up the compilation:

    tomographer-X.X/build> make -j4

Run the test suite with the command

    tomographer-X.X/build> make test

The test suite uses [CTest][ctest].  You can also run the test programs
individually.  Most test executables use the [Boost Unit Test
Framework][boost_test] and as such accept [various options][boost_test_options]
to tune verbosity, which tests to run, etc.

Currently, the test suite does not compile entirely under Windows/mingw32/g++
because g++ can't allocate enough memory. I'm not sure how to fix this at the
moment (might need to use 64-bit compiler).

[ctest]: http://www.cmake.org/Wiki/CMake/Testing_With_CTest
[boost_test]: http://www.boost.org/doc/libs/1_59_0/libs/test/doc/html/index.html
[boost_test_options]: http://www.boost.org/doc/libs/1_59_0/libs/test/doc/html/boost_test/runtime_config/summary.html


Feedback
--------

Please report bugs, issues and wishes at:

  https://github.com/Tomographer/tomographer/issues


Contributing
------------

Contributions are welcome.  The preferred way to submit enhancements is to fork the
repo and send me a pull request.  Also, don't hesitate to contact me for questions.


How to Cite
-----------

If you use this software in your research, please consider citing the following
works:

1. Philippe Faist and Renato Renner. Practical and Reliable Error Bars in
   Quantum Tomography. Physical Review Letters 117:1, 010404 (2016).
   arXiv:1509.06763

2. Philippe Faist. The Tomographer Project. Available at
   https://github.com/Tomographer/tomographer/.


Authors, Copyright, License
---------------------------

Author: Philippe Faist, phfaist@caltech.edu.

Copyright (c) 2016 ETH Zurich, Institute for Theoretical Physics, Philippe Faist

Copyright (c) 2017 Caltech, Institute for Quantum Information and Matter, Philippe Faist

Released under the terms of the MIT License (see file LICENSE.txt)
