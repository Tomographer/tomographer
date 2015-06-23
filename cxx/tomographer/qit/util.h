
#ifndef QIT_UTIL_H
#define QIT_UTIL_H

#include <complex>
#include <vector>

#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/StdVector>



// -----------------------------------------------------------------------------
// C++ Helper for Complex Type
// -----------------------------------------------------------------------------

namespace Tomographer
{
namespace Tools
{

/** \brief statically determine whether a type is complex
 *
 * This class provides an enum memeber named \a value which is either set to \c 1 if
 * the type \a Scalar is of type \a std::complex<>, or else set to \c 0.
 */
template<typename Scalar>
struct is_complex {
  // use Eigen's existing implementation
  enum { value = Eigen::NumTraits<Scalar>::IsComplex };
};

  /*
template<typename Scalar>
struct is_complex {
  enum { value = 0 };
};
template<typename T>
struct is_complex<std::complex<T> > {
  enum { value = 1 };
};
  */

} // namespace Tools
} // namespace Tomographer



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
 *   Tomographer::Tools::eigen_std_vector<Matrix4d>::type v(...);
 *   v[0].resize(...);
 *   // ... use as if std::vector<Matrix4d>
 * \endcode
 */
template<typename EigenType>
struct eigen_std_vector
{
  typedef std::vector<EigenType, Eigen::aligned_allocator<EigenType> > type;
};

} // namespace Tools
} // namespace Tomographer

// -----------------------------------------------------------------------------
// Random matrices in Eigen
// -----------------------------------------------------------------------------

namespace Tomographer {

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

} // namespace Tomographer

namespace Eigen {
  namespace internal {
    /** \internal */
    template<typename Rng, typename RndDist, typename Scalar>
    struct functor_traits<Tomographer::tomo_internal::random_generator<Rng, RndDist, Scalar> >
    { enum { Cost = 50 * NumTraits<Scalar>::MulCost, PacketAccess = false, IsRepeatable = false }; };
  }
} // end namespace Eigen


namespace Tomographer {


/** \brief a matrix populated with random entries using C++'s \c random framework
 *
 * \param rng the \c std::random generator, such as a \c std::mt19937 instance
 * \param rnddist the \c std::random distribution to sample from, such as a
 *   \c std::normal_distribution instance
 * \param sizes either no parameter, one parameter or two parameters specifying
 *   the sizes of the dense object to return.
 */
template<typename Der, typename Rng, typename RndDist, typename... IndexTypes>
inline auto dense_random(Rng & rng, RndDist &rnddist, IndexTypes... sizes)
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
  template<typename Scalar, typename IndexType>
  struct can_basis_vec_generator
  {
    typedef Scalar result_type;

    IndexType k;
    IndexType j;

    can_basis_vec_generator(IndexType k_, IndexType j_ = 0)
      : k(k_), j(j_)
    {
    }

    inline const result_type operator() (IndexType a, IndexType b = 0) const {
      return (a == k) && (b == j) ? result_type(1) : result_type(0);
    }
  };
} // namespace tomo_internal
} // namespace Tomographer
namespace Eigen {
  namespace internal {
    /** \internal */
    template<typename Scalar, typename IndexType>
    struct functor_traits<Tomographer::tomo_internal::can_basis_vec_generator<Scalar, IndexType> >
    { enum { Cost = 2 * NumTraits<Scalar>::MulCost, PacketAccess = false, IsRepeatable = true }; };
  }
} // end namespace Eigen

namespace Tomographer {


/** \brief Expression for the k-th canonical basis vector of given dimension
 *
 */
template<typename Der, typename IndexType>
inline auto can_basis_vec(IndexType k, IndexType size)
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

template<typename Der, typename IndexType>
inline auto can_basis_vec(IndexType k, IndexType j, IndexType rows, IndexType cols)
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
  template<typename Scalar>
  struct powers_of_two_generator
  {
    typedef typename Eigen::NumTraits<Scalar>::Real result_type;

    powers_of_two_generator() { }

    template<typename IndexType>
    inline const result_type operator() (IndexType a) const {
      return std::ldexp(result_type(1), a);
    }

    template<typename IndexType>
    inline const result_type operator() (IndexType a, IndexType b) const {
      eigen_assert(b == 0 && "powers_of_two_generator may only be used with 1-D objects!"); (void)b;
      return std::ldexp(result_type(1), a);
    }

  };
} // namespace tomo_internal
} // namespace Tomographer
namespace Eigen {
  namespace internal {
    /** \internal */
    template<typename Scalar>
    struct functor_traits<Tomographer::tomo_internal::powers_of_two_generator<Scalar> >
    { enum { Cost = 8 * NumTraits<Scalar>::MulCost, PacketAccess = false, IsRepeatable = true }; };

  //    template<typename Scalar>
  //    struct functor_has_linear_access<Tomographer::tomo_internal::powers_of_two_generator<Scalar> >
  //    { enum { ret = 0 }; };
  }
} // end namespace Eigen

namespace Tomographer {


/** \brief Expression for the k-th canonical basis vector of given dimension
 *
 */
template<typename Der, typename... IndexTypes>
inline auto powers_of_two(IndexTypes... sizes)
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
//
// Implementation for fixed RowFactorCTime and ColFactorCTime.
//
template<int RowFactorCTime, int ColFactorCTime, typename Derived,
	 typename std::enable_if<(RowFactorCTime != Eigen::Dynamic && ColFactorCTime != Eigen::Dynamic),
				 bool>::type dummy = true>
inline auto replicated(const Eigen::DenseBase<Derived> & x, int row_factor, int col_factor)
  -> const Eigen::Replicate<Derived, RowFactorCTime, ColFactorCTime>
{
  eigen_assert(row_factor == RowFactorCTime); (void)row_factor; // "unused argument" warning
  eigen_assert(col_factor == ColFactorCTime); (void)col_factor; // "unused argument" warning
  return x.template replicate<RowFactorCTime, ColFactorCTime>();
}


/* second attempt, after I couldn't find the bug in the above..... not needed in the end.

namespace tomo_internal {

template<int RowFactorCTime, int ColFactorCTime, bool KnownAtCompileTime>
struct replicator_helper {
  template<typename Derived>
  static inline auto impl(const Eigen::DenseBase<Derived> & x, int row_factor, int col_factor)
    -> const typename Eigen::DenseBase<Derived>::ReplicateReturnType
  {
    eigen_assert(RowFactorCTime == Eigen::Dynamic || row_factor == RowFactorCTime);
    eigen_assert(ColFactorCTime == Eigen::Dynamic || col_factor == ColFactorCTime);
    return x.replicate(row_factor, col_factor);
  }
};

template<int RowFactorCTime, int ColFactorCTime>
struct replicator_helper<RowFactorCTime, ColFactorCTime, true> {
  template<typename Derived>
  static inline auto impl(const Eigen::DenseBase<Derived> & x, int row_factor, int col_factor)
    -> const Eigen::Replicate<Derived, RowFactorCTime, ColFactorCTime>
  {
    eigen_assert(row_factor == RowFactorCTime); (void)row_factor; // "unused argument" warning
    eigen_assert(col_factor == ColFactorCTime); (void)col_factor; // "unused argument" warning
    return x.template replicate<RowFactorCTime, ColFactorCTime>();
  }
};

} // tomo_internal


template<int RowFactorCTime, int ColFactorCTime>
struct replicator {
  //
  // fixed size version
  //
  template<typename Derived>
  static inline auto replicate(const Eigen::DenseBase<Derived> & x, int row_factor, int col_factor)
    -> const Eigen::Replicate<Derived, RowFactorCTime, ColFactorCTime>
  {
    return tomo_internal::replicator_helper<
      RowFactorCTime,
      ColFactorCTime,
      true
      >::impl(
	  x, row_factor, col_factor
	  );
  }
};

template<int RowFactorCTime>
struct replicator<RowFactorCTime, Eigen::Dynamic>
{
  template<typename Derived>
  static inline auto replicate(const Eigen::DenseBase<Derived> & x, int row_factor, int col_factor)
    -> const typename Eigen::DenseBase<Derived>::ReplicateReturnType
  {
    return tomo_internal::replicator_helper<
      RowFactorCTime,
      Eigen::Dynamic,
      false
      >::impl(
	  x, row_factor, col_factor
	  );
  }
};
template<int ColFactorCTime>
struct replicator<Eigen::Dynamic, ColFactorCTime>
{
  template<typename Derived>
  static inline auto replicate(const Eigen::DenseBase<Derived> & x, int row_factor, int col_factor)
    -> const typename Eigen::DenseBase<Derived>::ReplicateReturnType
  {
    return tomo_internal::replicator_helper<
      Eigen::Dynamic,
      ColFactorCTime,
      false
      >::impl(
	  x, row_factor, col_factor
	  );
  }
};
*/




} // namespace Tomographer




#endif
