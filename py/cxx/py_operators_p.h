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


#ifndef TOMOPY_PY_OPERATORS_P_H
#define TOMOPY_PY_OPERATORS_P_H

//
// These overloads should probably feature in PyBind11 in some way, not sure if this is
// provided by PyBind11 in some other way I haven't noticed...
//


class pyop {
  py::object o;
public:
  pyop(py::object o_) : o(std::move(o_)) { }

  py::object object() const { return o; }
};


// See https://docs.python.org/2/c-api/object.html#c.PyObject_RichCompare
#define PY_OP_P_DEF_RICHCOMPARE_OPERATOR(CXXOP, PYOP)                   \
  inline bool CXXOP(const pyop & a, const pyop & b)                     \
  {                                                                     \
    PyObject *result_obj =                                              \
      PyObject_RichCompare(a.object().ptr(), b.object().ptr(), PYOP);   \
    if (result_obj == NULL) {                                           \
      throw py::error_already_set();                                    \
    }                                                                   \
    return py::reinterpret_steal<py::bool_>(result_obj).cast<bool>();   \
  }                                                                     \
  inline bool CXXOP(const pyop & a, const py::object & b)               \
  { return CXXOP( a, pyop(b) ) ; }                                      \
  inline bool CXXOP(const py::object & a, const pyop & b)               \
  { return CXXOP( pyop(a), b ) ; }


PY_OP_P_DEF_RICHCOMPARE_OPERATOR(operator<,  Py_LT)
PY_OP_P_DEF_RICHCOMPARE_OPERATOR(operator<=, Py_LE)
PY_OP_P_DEF_RICHCOMPARE_OPERATOR(operator==, Py_EQ)
PY_OP_P_DEF_RICHCOMPARE_OPERATOR(operator!=, Py_NE)
PY_OP_P_DEF_RICHCOMPARE_OPERATOR(operator>,  Py_GT)
PY_OP_P_DEF_RICHCOMPARE_OPERATOR(operator>=, Py_GE)



#define PY_OP_P_DEF_BINARY_NUMBER_OPERATOR(CXXOP, PYOPNAME)             \
  inline pyop CXXOP(const pyop & a, const pyop & b)                     \
  {                                                                     \
    PyObject * result_obj = PyNumber_ ## PYOPNAME (a.object().ptr(), b.object().ptr()); \
    if (result_obj == NULL) {                                           \
      throw py::error_already_set();                                    \
    }                                                                   \
    return py::reinterpret_steal<py::object>(result_obj);               \
  }                                                                     \
  inline pyop CXXOP(const pyop & a, const py::object & b)               \
  { return CXXOP(a, pyop(b)); }                                         \
  inline pyop CXXOP(const py::object & a, const pyop & b)               \
  { return CXXOP(pyop(a), b); }


PY_OP_P_DEF_BINARY_NUMBER_OPERATOR(operator+, Add)
PY_OP_P_DEF_BINARY_NUMBER_OPERATOR(operator-, Subtract)
PY_OP_P_DEF_BINARY_NUMBER_OPERATOR(operator*, Multiply)
PY_OP_P_DEF_BINARY_NUMBER_OPERATOR(operator/, TrueDivide) // Note: true division like in Python 3
PY_OP_P_DEF_BINARY_NUMBER_OPERATOR(operator%, Remainder)
PY_OP_P_DEF_BINARY_NUMBER_OPERATOR(operator<<, Lshift) // bitwise left shift
PY_OP_P_DEF_BINARY_NUMBER_OPERATOR(operator>>, Rshift) // bitwise right shift
PY_OP_P_DEF_BINARY_NUMBER_OPERATOR(operator&, And)     // bitwise AND
PY_OP_P_DEF_BINARY_NUMBER_OPERATOR(operator|, Or)      // bitwise OR
PY_OP_P_DEF_BINARY_NUMBER_OPERATOR(operator^, Xor)     // bitwise XOR





#endif
