
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
 * See <a href="http://eigen.tuxfamily.org/dox-devel/group__TopicStlContainers.html">this
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

} // namespace Tomographer




#endif
