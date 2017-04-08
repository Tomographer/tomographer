---
title: "Download"
displaytitle: "Download & Install Tomographer"
permalink: /download/
layout: default
---

<div class="auto-toc auto-toc-h2"></div>


Download Options
================

If you'd like to use the Python `tomographer` package, then you don't need to
download anything.  Just use `pip` [as described
here]({{site.baseurl}}/get-started)).

If you would like to use the `tomorun` executable, then it is recommended to
download the binary version corresponding to your system (see below).

If you'd like to compile `tomorun` or the Python interface from source, scroll
down and follow the corresponding instructions.


Downloading & Installing Tomorun Binary
=======================================

Download
--------

If there's a binary distribution for your system, that's your best option.

{% for b in site.tomographer_binaries %}
- [{{ b.title }}](https://github.com/Tomographer/tomographer/releases/download/{{ b.version }}/tomographer-{{ b.version }}-{{ b.system }}{{ b.ext }}){:target="_blank"}
{% endfor %}

If you couldn't find a binary for your system, or if the binary is incompatible or doesn't
work, then you'll have to compile Tomographer from sources (next section).

Install
-------

The installation of the binary is normally straightforward.

### Linux and Mac OS X

Unpack the archive anywhere on your system (say, somewhere in your home directory, or
system-wide like in `/opt/tomographer/`). You may then run the `tomorun` executable in the
Terminal by executing directly the binary inside the archive, inside the `bin/`
subdirectory:

    > /path/to/extracted/tomographer-{{ site.tomographer_latest_version }}-linux/bin/tomorun

The archive also contains the header files necessary if you want to develop projects using
the Tomographer C++ Framework.

It is not recommended to extract the archive in a system-standard location such as `/usr`
or `/usr/local`, because we may ship alternative versions of system libraries. These could
interfere with your system if they are placed in `/usr/local/lib` or some other standard
location. For a system-wide install, simply symlink the `tomorun` executable to, e.g.,
`/usr/local/bin/`:

    > cd /usr/local/bin
    > sudo ln -s /path/to/extracted/tomographer-{{ site.tomographer_latest_version }}-linux/bin/tomorun .

### Windows

Unpack the archive anywhere on your system, such as inside `C:\Program Files`. It will
create a single folder named `tomographer-<VERSION>-<SYS>`.

You can then access the `tomorun` executable, using the command-line DOS prompt, as

    >C:\Program Files\tomographer-<VERSION>-<SYS>\bin\tomorun.exe [options]


Downloading & Installing Tomorun From Source
============================================

Prerequisites
-------------

You'll need:

  - a recent C++ compiler (g++ >= 4.6, Intel ICC >= 14, LLVM/Clang++ >= 3.8)
  - [CMake >= 3.1](http://www.cmake.org/)
  - [Boost libraries](http://www.boost.org/)
  - [Eigen3 library >= 3.3](http://eigen.tuxfamily.org/)
  - [MatIO library](https://sourceforge.net/projects/matio/)

A recent C++ compiler is required as some C++11 features and elements of its
standard library are used. Also, make sure it supports OpenMP or you won't
benefit from parallelization. If you use LLVM/Clang++ on linux, you might need
to install additional packages for OpenMP (e.g. `libomp`).

Tested on Linux/Ubuntu, Mac OS X and Windows (MinGW32).


Download
--------

You may download the source in either of two ways:

- [Obtain a stable source distribution (TAR.GZ archive).](https://github.com/Tomographer/tomographer/releases/download/{{ site.tomographer_latest_version }}/tomographer-{{ site.tomographer_latest_version }}.tar.gz){:target="_blank"}
  (alternative formats: [ZIP](https://github.com/Tomographer/tomographer/releases/download/{{ site.tomographer_latest_version }}/tomographer-{{ site.tomographer_latest_version }}.zip){:target="_blank"}, [TAR.BZ2](https://github.com/Tomographer/tomographer/releases/download/{{ site.tomographer_latest_version }}/tomographer-{{ site.tomographer_latest_version }}.tar.bz2){:target="_blank"})

  If you're not too familiar with GIT and don't plan to modify Tomographer itself, this is
  what you should download. Unpack the archive anywhere you want and continue with the
  installation instructions below.

- [Clone the git repository.](https://github.com/Tomographer/tomographer){:target="_blank"} Do this if you
  know a bit of GIT, if you wish to keep up-to-date with future updates, or if you wish to
  contribute to the development of Tomographer itself.

  If you plan to contribute and are willing to send changes back (yes please!), go ahead
  and fork the repo in github, and send me pull requests.  Don't hesitate to contact me
  for questions and for mid-term or longer-term plans, especially for the APIs.

Note: do NOT use github's automatic "download ZIP" feature, as you'll either miss out on
GIT meta-information, or on auto-generated files in the source distributions.

Installation from Source
------------------------

In either case, you'll now have unpacked the sources at some location. You may now follow
the instructions detailed in the
[`README`]({{site.github_blob_baseurl}}{{site.tomographer_latest_version}}/README.md){:target="_blank"}
file. The main steps are:

### Set up a `build` directory and run `cmake`

    tomographer-{{ site.tomographer_latest_version }}> mkdir build
    tomographer-{{ site.tomographer_latest_version }}> cd build
    tomographer-{{ site.tomographer_latest_version }}/build> cmake ..

  You may re-run `cmake` multiple times in order to find all libraries correctly. Use the
  appropriate switches (see the
  [`README`]({{site.github_blob_baseurl}}{{site.tomographer_latest_version}}/README.md){:target="_blank"}
  file in the tomographer sources)

### Compile the project

    tomographer-{{ site.tomographer_latest_version }}/build> make

### (Optional) install the project to a system location

    tomographer-{{ site.tomographer_latest_version }}/build> make install/strip

You'll then have the `tomorun` executable, as well as the headers library, installed at a
system location or wherever you specified to `cmake`.
