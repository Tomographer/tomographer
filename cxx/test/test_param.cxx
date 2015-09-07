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

#include <iostream>

// we want `eigen_assert()` to raise an `eigen_assert_exception` here
#include <tomographer/tools/eigen_assert_exception.h>

#include <tomographer/qit/matrq.h>
#include <tomographer/qit/random_unitary.h>
#include <tomographer/qit/param_herm_x.h>
#include <tomographer/qit/param_rho_a.h>

#include <Eigen/Core>

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>


// tolerance, in PERCENT.
const double tol_percent = 1e-8;
const double tol = tol_percent * 0.01;


template<typename MatrQ_>
struct TestParamsFixture
{
  typedef MatrQ_ TheMatrQ;

  TestParamsFixture() { }
  ~TestParamsFixture() { }

  void test_param_x(TheMatrQ matq, const typename TheMatrQ::MatrixType & rho)
  {
    BOOST_MESSAGE("test_param_x(), rho = \n" << rho);
    //std::cout << "-----------------------------------------------\n";
    //std::cout << "rho = \n" << rho << "\n";

    typename TheMatrQ::VectorParamType x = matq.initVectorParamType();
    Tomographer::param_herm_to_x(x, rho);
    //std::cout << " --> x = " << x.transpose() << "\n";
    BOOST_CHECK_SMALL((x.block(0,0,matq.dim(),1) - rho.real().diagonal()).norm(), tol);
    // and convert back to rho
    typename TheMatrQ::MatrixType rhoback1 = matq.initMatrixType();
    Tomographer::param_x_to_herm(rhoback1, x);
    //std::cout << " --> and back to rho = \n" << rhoback1 << "\n";
    BOOST_CHECK_SMALL((rho - rhoback1).norm(), tol);
  }

  void test_param_a(TheMatrQ matq, const typename TheMatrQ::MatrixType & rho)
  {
    BOOST_MESSAGE("test_param_a(), rho = \n" << rho);

    Tomographer::ParamRhoA<TheMatrQ> param(matq);
    // display generalized Gell-Mann matrices
    for (std::size_t l = 0; l < matq.ndof(); ++l) {
      BOOST_MESSAGE("\tlambda[" << l << "] = \n" << param.getLambda(l));
      BOOST_CHECK_SMALL((param.getLambda(l).adjoint() - param.getLambda(l)).norm(), tol_percent);
    }
    // do checks that all HS inner products are correct
    Eigen::MatrixXcd inner_prods = Eigen::MatrixXcd::Zero(matq.ndof(), matq.ndof());
    for (std::size_t i = 0; i < matq.ndof(); ++i) {
      for (std::size_t j = 0; j < matq.ndof(); ++j) {
	inner_prods(i,j) = (param.getLambda(i).adjoint() * param.getLambda(j)).trace();
      }
    }
    BOOST_MESSAGE("Matrix of inner products [expected == 2*Ident]: tr(A'*B) = \n" << inner_prods);
    BOOST_CHECK_SMALL((inner_prods - 2*Eigen::MatrixXcd::Identity(matq.ndof(), matq.ndof())).norm(), tol_percent);

    // ---
    typename TheMatrQ::VectorParamNdofType a = matq.initVectorParamNdofType();
    param.rhoToA(a, rho);
    //std::cout << " --> a = " << a.transpose() << "\n";
    // and convert back to rho
    typename TheMatrQ::MatrixType rhoback2 = matq.initMatrixType();
    param.aToRho(rhoback2, a);
    //std::cout << " --> and back to rho = \n" << rhoback2 << "\n";

    BOOST_CHECK_SMALL((rho - rhoback2).norm(), tol_percent);
  }
};



struct TestFixtureQubitIdent : public TestParamsFixture<Tomographer::QubitPaulisMatrQ> {
  typedef typename TestParamsFixture<Tomographer::QubitPaulisMatrQ>::TheMatrQ TheMatrQ;

  TheMatrQ matq;
  typename TheMatrQ::MatrixType rho;

  TestFixtureQubitIdent()
    : matq(2), rho(matq.initMatrixType())
  {
    rho << 0.5, 0,
           0, 0.5;
  }
};

struct TestFixtureTritExample : public TestParamsFixture<Tomographer::MatrQ<3,1,double,int> > {
  typedef typename TestParamsFixture<Tomographer::MatrQ<3,1,double,int> >::TheMatrQ TheMatrQ;

  TheMatrQ matq;
  typename TheMatrQ::MatrixType rho;

  TestFixtureTritExample()
    : matq(3), rho(matq.initMatrixType())
  {
    typedef typename TheMatrQ::ComplexScalar CD;
    rho.setZero();
    rho(0,0) = 0.2;
    rho(0,1) = CD(0.1, 0.1);
    rho(1,0) = CD(0.1, -0.1);
    rho(1,1) = 0.1;
    rho(2,2) = 0.7;
  }
};

template<int Dim>
struct TestFixtureQuditPure0 : public TestParamsFixture<Tomographer::MatrQ<Dim,1,double,int> > {
  typedef typename TestParamsFixture<Tomographer::MatrQ<Dim,1,double,int> >::TheMatrQ TheMatrQ;

  TheMatrQ matq;
  typename TheMatrQ::MatrixType rho;

  TestFixtureQuditPure0()
    : matq(Dim), rho(matq.initMatrixType())
  {
    //    typedef typename TheMatrQ::ComplexScalar CD; // unused
    rho.setZero();
    rho(0,0) = 1.0;
  }
};

template<int Dim, int RandSeed = 123450>
struct TestFixtureQuditRand : public TestParamsFixture<Tomographer::MatrQ<Dim,1,double,int> > {
  typedef typename TestParamsFixture<Tomographer::MatrQ<Dim,1,double,int> >::TheMatrQ TheMatrQ;

  TheMatrQ matq;
  typename TheMatrQ::MatrixType rho;

  TestFixtureQuditRand()
    : matq(Dim), rho(matq.initMatrixType())
  {
    typename TheMatrQ::MatrixType U = matq.initMatrixType();

    std::mt19937 rng(RandSeed); // seeded, deterministic random number generator

    Tomographer::random_unitary(U, rng);

    //    BOOST_MESSAGE("U = \n" << U);

    rho.setZero();
    for (int k = 0; k < Dim; ++k) {
      rho(k,k) = 1.0 / (1+k);
    }
    rho /= rho.real().trace(); // normalize

    //    BOOST_MESSAGE("rho = \n" << rho);

    // and turn in some new crazy basis.
    rho = U * rho * U.adjoint();
    
    //    BOOST_MESSAGE("rho = \n" << rho);
  }
};



// TEST SUITES
// ---------------------

BOOST_AUTO_TEST_SUITE(test_param)

// X-PARAM

BOOST_AUTO_TEST_SUITE(test_param_x)

BOOST_FIXTURE_TEST_CASE(test_param_x_1, TestFixtureQubitIdent)
{
  test_param_x(matq, rho);
}
BOOST_FIXTURE_TEST_CASE(test_param_x_1b, TestFixtureQubitIdent)
{
  typename TheMatrQ::VectorParamType x = matq.initVectorParamType();
  Tomographer::param_herm_to_x(x, rho);
  BOOST_CHECK_CLOSE(x(0), 0.5, tol_percent);
  BOOST_CHECK_CLOSE(x(1), 0.5, tol_percent);
  BOOST_CHECK_SMALL(x(2), tol_percent);
  BOOST_CHECK_SMALL(x(3), tol_percent);
}
BOOST_FIXTURE_TEST_CASE(test_param_x_1c, TestFixtureQuditPure0<2>)
{
  Eigen::Matrix2cd rho2;
  rho2 << 0.5, 0.5,
          0.5, 0.5;
  typename TheMatrQ::VectorParamType x = matq.initVectorParamType();
  Tomographer::param_herm_to_x(x, rho2);
  BOOST_CHECK_CLOSE(x(0), 0.5, tol_percent);
  BOOST_CHECK_CLOSE(x(1), 0.5, tol_percent);
  BOOST_CHECK_CLOSE(x(2), std::sqrt(2.0)/2.0, tol_percent);
  BOOST_CHECK_SMALL(x(3), tol_percent);
}
BOOST_FIXTURE_TEST_CASE(test_param_x_2, TestFixtureTritExample)
{
  test_param_x(matq, rho);
}
BOOST_FIXTURE_TEST_CASE(test_param_x_3, TestFixtureQuditPure0<4>)
{
  test_param_x(matq, rho);
}
BOOST_FIXTURE_TEST_CASE(test_param_x_3b, TestFixtureQuditPure0<5>)
{
  test_param_x(matq, rho);
}
BOOST_FIXTURE_TEST_CASE(test_param_x_4, TestFixtureQuditRand<5>)
{
  test_param_x(matq, rho);
}

BOOST_AUTO_TEST_SUITE_END()

// A-PARAM

BOOST_AUTO_TEST_SUITE(test_param_a)

BOOST_FIXTURE_TEST_CASE(test_param_a_1, TestFixtureQubitIdent)
{
  test_param_a(matq, rho);
}
BOOST_FIXTURE_TEST_CASE(test_param_a_1b, TestFixtureQubitIdent)
{
  typename TheMatrQ::VectorParamNdofType a = matq.initVectorParamNdofType();
  Tomographer::ParamRhoA<TheMatrQ> param(matq);
  param.rhoToA(a, rho);
  BOOST_CHECK_SMALL(a(0), tol_percent);
  BOOST_CHECK_SMALL(a(1), tol_percent);
  BOOST_CHECK_SMALL(a(2), tol_percent);
}
BOOST_FIXTURE_TEST_CASE(test_param_a_1c, TestFixtureQuditPure0<2>)
{
  Eigen::Matrix2cd rho2;
  rho2 << 0.5, 0.5,
          0.5, 0.5;
  typename TheMatrQ::VectorParamNdofType a = matq.initVectorParamNdofType();
  Tomographer::ParamRhoA<TheMatrQ> param(matq);
  param.rhoToA(a, rho2);
  BOOST_CHECK_CLOSE(a(0), 1.0/std::sqrt(2.0), tol_percent);
  BOOST_CHECK_SMALL(a(1), tol_percent);
  BOOST_CHECK_SMALL(a(2), tol_percent);
}
BOOST_FIXTURE_TEST_CASE(test_param_a_2, TestFixtureTritExample)
{
  test_param_a(matq, rho);
}
BOOST_FIXTURE_TEST_CASE(test_param_a_3, TestFixtureQuditPure0<4>)
{
  test_param_a(matq, rho);
}
BOOST_FIXTURE_TEST_CASE(test_param_a_4, TestFixtureQuditRand<5>)
{
  test_param_a(matq, rho);
}
BOOST_AUTO_TEST_SUITE_END()



BOOST_AUTO_TEST_SUITE_END()
