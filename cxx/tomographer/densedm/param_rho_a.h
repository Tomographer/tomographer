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

#ifndef QIT_PARAM_RHO_A_H
#define QIT_PARAM_RHO_A_H

/** \file param_rho_a.h
 *
 * \brief Tools for parameterizing density matrices with the \ref pageParamsA.
 *
 */


#include <Eigen/Core>
#include <Eigen/StdVector>

#include <tomographer/qit/matrq.h>
#include <tomographer/qit/util.h>

#include <boost/math/constants/constants.hpp>


namespace Tomographer
{

// based on http://mathworld.wolfram.com/GeneralizedGell-MannMatrix.html

namespace tomo_internal
{

template<typename MatrQ_, typename Coeffs>
struct GenGellMannFunctor12
{
  typedef MatrQ_ MatrQ;
  typedef typename MatrQ::MatrixType::Index MatIndex;
  GenGellMannFunctor12(MatrQ matq_, MatIndex j_, MatIndex k_)
    : matq(matq_),
      j(j_),
      k(k_)
  {
    eigen_assert(j < k);
    eigen_assert(k <= (MatIndex)matq.dim());
  }
  MatrQ matq;
  MatIndex j;
  MatIndex k;

  template<typename Index>
  inline const typename MatrQ::ComplexScalar operator() (Index row, Index col) const
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

template<typename MatrQ>
struct coeffs1 {
  static inline typename MatrQ::ComplexScalar CoeffJK() { return typename MatrQ::ComplexScalar(1, 0); }
  static inline typename MatrQ::ComplexScalar CoeffKJ() { return typename MatrQ::ComplexScalar(1, 0); }
};
template<typename MatrQ>
struct coeffs2 {
  static inline typename MatrQ::ComplexScalar CoeffJK() { return typename MatrQ::ComplexScalar(0, -1); }
  static inline typename MatrQ::ComplexScalar CoeffKJ() { return typename MatrQ::ComplexScalar(0, +1); }
};

template<typename MatrQ>
struct GenGellMannFunctor1
{
  typedef GenGellMannFunctor12<MatrQ, coeffs1<MatrQ> > type;
};
template<typename MatrQ>
struct GenGellMannFunctor2
{
  typedef GenGellMannFunctor12<MatrQ, coeffs2<MatrQ> > type;
};


template<typename MatrQ_>
struct GenGellMannFunctor3
{
  typedef GenGellMannFunctor3<MatrQ_> type;

  typedef MatrQ_ MatrQ;
  typedef typename MatrQ::RealScalar RealScalar;
  typedef typename MatrQ::MatrixType::Index MatIndex;

  GenGellMannFunctor3(MatrQ matq_, MatIndex l_)
    : matq(matq_),
      l(l_)
  {
    // remember: l = 0, 1, ..., d-2   and not: 1, ..., d-1
    normalization = std::sqrt( RealScalar(2) / ((l+1)*(l+2)) );
    eigen_assert(l < (MatIndex)matq.dim()-1);
  }
  MatrQ matq;
  MatIndex l;
  RealScalar normalization;

  template<typename Index>
  inline const typename MatrQ::ComplexScalar operator() (Index row, Index col) const
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
template<typename MatrQ, typename coeffs>
struct functor_traits<Tomographer::tomo_internal::GenGellMannFunctor12<MatrQ, coeffs> >
{
  enum { Cost = 4*NumTraits<typename MatrQ::ComplexScalar>::AddCost, PacketAccess = false, IsRepeatable = true };
};
// functor_has_linear_access for GenGellMannFunctor12
template<typename MatrQ, typename coeffs>
struct functor_has_linear_access<Tomographer::tomo_internal::GenGellMannFunctor12<MatrQ, coeffs> >
{ enum { ret = 0 }; };
// functor_traits for GenGellMannFunctor3
template<typename MatrQ>
struct functor_traits<Tomographer::tomo_internal::GenGellMannFunctor3<MatrQ> >
{ enum { Cost = 4*NumTraits<typename MatrQ::ComplexScalar>::AddCost, PacketAccess = false, IsRepeatable = true }; };
// functor_has_linear_access for GenGellMannFunctor3
template<typename MatrQ>
struct functor_has_linear_access<Tomographer::tomo_internal::GenGellMannFunctor3<MatrQ> >
{ enum { ret = 0 }; };
// ---
} } // namespace Eigen::internal, Eigen

namespace Tomographer {


/** \brief Parameterization of density matrices in su(N) generators
 *
 * This class implements the parameterization descirbed on the page \ref pageParamsA.
 *
 * Additionally, this class gives access to the constructed generalized Gell-Mann matrices
 * (see \ref pageParamsA), see \ref getLambda().
 */
template<typename MatrQ_>
class ParamRhoA
{
public:
  //! The C++ types for quantum objects and parameterizations
  typedef MatrQ_ MatrQ;
  //! The Matrix type to use to describe hermitian matrices.
  typedef typename MatrQ::MatrixType MatrixType;
  //! The Matrix type to use to describe the \ref pageParamsA of a hermitian matrix.
  typedef typename MatrQ::VectorParamNdofType VectorParamNdofType;
  //! The real scalar type. Usually this is \c double.
  typedef typename MatrQ::RealScalar RealScalar;

private:
  MatrQ matq;

  //! Cached generators of \f$ su(d)\f$.
  typename Tools::eigen_std_vector<MatrixType>::type lambda;

public:
  /** \brief Construct an object which can perform A parameterization transformations.
   *
   * Specify the runtime \a MatrQ object to use (see \ref pageInterfaceMatrQ).
   *
   * Once constructed, this object has precalculated and cached the generalized Gell-Mann
   * matrices (see \ref pageParamsA), and these can be obtained with \ref getLambda().
   */
  ParamRhoA(MatrQ matq_)
    : matq(matq_)
  {
    // calculate and cache the generalized Gell-Mann matrices
    lambda.resize(matq.ndof());
    std::size_t count = 0;
    // first kind
    for (std::size_t j = 0; j < matq.dim(); ++j) {
      for (std::size_t k = j+1; k < matq.dim(); ++k) {
	lambda[count] = MatrixType::NullaryExpr(
	    matq.dim(),
	    matq.dim(),
	    typename tomo_internal::GenGellMannFunctor1<MatrQ>::type(matq, j, k)
	    );
	++count;
      }
    }
    // second kind
    for (std::size_t j = 0; j < matq.dim(); ++j) {
      for (std::size_t k = j+1; k < matq.dim(); ++k) {
	lambda[count] = MatrixType::NullaryExpr(
	    matq.dim(),
	    matq.dim(),
	    typename tomo_internal::GenGellMannFunctor2<MatrQ>::type(matq, j, k)
	    );
	++count;
      }
    }
    // third kind
    for (std::size_t l = 0; l < matq.dim()-1; ++l) {
      lambda[count] = MatrixType::NullaryExpr(
	  matq.dim(),
	  matq.dim(),
	  typename tomo_internal::GenGellMannFunctor3<MatrQ>::type(matq, l)
	  );
      ++count;
    }
    // got them all?
    eigen_assert(count == lambda.size());
    eigen_assert(count == (std::size_t)matq.ndof());
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
    eigen_assert(j < matq.ndof());
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
  inline void rhoToA(Eigen::Ref<VectorParamNdofType> a, const Eigen::Ref<const MatrixType> & rho) const
  {
    eigen_assert((std::size_t)a.size() == matq.ndof());
    eigen_assert((std::size_t)rho.rows() == matq.dim());
    eigen_assert((std::size_t)rho.cols() == matq.dim());
    for (std::size_t n = 0; n < lambda.size(); ++n) {
      a(n) = (rho * lambda[n].template selfadjointView<Eigen::Lower>())
	.real().trace() * boost::math::constants::half_root_two<RealScalar>();
    }
  }

  /** \brief Reconstruct a hermitian traceless matrix from its \ref pageParamsA.
   *
   * This method computes the hermitian matrix given by its corresponding \ref
   * pageParamsA. The matrix is shifted by the identity so that it has trace \a trace.
   */
  inline void aToRho(Eigen::Ref<MatrixType> rho, const Eigen::Ref<const VectorParamNdofType> & a,
		     RealScalar trace = 1.0) const
  {
    eigen_assert((std::size_t)a.size() == matq.ndof());
    eigen_assert((std::size_t)rho.rows() == matq.dim());
    eigen_assert((std::size_t)rho.cols() == matq.dim());
    rho = trace * MatrixType::Identity(rho.rows(), rho.cols()) / matq.dim();
    for (std::size_t n = 0; n < lambda.size(); ++n) {
      rho += a(n) * lambda[n].template selfadjointView<Eigen::Lower>()
	* boost::math::constants::half_root_two<RealScalar>();
    }
  }

}; // class ParamRhoA


} // namespace Tomographer

#endif
