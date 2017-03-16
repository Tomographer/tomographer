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

#ifndef TOMOGRAPHER_DENSEDM_PARAM_RHO_A_H
#define TOMOGRAPHER_DENSEDM_PARAM_RHO_A_H

/** \file param_rho_a.h
 *
 * \brief Tools for parameterizing density matrices with the \ref pageParamsA.
 *
 */


#include <Eigen/Core>
#include <Eigen/StdVector>

#include <tomographer/densedm/dmtypes.h>
#include <tomographer/tools/cxxutil.h> // tomographer_assert()
#include <tomographer/tools/eigenutil.h>

#include <boost/math/constants/constants.hpp>


namespace Tomographer
{

// based on http://mathworld.wolfram.com/GeneralizedGell-MannMatrix.html

namespace tomo_internal
{

template<typename DMTypes_, typename Coeffs>
struct GenGellMannFunctor12
{
  typedef DMTypes_ DMTypes;
  typedef typename DMTypes::MatrixType::Index MatIndex;
  GenGellMannFunctor12(DMTypes matq_, MatIndex j_, MatIndex k_)
    : matq(matq_),
      j(j_),
      k(k_)
  {
    tomographer_assert(j < k);
    tomographer_assert(k <= (MatIndex)matq.dim());
  }
  DMTypes matq;
  MatIndex j;
  MatIndex k;

  template<typename Index>
  inline const typename DMTypes::ComplexScalar operator() (Index row, Index col) const
  {
    if (row == j && col == k) {
      return Coeffs::CoeffJK();
    }
    if (row == k && col == j) {
      return Coeffs::CoeffKJ();
    }
    return 0;
  }
};

template<typename DMTypes>
struct coeffs1 {
  static inline typename DMTypes::ComplexScalar CoeffJK() { return typename DMTypes::ComplexScalar(1, 0); }
  static inline typename DMTypes::ComplexScalar CoeffKJ() { return typename DMTypes::ComplexScalar(1, 0); }
};
template<typename DMTypes>
struct coeffs2 {
  static inline typename DMTypes::ComplexScalar CoeffJK() { return typename DMTypes::ComplexScalar(0, -1); }
  static inline typename DMTypes::ComplexScalar CoeffKJ() { return typename DMTypes::ComplexScalar(0, +1); }
};

template<typename DMTypes>
struct GenGellMannFunctor1
{
  typedef GenGellMannFunctor12<DMTypes, coeffs1<DMTypes> > type;
};
template<typename DMTypes>
struct GenGellMannFunctor2
{
  typedef GenGellMannFunctor12<DMTypes, coeffs2<DMTypes> > type;
};


template<typename DMTypes_>
struct GenGellMannFunctor3
{
  typedef GenGellMannFunctor3<DMTypes_> type;

  typedef DMTypes_ DMTypes;
  typedef typename DMTypes::RealScalar RealScalar;
  typedef typename DMTypes::MatrixType::Index MatIndex;

  GenGellMannFunctor3(DMTypes matq_, MatIndex l_)
    : matq(matq_),
      l(l_)
  {
    // remember: l = 0, 1, ..., d-2   and not: 1, ..., d-1
    normalization = std::sqrt( RealScalar(2) / ((l+1)*(l+2)) );
    tomographer_assert(l < (MatIndex)matq.dim()-1);
  }
  DMTypes matq;
  MatIndex l;
  RealScalar normalization;

  template<typename Index>
  inline const typename DMTypes::ComplexScalar operator() (Index row, Index col) const
  {
    if (row != col) {
      return 0;
    } else if (row <= l) {
      return normalization;
    } else if (row == l+1) {
      return - (l+1) * normalization;
    } else {
      // row > l+1
      return 0;
    }
  }
};

} // namespace tomo_internal
} // namespace Tomographer

namespace Eigen { namespace internal {
// ---
// functor_traits for GenGellMannFunctor12
template<typename DMTypes, typename coeffs>
struct functor_traits<Tomographer::tomo_internal::GenGellMannFunctor12<DMTypes, coeffs> >
{
  enum { Cost = 4*NumTraits<typename DMTypes::ComplexScalar>::AddCost, PacketAccess = false, IsRepeatable = true };
};
// functor_has_linear_access for GenGellMannFunctor12
template<typename DMTypes, typename coeffs>
struct functor_has_linear_access<Tomographer::tomo_internal::GenGellMannFunctor12<DMTypes, coeffs> >
{ enum { ret = 0 }; };
// functor_traits for GenGellMannFunctor3
template<typename DMTypes>
struct functor_traits<Tomographer::tomo_internal::GenGellMannFunctor3<DMTypes> >
{ enum { Cost = 4*NumTraits<typename DMTypes::ComplexScalar>::AddCost, PacketAccess = false, IsRepeatable = true }; };
// functor_has_linear_access for GenGellMannFunctor3
template<typename DMTypes>
struct functor_has_linear_access<Tomographer::tomo_internal::GenGellMannFunctor3<DMTypes> >
{ enum { ret = 0 }; };
// ---
} } // namespace Eigen::internal, Eigen

namespace Tomographer {
namespace DenseDM {

/** \brief Parameterization of density matrices in su(N) generators
 *
 * This class implements the parameterization descirbed on the page \ref pageParamsA.
 *
 * Additionally, this class gives access to the constructed generalized Gell-Mann matrices
 * (see \ref pageParamsA), see \ref getLambda().
 *
 * \todo This class can probably be better optimized, as currently the inner products are
 *       calculated by Hilbert-Schmidt matrix multiplication ...
 */
template<typename DMTypes_>
TOMOGRAPHER_EXPORT class ParamA
{
public:
  //! The C++ types for quantum objects and parameterizations
  typedef DMTypes_ DMTypes;
  //! The Matrix type to use to describe hermitian matrices.
  typedef typename DMTypes::MatrixType MatrixType;
  //! The Matrix type to use to describe the \ref pageParamsA of a hermitian matrix.
  typedef typename DMTypes::VectorParamNdofType VectorParamNdofType;
  //! The real scalar type. Usually this is \c double.
  typedef typename DMTypes::RealScalar RealScalar;

private:
  const DMTypes _dmt;

  //! Cached generators of \f$ su(d)\f$.
  typename Tools::EigenStdVector<MatrixType>::type lambda;

public:
  /** \brief Construct an object which can perform A parameterization transformations.
   *
   * Specify the runtime \ref DMTypes object to use.
   *
   * Once constructed, this object has precalculated and cached the generalized Gell-Mann
   * matrices (see \ref pageParamsA), and these can be obtained with \ref getLambda().
   */
  ParamA(DMTypes dmt_)
    : _dmt(dmt_)
  {
    // calculate and cache the generalized Gell-Mann matrices
    lambda.resize(_dmt.ndof());
    std::size_t count = 0;
    // first kind
    for (std::size_t j = 0; j < _dmt.dim(); ++j) {
      for (std::size_t k = j+1; k < _dmt.dim(); ++k) {
	lambda[count] = MatrixType::NullaryExpr(
	    _dmt.dim(),
	    _dmt.dim(),
	    typename tomo_internal::GenGellMannFunctor1<DMTypes>::type(_dmt, j, k)
	    );
	++count;
      }
    }
    // second kind
    for (std::size_t j = 0; j < _dmt.dim(); ++j) {
      for (std::size_t k = j+1; k < _dmt.dim(); ++k) {
	lambda[count] = MatrixType::NullaryExpr(
	    _dmt.dim(),
	    _dmt.dim(),
	    typename tomo_internal::GenGellMannFunctor2<DMTypes>::type(_dmt, j, k)
	    );
	++count;
      }
    }
    // third kind
    for (std::size_t l = 0; l < _dmt.dim()-1; ++l) {
      lambda[count] = MatrixType::NullaryExpr(
	  _dmt.dim(),
	  _dmt.dim(),
	  typename tomo_internal::GenGellMannFunctor3<DMTypes>::type(_dmt, l)
	  );
      ++count;
    }
    // got them all?
    tomographer_assert(count == lambda.size());
    tomographer_assert(count == (std::size_t)_dmt.ndof());
  }

  /** \brief Generalized Gell-Mann matrices.
   *
   * Returns the \a j-th generalized Gell-Mann matrix, as described in \ref
   * pageParamsA. The matrices returned here are NOT normalized with the factor \f$
   * \sqrt2\f$, rather, they are directly the ones constructed in the references cited
   * \ref pageParamsA "here".
   *
   * \param j which matrix to return. There are in total \f$ d^2-1\f$ matrices. \a j must
   *        be in range or else prepare for an assert hara-kiri.
   *
   * This function executes quickly as it does not need to compute the matrices. All the
   * generalized Gell-Mann matrices for this dimension are precomputed in the constructor.
   *
   * The returned reference points directly to the internal cached version of the matrix,
   * so copy it somewhere else if you wish to change it.
   */
  inline const MatrixType & getLambda(std::size_t j) const
  {
    tomographer_assert(j < _dmt.ndof());
    return lambda[j];
  }

  /** \brief Compute the \ref pageParamsA of a hermitian matrix.
   *
   * This method computes the \ref pageParamsA of the traceless part of the matrix \a rho.
   *
   * \note The matrix \a rho need not be traceless, though the parameterization in \a a
   *       will only reflect its traceless part, i.e. \f$ \texttt{rho} -
   *       \mathrm{tr}(\texttt{rho})\mathbb{I}/d \f$.
   */
  inline VectorParamNdofType
  rhoToA(const Eigen::Ref<const MatrixType> & rho) const
  {
    VectorParamNdofType a(_dmt.initVectorParamNdofType());
    tomographer_assert((std::size_t)a.size() == _dmt.ndof());
    tomographer_assert((std::size_t)rho.rows() == _dmt.dim());
    tomographer_assert((std::size_t)rho.cols() == _dmt.dim());
    for (std::size_t n = 0; n < lambda.size(); ++n) {
      a(n) = (rho * lambda[n].template selfadjointView<Eigen::Lower>())
	.real().trace() * boost::math::constants::half_root_two<RealScalar>();
    }
    return a;
  }

  /** \brief Reconstruct a hermitian traceless matrix from its \ref pageParamsA.
   *
   * This method computes the hermitian matrix given by its corresponding \ref
   * pageParamsA. The matrix is shifted by the identity so that it has trace \a trace.
   */
  inline MatrixType
  aToRho(const Eigen::Ref<const VectorParamNdofType> & a, RealScalar trace = 1.0) const
  {
    MatrixType rho(_dmt.initMatrixType());
    tomographer_assert((std::size_t)a.size() == _dmt.ndof());
    tomographer_assert((std::size_t)rho.rows() == _dmt.dim());
    tomographer_assert((std::size_t)rho.cols() == _dmt.dim());
    rho = trace * MatrixType::Identity(rho.rows(), rho.cols()) / _dmt.dim();
    for (std::size_t n = 0; n < lambda.size(); ++n) {
      rho += a(n) * boost::math::constants::half_root_two<RealScalar>() * lambda[n];
    }
    return rho;
  }

}; // class ParamA


} // namespace DenseDM
} // namespace Tomographer

#endif
