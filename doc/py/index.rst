.. tomographer-python documentation master file, created by
   sphinx-quickstart on Fri Dec  9 23:10:47 2016.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.


Tomographer (Python Interface)
==============================

The `tomographer` package: Practical, Reliable Error Bars in Quantum Tomography.

These Python bindings provide an easy-to-use interface to the underlying C++ routines.

Please note that the Python interface is not meant to cover the entire extent of
the C++ API.  Rather, we aim to provide the necessary a high-level access to
core tasks implemented in optimized C++ routines.  We provide specific types
(such as for handling histograms, receiving a status report or passing
parameters of a random walk) in order to interact with those core tasks. The
Python interface is designed to allow a seamless integration of our tomography
procedure into an existing tomography workflow.  Currently, there is only one
"core task" implemented, :py:func:`tomographer.tomorun.tomorun()`, which is an
alternative to the `tomorun` executable program.

If you are interested, :tomocxx:`here is the detailed API documentation for the
C++ framework <index.html>`.


Modules and classes
-------------------

.. toctree::
   :maxdepth: 2

   tomographerpy/tomographer


Indices and tables
------------------

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`

