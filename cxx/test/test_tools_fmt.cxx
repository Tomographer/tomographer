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

#include <Eigen/Eigen>
#include <tomographer2/tools/fmt.h>



// -----------------------------------------------------------------------------
// fixture(s)


struct NotStreamableObject { int a, b, c;  };

// -----------------------------------------------------------------------------
// test suites


BOOST_AUTO_TEST_SUITE(test_tools_fmt)

BOOST_AUTO_TEST_CASE(fmts)
{
  BOOST_CHECK_EQUAL(Tomographer::Tools::fmts("%d", 10), "10") ;
  BOOST_CHECK_EQUAL(Tomographer::Tools::fmts("%#x", 10), "0xa") ;
  BOOST_CHECK_EQUAL(Tomographer::Tools::fmts("%20s", "1-2-3 hi"), "            1-2-3 hi") ;
  BOOST_CHECK_EQUAL(Tomographer::Tools::fmts("%d ++ %d", 10, 20), "10 ++ 20") ;

  // ### This is not a reliable test (e.g. MinGW's g++ 4.8 fails it), plus, it's not really important.
  //
  //  // see http://stackoverflow.com/a/11349904/1694896
  //  auto call = []() { return Tomographer::Tools::fmts("%lc", (wint_t)-1); } ;
  //  BOOST_CHECK_THROW(call(), // invalid format
  //                    Tomographer::Tools::bad_fmts_format) ;
}

BOOST_AUTO_TEST_CASE(teststreamstr)
{
  BOOST_CHECK_EQUAL(streamstr("one is " << 1 << " and two is " << std::string("*2*")),
                    "one is 1 and two is *2*") ;
  Eigen::Vector3d r; r << 1, 2, 3.4;
  BOOST_CHECK_EQUAL(streamstr("here is a row vector: " << r.transpose() ),
                    "here is a row vector:   1   2 3.4") ;
}

BOOST_AUTO_TEST_CASE(hasostreamop)
{
  BOOST_CHECK( false && "WRITE THIS TEST" );
}
BOOST_AUTO_TEST_CASE(streamifpossiblewrapper)
{
  BOOST_CHECK( false && "WRITE THIS TEST" );
}

BOOST_AUTO_TEST_CASE(streamifpossible)
{
  Eigen::Matrix2d m(Eigen::Matrix2d::Identity());
  NotStreamableObject x;
  std::ostringstream s;
  s << "The identity matrix is = \n" << Tomographer::Tools::streamIfPossible(m) << "\n"
    << "And the value of x is = " << Tomographer::Tools::streamIfPossible(x) << "\n";
  BOOST_MESSAGE(s.str());
  BOOST_CHECK_EQUAL("\n"+s.str(), std::string("\n")+
		    "The identity matrix is = \n"
		    "1 0\n"
		    "0 1\n"
		    "And the value of x is = <"+std::string(typeid(NotStreamableObject).name())+">\n");
}
BOOST_AUTO_TEST_CASE(streamifpossible2)
{
  Eigen::Matrix2d m(Eigen::Matrix2d::Identity());
  NotStreamableObject x;
  std::ostringstream s;
  s << Tomographer::Tools::streamIfPossible(m, "The identity matrix is = \n")
    << " -- and -- "
    << Tomographer::Tools::streamIfPossible(x, "x = ", ".", "Sorry, x is not streamable.")
    << "\n";
  BOOST_MESSAGE(s.str());
  BOOST_CHECK_EQUAL("\n"+s.str(), std::string("\n")+
		    "The identity matrix is = \n"
		    "1 0\n"
		    "0 1 -- and -- Sorry, x is not streamable.\n");
}

BOOST_AUTO_TEST_CASE(fmt_duration)
{
  BOOST_CHECK_EQUAL(Tomographer::Tools::fmt_duration(10.24), "0:00:10.240") ;
  BOOST_CHECK_EQUAL(Tomographer::Tools::fmt_duration(15*3600+43*60+10.24), "15:43:10.240") ;
  BOOST_CHECK_EQUAL(Tomographer::Tools::fmt_duration(125*3600+43*60+10.24), "125:43:10.240") ;

  BOOST_CHECK_EQUAL(Tomographer::Tools::fmt_duration(std::chrono::duration<int, std::kilo>(3)), // 3000 seconds
                    "0:50:00.000") ;
}


BOOST_AUTO_TEST_SUITE_END()

