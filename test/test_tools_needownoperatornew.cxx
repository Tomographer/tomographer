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
// <Eigen/...> or <tomographer/...> header
#include "test_tomographer.h"

#define TOMOGRAPHER_TEST_TOOLS_NEEDOWNOPERATORNEW_DEBUG_MEMBERS
#include <tomographer/tools/needownoperatornew.h>


// -----------------------------------------------------------------------------
// fixture(s)


struct ABC : public Tomographer::Tools::EigenAlignedOperatorNewProvider
{
  ABC() : m1(Eigen::Matrix4d::Identity()), m2(Eigen::Matrix4d::Zero()) { }
  
  Eigen::Matrix4d m1;
  Eigen::Matrix4d m2;
};

struct DEF : public Tomographer::Tools::NeedOwnOperatorNew<ABC>::ProviderType
{
  DEF() : member() { }
  
  ABC member;
};



// -----------------------------------------------------------------------------
// test suites


BOOST_AUTO_TEST_SUITE(test_needownoperatornew)

BOOST_AUTO_TEST_CASE(abc_def)
{
  DEF * def = new DEF;
  BOOST_CHECK( (((unsigned long)(void*)def) & 0xf) == 0 ) ;
  BOOST_CHECK(def->EigenAlignedOperatorNewIsActive) ;
}

BOOST_AUTO_TEST_SUITE_END()

