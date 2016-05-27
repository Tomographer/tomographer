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
#include <sstream>
#include <random>

#include <boost/math/constants/constants.hpp>

// include before <Eigen/*> !
#include "test_tomographer.h"

#include <tomographer/densedm/dmtypes.h>



// -----------------------------------------------------------------------------
// fixture(s)


struct test_dmtypes_fixture {
  test_dmtypes_fixture()
  {
  }
  ~test_dmtypes_fixture()
  {
  }
  template<typename DMTypes> void test_dmtypes(const int dim)
  {
    // has RealScalar and ComplexScalar types
    typename DMTypes::RealScalar a = 1.0;
    typename DMTypes::ComplexScalar z(3.0, 4.0);
    
    BOOST_CHECK_CLOSE(a, 1.0, tol_percent);
    BOOST_CHECK_CLOSE(std::abs(z), 5.0, tol_percent);
    
    DMTypes dmt(dim);

    // has dim() property
    BOOST_CHECK_EQUAL(dmt.dim(), dim);

    BOOST_CHECKPOINT("About to test that a wrong fixed dimension will throw");

    // if has fixed dim, make sure that if we attempt to construct a bad dimension,
    // something explodes
    if (DMTypes::FixedDim != Eigen::Dynamic) {
      //      fprintf(stderr, "entering block of interest...\n");
      EigenAssertTest::setting_scope settingvariable(true); // eigen_assert() should throw an exception.
      //      fprintf(stderr, "initialized setting variable\n");
      BOOST_CHECK_THROW(
     	  DMTypes baddmt(dim+1),
     	  ::Tomographer::Tools::eigen_assert_exception
     	  );
      //      fprintf(stderr, "exiting block of interest\n");
    }
    
    // matrix type
    BOOST_CHECKPOINT("Testing MatrixType");

    typename DMTypes::MatrixType rho(dmt.initMatrixType());
    BOOST_CHECK_EQUAL(rho.cols(), dim);
    BOOST_CHECK_EQUAL(rho.rows(), dim);

    // initializes to zero
    BOOST_CHECK_CLOSE(rho.norm(), 0, tol_percent);

    // rho is a superposition of the two first basis states
    rho(0,0) = typename DMTypes::ComplexScalar(.5, 0.0);
    rho(0,1) = typename DMTypes::ComplexScalar(0.0, .5);
    rho(1,0) = typename DMTypes::ComplexScalar(0.0, -.5);
    rho(1,1) = typename DMTypes::ComplexScalar(.5, 0.0);
    BOOST_CHECK_CLOSE(rho(0,1).imag(), .5, tol_percent);
    BOOST_CHECK_CLOSE(rho.trace().real(), 1.0, tol_percent);
    BOOST_CHECK_CLOSE(rho.trace().imag(), 0.0, tol_percent);
    BOOST_CHECK_CLOSE(rho.eigenvalues().real().maxCoeff(), 1.0, tol_percent);

    // vector param type
    BOOST_CHECKPOINT("Testing VectorParamType");

    typename DMTypes::VectorParamType x(dmt.initVectorParamType());
    BOOST_CHECK_EQUAL(x.cols(), 1);
    BOOST_CHECK_EQUAL(x.rows(), dim*dim);
    BOOST_CHECK_CLOSE(x.norm(), 0, tol_percent);
    
    // vector param Ndof type
    BOOST_CHECKPOINT("Testing VectorParamNdofType");
    typename DMTypes::VectorParamNdofType x2(dmt.initVectorParamNdofType());
    BOOST_CHECK_EQUAL(x2.cols(), 1);
    BOOST_CHECK_EQUAL(x2.rows(), dim*dim-1);
    BOOST_CHECK_CLOSE(x2.norm(), 0, tol_percent);
  }
};



// -----------------------------------------------------------------------------
// test suites


BOOST_FIXTURE_TEST_SUITE(test_densedm_densedmtypes, test_dmtypes_fixture)

BOOST_AUTO_TEST_CASE(qubit_static)
{
  test_dmtypes<Tomographer::DenseDM::DMTypes<2> >(2);
}
BOOST_AUTO_TEST_CASE(qubit_dyn)
{
  test_dmtypes<Tomographer::DenseDM::DMTypes<Eigen::Dynamic> >(2);
}
BOOST_AUTO_TEST_CASE(qudit_static)
{
  test_dmtypes<Tomographer::DenseDM::DMTypes<10> >(10);
}
BOOST_AUTO_TEST_CASE(qudit_dyn)
{
  test_dmtypes<Tomographer::DenseDM::DMTypes<Eigen::Dynamic> >(10);
}

BOOST_AUTO_TEST_SUITE_END()

