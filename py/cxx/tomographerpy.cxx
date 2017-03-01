/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
 * Copyright (c) 2017 Caltech, Institute for Quantum Information and Matter, Philippe Faist
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include "tomographerpy/common.h"
#include "common_p.h"

#include "tomographerpy/exc.h"


// TOMOGRAPHER_VERSION directly provided by setup.py
#ifndef TOMOGRAPHER_VERSION
#  include <tomographer/tomographer_version.h>
#endif



//void register_eigen_converter();

//void py_tomo_histogram();
//void py_tomo_multiproc();
//void py_tomo_densedm();
//void py_tomo_tomorun();
void py_tomo_mhrw(py::module rootmodule);
//void py_tomo_mhrwtasks();




namespace tpy
{
// common logger object
PyLogger logger;

// the main exception object
PyObject * TomographerCxxError;
}




PYBIND11_PLUGIN(_tomographer_cxx)
{
  py::module rootmodule("tomographer", "Tomographer module");
  // hack module name so that classes, objects, etc. appear in the module "tomographer"
  //py::scope().attr("__name__") = "tomographer"; -- perhaps not needed with pybind11??

  // python logging
  tpy::logger.initPythonLogger();
  auto logger = Tomographer::Logger::makeLocalLogger(TOMO_ORIGIN, tpy::logger);

  logger.debug("INIT TOMOGRAPHER");

//  py::docstring_options doc_options(true, false, false); -- no longer with pybind11

  // our basic library exception class
  tpy::TomographerCxxError = createExceptionClass(
      rootmodule,
      "TomographerCxxError",
      PyExc_Exception,
      // doc:
      "Run-time error indicating an inappropriate usage or call of a method of the Python tomographer "
      "API. For example, an index may be out of range.\n\n"
      ".. note:: C++ Eigen assertion failures generated by ``eigen_assert()`` raise Python exceptions "
      "(via C++ exceptions), are translated to this exception type.\n\n");


  // expose Python API for setting the C++ logger level
  py::class_<PyLogger>(rootmodule, "PyLogger")
    .def_property("level",
                  [](const PyLogger & l) { return l.level(); },
                  [](PyLogger & l, py::object newlevel) {
                    l.setLevel(l.fromPythonLevel(newlevel));
                  });
  
  rootmodule.attr("cxxlogger") = tpy::logger;


  // the version of this library module
  rootmodule.attr("__version__") = TOMOGRAPHER_VERSION;
  // add a version submodule with more precise version info
  logger.debug("version module ... ");
  py::module versionmodule = rootmodule.def_submodule("version", "Version information");
  versionmodule.attr("version_str") = TOMOGRAPHER_VERSION;
  {
    auto collections = py::module::import("collections");
    auto namedtuple = collections.attr("namedtuple");
    py::list verfields;
    verfields.append("major"); 
    verfields.append("minor");
    auto VersionInfoTuple = namedtuple("VersionInfo", verfields);
    versionmodule.attr("version_info") = VersionInfoTuple(TOMOGRAPHER_VERSION_MAJ, TOMOGRAPHER_VERSION_MIN);
  }

  // add info on how tomographer was compiled, so that other modules can use the same
  // tools

  // Eigen converters
//  register_eigen_converter();

  logger.debug("Registered eigen converters.");

//  py_tomo_histogram();

  py_tomo_mhrw(rootmodule);

//  py_tomo_multiproc();

//  py_tomo_densedm();

//  py_tomo_mhrwtasks();

//  py_tomo_tomorun();

  logger.debug("TOMOGRAPHER INIT COMPLETE.");

  return rootmodule.ptr();
}
