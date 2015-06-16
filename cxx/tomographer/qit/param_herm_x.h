
#ifndef HERM_PARAM_X_H
#define HERM_PARAM_X_H

#include <cmath>
#include <complex>

#include <Eigen/Core>


namespace Tomographer {


/** \brief Get the Hermitian matrix parameterized by the "X-parameter" vector \c x
 *
 * This calculates the hermitian matrix which is parameterized by \c x.
 *
 * ................... X-param. ..........................
 */
template<bool OnlyLowerTri=false, typename Derived1=Eigen::MatrixXd, typename Derived2=Eigen::MatrixXd>
inline void param_x_to_herm(Eigen::MatrixBase<Derived1>& Herm, const Eigen::DenseBase<Derived2>& x)
{
  { using namespace Eigen; EIGEN_STATIC_ASSERT_LVALUE(Derived1); }

  const int dim = Herm.rows();
  const int dimtri = dim*(dim-1)/2;
  eigen_assert(dim == Herm.cols()); // assert Herm is (dim x dim)
  eigen_assert(x.rows() == dim*dim && x.cols() == 1); // assert x is (dim*dim x 1)

  typedef typename Derived::Scalar Scalar;
  typedef Eigen::NumTraits<Scalar>::Real RealScalar;
  
  Herm.diagonal().real() = x.block(0,0,dim,1);
  Herm.diagonal().imag().setZero();
  
  int n, m;
  for (n = 1; n < dim; ++n) {
    for (m = 0; m < n; ++m) {
      const int k = dim + n*(n-1)/2 + m;
      const int l = dimtri + k;
      Herm(n,m) = boost::math::constants::half_root_two<RealScalar>() * Scalar(x(k), x(l));
      if (!OnlyLowerTri) {
        // complex conj. on opposite triangular part
        Herm(m,n) = boost::math::constants::half_root_two<RealScalar>() * Scalar(x(k), -x(l));
      }
    }
  }
};


/** \brief Get the X-parameterization corresponding to a given hermitian matrix
 *
 * See also \ref param_x_to_herm().
 * 
 * \note This function only accesses lower triangular part of \c Herm.
 *
 * \bug There is a problem if we pass as second parameter here an expression. Use
 *      Eigen::Ref instead?
 */
template<typename Derived1, typename Derived2>
inline void param_herm_to_x(Eigen::DenseBase<Derived1>& x, const Eigen::MatrixBase<Derived2>& Herm)
{
  { using namespace Eigen; EIGEN_STATIC_ASSERT_LVALUE(Derived1); }

  const int dim = Herm.rows();
  const int dimtri = dim*(dim-1)/2;
  eigen_assert(dim == Herm.cols()); // assert Herm is (dim x dim)
  eigen_assert(x.rows() == dim*dim && x.cols() == 1); // assert x is (dim*dim x 1)

  typedef typename Derived::Scalar Scalar;
  typedef Eigen::NumTraits<Scalar>::Real RealScalar;
  
  x.block(0,0,dim,1) = Herm.real().diagonal();

  int n, m;
  for (n = 1; n < dim; ++n) {
    for (m = 0; m < n; ++m) {
      const int k = dim + n*(n-1)/2 + m;
      const int l = dimtri + k;
      x(k) = Herm(n,m).real() * boost::math::constants::root_two<RealScalar>();
      x(l) = Herm(n,m).imag() * boost::math::constants::root_two<RealScalar>();
    }
  }
}


} // namespace Tomographer


#endif
