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

#include <tomographerpy/common.h>


// thanks http://stackoverflow.com/a/9690436/1694896
inline PyObject* createExceptionClass(const char* name, PyObject* baseTypeObj = PyExc_Exception,
                                      std::string docstring = "Exception class")
{
  using std::string;
  namespace bp = boost::python;
  
  string scopeName = bp::extract<string>(bp::scope().attr("__name__"));
  string qualifiedName0 = scopeName + "." + name;
  char* qualifiedName1 = const_cast<char*>(qualifiedName0.c_str());
  
  PyObject* typeObj = PyErr_NewExceptionWithDoc(qualifiedName1, const_cast<char*>(docstring.c_str()), baseTypeObj, 0);
  if (typeObj == NULL) {
    bp::throw_error_already_set();
  }

  auto bpObj = bp::object(bp::handle<>(bp::borrowed(typeObj)));

  // add to current module
  bp::scope().attr(name) = bpObj;

  return typeObj;
}





#endif
