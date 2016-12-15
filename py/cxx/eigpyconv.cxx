
#include "tomographerpy/common.h"

#include <boost/python.hpp>

#include <cstring>
#include <iostream>

#include "common_p.h"
#define TOMOGRAPHERPY_DO_NUMPY_IMPORT_ARRAY // import_array() is called in this C file
#include "tomographerpy/eigpyconv.h"
#include "tomographerpy/exc.h"







#if PY_VERSION_HEX >= 0x03000000
static PyObject * workaround_import_array() {
  import_array();
  return NULL;
}
#elif PY_VERSION_HEX >= 0x02070000
static void workaround_import_array() {
  import_array();
}
#else
#error "Unknown or unsupported Python version"
#endif



PyObject * pyTomographerNumpyConversionError = NULL;

// add the types you want to add
void register_eigen_converter()
{
  auto logger = Tomographer::Logger::makeLocalLogger(TOMO_ORIGIN, tpy_logger);

  logger.debug("register_eigen_converter() ...");

  workaround_import_array(); //< required, or conversion leads to segfault
  
  logger.debug("register_eigen_converter(): imported NumPy Array OK, registering types ...");

  // matrices...
  eigen_python_converter< Eigen::Matrix<long,Eigen::Dynamic,Eigen::Dynamic> >::to_python();
  eigen_python_converter< Eigen::Matrix<long,Eigen::Dynamic,Eigen::Dynamic> >::from_python();
  eigen_python_converter< Eigen::Matrix<int,Eigen::Dynamic,Eigen::Dynamic> >::to_python();
  eigen_python_converter< Eigen::Matrix<int,Eigen::Dynamic,Eigen::Dynamic> >::from_python();
  eigen_python_converter< Eigen::Matrix<unsigned long,Eigen::Dynamic,Eigen::Dynamic> >::to_python();
  eigen_python_converter< Eigen::Matrix<unsigned long,Eigen::Dynamic,Eigen::Dynamic> >::from_python();
  eigen_python_converter< Eigen::Matrix<unsigned int,Eigen::Dynamic,Eigen::Dynamic> >::to_python();
  eigen_python_converter< Eigen::Matrix<unsigned int,Eigen::Dynamic,Eigen::Dynamic> >::from_python();
  eigen_python_converter< Eigen::Matrix<float,Eigen::Dynamic,Eigen::Dynamic> >::to_python();
  eigen_python_converter< Eigen::Matrix<float,Eigen::Dynamic,Eigen::Dynamic> >::from_python();
  eigen_python_converter< Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic> >::to_python();
  eigen_python_converter< Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic> >::from_python();
  eigen_python_converter< Eigen::Matrix<std::complex<float>,Eigen::Dynamic,Eigen::Dynamic> >::to_python();
  eigen_python_converter< Eigen::Matrix<std::complex<float>,Eigen::Dynamic,Eigen::Dynamic> >::from_python();
  eigen_python_converter< Eigen::Matrix<std::complex<double>,Eigen::Dynamic,Eigen::Dynamic> >::to_python();
  eigen_python_converter< Eigen::Matrix<std::complex<double>,Eigen::Dynamic,Eigen::Dynamic> >::from_python();
  // ... and vectors
  eigen_python_converter< Eigen::Matrix<long,Eigen::Dynamic,1> >::to_python();
  eigen_python_converter< Eigen::Matrix<long,Eigen::Dynamic,1> >::from_python();
  eigen_python_converter< Eigen::Matrix<int,Eigen::Dynamic,1> >::to_python();
  eigen_python_converter< Eigen::Matrix<int,Eigen::Dynamic,1> >::from_python();
  eigen_python_converter< Eigen::Matrix<unsigned long,Eigen::Dynamic,1> >::to_python();
  eigen_python_converter< Eigen::Matrix<unsigned long,Eigen::Dynamic,1> >::from_python();
  eigen_python_converter< Eigen::Matrix<unsigned int,Eigen::Dynamic,1> >::to_python();
  eigen_python_converter< Eigen::Matrix<unsigned int,Eigen::Dynamic,1> >::from_python();
  eigen_python_converter< Eigen::Matrix<float,Eigen::Dynamic,1> >::to_python();
  eigen_python_converter< Eigen::Matrix<float,Eigen::Dynamic,1> >::from_python();
  eigen_python_converter< Eigen::Matrix<double,Eigen::Dynamic,1> >::to_python();
  eigen_python_converter< Eigen::Matrix<double,Eigen::Dynamic,1> >::from_python();
  eigen_python_converter< Eigen::Matrix<std::complex<float>,Eigen::Dynamic,1> >::to_python();
  eigen_python_converter< Eigen::Matrix<std::complex<float>,Eigen::Dynamic,1> >::from_python();
  eigen_python_converter< Eigen::Matrix<std::complex<double>,Eigen::Dynamic,1> >::to_python();
  eigen_python_converter< Eigen::Matrix<std::complex<double>,Eigen::Dynamic,1> >::from_python();
  
  pyTomographerNumpyConversionError = createExceptionClass(
      "TomographerNumpyConversionError", TomographerCxxError,
      // docstring:
      "Exception class which indicates an error in conversion of a `NumPy` object to or from a C++ Eigen matrix."
      );

  boost::python::register_exception_translator<EigenNumpyConversionError>(+[](const EigenNumpyConversionError & exc) {
      PyErr_SetString(pyTomographerNumpyConversionError, exc.what());
    });
  
  boost::python::register_exception_translator<Tomographer::Tools::EigenAssertException>(
      +[](const Tomographer::Tools::EigenAssertException & exc) {
        PyErr_SetString(TomographerCxxError, exc.what());
      });

  logger.debug("register_eigen_converter() done.");
}

