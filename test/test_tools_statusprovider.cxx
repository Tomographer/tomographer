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

#include <tomographer/tools/statusprovider.h>


// -----------------------------------------------------------------------------
// fixture(s)

struct SimpleDummyObject {
  std::string dummy_status_message;
  SimpleDummyObject(std::string msg) : dummy_status_message(msg) { }
};

namespace Tomographer { namespace Tools {
template<>
struct StatusProvider<SimpleDummyObject>
{
  static constexpr bool CanProvideStatusLine = true;

  static inline std::string getStatusLine(const SimpleDummyObject * stats)
  {
    return stats->dummy_status_message;
  }
};
} } // namespaces


// -----------------------------------------------------------------------------
// test suites


BOOST_AUTO_TEST_SUITE(test_tools_statusprovider)


BOOST_AUTO_TEST_CASE(base_no_status)
{
  TOMO_STATIC_ASSERT_EXPR( ! Tomographer::Tools::StatusQuery<int>::CanProvideStatusLine );
  BOOST_CHECK_EQUAL( Tomographer::Tools::StatusQuery<int>::getStatusLine(NULL) , std::string() );
}

BOOST_AUTO_TEST_CASE(base_simple_status)
{
  TOMO_STATIC_ASSERT_EXPR( Tomographer::Tools::StatusQuery<SimpleDummyObject>::CanProvideStatusLine );

  SimpleDummyObject sc1("status - 1");
  SimpleDummyObject sc2("status - 2");
  BOOST_CHECK_EQUAL( Tomographer::Tools::StatusQuery<SimpleDummyObject>::getStatusLine(&sc1) , "status - 1" );
  BOOST_CHECK_EQUAL( Tomographer::Tools::StatusQuery<SimpleDummyObject>::getStatusLine(&sc2) , "status - 2" );

  // check also that the symbol is defined, hence also usable in a non-constexpr context
  int x = Tomographer::Tools::StatusQuery<SimpleDummyObject>::CanProvideStatusLine ;
  BOOST_CHECK( x );


  TOMO_STATIC_ASSERT_EXPR( ! Tomographer::Tools::StatusQuery<SimpleDummyObject>::CanProvideStatusFullMessage );
  BOOST_CHECK_EQUAL( Tomographer::Tools::StatusQuery<SimpleDummyObject>::getStatusFullMessage(&sc1) , "" );
}

BOOST_AUTO_TEST_SUITE_END()

