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


// this file should be included in tests before other Eigen or tomographer headers. It
// also automatically includes all the needed boost test framework tools.

#ifndef TEST_TOMOGRAPHER_H
#define TEST_TOMOGRAPHER_H

#define EIGEN_INITIALIZE_MATRICES_BY_NAN

#ifdef NDEBUG
#error "Cannot compile tests with -DNDEBUG: this would disable assertion tests, defeating the purpose of the tests."
#endif

#include <cmath>
#include <cstdlib>

#include <complex>
#include <iostream>
#include <iomanip>
#include <type_traits>

// define the exception class, but don't override eigen's eigen_assert() macro itself
#include <tomographer/tools/eigen_assert_exception.h>



// It seems that Clang++ does not provide std::abs(std::complex<..> z). (???) So we'll
// need to implement it ourselves.
template<typename T, typename Enable = void>
struct MyAbs {
  static inline T calcAbs(T val) {
    using namespace std;
    return abs(val);
  };
};
template<typename RealType>
struct MyAbs<std::complex<RealType> > {
  typedef std::complex<RealType> T;
  static inline RealType calcAbs(T z) {
    using namespace std;
    return hypot(std::real(z), std::imag(z));
  };
};
// abs() for unsigned types. no-op by definition as unsigned types are always positive.
template<typename T>
struct MyAbs<T, typename std::enable_if<std::is_unsigned<T>::value >::type> {
  static inline T calcAbs(T val) {
    return val;
  };
};




/** 
 * \brief Tool to dynamically request eigen_assert() to either assert() or to throw an
 *        exception.
 *
 * We want true failures to cause an assert() failure, because they can be traced much
 * more easily (core dump). However, sometimes we want to test that an eigen_assert() is
 * truly triggered. In that case, we dynamically request eigen_assert() to trigger an
 * exception.
 *
 * An exception can be triggered in a scope by using the idiom
 * \code
 *  {
 *    // new code block
 *    EigenAssertTest::setting_scope scopevar(true); // true -> throw exception, false -> assert()
 *    ... // eigen_assert() here throws an exception
 *  }
 *  // setting is popped to previous value
 * \endcode
 */


namespace EigenAssertTest {

struct setting_scope;
extern setting_scope * setting_scope_ptr;


struct setting_scope
{
  setting_scope(bool throws)
    : throws_exception(throws), parent_scope(setting_scope_ptr)
  {
    /*std::fprintf(stderr, "(): setting_scope_ptr=%p, throws_exception=%d\n", (void*)::EigenAssertTest::setting_scope_ptr, (::EigenAssertTest::setting_scope_ptr ? (int)::EigenAssertTest::setting_scope_ptr->throws_exception : 999999));*/  \
    setting_scope_ptr = this;
  }

  ~setting_scope()
  {
    /*std::fprintf(stderr, "~(): setting_scope_ptr=%p, throws_exception=%d\n", (void*)::EigenAssertTest::setting_scope_ptr, (::EigenAssertTest::setting_scope_ptr ? (int)::EigenAssertTest::setting_scope_ptr->throws_exception : 999999));*/ \
    setting_scope_ptr = parent_scope;
  }

  const bool throws_exception;
private:
  setting_scope * parent_scope;
};


} // namespace EigenAssertTest


#define eigen_assert(x)                                                 \
  do {                                                                  \
    /*std::fprintf(stderr, "setting_scope_ptr=%p, throws_exception=%d\n", (void*)::EigenAssertTest::setting_scope_ptr, (int)(::EigenAssertTest::setting_scope_ptr ? ::EigenAssertTest::setting_scope_ptr->throws_exception : 999999));*/ \
    if (::EigenAssertTest::setting_scope_ptr && ::EigenAssertTest::setting_scope_ptr->throws_exception) { \
      /*std::fprintf(stderr, "an eigen_assert() failure will cause an exception!\n");*/ \
      if (!(x)) {                                                       \
        tomographer_eigen_assert_failure_cleanup();                             \
        throw (::Tomographer::Tools::EigenAssertException(#x, __FILE__, __LINE__)); \
      }                                                                 \
    } else {                                                            \
      if (!(x)) {                                                       \
        tomographer_eigen_assert_failure_cleanup();                             \
        assert(false && "eigen_assert() failure: " #x);                 \
      }                                                                 \
    }                                                                   \
  } while (false)


#include <Eigen/Core>





// internal -- useful when we compile with -Wconversion for picky debugging, but still
// want to allow constants passed to FixCommaInitializer to not generate tons of warnings
template<typename Scalar, typename Arg>
struct conv_to_scalar {
  static inline Scalar conv(const Arg& x) {
    return (Scalar)(x);
  }
};
template<typename RealScalar, typename Arg>
struct conv_to_scalar<std::complex<RealScalar>, Arg> {
  static inline std::complex<RealScalar> conv(const Arg& x) {
    return std::complex<RealScalar>((RealScalar)x, 0);
  }
};
template<typename RealScalar, typename RealArg>
struct conv_to_scalar<std::complex<RealScalar>, std::complex<RealArg> > {
  static inline std::complex<RealScalar> conv(const std::complex<RealArg>& x) {
    return std::complex<RealScalar>((RealScalar)x.real(), (RealScalar)x.imag());
  }
};

// more eigen magic ... override the CommaInitializer class to fix a bug where the
// destructor could generate an assertion failure while handling a prior exception, which
// would immediately call terminate() in any case (it appears).  That's the reason for the
// tomographer_eigen_assert_failure_cleanup() function above.
namespace Tomographer { namespace Tools { namespace tomo_internal {
template<typename XprType>
struct FixCommaInitializer
{
  typedef typename XprType::Scalar Scalar;

  
  template<typename T>
  FixCommaInitializer(XprType& xpr, const T& s)
    : m_xpr(xpr), m_row(0), m_col(1), m_currentBlockRows(1)
  {
    m_xpr.coeffRef(0,0) = conv_to_scalar<Scalar,T>::conv(s);
  }

  FixCommaInitializer(FixCommaInitializer&& o)
  : m_xpr(o.m_xpr), m_row(o.m_row), m_col(o.m_col), m_currentBlockRows(o.m_currentBlockRows) {
    // Mark original object as finished.
    o.m_row = m_xpr.rows();
    o.m_col = m_xpr.cols();
    o.m_currentBlockRows = 0;
  }

  /* inserts a scalar value in the target matrix */
  template<typename T>
  FixCommaInitializer& operator,(const T& s)
  {
    if (m_col==m_xpr.cols())
    {
      m_row+=m_currentBlockRows;
      m_col = 0;
      m_currentBlockRows = 1;
      eigen_assert(m_row<m_xpr.rows()
        && "Too many rows passed to comma initializer (operator<<)");
    }
    eigen_assert(m_col<m_xpr.cols()
      && "Too many coefficients passed to comma initializer (operator<<)");
    eigen_assert(m_currentBlockRows==1);
    m_xpr.coeffRef(m_row, m_col++) = conv_to_scalar<Scalar,T>::conv(s);
    return *this;
  }

  inline ~FixCommaInitializer() noexcept(false)
  {
    finished();
  }

  inline XprType& finished() noexcept(false)
  {
    eigen_assert(((m_row+m_currentBlockRows) == m_xpr.rows() || m_xpr.cols() == 0)
                 && m_col == m_xpr.cols()
                 && "Too few coefficients passed to comma initializer (operator<<)");
    return m_xpr;
  }

  XprType& m_xpr;           // target expression
  Eigen::Index m_row;              // current row id
  Eigen::Index m_col;              // current col id
  Eigen::Index m_currentBlockRows; // current block height

  void tomographer_eigen_assert_failure_cleanup() {
    // mark as finished, so that destructor doesn't further raise another exception
    m_row = m_xpr.rows(); m_col = m_xpr.cols(); m_currentBlockRows = 1;
  }
};
} } } // namespaces Tomographer::Tools::tomo_internal
template<typename Derived, typename Scalar>
inline Tomographer::Tools::tomo_internal::FixCommaInitializer<Derived>
operator<<(Eigen::DenseBase<Derived> & m, const Scalar& s)
{
  return Tomographer::Tools::tomo_internal::FixCommaInitializer<Derived>(m.derived(), s);
}


template<bool IsComplex, typename RealScalar>
struct maybe_complex { typedef RealScalar type; };
template<typename RealScalar>
struct maybe_complex<true, RealScalar> { typedef std::complex<RealScalar> type; };


template<typename ... Scalars>
struct PromotedScalars {
};
// workaround for GCC 4.6: this needs to be a specialization -- see http://stackoverflow.com/a/2118537/1694896
template<typename Scalar0, typename ... Scalars>
struct PromotedScalars<Scalar0, Scalars...>
{
  typedef decltype( typename PromotedScalars<Scalar0>::RealScalar(1)
                    + typename PromotedScalars<Scalars...>::RealScalar(1) )  RealScalar;
  typedef std::complex<RealScalar>  ComplexScalar;
  static constexpr bool IsComplex = PromotedScalars<Scalar0>::IsComplex || PromotedScalars<Scalars...>::IsComplex ;

  typedef typename maybe_complex<IsComplex,RealScalar>::type  Scalar;
};
template<typename Scalar0>
struct PromotedScalars<Scalar0>
{
  typedef Scalar0 RealScalar;
  typedef std::complex<RealScalar> ComplexScalar;
  static constexpr bool IsComplex = false;
};
template<typename RealScalar0>
struct PromotedScalars<std::complex<RealScalar0> >
{
  typedef RealScalar0 RealScalar;
  typedef std::complex<RealScalar0> ComplexScalar;
  static constexpr bool IsComplex = true;
};


#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/test/output_test_stream.hpp>

#if BOOST_VERSION > 105300
#  define BOOST_MESSAGE(msg) BOOST_TEST_MESSAGE(msg)
#  define BOOST_CHECKPOINT(msg) BOOST_TEST_CHECKPOINT(msg)
#endif

template<typename Derived1, typename Derived2, typename TolType = double>
boost::test_tools::predicate_result
check_eigen_dense_equal(const Eigen::DenseBase<Derived1> & a, const Eigen::DenseBase<Derived2> & b,
                        const TolType tol) // tol is absolute tolerance
{
  BOOST_MESSAGE("Comparing two Eigen dense objects.");

  if (a.rows() != b.rows()) {
    boost::test_tools::predicate_result res(false);
    res.message() << "a.rows() [="<<a.rows()<<"]  !=  b.rows() [="<<b.rows()<<"]";
    return res;
  }

  if (a.cols() != b.cols()) {
    boost::test_tools::predicate_result res(false);
    res.message() << "a.cols() [="<<a.cols()<<"]  !=  b.cols() [="<<b.cols()<<"]";
    return res;
  }

  Eigen::Array<typename Derived1::Scalar, Eigen::Dynamic, Eigen::Dynamic> a_eval(a.rows(), a.cols());
  Eigen::Array<typename Derived2::Scalar, Eigen::Dynamic, Eigen::Dynamic> b_eval(b.rows(), b.cols());
  a_eval = a;
  b_eval = b;

  // make sure it's at least 'double'
  typedef PromotedScalars<typename Derived1::Scalar, typename Derived2::Scalar, TolType, double>
    PromotedScalars;
  typedef typename PromotedScalars::Scalar PromotedScalar;

  Eigen::Array<PromotedScalar, Eigen::Dynamic, Eigen::Dynamic> diff
    =  a_eval.template cast<PromotedScalar>() - b_eval.template cast<PromotedScalar>();

  typedef typename PromotedScalars::RealScalar PromotedRealScalar;

  if (diff.isMuchSmallerThan(PromotedRealScalar(1), PromotedRealScalar(tol))) {
    return true;
  }
  boost::test_tools::predicate_result res(false);
  res.message() << "matrices are different: a=\n" << a_eval << "\n\t!=  b=\n" << b << "\n"
                << "\t[diff = \n" << diff << ",\n"
                << "\tnorm of difference = " << diff.matrix().norm() << "]\n";
  return res;

}



#define MY_BOOST_CHECK_EIGEN_EQUAL(a, b, tol)       \
  BOOST_CHECK( check_eigen_dense_equal((a), (b), (tol)) )

#define MY_BOOST_CHECK_FLOATS_EQUAL(a, b, tol_abs)	\
  do { if ((b) < (tol_abs) && -(b) < (tol_abs)) {       \
      BOOST_CHECK_SMALL((a), tol_abs) ;			\
    } else {						\
      BOOST_CHECK_CLOSE((a), (b), (tol_abs)*100) ;      \
    } } while (0)




template<typename T1, typename A1, typename T2, typename A2, typename ToleranceType = double>
boost::test_tools::predicate_result
check_std_vector_equal(const std::vector<T1, A1> & a, const std::vector<T2, A2> & b,
                       const ToleranceType tol = 1e-8) // tol is absolute tolerance
{
  BOOST_MESSAGE("Comparing two std::vector<T>'s");

  if (a.size() != b.size()) {
    boost::test_tools::predicate_result res(false);
    res.message() << "a.size() [="<<a.size()<<"]  !=  b.size() [="<<b.size()<<"]";
    return res;
  }

  typedef decltype(T1(1) + T2(1))  PromotedScalar;

  for (std::size_t j = 0; j < a.size(); ++j) {
    auto thedelta = MyAbs<PromotedScalar>::calcAbs(a[j] - b[j]);
    if (thedelta > tol) {
      boost::test_tools::predicate_result res(false);
      res.message() << "vectors are different: a["<<j<<"]=" << a[j] << "  !=  b["<<j<<"]=" << b[j] << "\n"
                    << "\t[diff = " << thedelta << "]\n";
      return res;
    }
  }
  return true;
}
template<typename T1, typename A1, std::size_t N2, typename T2, typename ToleranceType = double>
boost::test_tools::predicate_result
check_std_vector_equal(const std::vector<T1, A1> & a, const T2 (&b)[N2],
                       const ToleranceType tol = 1e-8) // tol is absolute tolerance
{
  return check_std_vector_equal(a, std::vector<T2>(b, b+N2), tol);
}


#define MY_BOOST_CHECK_STD_VECTOR_EQUAL(a, b, tol)       \
  BOOST_CHECK( check_std_vector_equal((a), (b), (tol)) )






// tolerance, in *PERCENT*
static const double tol_percent = 1e-12;
static const double tol = tol_percent * 0.01;


static const float tol_percent_f = 1e-4f;
static const float tol_f = tol_percent_f * 0.01f;






static constexpr int DumpHistogramTest_DefaultPrecision = 3;

// utility to check histograms.
//
// This dumps the histogram data (with or without error bars depending on the histogram
// type) in a particular format which will not change, and which may be used to compare to
// expected results by using simple text comparision.
//
template<typename HistogramType>
struct DumpHistogramTest
{
  static inline void dump(std::ostream & str, const HistogramType & histogram,
			  const int precision = DumpHistogramTest_DefaultPrecision)
  {
    str << "HISTOGRAM";
    if (HistogramType::HasErrorBars) {
      str << " (WITH ERROR BARS)\n";
    } else {
      str << "\n";
    }
    str << std::setprecision(precision) << std::scientific << std::right << std::setfill('0');
    str << "\n";
    str << "PARAMS = ["	<< histogram.params.min << ", " << histogram.params.max
	<< "] (" << histogram.numBins() << " bins)\n"
	<< "\n";
    for (std::size_t k = 0; k < (std::size_t)histogram.numBins(); ++k) {
      _count_str(str, histogram, k);
    }
    str << "\n";
    str << "OFF CHART = " << histogram.off_chart << "\n";
  }

private:
  template<bool dummy = true, typename std::enable_if<dummy && !HistogramType::HasErrorBars, bool>::type dummy2 = true>
  static inline void _count_str(std::ostream & str, const HistogramType & histogram, std::size_t k)
  {
    str << histogram.count(k) << "\n";
  }
  template<bool dummy = true, typename std::enable_if<dummy && HistogramType::HasErrorBars, bool>::type dummy2 = true>
  static inline void _count_str(std::ostream & str, const HistogramType & histogram, std::size_t k)
  {
    str << histogram.count(k) << " +- " << histogram.errorBar(k) << "\n";
  }
};

// helper function for automatic template type deduction
template<typename HistogramType>
void dump_histogram_test(std::ostream & str, const HistogramType & histogram,
			 const int precision = DumpHistogramTest_DefaultPrecision)
{
  DumpHistogramTest<HistogramType>::dump(str, histogram, precision);
}






// problems with C++11 ABI: see http://stackoverflow.com/a/33755176/1694896 
#ifdef TOMOGRAPHER_TESTS_LINK_AGAINST_CXX11_ABI_WITH_OLD_GCC
namespace boost { namespace unit_test { namespace ut_detail {
    std::string normalize_test_case_name(const_string name) {
        return ( name[0] == '&' ? std::string(name.begin()+1, name.size()-1) : std::string(name.begin(), name.size() ));
    }
}}}
#endif




#endif
