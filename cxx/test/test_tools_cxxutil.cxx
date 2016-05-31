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


#include "test_tomographer.h"

#include <tomographer/tools/cxxutil.h>




TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::extractFuncName("void class::subclass::subclass(int)") == "class::subclass::subclass");
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::extractFuncName("conststr ns::subclass::method()") == "ns::subclass::method");
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::extractFuncName("int ns::subclass::method(const int&, void, conststr *)") == "ns::subclass::method");
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::extractFuncName("int ns::subclass::operator==(int)") == "ns::subclass::operator==");
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::extractFuncName("int operator==(const ns::subclass&, char)") == "operator==(const ns::subclass&, char)");


BOOST_AUTO_TEST_SUITE(test_tools_cxxutil)

BOOST_AUTO_TEST_CASE(is_complex)
{
  BOOST_CHECK( ! Tomographer::Tools::is_complex<double>::value ) ;
  BOOST_CHECK( ! Tomographer::Tools::is_complex<float>::value ) ;
  BOOST_CHECK( ! Tomographer::Tools::is_complex<int>::value ) ;
  BOOST_CHECK( Tomographer::Tools::is_complex<std::complex<double> >::value ) ;
  BOOST_CHECK( Tomographer::Tools::is_complex<std::complex<float> >::value ) ;
  BOOST_CHECK( Tomographer::Tools::is_complex<std::complex<long double> >::value ) ;
}

BOOST_AUTO_TEST_CASE(is_positive)
{
  BOOST_CHECK( Tomographer::Tools::is_positive(1.0) ) ;
  BOOST_CHECK( Tomographer::Tools::is_positive(1.e-12) ) ;
  BOOST_CHECK( Tomographer::Tools::is_positive(0.0) ) ;
  BOOST_CHECK( ! Tomographer::Tools::is_positive(-1.e-12) ) ;
  BOOST_CHECK( ! Tomographer::Tools::is_positive(-1) ) ;
  BOOST_CHECK( Tomographer::Tools::is_positive(0) ) ;
  BOOST_CHECK( Tomographer::Tools::is_positive<unsigned int>(0xffffffffu) ) ;
  BOOST_CHECK(Tomographer::Tools::is_positive(1u)) ;
  BOOST_CHECK(Tomographer::Tools::is_positive(1)) ;
  BOOST_CHECK(Tomographer::Tools::is_positive(1.f)) ;
  BOOST_CHECK(Tomographer::Tools::is_positive(1.0)) ;
  BOOST_CHECK(!Tomographer::Tools::is_positive(-1)) ;
  BOOST_CHECK(!Tomographer::Tools::is_positive(-1.0)) ;
}

BOOST_AUTO_TEST_CASE(not_implemented)
{
  // need to implement more tests
  BOOST_CHECK(false);
}



BOOST_AUTO_TEST_SUITE_END()
