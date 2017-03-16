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

#ifndef TOMOGRAPHER_DENSEDM_DMTYPES_H
#define TOMOGRAPHER_DENSEDM_DMTYPES_H

#include <cstddef>
#include <cassert>

#include <Eigen/Eigen>

#include <tomographer/tools/cxxutil.h> // StaticOrDynamic, TOMOGRAPHER_ENABLED_IF

/** \file dmtypes.h
 * \brief C++ types for describing dense density matrices in various parameterizations
 *
 */


namespace Tomographer {
namespace DenseDM {


/** \brief C++ types needed to store a quantum state as a dense matrix.
 *
 * The \ref DMTypes template class stores compile-time and run-time information about the
 * types used to represent density operators as dense Eigen objects (storing a matrix
 * explicitly as a list of coefficients in memory, as you'd expect), as well as
 * remembering the dimension of the corresponding Hilbert space.  The latter may be fixed
 * either at compile-time or set dynamically at run-time.
 *
 * If a compile-time dimension is given, then the corresponding Eigen types (see \ref
 * Eigen::Matrix) will be allocated on the stack instead of the heap.  For small matrix
 * sizes, you should get a performance increase.
 *
 *
 * \tparam FixedDim The dimension of the Hilbert space, if known at compile-time, or \ref
 *         TutorialMatrixClass "Eigen::Dynamic".  If \ref TutorialMatrixClass
 *         "Eigen::Dynamic" is specified here, the dimension can be specified at run-time
 *         to the class's constructor.
 *
 * \tparam RealScalar The type to use as real scalar.  The default, \c double, should be
 *         sufficient in most (if not all) cases, but you could try to speed up
 *         computations by using \c float, or be more precise by using <code>long
 *         double</code>.
 *
 * This class provides several types which are to be used to store matrices acting on the
 * Hilbert space, a vector of real coefficients which can store an \ref pageParamsX of
 * Hermitian such matrices, and a vector of real coefficients of length corresponding to
 * the number of degrees of freedom of a density matrix (e.g. to store a \ref
 * pageParamsA).  For each such type, there is an initialization method (e.g. \ref
 * initMatrixType()), which initializes the corresponding object to zero.
 *
 * Whenever such a type is needed, one calls for example
 * \code
 *   int dim = ...;
 *   DMTypes<Eigen::Dynamic> dmt(dim);
 *   ...
 *   DMTypes<Eigen::Dynamic>::MatrixType matrix(dmt.initMatrixType());
 *   ... use `matrix' as a dim*dim complex matrix ...
 * \endcode
 *
 * - MatrixType : type used to store a matrix acting on the Hilbert space of dimension
 *   \ref dim().  This is simply a complex dim()-by-dim() matrix.
 *
 * - VectorParamType : a vector of <code>dim()*dim()</code> real entries, as needed for
 *   example to store the \ref pageParamsX of a Hermitian matrix.
 *
 * - VectorParamNdofType : a vector of <code>dim()*dim()-1</code> real entries, as needed
 *   for example to store the \ref pageParamsA of a density matrix.
 *
 * To each of these type corresponds a const-reference type, which is useful to specify
 * function arguments.
 */
template<int FixedDim_, typename RealScalar_ = double, int MaxFixedDim_ = FixedDim_>
TOMOGRAPHER_EXPORT struct DMTypes
{

  // assert: either FixedDim_ is dynamic (in which case MaxFixedDim_ can be anything, or
  // FixedDim_ is static, and has to be equal to MaxFixedDim_ (or less than is also ok but
  // no idea why you'd do it).
  TOMO_STATIC_ASSERT_EXPR((FixedDim_ == Eigen::Dynamic) ||
                          (FixedDim_ <= MaxFixedDim_)) ;

  //! Whether the dimension is specified dynamically at run-time or statically at compile-time
  static constexpr bool IsDynamicDim = (FixedDim_ == Eigen::Dynamic);
  //! The fixed dimension of the quantum system, or \ref TutorialMatrixClass "Eigen::Dynamic"
  static constexpr int FixedDim = FixedDim_;
  //! The square of the dimension of the quantum system, or \ref TutorialMatrixClass "Eigen::Dynamic"
  static constexpr int FixedDim2 = ((FixedDim!=Eigen::Dynamic) ? FixedDim*FixedDim : Eigen::Dynamic);
  /** \brief The square of the dimension of the quantum system minus one, or \ref
   *         TutorialMatrixClass "Eigen::Dynamic"
   */
  static constexpr int FixedNdof = ((FixedDim2!=Eigen::Dynamic) ? FixedDim2-1 : Eigen::Dynamic);

  //! Real scalar type, given in template parameter.  Usually \c double is fine
  typedef RealScalar_ RealScalar;
  //! The corresponding complex scalar type
  typedef std::complex<RealScalar> ComplexScalar;

  //! Utility to initialize a complex number using the current scalar type
  static inline ComplexScalar cplx(RealScalar a, RealScalar b)
  {
    return ComplexScalar(a, b);
  }

  //! Matrix type, to store the density operator as a dense matrix
  typedef Eigen::Matrix<ComplexScalar, FixedDim, FixedDim,
                        Eigen::Matrix<ComplexScalar,FixedDim,FixedDim>::Options, // the default options
                        MaxFixedDim_, MaxFixedDim_>  MatrixType;
  //! Shorthand for a const reference to a MatrixType-like Eigen object
  typedef const Eigen::Ref<const MatrixType> &  MatrixTypeConstRef;

  //! Real vector with dim*dim elements
  typedef Eigen::Matrix<RealScalar, FixedDim2, 1> VectorParamType;
  //! Shorthand for a const reference to a VectorParamType-like Eigen object
  typedef const Eigen::Ref<const VectorParamType> &  VectorParamTypeConstRef;

  /** \brief Real vector with dim*dim-1 elements
   *
   * The number of elements (\f$ \texttt{dim}^2-1\f$) corresponds to the number of degrees
   * of freedom of a density matrix of dimension \f$ \texttt{dim} \f$.
   */
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

  /** \brief get the dimension of the quantum system (dimension of the Hilbert space)
   *
   * If the dimension was fixed at compile-time, then that value is directly returned (and
   * the compiler should be able to optimize this).  If the dimension is dynamically set
   * at run-time, then we return that stored value.
   */
  inline std::size_t dim() const { return _dim.value(); }

  /** \brief get the square of the dimension of the quantum system
   *
   * Equivalently, this is the number of complex matrix entries, or the number of real
   * coefficients needed to specify a Hermitian matrix.  It is the size of a \ref
   * VectorParamType.
   */
  inline std::size_t dim2() const { return _dim.value()*_dim.value(); }

  /** \brief get the square of the dimension of the quantum system, minus one
   *
   * Equivalently, this is the number of degrees of freedom of a density matrix.  It is
   * the size of a \ref VectorParamNdofType.
   */
  inline std::size_t ndof() const { return dim2()-1; }


  /** \brief Zero initializer for a MatrixType [implementation for static dimension]
   *
   * This function returns an initializer, which when used as constructor argument to \ref
   * MatrixType, initializes it with a zero \f$ \texttt{dim}\times\texttt{dim} \f$ matrix.
   */
  TOMOGRAPHER_ENABLED_IF(!IsDynamicDim)
  inline typename MatrixType::ConstantReturnType initMatrixType() const
  {
    return MatrixType::Zero();
  }
  /** \brief Zero initializer for MatrixType [implementation for dynamic dimension]
   *
   * This function returns an initializer, which when used as constructor argument to \ref
   * MatrixType, initializes it with a zero \f$ \texttt{dim}\times\texttt{dim} \f$ matrix.
   */
  TOMOGRAPHER_ENABLED_IF(IsDynamicDim)
  inline typename MatrixType::ConstantReturnType initMatrixType() const
  {
    return MatrixType::Zero(_dim.value(), _dim.value());
  }

  /** \brief Zero initializer for a VectorParamType [implementation for static dimension]
   *
   * This function returns an initializer, which when used as constructor argument to \ref
   * VectorParamType, initializes it to a zero vector of \f$ \texttt{dim}^2 \f$ entries.
   */
  TOMOGRAPHER_ENABLED_IF(!IsDynamicDim)
  inline typename VectorParamType::ConstantReturnType initVectorParamType() const
  {
    return VectorParamType::Zero();
  }
  /** \brief Zero initializer for VectorParamType [implementation for dynamic dimension]
   *
   * This function returns an initializer, which when used as constructor argument to \ref
   * VectorParamType, initializes it to a zero vector of \f$ \texttt{dim}^2 \f$ entries.
   */
  TOMOGRAPHER_ENABLED_IF(IsDynamicDim)
  inline typename VectorParamType::ConstantReturnType initVectorParamType() const
  {
    return VectorParamType::Zero(dim2());
  }

  /** \brief Zero initializer for a VectorParamNdofType [implementation for static dimension]
   *
   * This function returns an initializer, which when used as constructor argument to \ref
   * VectorParamNdofType, initializes it to a zero vector of \f$ \texttt{dim}^2-1 \f$
   * entries.
   */
  TOMOGRAPHER_ENABLED_IF(!IsDynamicDim)
  inline typename VectorParamNdofType::ConstantReturnType initVectorParamNdofType() const
  {
    return VectorParamNdofType::Zero();
  }
  /** \brief Zero initializer for VectorParamNdofType [implementation for dynamic dimension]
   *
   * This function returns an initializer, which when used as constructor argument to \ref
   * VectorParamNdofType, initializes it to a zero vector of \f$ \texttt{dim}^2-1 \f$
   * entries.
   */
  TOMOGRAPHER_ENABLED_IF(IsDynamicDim)
  inline typename VectorParamNdofType::ConstantReturnType initVectorParamNdofType() const
  {
    return VectorParamNdofType::Zero(ndof());
  }


private:
  const Tools::StaticOrDynamic<std::size_t, IsDynamicDim, (std::size_t)FixedDim> _dim;
};





} // namespace DenseDM
} // namespace Tomographer








#endif
