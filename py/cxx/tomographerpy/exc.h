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

#ifndef TOMOGRAPHERPY_EXC_H
#define TOMOGRAPHERPY_EXC_H

#include <exception>
#include <stdexcept>

#include <tomographerpy/common.h>


namespace tpy {


#ifndef TOMOGRAPHER_PARSED_BY_DOXYGEN

// implementation
TOMOGRAPHER_DEFINE_MSG_EXCEPTION_BASE(TomographerCxxError, std::string(), std::runtime_error) ;

#else
// fake code for Doxygen

/** \brief Base exception class for errors in the \c tomographer python module.
 *
 * This C++ exception translates to a correspondingly named Python exception.
 */
class TomographerCxxError : public std::runtime_error { }; // fake

#endif



namespace tomo_internal {

// (Adapted from \c "pybind11::exception<Type>" in \c "pybind11/include/pybind11.h"
// sources.)
template<typename Type>
class TOMOGRAPHER_EXPORT ExceptionWithDocstring : public py::object {
public:
  ExceptionWithDocstring(py::handle scope, const char *name, PyObject *baseTypeObj = PyExc_Exception,
                         std::string docstring = std::string()) {
    std::string full_name = scope.attr("__name__").cast<std::string>() +
      std::string(".") + name;
    m_ptr = PyErr_NewExceptionWithDoc(const_cast<char*>(full_name.c_str()),
                                      const_cast<char*>(docstring.c_str()),
                                      baseTypeObj,
                                      NULL);
    if (py::hasattr(scope, name)) {
      py::pybind11_fail("Error during initialization: multiple incompatible "
                        "definitions with name \"" + std::string(name) + "\"");
    }
    scope.attr(name) = *this;
  }

  // Sets the current python exception to this exception object with the given message
  void operator()(const char *message) {
    PyErr_SetString(m_ptr, message);
  }
};
} // namespace tomo_internal

/** \brief Helper to register in Python a C++ exception with a docstring
 *
 * (Adapted from \c "pybind11::register_exception<Type>" in \c
 * "pybind11/include/pybind11.h" sources.)
 */
template <typename CppException>
tomo_internal::ExceptionWithDocstring<CppException> & registerExceptionWithDocstring(
    py::handle scope,
    const char *name,
    PyObject *base = PyExc_Exception,
    std::string docstring = std::string()
    ) {
  static tomo_internal::ExceptionWithDocstring<CppException> ex(scope, name, base, docstring);
  py::register_exception_translator([](std::exception_ptr p) {
      if (!p) return;
      try {
        std::rethrow_exception(p);
      } catch (const CppException &e) {
        ex(e.what());
      }
    });
  return ex;
}

} // namespace tpy


#endif
