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

#include <tomographer/tools/cxxutil.h> // static_or_dynamic


// // heavily inspired by Eigen::Replicate [Eigen/src/Core/Replicate.h]
// namespace Eigen {
// namespace internal {
// template<typename MatrixType,int RowFactor,int ColFactor>
// struct traits<Tomographer::DenseDM::internal::ParamHermToX<MatrixType,RowFactor,ColFactor> >
//   : traits<MatrixType>
// {
//   typedef Eigen::NumTraits<typename MatrixType::Scalar>::Real Scalar;
//   typedef typename traits<MatrixType>::StorageKind StorageKind;
//   typedef typename traits<MatrixType>::XprKind XprKind;
//   enum {
//     Factor = (RowFactor==Dynamic || ColFactor==Dynamic) ? Dynamic : RowFactor*ColFactor
//   };
//   typedef typename nested<MatrixType,1>::type MatrixTypeNested;
//   typedef typename remove_reference<MatrixTypeNested>::type _MatrixTypeNested;
//   enum {
//     RowsAtCompileTime = (int(MatrixType::RowsAtCompileTime)==Eigen::Dynamic
//                          ? Eigen::Dynamic
//                          : MatrixType::RowsAtCompileTime*MatrixType::RowsAtCompileTime ),
//     ColsAtCompileTime = 1,
//     MaxRowsAtCompileTime = RowsAtCompileTime,
//     MaxColsAtCompileTime = 1,
//     IsRowMajor = MaxRowsAtCompileTime==1 && MaxColsAtCompileTime!=1 ? 1
//                : MaxColsAtCompileTime==1 && MaxRowsAtCompileTime!=1 ? 0
//                : (MatrixType::Flags & RowMajorBit) ? 1 : 0,
//     Flags = (_MatrixTypeNested::Flags & HereditaryBits & ~RowMajorBit) | (IsRowMajor ? RowMajorBit : 0),
//     CoeffReadCost = _MatrixTypeNested::CoeffReadCost * Eigen::NumTraits<typename MatrixType::Scalar>::MulCost
//   };
// };
// } // namespace internal
// } // namespace Eigen


// namespace Tomographer {
// namespace DenseDM {
// namespace internal {

// template<typename MatrixType>
// class ParamHermToX : public Eigen::MatrixBase<ParamHermToX<MatrixType> >
// {
//   typedef typename internal::traits<ParamHermToX>::Scalar RealScalar;
//   typedef typename internal::traits<ParamHermToX>::MatrixTypeNested MatrixTypeNested;
//   typedef typename internal::traits<ParamHermToX>::_MatrixTypeNested _MatrixTypeNested;
// public:

//   typedef typename MatrixBase<ParamHermToX> Base;
//   EIGEN_DENSE_PUBLIC_INTERFACE(ParamHermToX)
  
//   template<typename OriginalMatrixType>
//   inline explicit ParamHermToX(const OriginalMatrixType& H)
//     : _H(H), _dimtri(H.rows()*(H.rows()-1)/2)
//   {
//     EIGEN_STATIC_ASSERT((internal::is_same<typename internal::remove_const<MatrixType>::type,OriginalMatrixType>::value),
//                         THE_MATRIX_OR_EXPRESSION_THAT_YOU_PASSED_DOES_NOT_HAVE_THE_EXPECTED_TYPE) ;
//     eigen_assert(H.rows() == H.cols())
//   }
  
//   inline Index rows() const { return _H.rows()*_H.rows(); }
//   inline Index cols() const { return 1; }

//   inline RealScalar coeff(Index rowId, Index colId) const
//   {
//     eigen_assert(colId == 0);

//     if (rowId < _H.rows()) {
//       return _H.coeff(rowId, rowId).real();
//     }
//     rowId -= _H.rows();
//     if (rowId < _dimtri.value()) {
//       return _H.coeff(......????)
//     }
//
//     ............ MAYBE NOT THE WAY TO GO .............

//     // try to avoid using modulo; this is a pure optimization strategy
//     const Index actual_row  = internal::traits<MatrixType>::RowsAtCompileTime==1 ? 0
//       : RowFactor==1 ? rowId
//       : rowId%m_matrix.rows();
//     const Index actual_col  = internal::traits<MatrixType>::ColsAtCompileTime==1 ? 0
//       : ColFactor==1 ? colId
//       : colId%m_matrix.cols();
    
//     return m_matrix.coeff(actual_row, actual_col);
//   }

//   const _MatrixTypeNested& nestedExpression() const
//   { 
//     return m_matrix;
//   }

// protected:
//   MatrixTypeNested _H;
//   const static_or_dynamic<Index,(MatrixType::RowsAtCompileTime==Eigen::Dynamic),
//                           Index(MatrixType::RowsAtCompileTime*(MatrixType::RowsAtCompileTime-1)/2> _dimtri;
// };

// } // namespace internal 
// } // namespace DenseDM
// } // namespace Tomographer


// ================================================================================


namespace Tomographer {
namespace DenseDM {

/** \brief Convert hermitian matrices to vectors via their \ref pageParamX
 *
 */
template<typename DenseDMTypes_>
class ParamX {
public:
  typename DenseDMTypes_ DenseDMTypes;
  typename DenseDMTypes::MatrixType MatrixType;
  typename DenseDMTypes::MatrixTypeConstRef MatrixTypeConstRef;
  typename DenseDMTypes::VectorParamType VectorParamType;
  typename DenseDMTypes::VectorParamTypeConstRef VectorParamTypeConstRef;
  typename DenseDMTypes::RealScalar RealScalar;
  typename DenseDMTypes::ComplexScalar ComplexScalar;
  typename typename MatrixType::Index IndexType;

  /** \brief Constructor.  Just give it the DenseDMTypes instance.
   *
   */
  ParamX(DenseDMTypes dmt) : _dmt(dmt) { }

  /** \brief Get the X-parameterization corresponding to a given hermitian matrix
   *
   * See also \ref pageParamsX and \ref param_x_to_herm().
   * 
   * \note This function only accesses lower triangular part of \c Herm.
   *
   * \todo Currently, we can't pass an expression as second parameter here. So use \ref
   *       Eigen::Ref instead to allow for that, too...
   */
  inline VectorParamType HermToX(MatrixTypeConstRef Herm)
  {
    // hope RVO kicks in
    VectorParamType x(_dmt.initVectorParamType());
    const IndexType dimtri = (_dmt.dim2 - _dmt.dim())/2;

    eigen_assert(_dmt.dim() == Herm.cols()); // assert Herm is (dim x dim)
    
    x.block(0,0,_dmt.dim(),1) = Herm.real().diagonal();

    IndexType k = _dmt.dim();
    IndexType n, m;
    for (n = 1; n < _dmt.dim(); ++n) {
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
  inline MatrixType XToHerm(VectorParamTypeConstRef x)
  {
    // should be optimized by compiler via RVO
    MatrixType Herm(_dmt.initMatrixType());
    
    const IndexType dimtri = (_dmt.dim2()-_dmt.dim())/2;
    eigen_assert(x.rows() == _dmt.dim2() && x.cols() == 1); // assert x is (dim*dim x 1)

    Herm.diagonal().real() = x.block(0,0,_dmt.dim(),1);
    Herm.diagonal().imag().setZero();
  
    IndexType k = _dmt.dim();
    IndexType n, m;
    for (n = 1; n < dim; ++n) {
      for (m = 0; m < n; ++m) {
        Herm(n,m) = boost::math::constants::half_root_two<RealScalar>() * Scalar(x(k), x(dimtri + k));
        if (!OnlyLowerTri) {
          // complex conj. on opposite triangular part
          Herm(m,n) = boost::math::constants::half_root_two<RealScalar>() * Scalar(x(k), -x(dimtri + k));
        }
        ++k;
      }
    }
    return Herm;
  }
  
protected:
  DenseDMTypes _dmt;
};




} // namespace DenseDM
} // namespace Tomographer


#endif
