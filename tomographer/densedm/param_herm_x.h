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

#ifndef TOMOGRAPHER_DENSEDM_PARAM_HERM_X_H
#define TOMOGRAPHER_DENSEDM_PARAM_HERM_X_H



/** \file param_herm_x.h
 *
 * \brief Tools for parameterizing hermitian matrices with the \ref pageParamsX.
 *
 */

#include <cmath>
#include <complex>

#include <boost/math/constants/constants.hpp>
#include <boost/serialization/serialization.hpp>

#include <Eigen/Core>

#include <tomographer/tools/cxxutil.h> // static_or_dynamic, tomographer_assert()



namespace Tomographer {
namespace DenseDM {

/** \brief Convert hermitian matrices to vectors via their \ref pageParamsX
 *
 */
template<typename DMTypes_>
class TOMOGRAPHER_EXPORT ParamX
{
public:
  typedef DMTypes_ DMTypes;
  typedef typename DMTypes::MatrixType MatrixType;
  typedef typename DMTypes::MatrixTypeConstRef MatrixTypeConstRef;
  typedef typename DMTypes::VectorParamType VectorParamType;
  typedef typename DMTypes::VectorParamTypeConstRef VectorParamTypeConstRef;
  typedef typename DMTypes::RealScalar RealScalar;
  typedef typename DMTypes::ComplexScalar ComplexScalar;
  typedef Eigen::Index IndexType;

  /** \brief Constructor.  Just give it the DMTypes instance.
   *
   */
  ParamX(DMTypes dmt) : dim(dmt.dim()) { }

  /** \brief Get the X-parameterization corresponding to a given hermitian matrix
   *
   * See also \ref pageParamsX and \ref XToHerm().
   * 
   * \note This function only accesses lower triangular part of \c Herm.
   *
   */
  inline VectorParamType HermToX(MatrixTypeConstRef Herm) const
  {
    // don't construct a DMTypes object, because it could have a special constructor...
    //DMTypes dmt(dim);
    const Eigen::Index dim2 = dim*dim;

    // hope RVO kicks in
    VectorParamType x = VectorParamType::Zero(dim2,1);
    const Eigen::Index dimtri = (dim2 - dim)/2;

    tomographer_assert(dim == Herm.cols()); // assert Herm is (dim x dim)
    
    x.block(0,0,dim,1) = Herm.real().diagonal();

    IndexType k = dim;
    IndexType n, m;
    for (n = 1; n < dim; ++n) {
      for (m = 0; m < n; ++m) {
        x(k)          = Herm(n,m).real() * boost::math::constants::root_two<RealScalar>();
        x(dimtri + k) = Herm(n,m).imag() * boost::math::constants::root_two<RealScalar>();
        ++k;
      }
    }

    return x;
  }
  
  /** \brief Get the Hermitian matrix parameterized by the "X-parameter" vector \c x
   *
   * This calculates the hermitian matrix which is parameterized by \c x.
   * See \ref pageParamsX.
   */
  template<bool OnlyLowerTri = false>
  inline MatrixType XToHerm(VectorParamTypeConstRef x) const
  {
    const Eigen::Index dim2 = dim*dim;

    // should be optimized by compiler via RVO
    MatrixType Herm = MatrixType::Zero(dim,dim);
    
    const IndexType dimtri = (dim2-dim)/2;
    tomographer_assert(x.rows() == dim2 && x.cols() == 1); // assert x is (dim*dim x 1)

    Herm.diagonal().real() = x.block(0,0,dim,1);
    Herm.diagonal().imag().setZero();
  
    IndexType k = dim;
    IndexType n, m;
    for (n = 1; n < dim; ++n) {
      for (m = 0; m < n; ++m) {
        Herm(n,m) = boost::math::constants::half_root_two<RealScalar>() * ComplexScalar(x(k), x(dimtri + k));
        if (!OnlyLowerTri) {
          // complex conj. on opposite triangular part
          Herm(m,n) = boost::math::constants::half_root_two<RealScalar>() * ComplexScalar(x(k), -x(dimtri + k));
        }
        ++k;
      }
    }
    return Herm;
  }
  

  /** \brief Construct an invalid ParamX object -- only for use with Boost.Serialization
   */
  ParamX() : dim(-1) { }

protected:
  Eigen::Index dim;

private:
  friend boost::serialization::access;
  template<typename Archive>
  void serialize(Archive & a, unsigned int /* version */)
  {
    a & dim;
  }
};




} // namespace DenseDM
} // namespace Tomographer


#endif
