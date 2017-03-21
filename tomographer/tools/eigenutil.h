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

#ifndef TOMOGRAPHER_TOOLS_EIGENUTIL_H
#define TOMOGRAPHER_TOOLS_EIGENUTIL_H

/** \file eigenutil.h
 *
 * \brief Basic utilities for dealing with Eigen matrices and other types.
 *
 */


#include <complex>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/StdVector>





// -----------------------------------------------------------------------------
// Eigen and std::vector
// -----------------------------------------------------------------------------

namespace Tomographer {
namespace Tools {

/** \brief Use this if you need an \a std::vector of any Eigen type
 *
 * See <a href="http://eigen.tuxfamily.org/dox/group__TopicStlContainers.html">this
 * discussion in Eigen's documentation</a> about using Eigen with C++ STL containers.
 *
 * This struct defines a helper type, which is the right type to use if you want an
 * std::vector of any Eigen type. Example:
 *
 * \code
 *   Tomographer::Tools::EigenStdVector<Matrix4d>::type v(...);
 *   v[0].resize(...);
 *   // ... use as if std::vector<Matrix4d>
 * \endcode
 */
template<typename EigenType>
struct EigenStdVector
{
  typedef std::vector<EigenType, Eigen::aligned_allocator<EigenType> > type;
};

} // namespace Tools
} // namespace Tomographer

// -----------------------------------------------------------------------------
// Random matrices in Eigen
// -----------------------------------------------------------------------------

namespace Tomographer {
namespace Tools {

namespace tomo_internal {

  template<typename Rng, typename RndDist, typename Scalar>
  struct random_generator
  {
    typedef Scalar result_type;

    Rng & rng;
    RndDist & rnddist;

    random_generator(Rng & rng_, RndDist & rnddist_)
      : rng(rng_), rnddist(rnddist_)
    {
    }

    template<typename Index>
    inline const result_type operator() (Index, Index = 0) const {
      return rnddist(rng);
    }
  };

  template<typename Rng, typename RndDist, typename RealScalar>
  struct random_generator<Rng, RndDist, std::complex<RealScalar> >
  {
    typedef std::complex<RealScalar> result_type;

    Rng & rng;
    RndDist & rnddist;

    random_generator(Rng & rng_, RndDist & rnddist_)
      : rng(rng_), rnddist(rnddist_)
    {
    }

    template<typename Index>
    inline const result_type operator() (Index, Index = 0) const {
      return result_type(rnddist(rng), rnddist(rng));
    }
  };
} // end namespace tomo_internal

} // namespace Tools
} // namespace Tomographer

namespace Eigen {
  namespace internal {
    /** \internal */
    template<typename Rng, typename RndDist, typename Scalar>
    struct functor_traits<Tomographer::Tools::tomo_internal::random_generator<Rng, RndDist, Scalar> >
    { enum { Cost = 50 * NumTraits<Scalar>::MulCost, PacketAccess = false, IsRepeatable = false }; };
  }
} // end namespace Eigen


namespace Tomographer {
namespace Tools {


/** \brief a matrix populated with random entries using C++'s \c random framework
 *
 * \param rng the \c std::random generator, such as a \c std::mt19937 instance
 * \param rnddist the \c std::random distribution to sample from, such as a
 *   \c std::normal_distribution instance
 * \param sizes either no parameter, one parameter or two parameters specifying
 *   the sizes of the dense object to return.
 */
template<typename Der, typename Rng, typename RndDist, typename... IndexTypes>
inline auto denseRandom(Rng & rng, RndDist &rnddist, IndexTypes... sizes)
  -> const Eigen::CwiseNullaryOp<
    tomo_internal::random_generator<Rng, RndDist, typename Eigen::internal::traits<Der>::Scalar>,
    Der
    >
{
  typedef typename Der::Scalar Scalar;

  return Eigen::DenseBase<Der>::NullaryExpr(
      sizes..., tomo_internal::random_generator<Rng, RndDist, Scalar>(rng, rnddist)
      );
}



// ---------------------------



namespace tomo_internal {
  /** \internal */
  template<typename Scalar, typename IndexType>
  struct can_basis_vec_generator
  {
    typedef Scalar result_type;

    const IndexType k;
    const IndexType j;

    can_basis_vec_generator(IndexType k_, IndexType j_ = 0)
      : k(k_), j(j_)
    {
    }

    inline const result_type operator() (IndexType a, IndexType b = 0) const {
      return (a == k) && (b == j) ? result_type(1) : result_type(0);
    }
  };
} // namespace tomo_internal
} // namespace Tools
} // namespace Tomographer
namespace Eigen {
  namespace internal {
    /** \internal */
    template<typename Scalar, typename IndexType>
    struct functor_traits<Tomographer::Tools::tomo_internal::can_basis_vec_generator<Scalar, IndexType> >
    { enum { Cost = 2 * NumTraits<Scalar>::MulCost, PacketAccess = false, IsRepeatable = true }; };
    template<typename Scalar, typename IndexType>
    struct functor_has_linear_access<Tomographer::Tools::tomo_internal::can_basis_vec_generator<Scalar, IndexType> >
    { enum { ret = 0 }; };
  }
} // end namespace Eigen

namespace Tomographer {
namespace Tools {

/** \brief Expression for the k-th canonical basis vector of given dimension
 *
 */
template<typename Der, typename IndexType>
inline auto canonicalBasisVec(IndexType k, IndexType size)
  -> const Eigen::CwiseNullaryOp<
    tomo_internal::can_basis_vec_generator<typename Eigen::internal::traits<Der>::Scalar, IndexType>,
    Der
    >
{
  typedef typename Der::Scalar Scalar;

  return Eigen::DenseBase<Der>::NullaryExpr(
      size, tomo_internal::can_basis_vec_generator<Scalar, IndexType>(k)
      );
}

/** \brief Expression for the (k,j)-th canonical basis matrix of given dimension
 *
 * This is a matrix of zeros except for the entry (k,j) which is one.
 */
template<typename Der, typename IndexType>
inline auto canonicalBasisVec(IndexType k, IndexType j, IndexType rows, IndexType cols)
  -> const Eigen::CwiseNullaryOp<
    tomo_internal::can_basis_vec_generator<typename Eigen::internal::traits<Der>::Scalar, IndexType>,
    Der
    >
{
  typedef typename Der::Scalar Scalar;

  return Eigen::DenseBase<Der>::NullaryExpr(
      rows, cols, tomo_internal::can_basis_vec_generator<Scalar, IndexType>(k, j)
      );
}




// ---------------------------



namespace tomo_internal {
  /** \internal */
  template<typename Scalar>
  struct powers_of_two_generator
  {
    typedef typename Eigen::NumTraits<Scalar>::Real result_type;

    powers_of_two_generator() { }

    template<typename IndexType>
    inline const result_type operator() (IndexType a) const {
      return std::ldexp(result_type(1), (int)a);
    }

    // Don't expose an operator()(i,j) method, because otherwise Eigen might think that we
    // don't have linear access
    // https://eigen.tuxfamily.org/dox/NullaryFunctors_8h_source.html#l00147
    //
    // template<typename IndexType>
    // inline const result_type operator() (IndexType a, IndexType b) const {
    //   eigen_assert(b == 0 && "powers_of_two_generator may only be used with 1-D objects or with linear access!");
    //   (void)b; // silence unused variable warning if eigen_assert is optimized out
    //   return std::ldexp(result_type(1), a);
    // }

  };
} // namespace tomo_internal
} // namespace Tools
} // namespace Tomographer
namespace Eigen {
  namespace internal {
    /** \internal */
    template<typename Scalar>
    struct functor_traits<Tomographer::Tools::tomo_internal::powers_of_two_generator<Scalar> >
    { enum { Cost = 8 * NumTraits<Scalar>::MulCost, PacketAccess = false, IsRepeatable = true }; };
  }
} // end namespace Eigen

namespace Tomographer {
namespace Tools {


/** \brief Expression for a 1-D expression of powers of two
 *
 * An Eigen template expression yielding powers of two as a column vector. The \f$ i\f$
 * -th item is \f$ 2^i \f$.
 *
 * \param sizes You may either specify the length of the vector (1 parameter), or the size
 *        of a column matrix (2 parameters, the second being equal to one).
 *
 * If you specify a 2-D matrix size here (not a column or row vector), then the elements
 * of the matrix are populated linearly with powers of two (with linear access, i.e. by
 * default column-wise).
 */
template<typename Der, typename... IndexTypes>
inline auto powersOfTwo(IndexTypes... sizes)
  -> const Eigen::CwiseNullaryOp<
    tomo_internal::powers_of_two_generator<typename Eigen::internal::traits<Der>::Scalar>,
    Der
    >
{
  typedef typename Der::Scalar Scalar;

  return Eigen::DenseBase<Der>::NullaryExpr(
      sizes..., tomo_internal::powers_of_two_generator<Scalar>()
      );
}





// -----------------------------------------------------------------------------

/** \brief Replicate a Eigen Dense object; same call for compile-time & run-time
 * dimensions
 *
 * Allow our libraries to use the same syntax, for example
 * \code
 *   Eigen::Matrix<...> m_replicated = replicated<1,ColFactorAtCompileTime>(matrix, 1, col_factor);
 * \endcode
 * for replicating a matrix for which one might, or might not, know the factor at compile
 * time.
 *
 * The corresponding factor template parameter should be either \a Eigen::Dynamic or a
 * fixed value. If a fixed value is given, you need to specify the \a same value in the
 * runtime argument (done as an \a eigen_assert() check, i.e. this is compiled out if \a
 * -DNDEBUG is used)
 */
template<int RowFactorCTime, int ColFactorCTime, typename Derived,
	 typename std::enable_if<(RowFactorCTime == Eigen::Dynamic || ColFactorCTime == Eigen::Dynamic),
				 bool>::type dummy = true>
inline auto replicated(const Eigen::DenseBase<Derived> & x, int row_factor, int col_factor)
  -> const Eigen::Replicate<Derived, Eigen::Dynamic, Eigen::Dynamic>
{
  eigen_assert(RowFactorCTime == Eigen::Dynamic || row_factor == RowFactorCTime);
  eigen_assert(ColFactorCTime == Eigen::Dynamic || col_factor == ColFactorCTime);
  return x.replicate(row_factor, col_factor);
}
/** \brief See \ref replicated<RowFactorCTime, ColFactorCTime, Derived, dummy>
 *
 * This is the implementation for fixed RowFactorCTime and ColFactorCTime.
 */
template<int RowFactorCTime, int ColFactorCTime, typename Derived,
	 typename std::enable_if<(RowFactorCTime != Eigen::Dynamic && ColFactorCTime != Eigen::Dynamic),
				 bool>::type dummy2 = true>
inline auto replicated(const Eigen::DenseBase<Derived> & x, int row_factor, int col_factor)
  -> const Eigen::Replicate<Derived, RowFactorCTime, ColFactorCTime>
{
  eigen_assert(row_factor == RowFactorCTime); (void)row_factor; // "unused argument" warning
  eigen_assert(col_factor == ColFactorCTime); (void)col_factor; // "unused argument" warning
  return x.template replicate<RowFactorCTime, ColFactorCTime>();
}




} // namespace Tools
} // namespace Tomographer




#endif
