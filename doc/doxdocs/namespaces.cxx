/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
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


/** \namespace Tomographer
 * \brief Base namespace for the \c Tomographer project
 *
 * This namespace contains the public API for the Tomographer project template library.
 */



/** \namespace Tomographer::tomo_internal
 * \internal 
 *
 * \brief Internal namespace for internal definitions.
 *
 * Do not access&mdash;the API might change at anytime!
 *
 */


/** \namespace Tomographer::Tools
 *
 * \brief Various useful tools.
 *
 */


/** \namespace Tomographer::MAT
 *
 * \brief Utilities and helpers for reading MATLAB \c .mat files
 *
 * The class \ref File represents an open MATLAB file from which you can extract
 * data. Variables inside the file are represented with \ref Var objects.
 *
 * The actual data can be extracted to some native C++ representation using \ref
 * Var::value<T>() for some selected C++ types. Example:
 *
 * \code
 *   Tomographer::MAT::File matfile("myfile.mat");
 *
 *   // find the variable in file named `x'
 *   Tomographer::MAT::Var var_x = matfile.var("x");
 *
 *   // get `x' as C++ double. If `x' in the file is not convertible to double, or if
 *   // it is not a scalar, then a VarTypeError is thrown.
 *   double var_x_value = var.value<double>();
 *
 *   // find variable `m' and get it as an Eigen::MatrixXd. Again, if the type is
 *   // incompatible a VarTypeError is thrown.
 *   Tomographer::MAT::Var var = matfile.var("m");
 *   Eigen::MatrixXd matrix = var.value<Eigen::MatrixXd>();
 * \endcode
 *
 * You can extend this mechanism easily to essentially any C++ type. Just specialize the
 * \ref VarValueDecoder<T> template for your C++ type.
 */


/** \namespace Tomographer::SphCoords
 *
 * Utilities for hyperspherical coordinates. See also the corresponding theory page \ref
 * pageParamsSphericalCoords.
 */
