
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
interfacing C++ with Python. It includes out-of-the-box interfacing of NumPy
with Eigen. Learn its basic usage before carrying on too far here.

Here is minimal but entirely self-contained and fully functional template to get
started with your custom C++/Python module. Create a directory called
``my_custom_module``, and place the two files
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
namespace <namespacetpy.html>` for the full API documentation.

The `tpy` namespace most importantly contains a set of :tomocxx:`C++ typedefs
<namespacetpy.html#typedef-members>`, which are types which are already exposed
as Python objects via the `tomographer` module.  For instance, the template
class :tomocxx:`MHRWParams <struct_tomographer_1_1_m_h_r_w_params.html>` is
typedef'ed as :tomocxx:`tpy::MHRWParams
<namespacetpy.html#a94b6e199d4b59e18713b380fa4daa784>`.  You'll also find the
types ``tpy::RealScalar``, ``tpy::CountRealType``, as well as
``tpy::IterCountIntType``, ``tpy::HistCountIntType``, ``tpy::TaskCountIntType``
and ``tpy::FreqCountIntType`` which specify the C++ types to use for
floating-point values and integer values storing information about matrix
elements, iteration counts, histogram counts, etc.

Note that some classes have slightly different implementations and aren't just a
typedef---for instance, :tomocxx:`tpy::Histogram <classtpy_1_1_histogram.html>`
is re-implemented to use `NumPy` objects, so it can store any data type, but at
the same type provides almost transparent conversion to and from
:tomocxx:`Tomographer::Histogram <class_tomographer_1_1_histogram.html>`.

The classes in the `tpy` namespace are already exposed to Python in the
`tomographer` module.  When writing your own custom module, you can take
advantage of these existing tools so that you don't have to expose all these
helper types yourself; rather they can be transparently passed between Python
to/from C++.  For instance, you can straightforwardly return a `tpy::Histogram`
from a C++ function exposed to Python, and the result will be accessible from
Python as a :py:class:`tomographer.Histogram` object.

.. note:: In order to set up the Tomographer Python API properly, you must call
          :tomocxx:`tpy::import_tomographer()
          <namespacetpy.html#a85146271201de94c995aa16c99ed9952>` at the
          beginning of your C++ module initialization function.


The Python ``setup.py`` file
----------------------------

This is a standard ``setup.py`` file for packaging Python packages, using
``setuptools``.  Read up on that.

There are various flags which need to be set when compiling your module, which
you can simply steal from the `tomographer` module: indeed, the `tomographer`
module :py:data:`exposes the flags it was compiled with
<tomographer.version.compile_info>`, so you can just recycle them.

This ``setup.py`` script allows the user (you!) to specify options as
environment variables, for instance::

  >  CXX_FLAGS="-O0 -g3 -UNDEBUG -march=generic -std=c++11"  python setup.py build

This is done by using the :py:class:`~tomographer.include.Vars` class: you give
it a set of variables and default values, if the variable exists as an
environment variable it is read there, else it takes the default value.

Note that if you need to find other custom libraries or include headers, you can
use the utilities :py:func:`tomographer.include.find_include_dir()` or
:py:func:`tomographer.include.find_lib()`.

.. note:: You might be tempted to move the import statements for ``pybind11``,
   ``numpy``, etc. inside a function, and add those modules as ``setup(...,
   setup_requires=...)`` and/or ``setup(..., install_requires=)``.  You'll
   realize that this doesn't work with PIP (``install_requires`` installs the
   package too late, and ``setup_requires`` does not know about PIP---it uses
   only `easy_install`---and the package might not be installed properly).  It's
   a mess.  Try it yourself, waste about a full day on that, but after that
   don't waste any more time and revert your changes back to how it was if you
   don't want to go insane as I almost did.  Conclusion: Let's avoid any
   unnecessary casualties, and stick to making sure the requirements are already
   installed from the start of the ``setup.py`` script; we just need to document
   this properly.
