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

#include <tomographer/densedm/param_rho_a.h>
#include <tomographer/densedm/param_herm_x.h>


#define SQRT2  boost::math::constants::root_two<double>()
#define INVSQRT2  boost::math::constants::half_root_two<double>()



// -----------------------------------------------------------------------------
// fixture(s)

#include "test_densedm_param_common.h"


// -----------------------------------------------------------------------------
// test suites


BOOST_AUTO_TEST_SUITE(test_densedm_param_rho_a)


BOOST_FIXTURE_TEST_CASE(test_param_a_1, TestFixtureQubitIdent)
{
  test_param_a(dmt, rho);
}
BOOST_FIXTURE_TEST_CASE(test_param_a_1b, TestFixtureQubitIdent)
{
  typename DMTypes::VectorParamNdofType a(dmt.initVectorParamNdofType());
  Tomographer::DenseDM::ParamA<DMTypes> param(dmt);
  a = param.rhoToA(rho);
  BOOST_CHECK_SMALL(a(0), tol_percent);
  BOOST_CHECK_SMALL(a(1), tol_percent);
  BOOST_CHECK_SMALL(a(2), tol_percent);
}
BOOST_FIXTURE_TEST_CASE(test_param_a_1c, TestFixtureQuditPure0<2>)
{
  Eigen::Matrix2cd rho2;
  rho2 << 0.5, 0.5,
          0.5, 0.5;
  Tomographer::DenseDM::ParamA<DMTypes> param(dmt);
  typename DMTypes::VectorParamNdofType a = param.rhoToA(rho2);
  BOOST_CHECK_CLOSE(a(0), 1.0/std::sqrt(2.0), tol_percent);
  BOOST_CHECK_SMALL(a(1), tol_percent);
  BOOST_CHECK_SMALL(a(2), tol_percent);
}
BOOST_FIXTURE_TEST_CASE(test_param_a_2, TestFixtureTritExample)
{
  test_param_a(dmt, rho);
}
BOOST_FIXTURE_TEST_CASE(test_param_a_3, TestFixtureQuditPure0<4>)
{
  test_param_a(dmt, rho);
}
BOOST_FIXTURE_TEST_CASE(test_param_a_4, TestFixtureQuditRand<5>)
{
  test_param_a(dmt, rho);
}


BOOST_AUTO_TEST_SUITE_END()

