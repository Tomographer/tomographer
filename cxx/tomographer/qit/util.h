
#ifndef QIT_UTIL_H
#define QIT_UTIL_H


#include <Eigen/Core>
#include <Eigen/Dense>


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

namespace Eigen {
  namespace internal {
    template<typename Rng, typename RndDist, typename Scalar>
    struct functor_traits<tomo_internal::random_generator<Rng, RndDist, Scalar> >
    { enum { Cost = 50 * NumTraits<Scalar>::MulCost, PacketAccess = false, IsRepeatable = false }; };
  }
} // end namespace Eigen


/** \brief a matrix populated with random entries using C++'s \c random framework
 *
 * \param sizes either no parameter, one parameter or two parameters specifying the sizes
 *        of the dense object to return.
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









#endif
