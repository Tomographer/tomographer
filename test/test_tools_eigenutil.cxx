/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
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

#include <cmath>

#include <string>
#include <iostream>
#include <random>

#include <boost/math/constants/constants.hpp>

// definitions for Tomographer test framework -- this must be included before any
// <Eigen/...> or <tomographer2/...> header
#include "test_tomographer.h"

#include <tomographer2/tools/eigenutil.h>
#include <tomographer2/tools/cxxutil.h>



// -----------------------------------------------------------------------------
// fixture(s)


// -----------------------------------------------------------------------------
// test suites

struct ABC { int a; };

TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::isComplex<int>::value);
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::isComplex<double>::value);
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::isComplex<float>::value);
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::isComplex<std::string>::value);
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::isComplex<ABC>::value);
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::isComplex<std::complex<int> >::value);
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::isComplex<std::complex<float> >::value);
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::isComplex<std::complex<double> >::value);



BOOST_AUTO_TEST_SUITE(test_mathtools_eigenutil)

BOOST_AUTO_TEST_CASE(denseRandom)
{
  std::mt19937 rng;
  std::uniform_real_distribution<double> dist(0.0, 1.0);
  std::uniform_real_distribution<float> distf(0.f, 1.f);

  constexpr int N = 10000;

  Eigen::VectorXd v(Tomographer::Tools::denseRandom<Eigen::VectorXd>(rng, dist, N));
  MY_BOOST_CHECK_FLOATS_EQUAL(v.sum(), 0.5*N, 2.0/std::sqrt(N)) ;

  Eigen::VectorXd v2(Tomographer::Tools::denseRandom<Eigen::Matrix<double,N,1> >(rng, dist, N));
  MY_BOOST_CHECK_FLOATS_EQUAL(v2.sum(), 0.5*N, 2.0/std::sqrt(N)) ;

  Eigen::Matrix<float,N,1> v3(Tomographer::Tools::denseRandom<Eigen::Matrix<float,Eigen::Dynamic,1> >(rng, distf, N));
  MY_BOOST_CHECK_FLOATS_EQUAL(v3.sum(), 0.5*N, 2.f/std::sqrt((float)N)) ;
}

BOOST_AUTO_TEST_CASE(canonicalBasisVec_1)
{
  auto v1 = Tomographer::Tools::canonicalBasisVec<Eigen::VectorXd>(3, 10);
  Eigen::VectorXd v2(10); v2 << 0,0,0,1,0,0,0,0,0,0;
  MY_BOOST_CHECK_EIGEN_EQUAL(v1, v2, tol);
}
BOOST_AUTO_TEST_CASE(canonicalBasisVec_2)
{
  auto v1 = Tomographer::Tools::canonicalBasisVec<Eigen::Matrix<double,10,1> >(3, 10);
  Eigen::VectorXd v2(10); v2 << 0,0,0,1,0,0,0,0,0,0;
  MY_BOOST_CHECK_EIGEN_EQUAL(v1, v2, tol);
}
BOOST_AUTO_TEST_CASE(canonicalBasisVec_mat)
{
  auto m1 = Tomographer::Tools::canonicalBasisVec<Eigen::Matrix<double,3,3> >(1,2, 3,3);
  Eigen::Matrix3d m2; m2 << 0,0,0, 0,0,1, 0,0,0 ;
  MY_BOOST_CHECK_EIGEN_EQUAL(m1, m2, tol);
}

BOOST_AUTO_TEST_SUITE(powersOfTwo)

BOOST_AUTO_TEST_CASE(basic)
{
  Eigen::VectorXd v1 = Tomographer::Tools::powersOfTwo<Eigen::VectorXd>(10);
  Eigen::VectorXd v2(10); v2 << 1, 2, 4, 8, 16, 32, 64, 128, 256, 512;
  MY_BOOST_CHECK_EIGEN_EQUAL(v1, v2, tol);
}
BOOST_AUTO_TEST_CASE(mat)
{
  Eigen::Matrix3d m1 = Tomographer::Tools::powersOfTwo<Eigen::Matrix3d>();
  Eigen::Matrix3d m2;
  m2 << 1,  8, 64,
        2, 16, 128, 
        4, 32, 256;
  MY_BOOST_CHECK_EIGEN_EQUAL(m1, m2, tol);
}
BOOST_AUTO_TEST_CASE(fixed)
{
  Eigen::Array<double,1,9> twopows = Tomographer::Tools::powersOfTwo<Eigen::Array<double,1,9> >().transpose();
  Eigen::Array<double,1,9> correct_twopows;
  correct_twopows << 1, 2, 4, 8, 16, 32, 64, 128, 256;

  MY_BOOST_CHECK_EIGEN_EQUAL(twopows, correct_twopows, tol);
}

BOOST_AUTO_TEST_CASE(dyn_vector)
{
  Eigen::VectorXd twopows(6);
  twopows = Tomographer::Tools::powersOfTwo<Eigen::VectorXd>(6);
  BOOST_MESSAGE("twopows = " << twopows);
  Eigen::VectorXd correct_twopows(6);
  (correct_twopows << 1, 2, 4, 8, 16, 32).finished();

  MY_BOOST_CHECK_EIGEN_EQUAL(twopows, correct_twopows, tol);
}

BOOST_AUTO_TEST_SUITE_END()



BOOST_AUTO_TEST_SUITE_END()

