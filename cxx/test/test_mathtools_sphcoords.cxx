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

//#include <iostream>
#include <cmath>

#include <string>
#include <sstream>
#include <random>

#include <boost/math/constants/constants.hpp>

// definitions for Tomographer test framework -- this must be included before any
// <Eigen/...> or <tomographer/...> header
#include "test_tomographer.h"

#include <tomographer/mathtools/sphcoords.h>
#include <tomographer/tools/eigenutil.h>
#include <tomographer/mathtools/check_derivatives.h>



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

// ------------------------------------------------------

static const double pi = boost::math::constants::pi<double>();

// utility to check for cart_to_sph_jacobian
template<int CART_DIM = 3, int SPH_DIM = CART_DIM - 1>
struct TestSphJacFixture
{
  TestSphJacFixture() { }
  ~TestSphJacFixture() { }

  double calc_montecarlo_vol(std::mt19937::result_type seed, std::size_t npoints)
  {
    // random number generator with reproducable results
    std::mt19937 rng(seed);
    // generate uniformly distributed numbers in [0.0, 1.0[
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    // do very naive monte carlo integration to obtain volume of a sphere
    const int ds = SPH_DIM;
    const double R = 1.0; // radius of ball
    
    double vol = 0;

    Eigen::VectorXd rtheta(CART_DIM);
    
    for (std::size_t k = 0; k < npoints; ++k) {
      // get a random point in theta-space; weigh with jacobian to estimate volume of n-ball
      rtheta = Tomographer::Tools::dense_random<Eigen::VectorXd>(rng, dist, CART_DIM);
      // so translate them to the correct ranges.
      rtheta(0) *= R; // rtheta(0) in [0, R]
      rtheta.block(1,0,ds-1,1) = rtheta.block(1,0,ds-1,1) * pi; // theta_i in [0, pi] for 1 <= i < ds
      rtheta(ds) = rtheta(ds) * 2 * pi; // theta_{ds} in [0, 2*pi]

      vol += Tomographer::SphCoords::cart_to_sph_jacobian(rtheta);
    }

    // average all volume elements
    vol /= npoints;

    // multiply by volume of parameter space
    vol *=  R * 2*pi *  Eigen::VectorXd::Constant(ds-1, pi).array().prod();

    return vol;
  }

  double calc_montecarlo_surf(std::mt19937::result_type seed, std::size_t npoints)
  {
    // random number generator with reproducable results
    std::mt19937 rng(seed);
    // generate uniformly distributed numbers in [0.0, 1.0[
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    // do very naive monte carlo integration to obtain volume of a sphere
    const int ds = SPH_DIM;
    const double pi = boost::math::constants::pi<double>();

    Eigen::VectorXd theta((Eigen::VectorXd::Index)ds);
    double surf = 0;

    for (std::size_t k = 0; k < npoints; ++k) {
      // get a random point in theta-space; add weighted with Jacobian to estimate surface of n-sphere
      theta = Tomographer::Tools::dense_random<Eigen::VectorXd>(rng, dist, SPH_DIM);
      // so translate them to the correct ranges.
      theta.block(0,0,ds-1,1) = theta.block(0,0,ds-1,1) * pi; // theta_i in [0, pi] for 0 <= i < ds-1
      theta(ds-1) = theta(ds-1) * 2 * pi; // theta_{ds-1} in [0, 2*pi]

      surf += Tomographer::SphCoords::surf_sph_jacobian(theta);
    }

    // average all volume elements
    surf /= npoints;

    // multiply by volume of parameter space: 2*pi * pi^(ds-1)
    surf *=  2*pi *  Eigen::VectorXd::Constant(ds-1, pi).array().prod();
    
    return surf;
  }
};

// -----------------------------------------------------------------------------
// test suites

BOOST_AUTO_TEST_SUITE(test_sph_cart)

BOOST_AUTO_TEST_CASE(test_cart_to_sph_3)
{
  Eigen::Vector3d cart;
  cart << 1.0, 2.0, 3.0; // a random point in 3d space

  Eigen::Vector3d rtheta;
  Tomographer::SphCoords::cart_to_sph(rtheta, cart); // cart -> rtheta

  BOOST_CHECK_CLOSE(rtheta(0), cart.norm(), tol_percent);
  BOOST_CHECK_CLOSE(rtheta(0)*std::cos(rtheta(1)), cart(0), tol_percent);
  BOOST_CHECK_CLOSE(rtheta(0)*std::sin(rtheta(1))*std::cos(rtheta(2)), cart(1), tol_percent);
  BOOST_CHECK_CLOSE(rtheta(0)*std::sin(rtheta(1))*std::sin(rtheta(2)), cart(2), tol_percent);

  Eigen::Vector3d backtocart = Eigen::Vector3d::Zero();
  Tomographer::SphCoords::sph_to_cart(backtocart, rtheta); // back to -> cart
  BOOST_CHECK_CLOSE(backtocart(0), cart(0), tol_percent);
  BOOST_CHECK_CLOSE(backtocart(1), cart(1), tol_percent);
  BOOST_CHECK_CLOSE(backtocart(2), cart(2), tol_percent);
  BOOST_CHECK_CLOSE(backtocart.norm(), rtheta(0), tol_percent);

  // test sphsurf
  Eigen::Vector3d cartonsphsurf = Eigen::Vector3d::Zero();
  Tomographer::SphCoords::sphsurf_to_cart(cartonsphsurf, rtheta.block(1,0,2,1));
  double orignorm = cart.norm();
  BOOST_CHECK_CLOSE(cartonsphsurf(0)*orignorm, cart(0), tol_percent);
  BOOST_CHECK_CLOSE(cartonsphsurf(1)*orignorm, cart(1), tol_percent);
  BOOST_CHECK_CLOSE(cartonsphsurf(2)*orignorm, cart(2), tol_percent);
}

BOOST_AUTO_TEST_CASE(test_cart_to_sph_7)
{
  Eigen::VectorXd cart(7);
  cart << 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0;

  Eigen::VectorXd rtheta(7);
  Tomographer::SphCoords::cart_to_sph(rtheta, cart); // cart -> rtheta

  BOOST_CHECK_CLOSE(rtheta(0), cart.norm(), tol_percent);
  BOOST_CHECK_CLOSE(rtheta(0)*std::cos(rtheta(1)), cart(0), tol_percent);
  BOOST_CHECK_CLOSE(rtheta(0)*std::sin(rtheta(1))*std::cos(rtheta(2)), cart(1), tol_percent);
  BOOST_CHECK_CLOSE(rtheta(0)*std::sin(rtheta(1))*std::sin(rtheta(2))*std::cos(rtheta(3)), cart(2), tol_percent);
  BOOST_CHECK_CLOSE(rtheta(0)*std::sin(rtheta(1))*std::sin(rtheta(2))*std::sin(rtheta(3))*std::cos(rtheta(4)), cart(3), tol_percent);
  BOOST_CHECK_CLOSE(rtheta(0)*std::sin(rtheta(1))*std::sin(rtheta(2))*std::sin(rtheta(3))*std::sin(rtheta(4))*std::cos(rtheta(5)), cart(4), tol_percent);
  BOOST_CHECK_CLOSE(rtheta(0)*std::sin(rtheta(1))*std::sin(rtheta(2))*std::sin(rtheta(3))*std::sin(rtheta(4))*std::sin(rtheta(5))*std::cos(rtheta(6)), cart(5), tol_percent);
  BOOST_CHECK_CLOSE(rtheta(0)*std::sin(rtheta(1))*std::sin(rtheta(2))*std::sin(rtheta(3))*std::sin(rtheta(4))*std::sin(rtheta(5))*std::sin(rtheta(6)), cart(6), tol_percent);

  Eigen::VectorXd backtocart = Eigen::VectorXd::Zero(7);
  Tomographer::SphCoords::sph_to_cart(backtocart, rtheta); // back to -> cart
  BOOST_CHECK_CLOSE(backtocart(0), cart(0), tol_percent);
  BOOST_CHECK_CLOSE(backtocart(1), cart(1), tol_percent);
  BOOST_CHECK_CLOSE(backtocart(2), cart(2), tol_percent);
  BOOST_CHECK_CLOSE(backtocart(3), cart(3), tol_percent);
  BOOST_CHECK_CLOSE(backtocart(4), cart(4), tol_percent);
  BOOST_CHECK_CLOSE(backtocart(5), cart(5), tol_percent);
  BOOST_CHECK_CLOSE(backtocart(6), cart(6), tol_percent);

  // test sphsurf
  Eigen::VectorXd cartonsphsurf = Eigen::VectorXd::Zero(7);
  Tomographer::SphCoords::sphsurf_to_cart(cartonsphsurf, rtheta.block(1,0,6,1));
  double orignorm = cart.norm();
  BOOST_CHECK_CLOSE(cartonsphsurf(0)*orignorm, cart(0), tol_percent);
  BOOST_CHECK_CLOSE(cartonsphsurf(1)*orignorm, cart(1), tol_percent);
  BOOST_CHECK_CLOSE(cartonsphsurf(2)*orignorm, cart(2), tol_percent);
  BOOST_CHECK_CLOSE(cartonsphsurf(3)*orignorm, cart(3), tol_percent);
  BOOST_CHECK_CLOSE(cartonsphsurf(4)*orignorm, cart(4), tol_percent);
  BOOST_CHECK_CLOSE(cartonsphsurf(5)*orignorm, cart(5), tol_percent);
  BOOST_CHECK_CLOSE(cartonsphsurf(6)*orignorm, cart(6), tol_percent);
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================

BOOST_AUTO_TEST_SUITE(test_sph_jacobians)

static const int NPOINTS = 1000000;

BOOST_FIXTURE_TEST_CASE(test_sph_jacobians_3, TestSphJacFixture<3>)
{
  const int CART_DIM = 3;

  // first time with seed=0
  double vol = calc_montecarlo_vol(0, NPOINTS);
  // another time with a different seed
  double vol2 = calc_montecarlo_vol(4689392, NPOINTS);

  const double ok_vol = known_vol_sph(CART_DIM);
  BOOST_CHECK_CLOSE(vol, ok_vol, 1.0/*one percent*/);
  BOOST_CHECK_CLOSE(vol2, ok_vol, 1.0/*one percent*/);

  // ... and do the same for the surface of a sphere
  double surf = calc_montecarlo_surf(0, NPOINTS);
  
  const double ok_surf = known_surf_sph(CART_DIM - 1);
  BOOST_CHECK_CLOSE(surf, ok_surf, 1.0/*one percent*/);
}

BOOST_FIXTURE_TEST_CASE(test_sph_jacobians_5, TestSphJacFixture<5>)
{
  const int CART_DIM = 5;

  // first time with seed=0
  double vol = calc_montecarlo_vol(0, NPOINTS);
  // another time with a different seed
  double vol2 = calc_montecarlo_vol(4689392, NPOINTS);

  const double ok_vol = known_vol_sph(CART_DIM);
  BOOST_CHECK_CLOSE(vol, ok_vol, 1.0/*one percent*/);
  BOOST_CHECK_CLOSE(vol2, ok_vol, 1.0/*one percent*/);

  // ... and do the same for the surface of a sphere
  double surf = calc_montecarlo_surf(0, NPOINTS);
  
  const double ok_surf = known_surf_sph(CART_DIM - 1);
  BOOST_CHECK_CLOSE(surf, ok_surf, 1.0/*one percent*/);
}

BOOST_AUTO_TEST_SUITE_END()

// ================================================================================

struct sphsurf_to_cart_fn {
  template<typename Der1, typename Der2>
  void operator()(Eigen::MatrixBase<Der2>& cart, const Eigen::MatrixBase<Der1>& theta) {
    Tomographer::SphCoords::sphsurf_to_cart(cart, theta);
  }
};

template<int N, int DS = N-1>
struct sphsurf_to_diffcart_fn {
  template<typename Der1, typename Der2>
  void operator()(Eigen::MatrixBase<Der2>& dxdthetalinear, const Eigen::MatrixBase<Der1>& theta) {
    Eigen::Array<double, N, DS> dxdtheta;
    //std::cout << "start fn eval\n";
    Tomographer::SphCoords::sphsurf_diffjac(dxdtheta, theta);
    //std::cout << "mid fn eval, dxdtheta's shape is (rows="<<dxdtheta.rows()<<",cols="<<dxdtheta.cols()<<")\n";
    for (int i = 0; i < DS; ++i) {
      dxdthetalinear.block(N*i, 0, N, 1) = dxdtheta.block(0, i, N, 1);
    }
    //std::cout << "end fn eval\n";
  }
};

template<int DEF_N_>
struct test_diffjac_fixture {
  enum {
    DEF_N = DEF_N_,
    DEF_DS = DEF_N_-1
  };

  Eigen::Matrix<double, DEF_DS, 1> theta;

  test_diffjac_fixture()
  {
    // some interesting theta point
    for (int k = 0; k < DEF_DS; ++k) {
      theta(k) = k;
    }
  }
};

BOOST_AUTO_TEST_SUITE(test_diffjacs)

const double tol_der = 1e-6;

BOOST_FIXTURE_TEST_CASE(test_diffjac, test_diffjac_fixture<11>)
{
  Eigen::Array<double, DEF_N, DEF_DS> dxdtheta;
  Tomographer::SphCoords::sphsurf_diffjac(dxdtheta, theta);

  std::stringstream msgstream;
  bool ok = Tomographer::MathTools::check_derivatives(
      dxdtheta, // derivatives
      theta, // point
      sphsurf_to_cart_fn(), // fn
      DEF_N, // valdims
      tol_der,
      tol_der,
      msgstream
      );
  std::string msg = msgstream.str();
  if (msg.size()) {
    BOOST_MESSAGE(msg.c_str());
  }
  BOOST_CHECK(ok);
}

BOOST_FIXTURE_TEST_CASE(test_diffjac2, test_diffjac_fixture<8>)
{
  // now, check second derivatives
  Eigen::Array<double, DEF_N, DEF_DS*DEF_DS> ddxddtheta;
  Tomographer::SphCoords::sphsurf_diffjac2(ddxddtheta, theta);

  Eigen::Array<double, DEF_N*DEF_DS, DEF_DS> ddxddtheta_reshaped;
  for (int k = 0; k < DEF_N; ++k) {
    for (int i = 0; i < DEF_DS; ++i) {
      for (int j = 0; j < DEF_DS; ++j) {
        ddxddtheta_reshaped(DEF_N*i + k, j) = ddxddtheta(k, i+DEF_DS*j);
      }
    }
  }

  std::stringstream msgstream;
  bool ok = Tomographer::MathTools::check_derivatives(
      ddxddtheta_reshaped, // derivatives of the derivatives :)
      theta, // point
      sphsurf_to_diffcart_fn<DEF_N,DEF_DS>(), //fn
      DEF_N*DEF_DS, // valdims
      tol_der,
      tol_der,
      msgstream
      );
  std::string msg = msgstream.str();
  if (msg.size()) {
    BOOST_MESSAGE(msg.c_str());
  }
  BOOST_CHECK(ok);
}

BOOST_AUTO_TEST_SUITE_END()

