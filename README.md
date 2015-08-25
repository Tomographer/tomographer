
THE TOMOGRAPHER PROJECT
=======================

— A toolbox for error analysis in quantum tomography.


Overview
--------

This project comprises two parts:

 * The `tomorun` program — you probably want this

 * The generic, `Tomographer` C++ framework — flexible, but you'll need to
   invest more time.

The `tomorun` executable produces a histogram of a figure of merit under the
distribution relevant for constructing confidence regions using the method
described in [Christandl & Renner, PRL (2012),
arXiv:1108.5329](http://arxiv.org/abs/1108.5329). The measurement data are
specified as independent POVM outcomes.

The C++ framework is a set of abstract and generic classes which you can combine
in your preferred way to implement this random walk for even more general
settings.


Prerequisites
-------------

You'll need:

  - a recent C++ compiler (g++ >= 4.6, Intel ICC >= 14, clang++?)
  - [CMake >= 2.8.5](http://www.cmake.org/)
  - [Boost libraries](http://www.boost.org/)
  - [Eigen3 library >= 3.2](http://eigen.tuxfamily.org/)
  - [MatIO library](https://sourceforge.net/projects/matio/)

A recent C++ compiler is required as some C++11 features and elements of its
standard library are used. Also, make sure it supports OpenMP or you won't
benefit from parallelization.

Tested on Linux/Ubuntu and Mac OS X. Should theoretically (*big flashing red
warning light*) also work on Windows.


Download
--------

There are currently no binary releases available, you'll have to compile from
source. Don't worry, that's not complicated. If you haven't already done so,
install all the usual development tools (`gcc`/`g++`/`make`/etc.)

There will soon be official source code releases (TODO!!). For now, you need
`git` and you should clone the repository (FIXME!!). Note that for the build
system, `git` must be in your PATH, or you should set the CMake variable "GIT"
appropriately.


Installation
------------

The configuration, compilation and installation process is done using
CMake. (You'll need CMake >= 2.8.5.)  Download an official release of
Tomographer, unpack it, and enter the unpacked directory. Then, issue the
commands:

    tomographer-1.0> mkdir build
    tomographer-1.0> cd build
    tomographer-1.0/build> cmake .. <ADDITIONAL CMAKE OPTIONS HERE>
    tomographer-1.0/build> make
    tomographer-1.0/build> make install/strip

And you'll have the `tomorun` installed on your system. You can specify some
standard CMake variables, such as CMAKE_INSTALL_PREFIX. If you installed a
recent compiler manually, you'll need to point CMake to that compiler, e.g. with

    > cmake .. -DCMAKE_C_COMPILER=/path/to/gcc -DCMAKE_CXX_COMPILER=/path/to/g++ 

To specify paths to the Eigen3 and MatIO libraries, use the CMake switches:

    -DEIGEN3_INCLUDE_DIR=/path/to/include/eigen3
    -DMATIO_INCLUDE_DIR=/path/to/include

You may of course also alternatively use CMake's graphical interface, CMake-GUI.

Note the compilation step (`make`) is quite computation-heavy because of the extensive
C++11 template metaprogramming. It might take a minute or two to complete depending on
your hardware.


Running Tomorun
---------------

Detailed information about how to use & run `tomorun` is obtained by querying
its help text:

    > tomorun --help

Data is read from a file in MATLAB format (see option --data-file-name), and
several options control which figure of merit to calculate as well as the
parameters and behavior of the random walk.

As `tomorun` is running, you may query on its progress by hitting CTRL-C
(Linux/Mac OS X). If you want to interrupt `tomorun` and quit the current task,
hit CTRL-C twice in short succession.

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

    # Step size of the random walk.
    step-size = 0.01

    # Sweep size. Keep only one in so many samples, to decorrelate them.
    # Choose of the order of 1/<step-size>
    n-sweep = 100
    # Run until we have recorded this number of samples.
    n-run = 32768

    # Choice of figure of merit. Here for example, tr(rho*rho_ref). If rho_ref
    # is pure, this is exactly the (squared) fidelity to rho_ref. "rho_ref" is
    # the name of a variable defined in 'the_data.mat'
    value-type=obs-value:rho_ref

    # The parameters of the histogram. MIN:MAX/NUM_BINS
    value-hist=0.9:1/50


API Documentation
-----------------

You can build the API documentation using [Doxygen >= 1.8][doxygen]. You'll also need
`dot` (`graphviz`). Enter the directory `doc/` and type:

    tomographer-1.0/doc/> doxygen Doxyfile

This will create API documentation in both HTML and LaTeX format.

[doxygen]: http://www.doxygen.org/


Test Suite (for developers)
---------------------------

There is a test suite which checks that the tomographer C++ framework works
correctly, and that no bugs or regressions are being introduced as the code is
being changed and maintained.

The test suite can be build by issuing the command

    tomographer-1.0/build> make -j4 test_tomographer

(Replace `4` by the number of CPUs available on your system.)

Run the test suite with the command

    tomographer-1.0/build> ./cxx/test/test_tomographer

As the test suite uses the [Boost Unit Test Framework][boost_test], this program
accepts [a set of options][boost_test_options] to tune verbosity, which tests to
run, etc.

[boost_test]: http://www.boost.org/doc/libs/1_59_0/libs/test/doc/html/index.html
[boost_test_options]: http://www.boost.org/doc/libs/1_59_0/libs/test/doc/html/boost_test/runtime_config/summary.html


Authors, Copyright, License
---------------------------

Philippe Faist
(C) 2015 ETH Zurich    ??????

Released under the terms of the ??????? License (TODO!!)
