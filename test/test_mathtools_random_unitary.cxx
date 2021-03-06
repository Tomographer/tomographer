/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
 * Copyright (c) 2017 Caltech, Institute for Quantum Information and Matter, Philippe Faist
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
// <Eigen/...> or <tomographer/...> header
#include "test_tomographer.h"

#include <tomographer/mathtools/random_unitary.h>



// -----------------------------------------------------------------------------
// fixture(s)



// -----------------------------------------------------------------------------
// test suites


BOOST_AUTO_TEST_SUITE(test_mathtools_randomUnitary)


BOOST_AUTO_TEST_CASE(basic)
{
  Eigen::MatrixXcd U(7,7);
  
  std::mt19937 rng(43423); // seeded, deterministic random number generator

  // check that the given U is unitary, for a couple tries
  
  for (int k = 0; k < 1000; ++k) {
    BOOST_MESSAGE("Running randomUnitary() for the "<<k<<"-th time") ;

    Tomographer::MathTools::randomUnitary(U, rng);
    
    MY_BOOST_CHECK_FLOATS_EQUAL((U * U.adjoint() - Eigen::MatrixXcd::Identity(7,7)).norm(), 0, 1e-12);
    MY_BOOST_CHECK_FLOATS_EQUAL((U.adjoint() * U - Eigen::MatrixXcd::Identity(7,7)).norm(), 0, 1e-12);
  }
}

//
// Test that if we average rho over many random unitaries from randomUnitary, we get the
// maximally mixed state; this then shows/indicates(?) that randomUnitary is indeed
// distributed according to the Haar measures.
//
BOOST_AUTO_TEST_CASE(twirl_rho_gives_identity)
{
  Eigen::Matrix3cd rho;
  rho << 0.2, 0, 0,
         0, 0.5, 0,
         0, 0, 0.3 ;

  std::mt19937 rng(4832342u);

  Eigen::Matrix3cd U;
  
  Eigen::Matrix3cd rhoTwirled(Eigen::Matrix3cd::Zero());

  const int n_points = 10000;
  for (int k = 0; k < n_points; ++k) {
    Tomographer::MathTools::randomUnitary(U, rng);

    rhoTwirled.noalias() += U*rho*U.adjoint();
  }

  rhoTwirled /= n_points;

  BOOST_MESSAGE("rho averaged over random unitaries is  rhoTwirled = \n" << rhoTwirled);

  MY_BOOST_CHECK_EIGEN_EQUAL(rhoTwirled, Eigen::Matrix3cd::Identity()/3.0, 0.5/std::sqrt((double)n_points));
}


BOOST_AUTO_TEST_SUITE_END()

