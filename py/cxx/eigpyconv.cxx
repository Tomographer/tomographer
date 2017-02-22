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

