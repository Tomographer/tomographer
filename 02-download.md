---
title: "Download"
displaytitle: "Download & Install Tomographer"
permalink: /download/
layout: default
---


Downloading & Installing Binary
===============================

Download
--------

If there's a binary distribution for your system, that's your best option.

{% for b in site.tomographer_binaries %}
- [{{ b.title }}](https://github.com/Tomographer/tomographer/releases/download/{{ b.version }}/tomographer-{{ b.version }}-{{ b.system }}{{ b.ext }})
{% endfor %}

If you couldn't find a binary for your system, then you'll have to compile Tomographer
from sources (next section).

Install
-------

The installation of the binary is normally straightforward.

### Mac OS X:
Unpack the archive anywhere on your system. You may then run the `tomorun`
executable in the Terminal by executing directly the binary inside the archive, inside the
`bin/` subdirectory:

    > /path/to/extracted/tomographer-{{ site.tomographer_latest_version }}-macosx/bin/tomorun

The archive also contains the header files necessary if you want to develop projects using
the Tomographer C++ Framework.


Downloading & Installing From Source
====================================

Prerequisites
-------------

You'll need:

- a recent C++ compiler (g++ >= 4.6, Intel ICC >= 14, clang++ >= 3.6 (>= 3.3 w/o
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

You may download the source in either of two ways:

- [Obtain the source distribution (TAR.GZ archive).](https://github.com/Tomographer/tomographer/releases/download/{{ site.tomographer_latest_version }}/tomographer-{{ site.tomographer_latest_version }}.tar.gz)
  (alternative formats: [ZIP](https://github.com/Tomographer/tomographer/releases/download/{{ site.tomographer_latest_version }}/tomographer-{{ site.tomographer_latest_version }}.zip), [TAR.BZ2](https://github.com/Tomographer/tomographer/releases/download/{{ site.tomographer_latest_version }}/tomographer-{{ site.tomographer_latest_version }}.tar.bz2))

  If you don't plan to modify Tomographer itself, this is what you should download. Unpack
  the archive anywhere you want and continue with the installation instructions below.

- [Clone the git repository.](https://github.com/Tomographer/tomographer) Do this if you
  know a bit of GIT, if you wish to keep up-to-date with future updates, or if you wish to
  develop Tomographer itself.

  If you plan to contribute and are willing to send changes back (yes please!), go ahead
  and fork the repo in github, and send me pull requests.  Don't hesitate to contact me
  for questions and for mid-term or longer-term plans, especially for the APIs.

Note: do NOT use github's automatic "download archive" feature, as you'll either miss out
on GIT meta-information, or on auto-generated files in the source distributions.

Installation from Source
------------------------

In either case, you'll now have unpacked the sources at some location. You may now follow
the instructions detailed in the
[`README`]({{site.github_blob_baseurl}}{{site.tomographer_latest_version}}/README.md)
file. The main steps are:

### Set up a `build` directory and run `cmake`:

    tomographer-{{ site.tomographer_latest_version }}> mkdir build
    tomographer-{{ site.tomographer_latest_version }}> cd build
    tomographer-{{ site.tomographer_latest_version }}/build> cmake ..

  You may re-run `cmake` multiple times in order to find all libraries correctly. Use the
  appropriate switches (see the
  [`README`]({{site.github_blob_baseurl}}{{site.tomographer_latest_version}}/README.md)
  file in the tomographer sources)

### Compile the project:

    tomographer-{{ site.tomographer_latest_version }}/build> make

### (Optional) install the project to a system location:

    tomographer-{{ site.tomographer_latest_version }}/build> make install/strip

You'll then have the `tomorun` executable, as well as the headers library, installed at a
system location.
