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

#ifndef TOMOGRAPHERPY_MHRW_H
#define TOMOGRAPHERPY_MHRW_H

#include <string>

#include <tomographerpy/common.h>

#include <tomographer/mhrw.h>
//#include <tomographer/mhrwtasks.h>


namespace tpy {

/** \brief The Tomographer::MHRWParams type exposed to Python (the MHWalkerParam can be
 *         represented by any Python object)
 */
typedef Tomographer::MHRWParams<py::object, CountIntType>  MHRWParams;



/** \brief C++ utility to convert a Python dictionary of fields into a valid C++
 *         MHWalkerParams object
 *
 * Different \ref pageInterfaceMHWalker may require different types of
 * parameters.  In C++, this is a template parameter which is passed on to
 * whoever cares about those parameters.  In python, the parameters are stored
 * simply as an aribtrary dictionary of parameter values.
 *
 * Specialize this template class to each C++ MHWalkerParams type so that it can
 * be easily converted to/from a Python dictionary.  In the conversion from
 * Python, we actually allow the user to specify any object; this allows us to
 * handle the case where they directly specify the step-size as a float instead
 * of the explicit dictionary <code>{'step_size': ...}</code>.
 */
template<typename MHWalkerParams>
struct TOMOGRAPHER_EXPORT PyMHWalkerParamsToDict
{
  static inline py::dict makeDict(const MHWalkerParams & ) { return {}; }
  static inline MHWalkerParams fromPyObj(py::object ) { return {}; }
};
//! Specialization of PyMHWalkerParamsToDict for \ref Tomographer::MHWalkerParamsStepSize
template<typename StepRealType>
struct TOMOGRAPHER_EXPORT PyMHWalkerParamsToDict<Tomographer::MHWalkerParamsStepSize<StepRealType> >
{
  static inline py::dict makeDict(const Tomographer::MHWalkerParamsStepSize<StepRealType> & p) {
    return py::dict(py::arg("step_size") = p.step_size);
  }
  static Tomographer::MHWalkerParamsStepSize<tpy::RealType> fromPyObj(py::object d)
  {
    if ( py::hasattr(d, "__getitem__") ) {
      // dict or dict-like, go. If key doesn't exist, send in zero and let the underlying
      // mhwalker handle it
      return d.attr("get")("step_size", 0).cast<tpy::RealType>();
    }
    if ( d.is_none() ) {
      // None: let the underlying mhwalker decide what to do with this
      return 0;
    }
    // try to iterpret the object itself as a float
    return d.cast<tpy::RealType>();
  }
};

/** \brief Helper for converting any MHWalkerParams into a dictionary, using automatic
 *         template parameter deduction
 */
template<typename MHWalkerParams>
inline py::dict pyMHWalkerParamsToDictInvoke(const MHWalkerParams & p)
{
  return PyMHWalkerParamsToDict<MHWalkerParams>::makeDict(p);
}
/** \brief Helper for converting any Python object into a given MHWalkerParams
 */
template<typename MHWalkerParams>
inline MHWalkerParams pyMHWalkerParamsFromPyObj(py::object o)
{
  return PyMHWalkerParamsToDict<MHWalkerParams>::fromPyObj(o);
}




} // namespace Py



#endif
