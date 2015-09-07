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

#ifndef _MATRQ_H
#define _MATRQ_H

#include <cstddef> // std::size_t
#include <complex>

#include <Eigen/Core>


/** \file matrq.h
 *
 * \brief Define types for quantum objects. See \ref Tomographer::MatrQ.
 *
 */



namespace Tomographer {



template<int FixedDim_ = Eigen::Dynamic, int FixedMaxParamList_ = Eigen::Dynamic,
         typename RealScalar_ = double, typename IntFreqType_ = int> struct MatrQ;

namespace tomo_internal
{
  /** \internal */
  template<typename Derived>
  struct matrq_traits { };

  /** \internal
   * \brief traits for the MatrQ class
   *
   * This is needed because \ref MatrQBase needs to access the derived class' \c FixedDim,
   * \c RealScalar, etc.
   *
   * See also \ref MatrQ.
   */
  template<int FixedDim_, int FixedMaxParamList_,
           typename RealScalar_, typename IntFreqType_>
  struct matrq_traits<MatrQ<FixedDim_, FixedMaxParamList_, RealScalar_, IntFreqType_> >
  {
    //! Compile-time fixed values
    enum {
      /** \internal
       * \brief A fixed dimension, or \ref Eigen::Dynamic */
      FixedDim = FixedDim_,
      /** \internal
       * \brief A fixed number of parameter vectors in a list, or \ref Eigen::Dynamic
       */
      FixedMaxParamList = FixedMaxParamList_
    };
    /** \internal
     * \brief The real scalar type of this data typeset
     */
    typedef RealScalar_ RealScalar;
    /** \internal
     * \brief The complex scalar type of this data typeset
     */
    typedef std::complex<RealScalar> ComplexScalar;
    /** \internal
     * \brief The integer frequency type of this data typeset
     *
     * This is used for the frequency counts in the tomography data, for example.
     */
    typedef IntFreqType_ IntFreqType;
  };

};


/** \brief Basic definitions for MatrQ
 *
 * This base class takes care of defining the basic types and constants.
 *
 * See also \ref MatrQ.
 */
template<typename Derived>
struct MatrQBase
{
  //! Values that are either fixed at compile-time, or set to \ref Eigen::Dynamic.
  enum {
    //! The dimension of the quantum system, or \ref Eigen::Dynamic
    FixedDim = tomo_internal::matrq_traits<Derived>::FixedDim,
    //! The square of the dimension of the quantum system, or \ref Eigen::Dynamic
    FixedDim2 = ((FixedDim!=Eigen::Dynamic) ? FixedDim*FixedDim : Eigen::Dynamic),
    /** \brief The square of the dimension of the quantum system minus one, or \ref
     *         Eigen::Dynamic
     */
    FixedNdof = ((FixedDim2!=Eigen::Dynamic) ? FixedDim2-1 : Eigen::Dynamic),
    /** \brief Maximum number of matrices in a list of \ref pageParamsX form, or \ref
     *         Eigen::Dynamic
     *
     * This is typically used for the list of POVM effects.
     */
    FixedMaxParamList = tomo_internal::matrq_traits<Derived>::FixedMaxParamList
  };
  //! Real scalar type (usually \c double)
  typedef typename tomo_internal::matrq_traits<Derived>::RealScalar RealScalar;
  //! Complex scalar type (usually \c std::complex<double>)
  typedef typename tomo_internal::matrq_traits<Derived>::ComplexScalar ComplexScalar;
  //! Integer type, used to count measurement frequencies. (\c int is fine)
  typedef typename tomo_internal::matrq_traits<Derived>::IntFreqType IntFreqType;

  /** \brief Complex dim x dim Matrix */
  typedef Eigen::Matrix<std::complex<RealScalar>, FixedDim, FixedDim> MatrixType;
  
  /** \brief Real dim*dim Vector */
  typedef Eigen::Matrix<RealScalar, FixedDim2, 1> VectorParamType;

  /** \brief Real dim*dim-1 Vector */
  typedef Eigen::Matrix<RealScalar, FixedNdof, 1> VectorParamNdofType;
  
  /** \brief dynamic Matrix with rows = dim*dim Vectors (row-major)
   * [maximum FixedMaxParamList rows, or Dynamic]
   */
  typedef Eigen::Matrix<RealScalar, Eigen::Dynamic, FixedDim2, Eigen::RowMajor,
                        FixedMaxParamList, FixedDim2> VectorParamListType;

  /** \brief dynamic Array of integers [maximum FixedMaxParamList entries or Dynamic]
   */
  typedef Eigen::Array<IntFreqType, Eigen::Dynamic, 1, 0 /*Options*/,
                       FixedMaxParamList, 1> FreqListType;

};


/** \brief Helper intermediate class for \ref MatrQ
 *
 * This helper class allows the definition of the initializer methods \ref
 * initMatrixType(), \ref initVectorParamType(), etc. via template specialization, without
 * having to specialize all the base definitions in e.g. \ref MatrQBase.
 *
 * This class stores the dimension given at runtime if \c has_fixed_dim is \c false, or,
 * if we have a fixed dimension, then provides statically fixed implementations of all its
 * member functions. In all cases, \ref dim() returns the dimension.
 *
 * In fact, we could have been directly defined the \c MatrQ class this way, but this
 * additional intermediary makes the specialization more flexible, i.e. one can specialize
 * the definition of \ref MatrQ not only a particular value of one of its template
 * parameters, but for a more global condition on its template parameters. (But which is
 * in fact not needed here.)
 */
template<typename Derived, bool has_fixed_dim>
struct MatrQBaseDimStore : public MatrQBase<Derived>
{
  //! The dimension of the quantum system. See \ref MatrQBase<Derived>::FixedDim
  inline std::size_t dim() const { return MatrQBase<Derived>::FixedDim; }
  //! The squared dimension of the quantum system. See \ref MatrQBase<Derived>::FixedDim2
  inline std::size_t dim2() const { return MatrQBase<Derived>::FixedDim2; }
  /** \brief The squared dimension of the quantum system, minus one. See \ref
   *         MatrQBase<Derived>::FixedNdof
   */
  inline std::size_t ndof() const { return MatrQBase<Derived>::FixedNdof; }

  /** Constructor. If a fixed dimension is given (\c has_fixed_dim is \c true), then we
   *  assert that the given dimension matches the compile-time fixed dimension \ref
   *  FixedDim (with \ref eigen_assert()). Otherwise, if the dimension is given at
   *  runtime, we store the dimension.
   *
   *  The value of the dimension can in all cases be obtained using the \ref dim()
   *  method.
   */
  MatrQBaseDimStore(std::size_t dim_)
  {
    (void)dim_; // don't warn of unused variable if eigen_assert() gets optimized out
    eigen_assert(MatrQBase<Derived>::FixedDim != Eigen::Dynamic && dim_ == dim());
  }

  /** \brief Zero initializer for a MatrixType
   *
   * This function returns an initializer, which when assigned to a \ref MatrixType,
   * should initialize it with zeros.
   */
  inline typename MatrQBase<Derived>::MatrixType::ConstantReturnType
  initMatrixType() const
  {
    return MatrQBase<Derived>::MatrixType::Zero();
  }

  /** \brief Zero initializer for a VectorParamType
   *
   * This function returns an initializer, which when assigned to a \ref VectorParamType,
   * should initialize it with zeros.
   */
  inline typename MatrQBase<Derived>::VectorParamType::ConstantReturnType
  initVectorParamType() const
  {
    return MatrQBase<Derived>::VectorParamType::Zero();
  }

  /** \brief Zero initializer for a VectorParamNdofType
   *
   * This function returns an initializer, which when assigned to a \ref
   * VectorParamNdofType, should initialize it with zeros.
   */
  inline typename MatrQBase<Derived>::VectorParamNdofType::ConstantReturnType
  initVectorParamNdofType() const
  {
    return MatrQBase<Derived>::VectorParamNdofType::Zero();
  }

  /** \brief Zero initializer for a VectorParamListType
   *
   * This function returns an initializer, which when assigned to a \ref
   * VectorParamListType, should initialize it with zeros.
   */
  inline typename MatrQBase<Derived>::VectorParamListType::ConstantReturnType
  initVectorParamListType(std::size_t len) const
  {
    return MatrQBase<Derived>::VectorParamListType::Zero(len,
                                                         MatrQBase<Derived>::FixedDim*MatrQBase<Derived>::FixedDim);
  }

  /** \brief Zero initializer for a FreqListType
   *
   * This function returns an initializer, which when assigned to a \ref FreqListType,
   * should initialize it with zeros.
   */
  inline typename MatrQBase<Derived>::FreqListType::ConstantReturnType
  initFreqListType(std::size_t len) const
  {
    return MatrQBase<Derived>::FreqListType::Zero(len);
  }
};

/** \brief Specialization of \ref MatrQBaseDimStore<Derived,has_fixed_dim> for dynamic sized types
 *
 * Mostly, we need to give good initializers to Eigen initializers for our types.
 */
template<typename Derived>
struct MatrQBaseDimStore<Derived, false> : public MatrQBase<Derived>
{
private:
  //! Store the dimension of the system at run-time
  const std::size_t _dim;
public:

  //! The dimension of the quantum system. See \ref MatrQBase<Derived>::FixedDim
  inline std::size_t dim() const { return _dim; }
  //! The squared dimension of the quantum system. See \ref MatrQBase<Derived>::FixedDim2
  inline std::size_t dim2() const { return _dim*_dim; }
  /** \brief The squared dimension of the quantum system, minus one. See \ref
   *         MatrQBase<Derived>::FixedNdof
   */
  inline std::size_t ndof() const { return _dim*_dim - 1; }

  /** \brief Constructor, initialized to the given dimension
   *
   * The dimension is fixed here and cannot be changed later.
   */
  MatrQBaseDimStore(std::size_t dim_)
    : _dim(dim_)
  {
    eigen_assert(MatrQBase<Derived>::FixedDim == Eigen::Dynamic);
  }

  /** \brief initializer for MatrixType. See \ref
   *         MatrQBaseDimStore<Derived,has_fixed_dim>::initMatrixType()
   */
  inline typename MatrQBase<Derived>::MatrixType::ConstantReturnType initMatrixType() const
  {
    return MatrQBase<Derived>::MatrixType::Zero(_dim, _dim);
  }
  /** \brief initializer for VectorParamType. See \ref
   *         MatrQBaseDimStore<Derived,has_fixed_dim>::initVectorParamType()
   */
  inline typename MatrQBase<Derived>::VectorParamType::ConstantReturnType initVectorParamType() const
  {
    return MatrQBase<Derived>::VectorParamType::Zero(_dim*_dim);
  }
  /** \brief initializer for VectorParamNdofType. See \ref
   *         MatrQBaseDimStore<Derived,has_fixed_dim>::initVectorParamNdofType()
   */
  inline typename MatrQBase<Derived>::VectorParamNdofType::ConstantReturnType initVectorParamNdofType() const
  {
    return MatrQBase<Derived>::VectorParamNdofType::Zero(_dim*_dim-1);
  }
  /** \brief initializer for VectorParamListType. See \ref
   *         MatrQBaseDimStore<Derived,has_fixed_dim>::initVectorParamListType()
   */
  inline typename MatrQBase<Derived>::VectorParamListType::ConstantReturnType
  initVectorParamListType(std::size_t len) const
  {
    return MatrQBase<Derived>::VectorParamListType::Zero(len, _dim*_dim);
  }
  /** \brief initializer for FreqListType. See \ref
   *         MatrQBaseDimStore<Derived,has_fixed_dim>::initFreqListType()
   */
  inline typename MatrQBase<Derived>::FreqListType::ConstantReturnType initFreqListType(std::size_t len) const
  {
    return MatrQBase<Derived>::FreqListType::Zero(len);
  }
};



/** \brief Defines the data types for a particular problem setting
 *
 * The dimension of the problem may either be given at compile-time or fixed at run-time,
 * as per the great tradition of \c Eigen types.
 *
 * This function inherits, and thus publicly exposes, the types defined in \ref MatrQBase
 * and the initializer methods defined in \ref MatrQBaseDimStore.
 */
template<int FixedDim_, int FixedMaxParamList_, typename RealScalar_, typename IntFreqType_>
struct MatrQ
  : public MatrQBaseDimStore<MatrQ<FixedDim_, FixedMaxParamList_, RealScalar_, IntFreqType_>,
                             FixedDim_ != Eigen::Dynamic>
{
  /** \brief Constructs a data type storage and initializer of given dimension
   *
   * If the dimension is fixed at compile time, then \c dim_ must match the fixed
   * dimension (or an \c assert() will fail). Otherwise, we store the dimension and
   * provide corresponding initializer methods, e.g. \ref initMatrixType() will return a
   * zero initializer for a <code>dim_</code>-by-<code>dim_</code> matrix.
   */
  MatrQ(std::size_t dim_)
    : MatrQBaseDimStore<MatrQ<FixedDim_, FixedMaxParamList_, RealScalar_, IntFreqType_>,
                        FixedDim_ != Eigen::Dynamic>(dim_)
  {
  }
};



/** \brief Data types for all arguments set at run-time
 *
 * In this convenience typedef the dimension and the maximum number of POVM effects are
 * set at runtime.
 *
 * You might incur a (very?) slight performance loss due to dynamic memory allocation each
 * time a vector or matrix type is instantiated.
 */
typedef MatrQ<Eigen::Dynamic, Eigen::Dynamic, double, int> DefaultMatrQ;

/** \brief Data types for Pauli measurements on a single qubit
 *
 * Convenience typedef for the data types corresponding to 6 POVM effects on a 2-level
 * quantum system.
 */
typedef MatrQ<2,6,double,int> QubitPaulisMatrQ;



} // namespace Tomographer


#endif
