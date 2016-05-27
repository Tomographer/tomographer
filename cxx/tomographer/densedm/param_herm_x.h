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

#ifndef HERM_PARAM_X_H
#define HERM_PARAM_X_H

/** \file param_herm_x.h
 *
 * \brief Tools for parameterizing hermitian matrices with the \ref pageParamsX.
 *
 */

#include <cmath>
#include <complex>

#include <boost/math/constants/constants.hpp>

#include <Eigen/Core>


namespace Tomographer {


/** \brief Get the Hermitian matrix parameterized by the "X-parameter" vector \c x
 *
 * This calculates the hermitian matrix which is parameterized by \c x.
 * See \ref pageParamsX.
 */
template<bool OnlyLowerTri=false, typename Derived1=Eigen::MatrixXd, typename Derived2=Eigen::MatrixXd>
inline void param_x_to_herm(Eigen::MatrixBase<Derived1>& Herm, const Eigen::DenseBase<Derived2>& x)
{
  { using namespace Eigen; EIGEN_STATIC_ASSERT_LVALUE(Derived1); }

  const int dim = Herm.rows();
  const int dimtri = dim*(dim-1)/2;
  eigen_assert(dim == Herm.cols()); // assert Herm is (dim x dim)
  eigen_assert(x.rows() == dim*dim && x.cols() == 1); // assert x is (dim*dim x 1)

  typedef typename Derived1::Scalar Scalar;
  typedef typename Eigen::NumTraits<Scalar>::Real RealScalar;
  
  Herm.diagonal().real() = x.block(0,0,dim,1);
  Herm.diagonal().imag().setZero();
  
  int n, m;
  for (n = 1; n < dim; ++n) {
    for (m = 0; m < n; ++m) {
      const int k = dim + n*(n-1)/2 + m;
      const int l = dimtri + k;
      Herm(n,m) = boost::math::constants::half_root_two<RealScalar>() * Scalar(x(k), x(l));
      if (!OnlyLowerTri) {
        // complex conj. on opposite triangular part
        Herm(m,n) = boost::math::constants::half_root_two<RealScalar>() * Scalar(x(k), -x(l));
      }
    }
  }
};


/** \brief Get the X-parameterization corresponding to a given hermitian matrix
 *
 * See also \ref pageParamsX and \ref param_x_to_herm().
 * 
 * \note This function only accesses lower triangular part of \c Herm.
 *
 * \todo Currently, we can't pass an expression as second parameter here. So use \ref
 *       Eigen::Ref instead to allow for that, too...
 */
template<typename Derived1, typename Derived2>
inline void param_herm_to_x(Eigen::DenseBase<Derived1>& x, const Eigen::MatrixBase<Derived2>& Herm)
{
  { using namespace Eigen; EIGEN_STATIC_ASSERT_LVALUE(Derived1); }

  const int dim = Herm.rows();
  const int dimtri = dim*(dim-1)/2;
  eigen_assert(dim == Herm.cols()); // assert Herm is (dim x dim)
  eigen_assert(x.rows() == dim*dim && x.cols() == 1); // assert x is (dim*dim x 1)

  typedef typename Derived1::Scalar Scalar;
  typedef typename Eigen::NumTraits<Scalar>::Real RealScalar;
  
  x.block(0,0,dim,1) = Herm.real().diagonal();

  int n, m;
  for (n = 1; n < dim; ++n) {
    for (m = 0; m < n; ++m) {
      const int k = dim + n*(n-1)/2 + m;
      const int l = dimtri + k;
      x(k) = Herm(n,m).real() * boost::math::constants::root_two<RealScalar>();
      x(l) = Herm(n,m).imag() * boost::math::constants::root_two<RealScalar>();
    }
  }
}


} // namespace Tomographer


#endif
