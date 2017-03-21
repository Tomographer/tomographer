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


TOMOGRAPHER_DEFINE_MSG_EXCEPTION_BASE(TomographerCxxError, std::string(), std::runtime_error) ;



// adapted from pybind11::exception<Type> in "pybind11/include/pybind11.h"
//
template<typename Type>
class exception_with_docstring : public py::object {
public:
  exception_with_docstring(py::handle scope, const char *name, PyObject *baseTypeObj = PyExc_Exception,
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
//
// adapted from pybind11::register_exception<Type> in "pybind11/include/pybind11.h"
//
template <typename CppException>
exception_with_docstring<CppException> &register_exception_with_docstring(
    py::handle scope,
    const char *name,
    PyObject *base = PyExc_Exception,
    std::string docstring = std::string()
    ) {
  static exception_with_docstring<CppException> ex(scope, name, base, docstring);
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


#endif
