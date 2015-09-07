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

#ifndef CHECK_DERIVATIVES_H
#define CHECK_DERIVATIVES_H

#include <cstddef>
#include <iostream>

#include <Eigen/Core>

/** \file check_derivatives.h
 *
 * \brief Tools to check numerical derivatives.
 *
 * See \ref Tomographer::Tools::check_derivatives().
 *
 */

namespace Tomographer
{
namespace Tools
{

/** \brief Check given derivatives against numerically-calculated finite differences
 *
 * This debugging utility is useful for making sure an analytical expression for the
 * derivatives of a function is correct.
 *
 * The function may be any general vector function, i.e.
 * \f[
 *      f : \mathbb{R}^{\texttt{xdims}} \rightarrow \mathbb{R}^{\texttt{valdims}} ,
 * \f]
 * where we denote by \f$ f_i \f$ the \a i -th component of the value calculated by \f$ f
 * \f$.
 *
 * \param derivatives the values claimed to be the derivatives of \a fn at the given \a
 * point, and which are to be checked against numerically-calculated finite
 * differences. \a derivatives is an Eigen \a Array type such that
 * \f[
 *     \texttt{derivatives(i,k)} = \frac{\partial f_i}{\partial x_k}
 * \f]
 *
 * \param point the point at which to calculate the function derivatives. It should be a
 * column vector; the number of items in this vector (\a xdims above) should match the
 * number of columns in \a derivatives.
 *
 * \param fn a callable which calculates the values of the function at given points. The
 * first argument is a reference to a vector which should be filled with the function
 * values, the second argument is the point at which to evaluate the function. The object
 * or function \a fn will be called as
 * \code
 *    Eigen::VectorXd x(xdims) = ...;
 *    Eigen::VectorXd val(valdims);
 *    fn(val, x);
 *    // val[i] should now store the value of f_i(x)
 * \endcode
 * 
 * \param valdims the number of values calculated by \a fn, or, equivalently, the
 * dimension of the output space of \f$ f \f$.
 *
 * \param delta the step to deviate from \a point to calculate a finite difference
 * derivative in a particular direction.
 *
 * \param tol the tolerance we should accept for the difference between the numerically
 * calculated derivative with finite differences, and the given value in \a derivatives.
 *
 * \param error_stream If the derivatives don't match, a report is also written to the
 * given stream \a error_stream, which defaults to <em>std::cerr</em>.
 *
 * \return This function returns \c true if all checked derivatives were within the given
 * tolerance, or \c false if a failure was detected.
 */
template<typename Der1, typename Der2, typename fnType, typename ErrorStream>
bool check_derivatives(const Eigen::ArrayBase<Der1> & derivatives, const Eigen::MatrixBase<Der2> & point,
                       fnType fn, std::size_t valdims,
                       typename Eigen::MatrixBase<Der2>::Scalar delta = 1e-6,
                       typename Eigen::MatrixBase<Der1>::Scalar tol = 1e-6,
		       ErrorStream & error_stream = std::cerr
                       )
{
  bool ok = true;

  const std::size_t xdims = derivatives.cols();
  eigen_assert(point.rows() == (int)xdims);
  eigen_assert(point.cols() == (int)1);
  eigen_assert(derivatives.rows() == (int)valdims);

  Eigen::VectorXd val0(valdims);
  Eigen::VectorXd dval1(valdims);
  Eigen::VectorXd dvalFromDer(valdims);

  Eigen::VectorXd pt2(point.rows());

  // calculate the base point
  fn(val0, point);

  std::size_t i;
  for (i = 0; i < xdims; ++i) {
    // numerically calculate the differences ...

    pt2 = point;
    pt2(i) += delta;

    fn(dval1, pt2);
    dval1 -= val0;

    dvalFromDer = delta * derivatives.matrix().col(i);//derivatives.matrix() * (delta*dir);

    double thediff = (dval1 - dvalFromDer).norm();

    if (thediff/delta > tol ) {
      // Error in the derivative

      // direction in which we probed, for the error_stream
      Eigen::VectorXd dir = Eigen::VectorXd::Zero(xdims);
      dir(i) = 1.0;

      ok = false;
      error_stream
	<< "Error in derivative check: Derivative wrong in direction\n"
	<< "dir = " << dir.transpose() << "   [basis vector #"<<i<<"]\n"
	<< "\tpoint = \t" << point.transpose() << "\n"
	<< "\tval0  = \t" << val0.transpose() << "\n"
	<< "\tdval1 = \t" << dval1.transpose() << "\n"
	<< "\tdvalFromDer = \t"<<dvalFromDer.transpose() << "\n"
	<< "\tderivative in this direction =\n\t\t\t\t" << derivatives.transpose().block(i,0,1,valdims) << "\n"
	<< "--> difference in p2-points: \t" << thediff << "\n"
	<< "--> difference in derivatives: \t" << thediff/delta << "\n\n";
    }
  }

  return ok;
}



} // Tools
} // Tomographer



#endif
