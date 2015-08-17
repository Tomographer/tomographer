================================================================================
			    THE TOMOGRAPHER PROJECT
================================================================================

A toolbox for error analysis in quantum tomography.


Overview
--------

This project comprises two parts:

     * The `tomorun` program -- you probably want this

     * The generic, C++ framework -- flexible, but you'll need to invest time.

The `tomorun` executable produces a histogram of a figure of merit under the
distribution relevant for constructing confidence regions using the method
described in [Christandl & Renner, PRL (2012), arXiv:1108.5329]. The measurement
data is specified with independent POVM outcomes.

The C++ framework is a set of abstract and generic classes which you can combine
in your preferred way to implement this random walk for even more general
settings.


Prerequisites
-------------

You'll need:
    - a recent C++ compiler (g++ >= 4.6)
    - CMake >= 2.8.5 http://www.cmake.org/
    - Boost libraries, http://www.boost.org/
    - Eigen3 library >= 3.2, http://eigen.tuxfamily.org/
    - MatIO library, https://sourceforge.net/projects/matio/

A recent C++ compiler is required as some C++11 features and elements of its
standard library are used. Also, make sure it supports OpenMP or you won't
benefit from parallelization.

[Successful setup:
    - g++ 4.6.3
    - CMake 2.8.7
    - Boost 1.48
    - Eigen 3.2.1
    - MatIO 1.5.2]

Tested on Linux/Ubuntu and Mac OS X. Should theoretically (*big flashing red
warning light*) also work on Windows.


Download
--------

There will soon be official source code releases (TODO!!). For now, you need
`git` and you should clone the repository (FIXME!!). Note that for the build
system, `git` must be in your PATH, or you should set the CMake variable "GIT"
appropriately.


Installation
------------

The installation process is done using CMake. (You'll need CMake >= 2.8.5.)
Download an official release of Tomographer, unpack it, and enter the unpacked
directory. Then, issue the commands:

    tomographer-1.0> mkdir build
    tomographer-1.0> cd build
    tomographer-1.0/build> cmake .. <ADDITIONAL CMAKE OPTIONS HERE>
    tomographer-1.0/build> make -j4
    tomographer-1.0/build> make install/strip

And you'll have the `tomorun` installed on your system. You can specify some
standard CMake variables, such as CMAKE_INSTALL_PREFIX. If you installed a
recent compiler manually, you'll need to point CMake to that compiler, e.g. with

    > cmake .. -DCMAKE_C_COMPILER=/path/to/gcc -DCMAKE_CXX_COMPILER=/path/to/g++ 

To specify paths to the Eigen3 and MatIO libraries, use the CMake switches:

    -DEIGEN3_INCLUDE_DIR=/path/to/include/eigen3
    -DMATIO_INCLUDE_DIR=/path/to/include

You may of course also alternatively use CMake's graphical interface, CMake-GUI.


Running Tomorun
---------------

Detailed information about how to use & run `tomorun` is obtained by querying
its help text:

    > tomorun --help

It is often more convenient to make `tomorun` read its options from a
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


Api Documentation
-----------------

You can build the API documentation using Doxygen >= 1.8
(http://www.doxygen.org/). You'll also need `dot` (`graphviz`). Enter the
directory `doc/` and type:

    tomographer-1.0/doc/> doxygen Doxyfile

This will create API documentation in both HTML and LaTeX format.


Authors, Copyright, License
---------------------------

Philippe Faist
(C) 2015 ETH Zurich

Released under the terms of the ??????? License (TODO!!/FIXME!!)
