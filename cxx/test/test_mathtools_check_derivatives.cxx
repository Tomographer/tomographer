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

// definitions for Tomographer test framework -- this must be included before any
// <Eigen/...> or <tomographer2/...> header
#include "test_tomographer.h"

#include <tomographer2/mathtools/check_derivatives.h>
#include <tomographer2/tools/eigenutil.h>


// -----------------------------------------------------------------------------
// fixture(s)


template<typename EigenPointType_, typename EigenDerivativesType_, int XDims, int ValDims>
struct check_derivatives_fixture
{
  static constexpr int xdims = XDims;
  static constexpr int valdims = ValDims;

  typedef EigenPointType_ EigenPointType;
  typedef EigenDerivativesType_ EigenDerivativesType;

  typedef typename EigenPointType::Scalar XScalar;
  typedef typename EigenDerivativesType::Scalar ValScalar;

  check_derivatives_fixture() { }
  ~check_derivatives_fixture() { }

  inline EigenPointType random_point(int seed) const
  {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<XScalar> dist(0.1, 2.0);
    return Tomographer::Tools::denseRandom<EigenPointType>(rng, dist, xdims);
  }

  //
  // Function: f_j(\vec x) =  \sum (1+i+2*j) * x_i^(1+(i%2))
  //
  static void fn(Eigen::Ref<Eigen::Matrix<ValScalar, Eigen::Dynamic, 1> > vals,
                 const Eigen::Ref<const EigenPointType> & x)
  {
    vals = Eigen::Matrix<ValScalar, Eigen::Dynamic, 1>::Zero(valdims);
    for (int j = 0; j < valdims; ++j) {
      for (int i = 0; i < xdims; ++i) {
        vals(j) += (1+i+2*j) * std::pow(x(i), 1+(i%2));
      }
    }
  }

  void derivative_at(Eigen::Ref<EigenDerivativesType> derivatives,
                     const Eigen::Ref<const EigenPointType> & x)
  {
    for (int j = 0; j < valdims; ++j) {
      for (int i = 0; i < xdims; ++i) {
        derivatives(j,i) = (1+i+2*j) * (1+(i%2)) * std::pow(x(i),i%2);
      }
    }
  }
  
};

template<typename EigenPointType_, typename EigenDerivativesType_, int XDims, int ValDims>
constexpr int check_derivatives_fixture<EigenPointType_,EigenDerivativesType_, XDims, ValDims>::xdims;
template<typename EigenPointType_, typename EigenDerivativesType_, int XDims, int ValDims>
constexpr int check_derivatives_fixture<EigenPointType_,EigenDerivativesType_, XDims, ValDims>::valdims;

// -----------------------------------------------------------------------------
// test suites


typedef check_derivatives_fixture<Eigen::VectorXd, Eigen::ArrayXXd, 4, 1> Fixture1val;
typedef check_derivatives_fixture<Eigen::VectorXd, Eigen::ArrayXXf, 4, 1> Fixture1val_d_f;
typedef check_derivatives_fixture<Eigen::VectorXd, Eigen::ArrayXXd, 10, 6> FixtureSeveralVals;

BOOST_AUTO_TEST_SUITE(test_mathtools_check_derivatives)

BOOST_FIXTURE_TEST_CASE(one_val, Fixture1val)
{
  BOOST_MESSAGE("enter test; xdims="<<xdims<<", valdims="<<valdims) ;

  EigenPointType x(random_point(0));
  EigenDerivativesType der(EigenDerivativesType::Zero(valdims,xdims));

  derivative_at(der, x);

  BOOST_MESSAGE("Derivatives = \n" << der) ;

  BOOST_MESSAGE("test correct ...") ;
  {
    std::stringstream stream;
    bool ok = Tomographer::MathTools::check_derivatives(der, x, fn, valdims, 1e-6, 1e-4, stream);
    BOOST_MESSAGE(stream.str()) ;
    BOOST_CHECK(ok) ;
  }

  der(0,2) = der(0,2)*2 + 1.0;

  BOOST_MESSAGE("test wrong ...") ;
  {
    std::stringstream stream;
    bool ok = Tomographer::MathTools::check_derivatives(der, x, fn, valdims, 1e-6, 1e-4, stream);
    BOOST_MESSAGE(stream.str()) ;
    BOOST_CHECK( !ok ) ;
  }
}

BOOST_FIXTURE_TEST_CASE(one_val_d_f, Fixture1val_d_f)
{
  EigenPointType x(random_point(90876));
  EigenDerivativesType der(EigenDerivativesType::Zero(valdims, xdims));

  derivative_at(der, x);

  {
    std::stringstream stream;
    bool ok = Tomographer::MathTools::check_derivatives(der, x, fn, valdims, 1e-4, 1e-2f, stream);
    BOOST_MESSAGE(stream.str()) ;
    BOOST_CHECK(ok) ;
  }

  der(0,2) = der(0,2)*2 + 1.0;

  {
    std::stringstream stream;
    bool ok = Tomographer::MathTools::check_derivatives(der, x, fn, valdims, 1e-4, 1e-2f, stream);
    BOOST_MESSAGE(stream.str()) ;
    BOOST_CHECK( !ok ) ;
  }
}

BOOST_FIXTURE_TEST_CASE(several_vals, FixtureSeveralVals)
{
  EigenPointType x(random_point(151));
  EigenDerivativesType der(EigenDerivativesType::Zero(valdims, xdims));

  derivative_at(der, x);

  {
    std::stringstream stream;
    bool ok = Tomographer::MathTools::check_derivatives(der, x, fn, valdims, 1e-6, 1e-4, stream);
    BOOST_MESSAGE(stream.str()) ;
    BOOST_CHECK(ok) ;
  }

  der(0,2) = der(0,2)*2 + 1.0;

  {
    std::stringstream stream;
    bool ok = Tomographer::MathTools::check_derivatives(der, x, fn, valdims, 1e-6, 1e-4, stream);
    BOOST_MESSAGE(stream.str()) ;
    BOOST_CHECK( !ok ) ;
  }
}



// -----------------------------------------------

BOOST_FIXTURE_TEST_CASE(checks_for_nan, Fixture1val)
{
  BOOST_MESSAGE("enter test; xdims="<<xdims<<", valdims="<<valdims) ;

  EigenPointType x(random_point(3242));
  EigenDerivativesType der(EigenDerivativesType::Zero(valdims,xdims));

  derivative_at(der, x);

  der(0,2) = std::numeric_limits<double>::quiet_NaN();

  BOOST_MESSAGE("Derivatives = \n" << der) ;

  BOOST_MESSAGE("check that check_derivatives() complains for NaN ...") ;
  {
    EigenAssertTest::setting_scope mysettingvar(true);// eigen_assert() should throw an exception.
    std::stringstream stream;
    auto call_test = [&]() { Tomographer::MathTools::check_derivatives(der, x, fn, valdims, 1e-6, 1e-4, stream); };
    BOOST_CHECK_THROW(call_test(), Tomographer::Tools::EigenAssertException);
    BOOST_MESSAGE(stream.str()) ;
  }
}


BOOST_AUTO_TEST_SUITE_END()

