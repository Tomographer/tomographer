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

#include <tomographer2/mathtools/pos_semidef_util.h>
#include <tomographer2/mathtools/random_unitary.h>



// -----------------------------------------------------------------------------
// fixture(s)


struct HEADER_fixture {
  HEADER_fixture() { }
  ~HEADER_fixture() { }
  void method() { }
};


// -----------------------------------------------------------------------------
// test suites


BOOST_AUTO_TEST_SUITE(test_mathtools_pos_semidef_util)

BOOST_AUTO_TEST_CASE(force_pos_semidef)
{
  Eigen::Matrix4cd rho;
  rho <<
    -0.1, 0, 0, 0,
    0, 0.05, 0, 0,
    0, 0, 0.55, 0,
    0, 0, 0, 0.5;

  BOOST_CHECK_CLOSE(rho.trace().real(), 1.0, tol_percent);

  Eigen::Matrix4cd rhopos;

  rhopos = Tomographer::MathTools::force_pos_semidef<Eigen::Matrix4cd>(rho, 0.1); // high tolerance, check our algo

  BOOST_CHECK_CLOSE(rhopos.trace().real(), 1.0, tol_percent);

  Eigen::Matrix4cd rhopos_ref_withtol;
  rhopos_ref_withtol <<
    0.1, 0, 0, 0,
    0, 0.1, 0, 0,
    0, 0, 0.425, 0,
    0, 0, 0, 0.375;
  // the 0.25 "excess trace" is evenly subtracted from all good eigenvalues

  MY_BOOST_CHECK_EIGEN_EQUAL(rhopos, rhopos_ref_withtol, tol);


  // should get the same behavior if we apply some Unitary

  // get some nontrivial unitary (fixed by deterministic seeded rng)
  Eigen::Matrix4cd Unitary;
  std::mt19937 rng(1); // seeded, deterministic random number generator
  Tomographer::MathTools::random_unitary(Unitary, rng);

  BOOST_MESSAGE("Chose Unitary = \n" << Unitary) ;

  Eigen::Matrix4cd rhoposU;
  rhoposU = Tomographer::MathTools::force_pos_semidef<Eigen::Matrix4cd>(Unitary*rho*Unitary.adjoint(), 0.1);

  BOOST_CHECK_CLOSE(rhoposU.trace().real(), 1.0, tol_percent);
  MY_BOOST_CHECK_EIGEN_EQUAL(rhoposU, Unitary*rhopos_ref_withtol*Unitary.adjoint(), tol);
}


BOOST_AUTO_TEST_CASE(safe_ops1)
{
  Eigen::Matrix3cd A;
  A <<
    0, 0, 0,
    0, 0, 0,
    0, 0, 1;

  MY_BOOST_CHECK_EIGEN_EQUAL(Tomographer::MathTools::safe_operator_sqrt<Eigen::Matrix3cd>(A, 1e-18), A, 2e-9);
  MY_BOOST_CHECK_EIGEN_EQUAL(Tomographer::MathTools::safe_operator_inv_sqrt<Eigen::Matrix3cd>(A, 1e-12), A, 2e-6);
}

BOOST_AUTO_TEST_CASE(safe_ops2)
{
  Eigen::Matrix3cd U;
  std::mt19937 rng(3982);
  Tomographer::MathTools::random_unitary(U, rng);

  Eigen::Matrix3cd A;
  A <<
    0, 0, 0,
    0, 0, 0,
    0, 0, 1;

  MY_BOOST_CHECK_EIGEN_EQUAL(Tomographer::MathTools::safe_operator_sqrt<Eigen::Matrix3cd>(2*U*A*U.adjoint(), 1e-18),
                             U*A*U.adjoint()*boost::math::constants::root_two<double>(), 2e-9);
  MY_BOOST_CHECK_EIGEN_EQUAL(Tomographer::MathTools::safe_operator_inv_sqrt<Eigen::Matrix3cd>(2*U*A*U.adjoint(), 1e-12),
                             U*A*U.adjoint()*boost::math::constants::half_root_two<double>(), 2e-6);
}

BOOST_AUTO_TEST_SUITE_END();
