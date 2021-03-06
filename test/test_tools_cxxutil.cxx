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


#include <type_traits>
#include <stdexcept>

#include "test_tomographer.h"

#include <tomographer/tools/cxxutil.h>


// utils & fixtures

struct Dummy { };

// tei=tomographer-enabled-if
template<typename Type>
struct teiA {
  TOMOGRAPHER_ENABLED_IF(std::is_same<Type, int>::value)
  static inline bool is_int() { return true; }
  TOMOGRAPHER_ENABLED_IF(!std::is_same<Type, int>::value)
  static inline bool is_int() { return false; }

  template<typename Type2, TOMOGRAPHER_ENABLED_IF_TMPL(std::is_same<Type, Type2>::value)>
  static inline bool is_same_as() { return true; }
  template<typename Type2, TOMOGRAPHER_ENABLED_IF_TMPL(!std::is_same<Type, Type2>::value)>
  static inline bool is_same_as() { return false; }
};



template<typename IntType, bool enable>
struct test_addition_will_overflow_pos {
  static inline void go()
  {
    { constexpr auto yn = Tomographer::Tools::additionWillOverflow<IntType>(
          IntType(-10), IntType(-250)) ;
      BOOST_CHECK(!yn) ; }
    { constexpr auto yn = Tomographer::Tools::additionWillOverflow<IntType>(
          IntType(10), IntType(-250)) ;
      BOOST_CHECK(!yn) ; }
    { constexpr auto yn = Tomographer::Tools::additionWillOverflow<IntType>(
          IntType(10), std::numeric_limits<IntType>::min() ) ;
      BOOST_CHECK(!yn) ; }

    { constexpr auto yn = Tomographer::Tools::additionWillOverflow<IntType>(
          std::numeric_limits<IntType>::min(), IntType(+1)) ;
      BOOST_CHECK(!yn) ; }
    { constexpr auto yn = Tomographer::Tools::additionWillOverflow<IntType>(
          std::numeric_limits<IntType>::max(), IntType(-1)) ;
      BOOST_CHECK(!yn) ; }
    { constexpr auto yn = Tomographer::Tools::additionWillOverflow<IntType>(
          std::numeric_limits<IntType>::min(), IntType(-1)) ;
      BOOST_CHECK(yn) ; }
  }
};
template<typename IntType>
struct test_addition_will_overflow_pos<IntType,false> { static inline void go() { } }; // no-op for unsigned

template<typename IntType>
void test_addition_will_overflow()
{
  { constexpr auto yn = Tomographer::Tools::additionWillOverflow<IntType>(
        IntType(153), IntType(102)) ; // still safe on 8 bits
    BOOST_CHECK(!yn) ; }
  { constexpr auto yn = Tomographer::Tools::additionWillOverflow<IntType>(
        IntType(0), std::numeric_limits<IntType>::max()) ;
    BOOST_CHECK(!yn) ; }
  { constexpr auto yn = Tomographer::Tools::additionWillOverflow<IntType>(
        IntType(1), std::numeric_limits<IntType>::max()) ;
    BOOST_CHECK(yn) ; }

  { constexpr auto yn = Tomographer::Tools::additionWillOverflow<IntType>(
        IntType(0), IntType(1), IntType(3), IntType(10), IntType(std::numeric_limits<IntType>::max()-14)) ;
    BOOST_CHECK(!yn) ; }
  { constexpr auto yn = Tomographer::Tools::additionWillOverflow<IntType>(
        IntType(0), IntType(2), IntType(3), IntType(10), IntType(std::numeric_limits<IntType>::max()-14)) ;
    BOOST_CHECK(yn) ; }

  { constexpr auto yn = Tomographer::Tools::additionWillOverflow<IntType>(
        std::numeric_limits<IntType>::max()/2, std::numeric_limits<IntType>::max()/2) ;
    BOOST_CHECK(!yn) ; }
  { constexpr auto yn = Tomographer::Tools::additionWillOverflow<IntType>(
        IntType(std::numeric_limits<IntType>::max()/2+1), IntType(1), std::numeric_limits<IntType>::max()/2) ;
    BOOST_CHECK(yn) ; }

  { constexpr auto yn = Tomographer::Tools::additionWillOverflow<IntType>(
        std::numeric_limits<IntType>::min(), std::numeric_limits<IntType>::max()) ;
    BOOST_CHECK(!yn) ; }

  test_addition_will_overflow_pos<IntType, std::is_signed<IntType>::value>::go() ;
};




// test cases

BOOST_AUTO_TEST_SUITE(test_tools_cxxutil)

// test TOMOGRAPHER_ENABLED_IF[_TMPL]

BOOST_AUTO_TEST_CASE(tomographer_enabled_if)
{
  BOOST_CHECK(teiA<int>::is_int());
  BOOST_CHECK(!teiA<long>::is_int());
  BOOST_CHECK(!teiA<Dummy>::is_int());
  
  BOOST_CHECK(teiA<int>::is_same_as<int>());
  BOOST_CHECK(!teiA<long>::is_same_as<int>());
  BOOST_CHECK(teiA<Dummy>::is_same_as<Dummy>());
  BOOST_CHECK(!teiA<Dummy>::is_same_as<std::string>());
}

BOOST_AUTO_TEST_CASE(finally)
{
  bool flag = false;
  { // a scope block
    auto do_this_finally = Tomographer::Tools::finally([&flag]() { flag = true; });
    BOOST_CHECK(!flag);
  }
  BOOST_CHECK(flag);
}

BOOST_AUTO_TEST_SUITE(StaticOrDynamic)
TOMO_STATIC_ASSERT_EXPR(sizeof(Tomographer::Tools::StaticOrDynamic<long, false, 0x05060708L>) < sizeof(long)) ;
TOMO_STATIC_ASSERT_EXPR(sizeof(Tomographer::Tools::StaticOrDynamic<long, true>) >= sizeof(long)) ;
TOMO_STATIC_ASSERT_EXPR(sizeof(Tomographer::Tools::StaticOrDynamic<long, true, 0x05060708L>) >= sizeof(long)) ;
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::StaticOrDynamic<long, false, 0x05060708L>::IsDynamic == false) ;
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::StaticOrDynamic<long, false, 0x05060708L>::StaticValue == 0x05060708L) ;
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::StaticOrDynamic<long, true>::IsDynamic == true) ;
BOOST_AUTO_TEST_CASE(static_1)
{
  Tomographer::Tools::StaticOrDynamic<long, false, 0x1234L> x;

  typedef Tomographer::Tools::StaticOrDynamic<long, false, 0x1234L> TheType;
  BOOST_CHECK(!TheType::IsDynamic);
  BOOST_CHECK_EQUAL(TheType::StaticValue, 0x1234L);
  BOOST_CHECK_EQUAL(x(), 0x1234L);
  BOOST_CHECK_EQUAL(x.value(), 0x1234L);
}
BOOST_AUTO_TEST_CASE(static_2)
{
  Tomographer::Tools::StaticOrDynamic<long, false, 0x1234L> x(0x1234L);
  typedef Tomographer::Tools::StaticOrDynamic<long, false, 0x1234L> TheType;
  BOOST_CHECK(!TheType::IsDynamic);
  BOOST_CHECK_EQUAL(x(), 0x1234L);
  BOOST_CHECK_EQUAL(x.value(), 0x1234L);
}
BOOST_AUTO_TEST_CASE(static_3)
{
  EigenAssertTest::setting_scope settingvariable(true);
  auto test = []() {
    Tomographer::Tools::StaticOrDynamic<long, false, 0x1234L> x(0x5678L); // wrong dynamic argument
    BOOST_MESSAGE("[!!!!!!This point should never be reached!!!!] Value of x = " << x()) ;
  };
  BOOST_CHECK_THROW(test(), Tomographer::Tools::EigenAssertException) ;
}
BOOST_AUTO_TEST_CASE(dyn)
{
  Tomographer::Tools::StaticOrDynamic<long, true> x(0x1234L);
  typedef Tomographer::Tools::StaticOrDynamic<long, true> TheType;
  BOOST_CHECK(TheType::IsDynamic);
  BOOST_CHECK_EQUAL(x(), 0x1234L);
  BOOST_CHECK_EQUAL(x.value(), 0x1234L);
}
BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(StoreIfEnabled)
struct TestBigObject {
  char d[1024];
  TestBigObject(char a, char b, char c) : d{a, b, c, 0} { }
  TestBigObject() : d{0} { }
};
std::ostream & operator<<(std::ostream& str, const TestBigObject x) {
  str << "*" << x.d << "*";
  return str;
}
TOMO_STATIC_ASSERT_EXPR(sizeof(Tomographer::Tools::StoreIfEnabled<TestBigObject, false>) < sizeof(TestBigObject));
TOMO_STATIC_ASSERT_EXPR(sizeof(Tomographer::Tools::StoreIfEnabled<TestBigObject, true>) >= sizeof(TestBigObject));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::StoreIfEnabled<TestBigObject, false>::IsEnabled);
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::StoreIfEnabled<TestBigObject, true>::IsEnabled);
BOOST_AUTO_TEST_CASE(disabled_1)
{
  Tomographer::Tools::StoreIfEnabled<TestBigObject, false> x('c', 'a', 'c', 1, 1.f); // should ignore its arguments
  typedef Tomographer::Tools::StoreIfEnabled<TestBigObject, false> TheType;
  BOOST_CHECK(!TheType::IsEnabled);
}
BOOST_AUTO_TEST_CASE(enabled_1)
{
  Tomographer::Tools::StoreIfEnabled<long, true> x(0x1234L);
  typedef Tomographer::Tools::StoreIfEnabled<long, true> TheType;
  BOOST_CHECK(TheType::IsEnabled);
  BOOST_CHECK_EQUAL(x.value, 0x1234L);
  x.value = 0x05060708L;
  BOOST_CHECK_EQUAL(x.value, 0x05060708L);
}
BOOST_AUTO_TEST_CASE(ostream_disabled)
{
  Tomographer::Tools::StoreIfEnabled<TestBigObject, false> x('c', 'a', 'z', 1, 2.f); // should ignore its arguments
  std::stringstream s;
  s << x;
  BOOST_CHECK_EQUAL(s.str(), std::string("[-]"));
}
BOOST_AUTO_TEST_CASE(ostream_enabled)
{
  Tomographer::Tools::StoreIfEnabled<TestBigObject, true> x('c', 'a', 'z');
  std::stringstream s;
  s << x;
  BOOST_CHECK_EQUAL(s.str(), std::string("*caz*"));
}
BOOST_AUTO_TEST_SUITE_END()


TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::isPowerOfTwo(1));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::isPowerOfTwo(2));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::isPowerOfTwo(4));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::isPowerOfTwo(8));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::isPowerOfTwo(16));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::isPowerOfTwo(32));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::isPowerOfTwo(64));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::isPowerOfTwo(128));
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::isPowerOfTwo(1024));
TOMO_STATIC_ASSERT_EXPR( Tomographer::Tools::isPowerOfTwo(0x0001000000000000UL));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::isPowerOfTwo(0));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::isPowerOfTwo(3));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::isPowerOfTwo(5));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::isPowerOfTwo(6));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::isPowerOfTwo(7));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::isPowerOfTwo(9));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::isPowerOfTwo(30));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::isPowerOfTwo(31));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::isPowerOfTwo(33));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::isPowerOfTwo(34));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::isPowerOfTwo(0x0001000100000000UL));
TOMO_STATIC_ASSERT_EXPR(!Tomographer::Tools::isPowerOfTwo(0x0000ffff00000000UL));


struct ABCZ {
  ABCZ() = delete;
  ABCZ(int, char, long, const char *) { }
  bool test() const { return true; }
};

BOOST_AUTO_TEST_CASE(ComplexRealScalar)
{
  Tomographer::Tools::ComplexRealScalar<std::complex<int> >::type x(100);
  Tomographer::Tools::ComplexRealScalar<std::complex<double> >::type y(1.4);
  Tomographer::Tools::ComplexRealScalar<std::complex<float> >::type z(1.4f);
  Tomographer::Tools::ComplexRealScalar<std::complex<ABCZ> >::type w(1, 'c', 5L, "hello");
  BOOST_CHECK_EQUAL(x, 100);
  BOOST_CHECK_CLOSE(y, 1.4, tol_percent);
  BOOST_CHECK_CLOSE(z, 1.4f, tol_percent);
  BOOST_CHECK(w.test());
}

BOOST_AUTO_TEST_CASE(isPositive)
{
  BOOST_CHECK( Tomographer::Tools::isPositive(1.0) ) ;
  BOOST_CHECK( Tomographer::Tools::isPositive(1.e-12) ) ;
  BOOST_CHECK( Tomographer::Tools::isPositive(0.0) ) ;
  BOOST_CHECK( ! Tomographer::Tools::isPositive(-1.e-12) ) ;
  BOOST_CHECK( ! Tomographer::Tools::isPositive(-1) ) ;
  BOOST_CHECK( Tomographer::Tools::isPositive(0) ) ;
  BOOST_CHECK( Tomographer::Tools::isPositive<unsigned int>(0xffffffffu) ) ;
  BOOST_CHECK(Tomographer::Tools::isPositive(1u)) ;
  BOOST_CHECK(Tomographer::Tools::isPositive(1)) ;
  BOOST_CHECK(Tomographer::Tools::isPositive(1.f)) ;
  BOOST_CHECK(Tomographer::Tools::isPositive(1.0)) ;
  BOOST_CHECK(!Tomographer::Tools::isPositive(-1)) ;
  BOOST_CHECK(!Tomographer::Tools::isPositive(-1.0)) ;
}

BOOST_AUTO_TEST_CASE(additionWillOverflow)
{
  test_addition_will_overflow<int8_t>();
  test_addition_will_overflow<uint8_t>();
  test_addition_will_overflow<int32_t>();
  test_addition_will_overflow<uint32_t>();
  test_addition_will_overflow<int64_t>();
  test_addition_will_overflow<uint64_t>();
}

BOOST_AUTO_TEST_CASE(multiplicationWillOverflow)
{
  { constexpr auto yn = Tomographer::Tools::multiplicationWillOverflow<int32_t>(
        18394, 12345) ;
    BOOST_CHECK(!yn) ; }
  { constexpr auto yn = Tomographer::Tools::multiplicationWillOverflow<int32_t>(
        0, std::numeric_limits<int32_t>::max() ) ;
    BOOST_CHECK(!yn) ; }
  { constexpr auto yn = Tomographer::Tools::multiplicationWillOverflow<int32_t>(
        1, std::numeric_limits<int32_t>::max() ) ;
    BOOST_CHECK(!yn) ; }
  { constexpr auto yn = Tomographer::Tools::multiplicationWillOverflow<int32_t>(
        std::numeric_limits<int32_t>::max(), 0 ) ;
    BOOST_CHECK(!yn) ; }
  { constexpr auto yn = Tomographer::Tools::multiplicationWillOverflow<int32_t>(
        std::numeric_limits<int32_t>::max(), 1 ) ;
    BOOST_CHECK(!yn) ; }
  { constexpr auto yn = Tomographer::Tools::multiplicationWillOverflow<int32_t>(
        std::numeric_limits<int16_t>::max()-1,std::numeric_limits<int16_t>::max() );
    BOOST_CHECK(!yn) ; }

  { constexpr auto yn = Tomographer::Tools::multiplicationWillOverflow<int32_t>(
        (int32_t)std::numeric_limits<int16_t>::max(),(int32_t)std::numeric_limits<int16_t>::max() );
    BOOST_CHECK(!yn) ; }
  { constexpr auto yn = Tomographer::Tools::multiplicationWillOverflow<int32_t>(
        (int32_t)std::numeric_limits<uint16_t>::max(),
        (int32_t)std::numeric_limits<uint16_t>::max() );
    BOOST_CHECK(yn) ; }
  { constexpr auto yn = Tomographer::Tools::multiplicationWillOverflow<int32_t>(
        (int32_t)std::numeric_limits<int16_t>::max()+1323928,(int32_t)std::numeric_limits<int16_t>::max()+1 );
    BOOST_CHECK(yn) ; }
  { constexpr auto yn = Tomographer::Tools::multiplicationWillOverflow<int32_t>(
        (int32_t)std::numeric_limits<int32_t>::max(), 2 );
    BOOST_CHECK(yn) ; }
  { constexpr auto yn = Tomographer::Tools::multiplicationWillOverflow<int32_t>(
        std::numeric_limits<int32_t>::max()-1,std::numeric_limits<int32_t>::max()-1 );
    BOOST_CHECK(yn) ; }

  { constexpr auto yn = Tomographer::Tools::multiplicationWillOverflow<uint32_t>(
        18394, 123456) ;
    BOOST_CHECK(!yn) ; }
  { constexpr auto yn = Tomographer::Tools::multiplicationWillOverflow<uint32_t>(
        0, std::numeric_limits<uint32_t>::max() ) ;
    BOOST_CHECK(!yn) ; }
  { constexpr auto yn = Tomographer::Tools::multiplicationWillOverflow<uint32_t>(
        1, std::numeric_limits<uint32_t>::max() ) ;
    BOOST_CHECK(!yn) ; }
  { constexpr auto yn = Tomographer::Tools::multiplicationWillOverflow<uint32_t>(
        std::numeric_limits<uint32_t>::max(), 0 ) ;
    BOOST_CHECK(!yn) ; }
  { constexpr auto yn = Tomographer::Tools::multiplicationWillOverflow<uint32_t>(
        std::numeric_limits<uint32_t>::max(), 1 ) ;
    BOOST_CHECK(!yn) ; }
  { constexpr auto yn = Tomographer::Tools::multiplicationWillOverflow<uint32_t>(
        std::numeric_limits<int16_t>::max()-1,std::numeric_limits<int16_t>::max() );
    BOOST_CHECK(!yn) ; }

  { constexpr auto yn = Tomographer::Tools::multiplicationWillOverflow<uint32_t>(
        (uint32_t)(std::numeric_limits<uint16_t>::max()+1),
        (uint32_t)(std::numeric_limits<uint16_t>::max()+1) );
    BOOST_CHECK(yn) ; }
  { constexpr auto yn = Tomographer::Tools::multiplicationWillOverflow<uint32_t>(
        (uint32_t)(std::numeric_limits<uint16_t>::max()+1),
        (uint32_t)std::numeric_limits<uint16_t>::max() );
    BOOST_CHECK(!yn) ; }
  { constexpr auto yn = Tomographer::Tools::multiplicationWillOverflow<uint32_t>(
        (uint32_t)std::numeric_limits<uint32_t>::max(), 2 );
    BOOST_CHECK(yn) ; }
  { constexpr auto yn = Tomographer::Tools::multiplicationWillOverflow<uint32_t>(
        std::numeric_limits<uint32_t>::max()-1,std::numeric_limits<uint32_t>::max()-1 );
    BOOST_CHECK(yn) ; }

  
  { constexpr auto yn = Tomographer::Tools::multiplicationWillOverflow<int32_t>(
        std::numeric_limits<int32_t>::min(),-1 );
    BOOST_CHECK(yn) ; }
  { constexpr auto yn = Tomographer::Tools::multiplicationWillOverflow<int32_t>(
        std::numeric_limits<int32_t>::min(),0 );
    BOOST_CHECK(!yn) ; }
  { constexpr auto yn = Tomographer::Tools::multiplicationWillOverflow<int32_t>(
        -std::numeric_limits<int32_t>::max(), 2 );
    BOOST_CHECK(yn) ; }
  { constexpr auto yn = Tomographer::Tools::multiplicationWillOverflow<int32_t>(
        std::numeric_limits<int32_t>::min()/2, -3 );
    BOOST_CHECK(yn) ; }
  { constexpr auto yn = Tomographer::Tools::multiplicationWillOverflow<int32_t>(
        -2, std::numeric_limits<int32_t>::max() );
    BOOST_CHECK(yn) ; }
  { constexpr auto yn = Tomographer::Tools::multiplicationWillOverflow<int32_t>(
        -1, std::numeric_limits<int32_t>::max() );
    BOOST_CHECK(!yn) ; }
}


TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::extractFuncName("void class::subclass::subclass(int)") == "class::subclass::subclass");
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::extractFuncName("conststr ns::subclass::method()") == "ns::subclass::method");
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::extractFuncName("int ns::subclass::method(const int&, void, conststr *)") == "ns::subclass::method");
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::extractFuncName("int ns::subclass::operator==(int)") == "ns::subclass::operator==");
TOMO_STATIC_ASSERT_EXPR(Tomographer::Tools::extractFuncName("int operator==(const ns::subclass&, char)") == "operator==(const ns::subclass&, char)");




// exception class utils

TOMOGRAPHER_DEFINE_MSG_EXCEPTION(test_except_1, "Exception 1: ") ;
TOMOGRAPHER_DEFINE_MSG_EXCEPTION_BASE(test_except_2, "Exception 2: ", std::logic_error) ;
TOMOGRAPHER_DEFINE_MSG_EXCEPTION_BASE(test_except_3, "Exception 3: ", test_except_1) ;

struct myfix_exctest {
  myfix_exctest() { }
  template<typename Exc, typename GoodBaseClass>
  void testexc(std::string good_prefix) {
    Exc e("abc");
    BOOST_CHECK_EQUAL(e.what(), good_prefix + "abc");
    GoodBaseClass * b = dynamic_cast<GoodBaseClass*>(&e);
    BOOST_CHECK(b != NULL) ; // Exc must inherit GoodBaseClass
    BOOST_CHECK_EQUAL(b->what(), good_prefix + "abc");
  }
};

BOOST_AUTO_TEST_SUITE(testExceptionUtils) ;

BOOST_FIXTURE_TEST_CASE(class_well_formed, myfix_exctest)
{
  testexc<test_except_1, std::exception>("Exception 1: ");
  testexc<test_except_2, std::logic_error>("Exception 2: ");
  testexc<test_except_3, test_except_1>("Exception 1: Exception 3: ");

  // only those exceptions which are defined using "TOMOGRAPHER_DEFINE_MSG_EXCEPTION"
  // (i.e. w/o delegation to base class) have the .msg() method.
  test_except_1 e("xyz");
  BOOST_CHECK_EQUAL(e.msg(), std::string("Exception 1: xyz")) ;
}

BOOST_AUTO_TEST_CASE(ensure_utils)
{
  BOOST_CHECK_NO_THROW( Tomographer::Tools::tomographerEnsure<std::logic_error>(true, "ERROR!")  );
  BOOST_CHECK_THROW( Tomographer::Tools::tomographerEnsure<std::invalid_argument>(false, "ERROR!") ,
                     std::invalid_argument );
  BOOST_CHECK_THROW( Tomographer::Tools::tomographerEnsure<test_except_2>(false, "ERROR!")  ,
                     test_except_2 );
  BOOST_CHECK_THROW( Tomographer::Tools::tomographerEnsure<test_except_2>(false, "ERROR!")  ,
                     std::logic_error );

  try {
    Tomographer::Tools::tomographerEnsure<test_except_3>(1+1 == 3, "Error, 1+1!=3");
    BOOST_CHECK( false ); // above must have thrown an exception
  } catch (const test_except_3 & e) {
    BOOST_CHECK_EQUAL(e.what(), "Exception 1: Exception 3: Error, 1+1!=3") ;
    BOOST_CHECK_EQUAL(e.msg(), "Exception 1: Exception 3: Error, 1+1!=3") ;
  }

  try {
    Tomographer::Tools::tomographerEnsure(1+1 == 3, "Error, 1+1!=3");
    BOOST_CHECK( false ); // above must have thrown an exception
  } catch (const std::exception & e) {
    std::string s = e.what();
    BOOST_MESSAGE("Exception string = " << s);
    BOOST_CHECK(s.find("Error, 1+1!=3") != std::string::npos) ;
  }
}


BOOST_AUTO_TEST_SUITE_END() ;


BOOST_AUTO_TEST_SUITE_END()





