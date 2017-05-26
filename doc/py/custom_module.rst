
Writing Special-Purpose C++ with a Python Interface
---------------------------------------------------

If you want to perform a specific task which you can't do with the general
:py:func:`~tomographer.tomorun.tomorun()` function (say, implement a different
jump function or use a different representation of density matrices), then you
can write your basic components in C++ while providing a convenient Python
interface for easily controlling and using your code.

For instance, suppose you would like to implement a calculation of the
log-likelihood function for measurement outcomes which are not a product of
individual POVM effects.  You can't simply run
:py:func:`~tomographer.tomorun.tomorun()`, because internally that function uses
:py:class:`tomographer.densedm.IndepMeasLLH` which can only handle product POVM
effects.

The Tomographer Python Interface provides several C++ tools which you can use
and which significantly simplifies your task.  Also, all Python objects defined
in the Tomographer Python Interface are available via C++.

We use the nice `PyBind11 <https://github.com/pybind/pybind11>`_ tool for
interfacing C++ with Python. It includes out-of-the-box interfacing NumPy with
Eigen.

Here is a template to get started with your custom C++/Python module.

File `my_custom_module.cxx`::

