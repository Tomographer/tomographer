
#ifndef HERM_PARAM_X_H
#define HERM_PARAM_X_H

#include <cmath>
#include <complex>

#include <Eigen/Core>


namespace tomo_internal {
  static const double SQRT_2 = std::sqrt(2.0);
  static const double SQRT_12 = 1.0/std::sqrt(2.0);
}



template<bool OnlyLowerTri=false, typename Derived1=Eigen::MatrixXd, typename Derived2=Eigen::MatrixXd>
inline void param_x_to_herm(Eigen::MatrixBase<Derived1>& Herm, const Eigen::DenseBase<Derived2>& x)
{
  { using namespace Eigen; EIGEN_STATIC_ASSERT_LVALUE(Derived1); }

  const int dim = Herm.rows();
  const int dimtri = dim*(dim-1)/2;
  eigen_assert(dim == Herm.cols()); // assert Herm is (dim x dim)
  eigen_assert(x.rows() == dim*dim && x.cols() == 1); // assert x is (dim*dim x 1)
  
  Herm.diagonal().real() = x.block(0,0,dim,1);
  Herm.diagonal().imag().setZero();
  
  int n, m;
  for (n = 1; n < dim; ++n) {
    for (m = 0; m < n; ++m) {
      const int k = dim + n*(n-1)/2 + m;
      const int l = dimtri + k;
      Herm(n,m) = tomo_internal::SQRT_12 * typename Eigen::internal::traits<Derived1>::Scalar(x(k), x(l));
      if (!OnlyLowerTri) {
        // complex conj. on opposite triangular part
        Herm(m,n) = tomo_internal::SQRT_12 * typename Eigen::internal::traits<Derived1>::Scalar(x(k), -x(l));
      }
    }
  }
};


/**
 * \note only accesses lower triangular part of \c Herm.
 */
template<typename Derived1, typename Derived2>
inline void param_herm_to_x(Eigen::DenseBase<Derived1>& x, const Eigen::MatrixBase<Derived2>& Herm)
{
  { using namespace Eigen; EIGEN_STATIC_ASSERT_LVALUE(Derived1); }

  const int dim = Herm.rows();
  const int dimtri = dim*(dim-1)/2;
  eigen_assert(dim == Herm.cols()); // assert Herm is (dim x dim)
  eigen_assert(x.rows() == dim*dim && x.cols() == 1); // assert x is (dim*dim x 1)

  x.block(0,0,dim,1) = Herm.real().diagonal();

  int n, m;
  for (n = 1; n < dim; ++n) {
    for (m = 0; m < n; ++m) {
      const int k = dim + n*(n-1)/2 + m;
      const int l = dimtri + k;
      x(k) = Herm(n,m).real() * tomo_internal::SQRT_2;
      x(l) = Herm(n,m).imag() * tomo_internal::SQRT_2;
    }
  }
}



#endif
