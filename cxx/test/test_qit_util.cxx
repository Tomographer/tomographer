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

#include <cstdio>
#include <random>
#include <iostream>
#include <vector>

// definitions for Tomographer test framework -- this must be included before any
// <tomographer/...> header
#include "test_tomographer.h"

#include <tomographer/qit/util.h>
#include <tomographer/tools/util.h>

struct ABC { int a; };


TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::is_complex<int>::value);
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::is_complex<double>::value);
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::is_complex<float>::value);
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::is_complex<std::string>::value);
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::is_complex<ABC>::value);
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::is_complex<std::complex<int> >::value);
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::is_complex<std::complex<float> >::value);
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::is_complex<std::complex<double> >::value);


BOOST_AUTO_TEST_SUITE(test_misc)

BOOST_AUTO_TEST_CASE(is_positive)
{
  BOOST_CHECK(Tomographer::Tools::is_positive(1u)) ;
  BOOST_CHECK(Tomographer::Tools::is_positive(1)) ;
  BOOST_CHECK(Tomographer::Tools::is_positive(1.f)) ;
  BOOST_CHECK(Tomographer::Tools::is_positive(1.0)) ;
  BOOST_CHECK(!Tomographer::Tools::is_positive(-1)) ;
  BOOST_CHECK(!Tomographer::Tools::is_positive(-1.0)) ;
}

BOOST_AUTO_TEST_SUITE(powers_of_two)

BOOST_AUTO_TEST_CASE(fixed)
{
  Eigen::Array<double,1,9> twopows = Tomographer::powers_of_two<Eigen::Array<double,1,9> >().transpose();
  Eigen::Array<double,1,9> correct_twopows;
  correct_twopows << 1, 2, 4, 8, 16, 32, 64, 128, 256;

  MY_BOOST_CHECK_EIGEN_EQUAL(twopows, correct_twopows, tol);
}

BOOST_AUTO_TEST_CASE(dyn_vector)
{
  Eigen::VectorXd twopows(6);
  twopows = Tomographer::powers_of_two<Eigen::VectorXd>(6);
  BOOST_MESSAGE("twopows = " << twopows);
  Eigen::VectorXd correct_twopows(6);
  (correct_twopows << 1, 2, 4, 8, 16, 32).finished();

  MY_BOOST_CHECK_EIGEN_EQUAL(twopows, correct_twopows, tol);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
