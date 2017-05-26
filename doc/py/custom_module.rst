
Writing Special-Purpose C++ Code with a Python Interface
========================================================

If you want to perform a specific task which you can't do with the general
:py:func:`~tomographer.tomorun.tomorun()` function (say, implement a different
jump function or use a different representation of density matrices), then you
can write your basic components in C++ while providing a convenient Python
interface for easily controlling and using your code.

The Tomographer Python Interface provides several C++ tools which you can use
and which significantly simplifies your task.  Also, all Python objects defined
in the Tomographer Python Interface are available via C++.

We use the nice `pybind11 <https://github.com/pybind/pybind11>`_ tool for
interfacing C++ with Python. It includes out-of-the-box interfacing NumPy with
Eigen. Learn its basic usage before carrying on too far.

Here is minimal but entirely self-contained and fully functional template to get
started with your custom C++/Python module.

Create a directory called ``my_custom_module``, and place the two files
:download:`my_custom_module/my_custom_module.cxx` and
:download:`my_custom_module/setup.py` in there.  Enter that directory, and run
compile the Python module::

  > python setup.py build

You will notice that a directory ``build/lib.XXXXXXXX`` should have appeared.
To test your compiled module, try out :download:`this little script
<my_custom_module/test_run.py>`, place it also in the ``my_custom_module``
directory and run::
  
  > PYTHONPATH=build/lib.XXXXXXXX  python test_run.py

(Of course, you can also run the usual ``python setup.py install`` or ``python
setup.py bdist_wheel`` etc.)


Here is a little overview of the contents of each file.


The C++ Source File(s)
----------------------

The idea of the C++ file is to write all the computation in C++, using the
classes from the :tomocxx:`Tomographer framework </>`.

Most of the useful generic classes in the Tomographer framework are instantiated
with standard template arguments and exposed to Python.  See :tomocxx:`the tpy
namespace <namespacetpy.html>` for the API documentation.

A set of :tomocxx:`C++ typedefs are provided
<namespacetpy.html#typedef-members>`, which are already exposed to Python via
the `tomographer` module.  You'll find the types ``tpy::RealType`` and
``tpy::CountIntType`` (by default, ``double`` and ``int``, respectively) which
serve as template arguments for most classes.  For instance, the template class
:tomocxx:`Histogram <class_tomographer_1_1_histogram.html>` is typedef'ed as
:tomocxx:`tpy::Histogram <namespacetpy.html#a695433d4c090bebbf94083b96f36b5be>`.

These classes are then exposed to Python in the `tomographer` module.  When
writing your own custom module, you can take advantage of these existing tools
so that you don't have to expose all these helper types yourself; rather they
can be transparently passed between Python to/from C++.


The Python ``setup.py`` file
----------------------------

This is a standard ``setup.py`` file for packaging Python packages, using
``setuptools``.  Read up on that (and then try to not commit hara-kiri).

There are various flags which need to be set when compiling your module, which
you can simply steal from the `tomographer` module: indeed, the `tomographer`
module :py:data:`exposes the flags it was compiled with
<tomographer.compile_info>`, so you can just recycle them.

This ``setup.py`` script allows the user (you!) to specify options as
environment variables, for instance::

  >  CXX_FLAGS="-O0 -g3 -UNDEBUG -march=generic -std=c++11"  python setup.py build

This is done by using the :py:class:`~tomographer.include.Vars` class: you give
it a set of variables and default values, if the variable exists as an
environment variable it is read there, else it takes the default value.

Note that if you need to find other custom libraries or include headers, you can
use the utilities :py:func:`tomographer.include.find_include_dir()` or
:py:func:`tomographer.include.find_lib()`.

