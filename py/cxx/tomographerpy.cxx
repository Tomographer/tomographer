
#include "tomographerpy/common.h"
#include "common_p.h"

//#include "tomographerpy/eigpyconv.h"


// TOMOGRAPHER_VERSION directly provided by setup.py
#ifndef TOMOGRAPHER_VERSION
#  include <tomographer/tomographer_version.h>
#endif



void register_eigen_converter();

void py_tomo_histogram();
void py_tomo_multiproc();
void py_tomo_tomorun();
void py_tomo_mhrwtasks();




PyLogger tpy_logger;




BOOST_PYTHON_MODULE(_tomographer_cxx)
{
  // hack module name so that classes, objects, etc. appear in the module "tomographer"
  boost::python::scope().attr("__name__") = "tomographer";

  // python logging
  tpy_logger.initPythonLogger();
  auto logger = Tomographer::Logger::makeLocalLogger(TOMO_ORIGIN, tpy_logger);

  logger.debug("INIT TOMOGRAPHER");

  boost::python::docstring_options doc_options(true, false, false);

  // expose Python API for setting the C++ logger level
  boost::python::class_<PyLogger>("PyLogger")
    .add_property("level", +[](const PyLogger & l) { return l.level(); },
                  +[](PyLogger & l, boost::python::object newlevel) {
                    l.setLevel(l.fromPythonLevel(newlevel));
                  });
  
  boost::python::scope().attr("cxxlogger") = boost::ref(tpy_logger);

  // the version of this library module
  boost::python::scope().attr("__version__") = TOMOGRAPHER_VERSION;

  // Eigen converters
  register_eigen_converter();

  logger.debug("Registered eigen converters.");

  py_tomo_histogram();

  py_tomo_multiproc();

  py_tomo_mhrwtasks();

  py_tomo_tomorun();

  logger.debug("TOMOGRAPHER INIT COMPLETE.");
}
