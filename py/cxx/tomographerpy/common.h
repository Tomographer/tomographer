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

#ifndef TOMOPY_COMMON_H
#define TOMOPY_COMMON_H

#include <cstdio>
#include <string>

#include <pybind11/pybind11.h>

namespace py = pybind11;


#ifdef _WIN32
#  ifdef _tomographer_cxx_EXPORTS
#    define TOMOGRAPHER_EXPORT __declspec(dllexport)
#  else
#    define TOMOGRAPHER_EXPORT __declspec(dllimport)
#  endif
#else
#  define TOMOGRAPHER_EXPORT __attribute__((visibility("default")))
#endif


// DEBUGGING ONLY: set TOMOGRAPHERPY_DEBUG_EIGEN_ASSERT_CAUSES_ABORT to cause eigen_assert() failures to abort() and dump core
#ifndef TOMOGRAPHERPY_DEBUG_EIGEN_ASSERT_CAUSES_ABORT
#  define TOMOGRAPHER_EIGEN_ASSERT_EXCEPTION
#endif
#include <tomographer/tools/eigen_assert_exception.h>

// include this AFTER eigen_assert_exception
#pragma GCC visibility push(default)
#include <pybind11/eigen.h>

#ifdef EIGEN_NO_DEBUG
#  error "TomographerPy requires enabled Eigen assertions, otherwise `TomographerCxxError` won't be raised as documented."
#endif

#include <Eigen/Core>
#pragma GCC visibility pop

#include <tomographer/tomographer_version.h>
#include <tomographer/tools/cxxutil.h>
#include <tomographer/tools/loggers.h>

#include <tomographerpy/pylogger.h>


// get a demangle() function from Boost, either with boost::core::demangle() (boost >=
// 1.56) or boost::units::detail::demangle() (boost before that)
#include <boost/version.hpp>
#if BOOST_VERSION >= 105600
#include <boost/core/demangle.hpp>
#else
#include <boost/units/detail/utility.hpp>
namespace boost { namespace core {  using boost::units::detail::demangle; } }
#endif


namespace tpy {

//! Real type for template arguments (double)
typedef double RealType;
//! Integer type for template arguments (int)
typedef int CountIntType;

//! Shorthand, a 1-D Eigen::Matrix of RealType's
typedef Eigen::Matrix<RealType, Eigen::Dynamic, 1> RealVectorType;
//! Shorthand, a 2-D Eigen::Matrix of RealType's
typedef Eigen::Matrix<RealType, Eigen::Dynamic, Eigen::Dynamic> RealMatrixType;
//! Shorthand, a 1-D Eigen::Matrix of std::complex<RealType>'s
typedef Eigen::Matrix<std::complex<RealType>, Eigen::Dynamic, 1> CplxVectorType;
//! Shorthand, a 2-D Eigen::Matrix of std::complex<RealType>'s
typedef Eigen::Matrix<std::complex<RealType>, Eigen::Dynamic, Eigen::Dynamic> CplxMatrixType;

//! Shorthand, a 1-D Eigen::Matrix of CountIntType's
typedef Eigen::Matrix<CountIntType, Eigen::Dynamic, 1> CountIntVectorType;

/** \brief Import tomographer definitions into other Python modules
 *
 * If you write your own C++/Python module, then make sure you call \a
 * import_tomographer() towards the beginning of your initialization function.  This
 * function also ensures that the same versions of tomographer and of pybind11 are being
 * used between the compiled tomographer module and the available tomographerpy headers.
 *
 * In particular, this function make sure that the version of the \c tomographer Python
 * module is exactly the same as the headers which are currently available.  Same for \a
 * pybind11.
 *
 * \return The \a py::module object representing the \a tomographer Python module.
 */
inline py::module import_tomographer()
{
  auto tomographer_module = py::module::import("tomographer");
  if (PyErr_Occurred() != NULL) {
    throw py::error_already_set();
  }
  const std::string module_tomographer_version =
    tomographer_module.attr("__version__").cast<std::string>();
  if (module_tomographer_version != TOMOGRAPHER_VERSION) {
    throw std::runtime_error(
        "Error: Version of compiled tomographer python module ("+module_tomographer_version +
        ") does not match version used to compile the current module (" + std::string(TOMOGRAPHER_VERSION)
        + "). If you updated tomographer, please recompile all dependent modules."
        ) ;
  }
  const std::string this_pybind11_ver =
    std::to_string(PYBIND11_VERSION_MAJOR) + std::string(".") +
    std::to_string(PYBIND11_VERSION_MINOR) + std::string(".") +
    std::to_string(PYBIND11_VERSION_PATCH) ;
  const std::string module_tomographer_pybind11_ver =
    tomographer_module.attr("version").attr("compile_info").attr("get")("pybind11", "").cast<std::string>();
  if (module_tomographer_pybind11_ver != this_pybind11_ver) {
    throw std::runtime_error(
        "Error: Compiled tomographer's version of PyBind11 (" + module_tomographer_pybind11_ver +
        ") does not match version used to compile the current module (" + this_pybind11_ver
        + "). Please recompile all modules using the same PyBind11 version."
        ) ;
  }
  return tomographer_module;
}



} // namespace tpy



#endif
