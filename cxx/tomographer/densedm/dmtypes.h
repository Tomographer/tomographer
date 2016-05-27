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

#ifndef _TOMOGRAPHER_DENSEDM_DENSEDMTYPES_H
#define _TOMOGRAPHER_DENSEDM_DENSEDMTYPES_H

#include <cstddef>
#include <cassert>

#include <Eigen/Eigen>
#include <tomographer/tools/cxxutil.h> // static_or_dynamic, TOMOGRAPHER_ENABLED_IF

/** \file densedmtypes.h
 * \brief C++ types for describing dense density matrices in various parameterizations
 *
 */


namespace Tomographer {
namespace DenseDM {


/** \brief C++ types needed to store a quantum state as a dense matrix.
 *
 * \todo NEEDS DOC...........
 *
 * - The dimension is either fixed at compile-time (FixedDim given to positive value), or
 *   at runtime (FixedDim == Eigen::Dynamic (== -1)).
 *
 * Types provided:
 *
 * - MatrixType
 * - VectorParamType
 * - VectorParamNdofType
 *
 */
template<int FixedDim_, typename RealScalar_ = double>
struct DMTypes {

  //! Whether the dimension is specified dynamically at run-time or statically at compile-time
  static constexpr bool IsDynamicDim = (FixedDim_ == Eigen::Dynamic);
  //! The fixed dimension of the quantum system, or \ref Eigen::Dynamic
  static constexpr int FixedDim = FixedDim_;
  //! The square of the dimension of the quantum system, or \ref Eigen::Dynamic
  static constexpr int FixedDim2 = ((FixedDim!=Eigen::Dynamic) ? FixedDim*FixedDim : Eigen::Dynamic);
  /** \brief The square of the dimension of the quantum system minus one, or \ref
   *         Eigen::Dynamic
   */
  static constexpr int FixedNdof = ((FixedDim2!=Eigen::Dynamic) ? FixedDim2-1 : Eigen::Dynamic);


  //! Real scalar type, given in template parameter.  Usually \c double is fine
  typedef RealScalar_ RealScalar;
  //! The corresponding complex scalar type
  typedef std::complex<RealScalar> ComplexScalar;

  //! Matrix type, to store the density operator as a dense matrix
  typedef Eigen::Matrix<ComplexScalar, FixedDim, FixedDim>  MatrixType;
  //! Shorthand for a const reference to a MatrixType-like Eigen object
  typedef const Eigen::Ref<const MatrixType> &  MatrixTypeConstRef;

  //! Real dim*dim Vector
  typedef Eigen::Matrix<RealScalar, FixedDim2, 1> VectorParamType;
  //! Shorthand for a const reference to a VectorParamType-like Eigen object
  typedef const Eigen::Ref<const VectorParamType> &  VectorParamTypeConstRef;

  /** \brief Real dim*dim-1 Vector */
  typedef Eigen::Matrix<RealScalar, FixedNdof, 1> VectorParamNdofType;
  //! Shorthand for a const reference to a VectorParamNdofType-like Eigen object
  typedef const Eigen::Ref<const VectorParamNdofType> &  VectorParamNdofTypeConstRef;
  


  /** \brief Constructor [only for statically-fixed dim]
   *
   */
  TOMOGRAPHER_ENABLED_IF(!IsDynamicDim)
  inline DMTypes()
  : _dim()
  {
  }

  /** \brief Constructor [works for both static or dynamic dim]
   *
   * The dimension fixed at run-time must be specified here, and cannot later be changed.
   *
   * If the dimension was fixed at compile-time with \c FixedDim_, then \c d must equal \c
   * FixedDim_ (or you'll get an assert failure).
   *
   */
  inline DMTypes(std::size_t d)
    : _dim(d)
  {
  }

  inline std::size_t dim() const { return _dim.value(); }
  inline std::size_t dim2() const { return _dim.value()*_dim.value(); }
  inline std::size_t ndof() const { return dim2()-1; }


  /** \brief Zero initializer for a MatrixType [implementation for static dimension]
   *
   * This function returns an initializer, which when used as constructor argument to \ref
   * MatrixType, initializes it with a zero matrix.
   */
  TOMOGRAPHER_ENABLED_IF(!IsDynamicDim)
  inline typename MatrixType::ConstantReturnType initMatrixType() const
  {
    return MatrixType::Zero();
  }
  /** \brief Zero initializer for MatrixType [implementation for dynamic dimension]
   *
   * This function returns an initializer, which when used as constructor argument to \ref
   * MatrixType, initializes it with a zero matrix.
   */
  TOMOGRAPHER_ENABLED_IF(IsDynamicDim)
  inline typename MatrixType::ConstantReturnType initMatrixType() const
  {
    return MatrixType::Zero(_dim.value(), _dim.value());
  }

  /** \brief Zero initializer for a VectorParamType [implementation for static dimension]
   *
   * This function returns an initializer, which when used as constructor argument to \ref
   * VectorParamType, initializes it with a zero matrix.
   */
  TOMOGRAPHER_ENABLED_IF(!IsDynamicDim)
  inline typename VectorParamType::ConstantReturnType initVectorParamType() const
  {
    return VectorParamType::Zero();
  }
  /** \brief Zero initializer for VectorParamType [implementation for dynamic dimension]
   *
   * This function returns an initializer, which when used as constructor argument to \ref
   * VectorParamType, initializes it with a zero matrix.
   */
  TOMOGRAPHER_ENABLED_IF(IsDynamicDim)
  inline typename VectorParamType::ConstantReturnType initVectorParamType() const
  {
    return VectorParamType::Zero(dim2());
  }

  /** \brief Zero initializer for a VectorParamNdofType [implementation for static dimension]
   *
   * This function returns an initializer, which when used as constructor argument to \ref
   * VectorParamNdofType, initializes it with a zero matrix.
   */
  TOMOGRAPHER_ENABLED_IF(!IsDynamicDim)
  inline typename VectorParamNdofType::ConstantReturnType initVectorParamNdofType() const
  {
    return VectorParamNdofType::Zero();
  }
  /** \brief Zero initializer for VectorParamNdofType [implementation for dynamic dimension]
   *
   * This function returns an initializer, which when used as constructor argument to \ref
   * VectorParamNdofType, initializes it with a zero matrix.
   */
  TOMOGRAPHER_ENABLED_IF(IsDynamicDim)
  inline typename VectorParamNdofType::ConstantReturnType initVectorParamNdofType() const
  {
    return VectorParamNdofType::Zero(ndof());
  }


private:
  const Tools::static_or_dynamic<std::size_t, IsDynamicDim, (std::size_t)FixedDim> _dim;
};





} // namespace DenseDM
} // namespace Tomographer








#endif
