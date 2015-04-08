
#ifndef QIT_UTIL_H
#define QIT_UTIL_H


#include <Eigen/Core>
#include <Eigen/Dense>


// -----------------------------------------------------------------------------
// Some C++ helpers
// -----------------------------------------------------------------------------

namespace Tomographer {

  // taken from http://stackoverflow.com/a/25510879/1694896

  namespace tomo_internal {
    template <typename F>
    struct FinalAction {
      FinalAction(F f) : clean_{f} {}
      ~FinalAction() { clean_(); }
      F clean_;
    };
  } // namespace tomo_internal


  /** \brief implementation of a \c finally clause, somewhat like in Python
   *
   * Example Usage:
   * \code
   *   SomeResource * ptr = new SomeResource(..)
   *   auto delete_ptr = finally([ptr] { delete ptr; });
   *   // Now, the pointer ptr will be 'delete'd at end of the current block.
   *   ...
   *
   * \endcode
   */
  template <typename F>
  tomo_internal::FinalAction<F> finally(F f)
  {
    return tomo_internal::FinalAction<F>(f);
  }
  
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
inline const Eigen::CwiseNullaryOp<tomo_internal::random_generator<Rng, RndDist, typename Eigen::internal::traits<Der>::Scalar>, Der>
dense_random(Rng & rng, RndDist &rnddist, IndexTypes... sizes)
{
  typedef typename Eigen::internal::traits<Der>::Scalar Scalar;

  return Eigen::DenseBase<Der>::NullaryExpr(
      sizes..., tomo_internal::random_generator<Rng, RndDist, Scalar>(rng, rnddist)
      );
}

} // namespace Tomographer




#endif
