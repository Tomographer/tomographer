
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>

#include <Eigen/Eigen>

#include <tomographer/tools/check_derivatives.h>
#include <tomographer/tools/sphcoords.h>


// --------------------------------------------------------------------------------


void test_coordtransform_and_jac();
void test_jacdiff();


// ================================================================================
int main()
{
  test_coordtransform_and_jac();
  test_jacdiff();
  return 0;
}





// --------------------------------------------------------------------------------

// for checking debug results
// see http://en.wikipedia.org/wiki/N-sphere

inline double known_vol_sph(int);

inline double known_surf_sph(int sphdim)
{
  if (sphdim == 0) {
    return 2;
  }
  return 2 * M_PI * known_vol_sph(sphdim - 1);
}
inline double known_vol_sph(int cartdim)
{
  if (cartdim == 0) {
    return 1;
  }
  return known_surf_sph(cartdim - 1) / cartdim;
}


// ================================================================================

#define CART_DIM  3
#define SPH_DIM (CART_DIM - 1)

void test_coordtransform_and_jac()
{
  Eigen::VectorXd cart(CART_DIM);
  cart.setOnes();

  Eigen::VectorXd rtheta(CART_DIM);
  Tomographer::Tools::cart_to_sph(rtheta, cart); // cart -> rtheta

  std::cout << "cart = " << cart.transpose() << "\n--> rtheta = " << rtheta.transpose() << "\n";

  cart.setZero();
  Tomographer::Tools::sph_to_cart(cart, rtheta); // back to -> cart

  std::cout << "--> back to cart = " << cart.transpose() << "\n";

  // --------------------------------------------------

  std::cout << "More testing...\n";

  Eigen::VectorXd mytheta(SPH_DIM);
  (mytheta << 1.34677, 0.835269 /*, 1.5708*/).finished();

  std::cout << "mytheta = " << mytheta.transpose() << "\n";

  Eigen::VectorXd x2(CART_DIM);
  Tomographer::Tools::sphsurf_to_cart(x2, mytheta);

  std::cout << "--> x2 = " << x2.transpose() << "\n";

  Eigen::VectorXd rthetaback(CART_DIM);
  Tomographer::Tools::cart_to_sph(rthetaback, x2);

  std::cout << "back to --> rthetaback = " << rthetaback.transpose() << "\n";


  srand(time(NULL));

  // now, check out the volume & surface elements

  // do very naive monte carlo integration to obtain volume of a sphere

  const int ds = SPH_DIM;

  double vol = 0;

  size_t k;
  size_t kmax = 100000;
  for (k = 0; k < kmax; ++k) {
    // get a random point in theta-space; weigh with jacobian to estimate volume of n-ball
    // MatrixXd::Random returns uniform random numbers in (-1, 1)
    rtheta.setRandom();
    // so translate them to the correct ranges. Remember:  rtheta(i) + 1  is in the interval (0, 2)
    rtheta(0) = (rtheta(0) + 1) / 2; // R in [0, 1]
    rtheta.block(1,0,ds-1,1) = (rtheta.block(1,0,ds-1,1) + Eigen::VectorXd::Constant(ds-1, 1.0)) * M_PI / 2; // theta_i in [0, pi] for 1 <= i < ds
    rtheta(ds) = (rtheta(ds) + 1) * M_PI; // theta_{ds} in [0, 2*pi]

    //std::cout << rtheta.transpose() << "\n";

    vol += Tomographer::Tools::cart_to_sph_jacobian(rtheta);
  }

  // average all volume elements
  vol /= kmax;

  // multiply by volume of parameter space
  vol *=  1.0 * 2*M_PI *  Eigen::VectorXd::Constant(ds-1, M_PI).array().prod();

  std::cout << "Volume of the " << ds << "-sphere was approximated to be = " << vol << "  [should be: "<< known_vol_sph(CART_DIM) <<"]\n";


  // ... and do the same for the surface of a sphere

  Eigen::VectorXd theta(SPH_DIM);
  double surf = 0;

  for (k = 0; k < kmax; ++k) {
    // get a random point in theta-space; add weighted with Jacobian to estimate surface of n-sphere
    // MatrixXd::Random returns uniform random numbers in (-1, 1)
    theta.setRandom();
    // so translate them to the correct ranges. Remember:  rtheta(i) + 1  is in the interval (0, 2)
    theta.block(0,0,ds-1,1) = (theta.block(0,0,ds-1,1) + Eigen::VectorXd::Constant(ds-1, 1.0)) * M_PI / 2; // theta_i in [0, pi] for 1 <= i < ds
    theta(ds-1) = (rtheta(ds-1) + 1) * M_PI; // theta_{ds} in [0, 2*pi]

    //std::cout << theta.transpose() << "\n";

    surf += Tomographer::Tools::surf_sph_jacobian(theta);
  }

  // average all volume elements
  surf /= kmax;

  // multiply by volume of parameter space
  surf *=  2*M_PI *  Eigen::VectorXd::Constant(ds-1, M_PI).array().prod();

  std::cout << "Surface of the " << ds << "-sphere was approximated to be = " << surf << "  [should be: "<< known_surf_sph(SPH_DIM) <<"]\n";


}


// ================================================================================

struct sphsurf_to_cart_fn {
  template<typename Der1, typename Der2>
  void operator()(Eigen::MatrixBase<Der2>& cart, const Eigen::MatrixBase<Der1>& theta) {
    Tomographer::Tools::sphsurf_to_cart(cart, theta);
  }
};

template<int N, int DS>
struct sphsurf_to_diffcart_fn {
  template<typename Der1, typename Der2>
  void operator()(Eigen::MatrixBase<Der2>& dxdthetalinear, const Eigen::MatrixBase<Der1>& theta) {
    Eigen::Array<double, N, DS> dxdtheta;
    //std::cout << "start fn eval\n";
    Tomographer::Tools::sphsurf_diffjac(dxdtheta, theta);
    //std::cout << "mid fn eval, dxdtheta's shape is (rows="<<dxdtheta.rows()<<",cols="<<dxdtheta.cols()<<")\n";
    for (int i = 0; i < DS; ++i) {
      dxdthetalinear.block(N*i, 0, N, 1) = dxdtheta.block(0, i, N, 1);
    }
    //std::cout << "end fn eval\n";
  }
};


const int DEF_N = 10;
const int DEF_DS = (DEF_N-1);


void test_jacdiff()
{
  // Choose a random point, and test the derivatives there
  Eigen::Matrix<double, DEF_DS, 1> theta;
  theta.setRandom();
  // but we need it on the surface of the sphere
  theta /= theta.norm();
  //theta << M_PI/2, M_PI;

  Eigen::Array<double, DEF_N, DEF_DS> dxdtheta;
  Tomographer::Tools::sphsurf_diffjac(dxdtheta, theta);

  std::cout << "dxdtheta at theta=\t"<<theta.transpose() <<":\n"<<dxdtheta << "\n";

  Eigen::Matrix<double, DEF_N, 1> x;
  Tomographer::Tools::sphsurf_to_cart(x, theta);
  std::cout << "x(theta) = "<< x.transpose()<< "\n"
	    << "About to check_derivatives. Don't worry, you'll get warnings if there's something wrong.\n";

  Tomographer::Tools::check_derivatives(dxdtheta, // derivatives
					theta, // point
					sphsurf_to_cart_fn(), // fn
					DEF_N // valdims
      );

  // now, check second derivatives

  Eigen::Array<double, DEF_N, DEF_DS*DEF_DS> ddxddtheta;
  Tomographer::Tools::sphsurf_diffjac2(ddxddtheta, theta);

  Eigen::Array<double, DEF_N*DEF_DS, DEF_DS> ddxddtheta_reshaped;
  for (int k = 0; k < DEF_N; ++k) {
    for (int i = 0; i < DEF_DS; ++i) {
      for (int j = 0; j < DEF_DS; ++j) {
        ddxddtheta_reshaped(DEF_N*i + k, j) = ddxddtheta(k, i+DEF_DS*j);
      }
    }
  }

  std::cout << "about to check second derivatives\n"
	    << "(You'll get warnings if there's something wrong.)\n";

  Tomographer::Tools::check_derivatives(ddxddtheta_reshaped, // derivatives of the derivatives :)
					theta, // point
					sphsurf_to_diffcart_fn<DEF_N,DEF_DS>(), //fn
					DEF_N*DEF_DS // valdims
      );

  std::cout << "done.\n";
}
