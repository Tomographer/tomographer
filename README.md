
THE TOMOGRAPHER PROJECT
=======================

A toolbox for error analysis in quantum tomography.


Overview
--------

This project comprises two parts:

 * The `tomorun` program — you probably want this

 * The generic, `Tomographer` C++ framework — flexible, but you'll need to
   invest more time.

The `tomorun` executable produces a histogram of a figure of merit under the
distribution relevant for a reliable error analysis as described in [Faist &
Renner, Practical Reliable Error Bars in Quantum Tomography, arXiv:XXXX.XXXXX
(September 2015)].  The measurement data are specified as independent POVM
outcomes.

The C++ framework is a set of abstract and generic classes which you can combine
in your preferred way to implement this random walk for even more general
settings.


Prerequisites
-------------

If you're lucky and there's a binary release for your system, you won't need
anything else.

To compile from source, you'll need:

  - a recent C++ compiler (g++ >= 4.6, Intel ICC >= 14, clang++ >= 3.6 (3.3 w/o
    OpenMP))
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

Tomographer can be downloaded from our github releases page:

https://github.com/Tomographer/tomographer/releases

There is currently only a binary release for Mac OS X available.  For other
platforms, you'll have to compile from source.  Don't worry, that's not
complicated. If you haven't already done so, install all the usual development
tools (`gcc`/`g++`/`make`/etc.) and follow the *Installation* instructions
below.

If you compile from source, make sure you download the offical source package
from our releases page. If you use github's automatic `Download TAR.GZ or ZIP'
links from the source code, you might end up with missing files.

If you wish to contribute to development, don't hesitate to fork the repo on
github and send me pull requests. Contact me if you have questions.


Installation
------------

If you found a binary release for your system, simply unpack it. It is then
ready for use.

The rest of this section concerns compiling Tomographer/Tomorun from source.

The configuration, compilation and installation process is done using CMake.
(You'll need CMake >= 2.8.5.)  Download an official release of Tomographer,
unpack it, and enter the unpacked directory.  Then, issue the commands:

    tomographer-1.0> mkdir build
    tomographer-1.0> cd build
    tomographer-1.0/build> cmake .. <ADDITIONAL CMAKE OPTIONS HERE>
    tomographer-1.0/build> make
    tomographer-1.0/build> make install/strip

And you'll have the `tomorun` installed on your system.

You may also run CMake multiple times to adjust all the relevant options.  You
can specify some standard CMake variables, such as CMAKE_INSTALL_PREFIX.  If you
installed a recent compiler manually, you'll need to point CMake to that
compiler, e.g. with

    > cmake .. -DCMAKE_C_COMPILER=/path/to/gcc -DCMAKE_CXX_COMPILER=/path/to/g++ 

To specify paths to the Boost, Eigen3 and MatIO libraries, use the CMake
switches:

    -DEIGEN3_INCLUDE_DIR=/path/to/include/eigen3
    -DMATIO_LIBRARY=/path/to/libmatio.a
    -DMATIO_INCLUDE_DIR=/path/to/include

(See [here][cmake_findboost] for switches relating to Boost libraries.)

[cmake_findboost]: http://www.cmake.org/cmake/help/v3.0/module/FindBoost.html

You may of course also alternatively use CMake's graphical interface, CMake-GUI.

Note the compilation step (`make`) is quite computation-heavy because of the
extensive C++11 template metaprogramming. It might take a minute or two to
complete depending on your hardware, and might be pretty greedy on RAM.


Running Tomorun
---------------

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


MATLAB Tools
------------

Some tools are provided for fitting the resulting histogram to our theoretical
model. These are MATLAB scripts located in the `tools` subdirectory.

For more information on how to run the histogram analyzer/fitter, run:

    >> analyze_tomorun_histogram('Help')


API Documentation
-----------------

You can build the API documentation using [Doxygen >= 1.8][doxygen]. You'll also
need `dot` (from the `graphviz` suite). To build the documentation, simply run

    tomographer-1.0/build> make doc

This will create API documentation in both HTML and LaTeX format. The HTML
output is located in `build/html` and the latex files are in `build/latex`.

If `doxygen` was not found by CMake, then re-run `cmake` specifying the location
of Doxygen using the `-DDOXYGEN_EXECUTABLE=/location/of/doxygen` switch.

To build the latex documentation, enter the `latex` directory and run `make`.
The output file is called `refman.pdf`.

[doxygen]: http://www.doxygen.org/


Test Suite (for developers)
---------------------------

There is a test suite which checks that the tomographer C++ framework works
correctly, and that no bugs or regressions are being introduced as the code is
being changed and maintained.

To compile and run the test suite, you need to specify to cmake that you want to
build it:

    tomographer-1.0/build> cmake .. -DTOMOGRAPHER_ENABLE_TESTS=on

The test suite will then be compiled normally when you run `make`. Use the `-j`
option with the number of CPU cores on your system to speed up the compilation:

    tomographer-1.0/build> make -j4

Run the test suite with the command

    tomographer-1.0/build> make test

The test suite uses [CTest][ctest].  You can also run the test programs
individually.  Most test executables use the [Boost Unit Test
Framework][boost_test] and as such accept [various options][boost_test_options]
to tune verbosity, which tests to run, etc.

[ctest]: http://www.cmake.org/Wiki/CMake/Testing_With_CTest
[boost_test]: http://www.boost.org/doc/libs/1_59_0/libs/test/doc/html/index.html
[boost_test_options]: http://www.boost.org/doc/libs/1_59_0/libs/test/doc/html/boost_test/runtime_config/summary.html


How to Cite
-----------

If you use this software in your research, please consider citing the following
works:

1. Philippe Faist and Renato Renner. Practical, Reliable Error Bars in Quantum
   Tomography (2015). arXiv:XXXX.XXXXX

2. Philippe Faist. The Tomographer Project. Available at
   https://github.com/Tomographer/tomographer/.


Authors, Copyright, License
---------------------------

Author: Philippe Faist

Copyright (c) 2015 ETH Zurich, Institute for Theoretical Physics, Philippe Faist

Released under the terms of the MIT License (see file LICENSE.txt)
