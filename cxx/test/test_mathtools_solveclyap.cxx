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

#include <tomographer2/mathtools/solveclyap.h>
#include <tomographer2/tools/eigenutil.h> // denseRandom
#include <tomographer2/tools/loggers.h>
#include <tomographer2/mathtools/random_unitary.h>

#include <tomographer2/tools/boost_test_logger.h>


// -----------------------------------------------------------------------------
// fixture(s)


// template<typename MatrixType_, typename Rng_>
// class RandomPosSemiDef
// {
// public:
//   typedef MatrixType_ MatrixType;
//   typedef typename MatrixType::Scalar Scalar;
//   typedef typename Eigen::NumTraits<Scalar>::Real RealScalar;
//   typedef typename MatrixType::Index IndexType;
//   typedef Eigen::Matrix<RealScalar, Eigen::Dynamic, 1> RealVectorType;

//   typedef Rng_ Rng;

// private:
//   Rng & _rng;
//   IndexType _dim;

//   MatrixType _U;
//   RealVectorType _diag;

// public:
//   RandomPosSemiDef(Rng & rng, IndexType dim)
//     : _rng(rng), _dim(dim), _U(dim, dim), _diag(dim)
//   {
//   }

//   MatrixType withUnifEig(RealScalar max_eigenval = RealScalar(1),
// 			 IndexType max_rank = std::numeric_limits<IndexType>::max())
//   {
//     std::uniform_real_distribution<RealScalar> dist(0, max_eigenval);

//     Tomographer::MathTools::randomUnitary<MatrixType>(_U, _rng);
    
//     _diag = Tomographer::Tools::denseRandom<Eigen::VectorXd>(_rng, dist, _dim);

//     for (int i = max_rank; i < _dim; ++i) {
//       _diag(i) = 0;
//     }

//     // this is positive semidefinite.
//     return _U*_diag.asDiagonal()*_U.adjoint();
//   }

//   inline const MatrixType & lastU() const { return _U; }
//   inline const RealVectorType & lastDiag() const { return _diag; }

// };




struct test_solveclyap_fixture
{
  template<typename Rng>
  void do_test(Rng & rng, int d, int A_rank)
  {
    typedef Eigen::MatrixXcd MatType;

    std::uniform_real_distribution<double> dist(0.0, 1.0);

    MatType U(d,d);
    Tomographer::MathTools::randomUnitary<MatType>(U, rng);
    MatType W(U.block(0,0,d,A_rank));

    Eigen::VectorXd eigvals(Tomographer::Tools::denseRandom<Eigen::VectorXd>(rng, dist, A_rank));
    // this is positive semidefinite.
    MatType A(W * eigvals.asDiagonal() * W.adjoint());

    BOOST_MESSAGE("A = " << A) ;

    // create a random X in the support of A
    // MatType X(W * RandomPosSemiDef<MatType, std::mt19937>(rng, A_rank).withUnifEig(1.0, A_rank) * W.adjoint());
    MatType X(W * Tomographer::Tools::denseRandom<MatType>(rng, dist, A_rank, A_rank) * W.adjoint());

    BOOST_MESSAGE("X = " << X) ;

    const MatType C(A.adjoint()*X + X*A);

    BOOST_MESSAGE("--> C = " << C) ;

    MatType X2(d,d);

    BoostTestLogger logger(Tomographer::Logger::DEBUG);

    Tomographer::MathTools::SolveCLyap::solve<true>(X2, A, C, logger, 1e-8);

    MY_BOOST_CHECK_EIGEN_EQUAL(X, X2, 1e-8);
  }
};




// -----------------------------------------------------------------------------
// test suites


BOOST_FIXTURE_TEST_SUITE(test_mathtools_solveclyap, test_solveclyap_fixture)

BOOST_AUTO_TEST_CASE(random_test_7_4)
{
  std::mt19937 rng(4938221);

  const int d = 7; // dimension of problem
  const int A_rank = 4; // rank of A
  for (int repeat = 0; repeat < 1000; ++repeat) {
    BOOST_MESSAGE("Repeat : iteration #" << repeat) ;
    do_test(rng, d, A_rank);
  }
}

BOOST_AUTO_TEST_CASE(random_test_15_15)
{
  std::mt19937 rng(89120);

  const int d = 15; // dimension of problem
  const int A_rank = 15; // rank of A

  for (int repeat = 0; repeat < 100; ++repeat) {
    BOOST_MESSAGE("Repeat : iteration #" << repeat) ;
    do_test(rng, d, A_rank);
  }
}

BOOST_AUTO_TEST_SUITE_END()

