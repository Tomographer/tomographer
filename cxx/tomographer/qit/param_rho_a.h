

#ifndef QIT_PARAM_RHO_A_H
#define QIT_PARAM_RHO_A_H



#include <Eigen/Core>
#include <Eigen/StdVector>

#include <tomographer/qit/matrq.h>
#include <tomographer/qit/util.h>



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
  typedef typename MatrQ::MatrixType::Index MatIndex;

  GenGellMannFunctor3(MatrQ matq_, MatIndex l_)
    : matq(matq_),
      l(l_)
  {
    // remember: l = 0, 1, ..., d-2   and not: 1, ..., d-1
    normalization = std::sqrt( 2.0 / ((l+1)*(l+2)) );
    eigen_assert(l < (MatIndex)matq.dim()-1);
  }
  MatrQ matq;
  MatIndex l;
  typename MatrQ::RealScalar normalization;

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

namespace Tomographer
{


/** \brief Parameterization of density matrices in su(N) generators
 *
 * \todo DOC...................
 */
template<typename MatrQ_>
class ParamRhoA
{
public:
  typedef MatrQ_ MatrQ;
  typedef typename MatrQ::MatrixType MatrixType;
  typedef typename MatrQ::VectorParamNdofType VectorParamNdofType;

private:
  MatrQ matq;

  typename Tools::eigen_std_vector<MatrixType>::type lambda;

public:
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
    assert(count == lambda.size());
    assert(count == (std::size_t)matq.ndof());
  }

  inline const MatrixType & getLambda(std::size_t genindex) const
  {
    return lambda[genindex];
  }

  inline void rhoToA(Eigen::Ref<VectorParamNdofType> a, const Eigen::Ref<const MatrixType> & rho)
  {
    assert((std::size_t)a.size() == matq.ndof());
    assert((std::size_t)rho.rows() == matq.dim());
    assert((std::size_t)rho.cols() == matq.dim());
    for (std::size_t n = 0; n < lambda.size(); ++n) {
      a(n) = (rho * lambda[n]).real().trace() / sqrt(2.0);
    }
  }

  inline void aToRho(Eigen::Ref<MatrixType> rho, const Eigen::Ref<const VectorParamNdofType> & a)
  {
    assert((std::size_t)a.size() == matq.ndof());
    assert((std::size_t)rho.rows() == matq.dim());
    assert((std::size_t)rho.cols() == matq.dim());
    rho = MatrixType::Identity(rho.rows(), rho.cols()) / matq.dim();
    for (std::size_t n = 0; n < lambda.size(); ++n) {
      rho += a(n) * lambda[n] / sqrt(2.0);
    }
  }

}; // class ParamRhoA


} // namespace Tomographer

#endif
