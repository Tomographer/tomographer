.. tomographer-python documentation master file, created by
   sphinx-quickstart on Fri Dec  9 23:10:47 2016.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.


Tomographer Python Interface (|version|)
========================================

The `tomographer` package provides tools for practical and reliable error bars
in quantum tomography.

These Python bindings provide an easy-to-use interface to the underlying C++
routines.  If you are interested, :tomocxx:`here is the API documentation for
the Tomographer C++ framework <index.html>`.

Please note that the Python interface is not meant to cover the entire extent of
the C++ API.  Rather, we aim to provide a high-level access to core tasks
implemented in optimized C++ routines.  We provide specific types (such as for
handling histograms, receiving a status report or passing parameters of a random
walk) in order to interact with those core tasks. The Python interface is
designed to allow a seamless integration of our tomography procedure into an
existing experimental workflow.  Currently, there is only one "core task"
implemented, :py:func:`tomographer.tomorun.tomorun()`, which is an alternative
to the `tomorun` executable program.

Observe that some classes may be loaded and dumped using the :py:mod:`pickle`
module.  You may find this useful for caching intermediate results or for saving
the result of a computation.  Classes which can be :py:mod:`pickle`\ d are
documented as such.


Modules and classes
-------------------

.. toctree::
   :maxdepth: 2

   tomographer


Indices and tables
------------------

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`

