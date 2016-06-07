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

#include <tomographer2/tools/cxxutil.h>




BOOST_AUTO_TEST_SUITE(test_tools_cxxutil)

BOOST_AUTO_TEST_CASE(finally)
{
  bool flag = false;
  { // a scope block
    auto do_this_finally = Tomographer::Tools::finally([&flag]() { flag = true; });
    BOOST_CHECK(!flag);
  }
  BOOST_CHECK(flag);
}

BOOST_AUTO_TEST_SUITE(static_or_dynamic)
TOMO_STATIC_ASSERT_EXPR(sizeof(Tomographer::Tools::static_or_dynamic<long, false, 0x0102030405060708L>) < sizeof(long)) ;
TOMO_STATIC_ASSERT_EXPR(sizeof(Tomographer::Tools::static_or_dynamic<long, true>) >= sizeof(long)) ;
TOMO_STATIC_ASSERT_EXPR(sizeof(Tomographer::Tools::static_or_dynamic<long, true, 0x0102030405060708L>) >= sizeof(long)) ;
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::static_or_dynamic<long, false, 0x0102030405060708L>::IsDynamic == false) ;
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::static_or_dynamic<long, false, 0x0102030405060708L>::StaticValue == 0x0102030405060708L) ;
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::static_or_dynamic<long, true>::IsDynamic == true) ;
BOOST_AUTO_TEST_CASE(static_1)
{
  Tomographer::Tools::static_or_dynamic<long, false, 0x1234L> x;

  typedef Tomographer::Tools::static_or_dynamic<long, false, 0x1234L> TheType;
  BOOST_CHECK(!TheType::IsDynamic);
  BOOST_CHECK_EQUAL(TheType::StaticValue, 0x1234L);
  BOOST_CHECK_EQUAL(x(), 0x1234L);
  BOOST_CHECK_EQUAL(x.value(), 0x1234L);
}
BOOST_AUTO_TEST_CASE(static_2)
{
  Tomographer::Tools::static_or_dynamic<long, false, 0x1234L> x(0x1234L);
  typedef Tomographer::Tools::static_or_dynamic<long, false, 0x1234L> TheType;
  BOOST_CHECK(!TheType::IsDynamic);
  BOOST_CHECK_EQUAL(x(), 0x1234L);
  BOOST_CHECK_EQUAL(x.value(), 0x1234L);
}
BOOST_AUTO_TEST_CASE(static_3)
{
  EigenAssertTest::setting_scope settingvariable(true);
  auto test = []() {
    Tomographer::Tools::static_or_dynamic<long, false, 0x1234L> x(0x5678L); // wrong dynamic argument
    BOOST_MESSAGE("[!!!!!!This point should never be reached!!!!] Value of x = " << x()) ;
  };
  BOOST_CHECK_THROW(test(), Tomographer::Tools::eigen_assert_exception) ;
}
BOOST_AUTO_TEST_CASE(dyn)
{
  Tomographer::Tools::static_or_dynamic<long, true> x(0x1234L);
  typedef Tomographer::Tools::static_or_dynamic<long, true> TheType;
  BOOST_CHECK(TheType::IsDynamic);
  BOOST_CHECK_EQUAL(x(), 0x1234L);
  BOOST_CHECK_EQUAL(x.value(), 0x1234L);
}
BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(store_if_enabled)
struct TestBigObject {
  char d[1024];
  TestBigObject(char a, char b, char c) : d{a, b, c, 0} { }
  TestBigObject() : d{0} { }
};
std::ostream & operator<<(std::ostream& str, const TestBigObject x) {
  str << "*" << x.d << "*";
  return str;
}
TOMO_STATIC_ASSERT_EXPR(sizeof(Tomographer::Tools::store_if_enabled<TestBigObject, false>) < sizeof(TestBigObject));
TOMO_STATIC_ASSERT_EXPR(sizeof(Tomographer::Tools::store_if_enabled<TestBigObject, true>) >= sizeof(TestBigObject));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::store_if_enabled<TestBigObject, false>::IsEnabled);
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::store_if_enabled<TestBigObject, true>::IsEnabled);
BOOST_AUTO_TEST_CASE(disabled_1)
{
  Tomographer::Tools::store_if_enabled<TestBigObject, false> x('c', 'a', 'c'); // should ignore its arguments
  typedef Tomographer::Tools::store_if_enabled<TestBigObject, false> TheType;
  BOOST_CHECK(!TheType::IsEnabled);
}
BOOST_AUTO_TEST_CASE(enabled_1)
{
  Tomographer::Tools::store_if_enabled<long, true> x(0x1234L);
  typedef Tomographer::Tools::store_if_enabled<long, true> TheType;
  BOOST_CHECK(TheType::IsEnabled);
  BOOST_CHECK_EQUAL(x.value, 0x1234L);
  x.value = 0x0102030405060708L;
  BOOST_CHECK_EQUAL(x.value, 0x0102030405060708L);
}
BOOST_AUTO_TEST_CASE(ostream_disabled)
{
  Tomographer::Tools::store_if_enabled<TestBigObject, false> x('c', 'a', 'z'); // should ignore its arguments
  std::stringstream s;
  s << x;
  BOOST_CHECK_EQUAL(s.str(), std::string("[-]"));
}
BOOST_AUTO_TEST_CASE(ostream_enabled)
{
  Tomographer::Tools::store_if_enabled<TestBigObject, true> x('c', 'a', 'z'); // should ignore its arguments
  std::stringstream s;
  s << x;
  BOOST_CHECK_EQUAL(s.str(), std::string("*caz*"));
}
BOOST_AUTO_TEST_SUITE_END()


TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::is_power_of_two(1));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::is_power_of_two(2));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::is_power_of_two(4));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::is_power_of_two(8));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::is_power_of_two(16));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::is_power_of_two(32));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::is_power_of_two(64));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::is_power_of_two(128));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::is_power_of_two(1024));
TOMO_STATIC_ASSERT_EXPR( Tomographer::Tools::is_power_of_two(0x0001000000000000UL));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::is_power_of_two(0));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::is_power_of_two(3));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::is_power_of_two(5));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::is_power_of_two(6));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::is_power_of_two(7));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::is_power_of_two(9));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::is_power_of_two(30));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::is_power_of_two(31));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::is_power_of_two(33));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::is_power_of_two(34));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::is_power_of_two(0x0001000100000000UL));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::is_power_of_two(0x0000ffff00000000UL));


BOOST_AUTO_TEST_CASE(is_complex)
{
  BOOST_CHECK( ! Tomographer::Tools::is_complex<double>::value ) ;
  BOOST_CHECK( ! Tomographer::Tools::is_complex<float>::value ) ;
  BOOST_CHECK( ! Tomographer::Tools::is_complex<int>::value ) ;
  BOOST_CHECK( Tomographer::Tools::is_complex<std::complex<double> >::value ) ;
  BOOST_CHECK( Tomographer::Tools::is_complex<std::complex<float> >::value ) ;
  BOOST_CHECK( Tomographer::Tools::is_complex<std::complex<long double> >::value ) ;
}

struct ABCZ {
  ABCZ() = delete;
  ABCZ(int, char, long, const char *) { }
  bool test() const { return true; }
};

BOOST_AUTO_TEST_CASE(complex_real_scalar)
{
  Tomographer::Tools::complex_real_scalar<std::complex<int> >::type x(100);
  Tomographer::Tools::complex_real_scalar<std::complex<double> >::type y(1.4);
  Tomographer::Tools::complex_real_scalar<std::complex<float> >::type z(1.4f);
  Tomographer::Tools::complex_real_scalar<std::complex<ABCZ> >::type w(1, 'c', 5L, "hello");
  BOOST_CHECK_EQUAL(x, 100);
  BOOST_CHECK_CLOSE(y, 1.4, tol_percent);
  BOOST_CHECK_CLOSE(z, 1.4f, tol_percent);
  BOOST_CHECK(w.test());
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


TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::extractFuncName("void class::subclass::subclass(int)") == "class::subclass::subclass");
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::extractFuncName("conststr ns::subclass::method()") == "ns::subclass::method");
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::extractFuncName("int ns::subclass::method(const int&, void, conststr *)") == "ns::subclass::method");
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::extractFuncName("int ns::subclass::operator==(int)") == "ns::subclass::operator==");
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::extractFuncName("int operator==(const ns::subclass&, char)") == "operator==(const ns::subclass&, char)");


BOOST_AUTO_TEST_SUITE_END()





