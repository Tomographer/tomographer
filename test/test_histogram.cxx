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

#include <iostream>
#include <iomanip>
#include <stdexcept>

#include "test_tomographer.h"

#include <tomographer/histogram.h>

#include <Eigen/Core>
#include <Eigen/Eigenvalues>





template<typename T>
inline Eigen::Matrix<T,4,1> inline_vector_4(T a1, T a2, T a3, T a4)
{
  Eigen::Matrix<T,4,1> v;
  (v << a1, a2, a3, a4).finished();
  return v;
}



BOOST_AUTO_TEST_SUITE(test_histogram);
// =============================================================================


BOOST_AUTO_TEST_SUITE(uniform_bins_histogram);

BOOST_AUTO_TEST_CASE(basic)
{
  Tomographer::Histogram<float, long> hist(0.0f, 1.0f, 10);
  hist.record(0.42323f);
  hist.record(0.933f);
  hist.record(0.5f);
  hist.record(0.55555232f);
  hist.record(0.4999f);
  hist.record(0.52f);
  hist.record(1.2f);

  BOOST_CHECK_EQUAL(hist.numBins(), 10u);
  BOOST_CHECK_SMALL(hist.params.min, tol_f);
  BOOST_CHECK_CLOSE(hist.params.max, 1.0f, tol_percent_f);

  BOOST_CHECK_EQUAL(hist.count(0), 0);
  BOOST_CHECK_EQUAL(hist.count(1), 0);
  BOOST_CHECK_EQUAL(hist.count(2), 0);
  BOOST_CHECK_EQUAL(hist.count(3), 0);
  BOOST_CHECK_EQUAL(hist.count(4), 2);
  BOOST_CHECK_EQUAL(hist.count(5), 3);
  BOOST_CHECK_EQUAL(hist.count(6), 0);
  BOOST_CHECK_EQUAL(hist.count(7), 0);
  BOOST_CHECK_EQUAL(hist.count(8), 0);
  BOOST_CHECK_EQUAL(hist.count(9), 1);
  BOOST_CHECK_EQUAL(hist.off_chart, 1);

  for (int k = 0; k < 10; ++k) {
    BOOST_CHECK_EQUAL(hist.count(k), hist.bins(k));
  }

  typedef Tomographer::Histogram<float, long> MyHistogramType;
  BOOST_CHECK( ! MyHistogramType::HasErrorBars );
}


BOOST_AUTO_TEST_CASE(boundaries)
{
  Tomographer::Histogram<float, int> hist(0.0f, 1.f, 10);

  BOOST_CHECK(hist.isWithinBounds(0.43f));
  BOOST_CHECK(!hist.isWithinBounds(-0.01f));
  BOOST_CHECK(!hist.isWithinBounds(1.2f));
  BOOST_CHECK(!hist.isWithinBounds(std::numeric_limits<float>::quiet_NaN()));
  BOOST_CHECK(!hist.isWithinBounds(std::numeric_limits<float>::infinity()));
  BOOST_CHECK(!hist.isWithinBounds(-std::numeric_limits<float>::infinity()));

  BOOST_CHECK_EQUAL(hist.binIndex(0.13f), 1u);
  BOOST_CHECK_EQUAL(hist.binIndex(0.99f), 9u);
  BOOST_CHECK_EQUAL(hist.binIndex(0.34f), 3u);

  BOOST_CHECK_SMALL(hist.binLowerValue(0), tol_f);
  BOOST_CHECK_CLOSE(hist.binLowerValue(1), 0.1f, tol_percent_f);
  BOOST_CHECK_CLOSE(hist.binLowerValue(6), 0.6f, tol_percent_f);

  BOOST_CHECK_CLOSE(hist.binUpperValue(0), 0.1f, tol_percent_f);
  BOOST_CHECK_CLOSE(hist.binUpperValue(5), 0.6f, tol_percent_f);
  BOOST_CHECK_CLOSE(hist.binUpperValue(9), 1.0f, tol_percent_f);

  BOOST_CHECK_CLOSE(hist.binCenterValue(1), 0.15f, tol_percent_f);
  
  BOOST_CHECK_CLOSE(hist.binResolution(), 0.1f, tol_percent_f);

  {
    EigenAssertTest::setting_scope settingvariable(true); // eigen_assert() should throw an exception.
    BOOST_CHECK_THROW(
        hist.binLowerValue(-1),
        ::Tomographer::Tools::EigenAssertException
        );
    BOOST_CHECK_THROW(
        hist.binLowerValue(11),
        ::Tomographer::Tools::EigenAssertException
        );
    BOOST_CHECK_THROW(
        hist.binCenterValue(-1),
        ::Tomographer::Tools::EigenAssertException
        );
    BOOST_CHECK_THROW(
        hist.binUpperValue(-1),
        ::Tomographer::Tools::EigenAssertException
        );
  }
}


BOOST_AUTO_TEST_CASE(values)
{
  Tomographer::HistogramParams<double> params(0.0f, 1.f, 10);

  auto vcenter = params.valuesCenter();
  BOOST_CHECK_EQUAL(vcenter.size(), 10);
  MY_BOOST_CHECK_FLOATS_EQUAL(vcenter(0), 0.05, tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(vcenter(4), 0.45, tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(vcenter(9), 0.95, tol);

  auto vlower = params.valuesLower();
  BOOST_CHECK_EQUAL(vlower.size(), 10);
  MY_BOOST_CHECK_FLOATS_EQUAL(vlower(0), 0.0, tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(vlower(9), 0.9, tol);
  
  auto vupper = params.valuesUpper();
  BOOST_CHECK_EQUAL(vupper.size(), 10);
  MY_BOOST_CHECK_FLOATS_EQUAL(vupper(0), 0.1, tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(vupper(9), 1.0, tol);
}


BOOST_AUTO_TEST_CASE(add_load_reset)
{
  Tomographer::Histogram<float, long> hist(0.0f, 1.0f, 10);
  hist.record(0.42323f);
  hist.record(0.933f);
  hist.record(0.5f);
  hist.record(0.55555232f);
  hist.record(0.4999f);
  hist.record(0.52f);
  hist.record(1.2f);

  Tomographer::Histogram<double, unsigned int> hist2(0.0, 1.0, 10);
  hist2.add(hist);

  int k;

  for (k = 0; k < 10; ++k) {
    BOOST_CHECK_EQUAL(hist.count(k), (long)hist2.count(k));
  }
  BOOST_CHECK_EQUAL(hist.off_chart, (long)hist2.off_chart);

  hist2.load(Eigen::Matrix<int,10,1>::Constant(80));
  for (k = 0; k < 10; ++k) {
    BOOST_CHECK_EQUAL(hist2.count(k), 80u);
  }

  Eigen::Matrix<unsigned int,10,1> m;
  (m << 0,    1,  4, 30, 95,
        150, 77, 18,  5,  1).finished();

  hist2.load(m, 42);

  MY_BOOST_CHECK_EIGEN_EQUAL(hist2.bins, m, tol);
  BOOST_CHECK_EQUAL(hist2.off_chart, 42u);

  Eigen::Matrix<unsigned int,10,1> m2;
  (m2 <<  0,  0,  0, 10, 10,
         10, 10, 10,  0,  0).finished();
  hist2.add(m2.array());

  MY_BOOST_CHECK_EIGEN_EQUAL(hist2.bins, m+m2, tol);
  BOOST_CHECK_EQUAL(hist2.off_chart, 42u);

  hist2.reset();
  auto zeros = Eigen::Array<unsigned int,10,1>::Zero();
  MY_BOOST_CHECK_EIGEN_EQUAL(hist2.bins, zeros, tol);
  BOOST_CHECK_EQUAL(hist2.off_chart, 0u);
}


BOOST_AUTO_TEST_CASE(floatcounttype)
{
  Tomographer::Histogram<float, double> hist(0.0f, 1.0f, 10);

  hist.record(0.21f);
  hist.record(0.55f, 2.01);
  hist.record(0.743f);
  hist.record(1.334f, 120.399);
  hist.record(0.781f, 380.4);
  hist.record(0.58f);
  hist.record(0.64f, 1.2);

  Eigen::Array<double,10,1> a;
  //   0.0  0.1  0.2  0.3  0.4   0.5  0.6    0.7  0.8  0.9
  a <<   0,   0, 1.0,   0,   0, 3.01, 1.2, 381.4,   0,   0;

  BOOST_MESSAGE(hist.prettyPrint(100));

  MY_BOOST_CHECK_EIGEN_EQUAL(hist.bins, a, tol);
  BOOST_CHECK_CLOSE(hist.off_chart, 120.399, tol_percent);
}

BOOST_AUTO_TEST_CASE(normalization)
{
  { Tomographer::Histogram<float, int> hist(0.0f, 4.0f, 4);
    hist.load( inline_vector_4<int>(0, 3, 19, 24), 10 );
    MY_BOOST_CHECK_FLOATS_EQUAL(hist.normalization(), 0.f+3+19+24+10, tol_f); }

  { Tomographer::Histogram<float, int> hist(0.0f, 1.0f, 4);
    hist.load( inline_vector_4<int>(0, 3, 19, 24), 10 );
    BOOST_CHECK_EQUAL(hist.normalization<int>(), int((0+3+19+24)*0.25 + 10)); }

  { Tomographer::Histogram<float, int> hist(0.0f, 1.0f, 4);
    hist.load( inline_vector_4<int>(0, 3, 19, 24), 10 );
    MY_BOOST_CHECK_FLOATS_EQUAL(hist.normalization<double>(), (0.0+3+19+24)*0.25 + 10, tol); }
}
BOOST_AUTO_TEST_CASE(normalized)
{
  { Tomographer::Histogram<float, int> hist(0.0f, 4.0f, 4);
    hist.load( inline_vector_4<int>(0, 3, 19, 24), 10 );
    auto hn = hist.normalized();
    auto n = hist.normalization();
    MY_BOOST_CHECK_EIGEN_EQUAL(hn.bins, inline_vector_4<float>(0, 3, 19, 24)/n, tol_f);
    MY_BOOST_CHECK_FLOATS_EQUAL(hn.off_chart, 10.f/n, tol_f);
    MY_BOOST_CHECK_FLOATS_EQUAL(hn.normalization(), 1.f, tol_f);
  }
  { Tomographer::Histogram<float, int> hist(0.0f, 1.0f, 4);
    hist.load( inline_vector_4<int>(0, 3, 19, 24), 10 );
    auto hn = hist.normalized<double>();
    auto n = hist.normalization<double>();
    MY_BOOST_CHECK_EIGEN_EQUAL(hn.bins, inline_vector_4<double>(0, 3, 19, 24)/n, tol);
    MY_BOOST_CHECK_FLOATS_EQUAL(hn.off_chart, 10.0/n, tol);
    MY_BOOST_CHECK_FLOATS_EQUAL(hn.normalization(), 1.0, tol);
  }
}
BOOST_AUTO_TEST_CASE(normalized_counts)
{
  { Tomographer::Histogram<float, int> hist(0.0f, 4.0f, 4);
    hist.load( inline_vector_4<int>(0, 3, 19, 24), 10 );
    BOOST_CHECK_EQUAL(hist.totalCounts(), hist.bins.sum() + hist.off_chart);
    auto hn = hist.normalizedCounts();
    auto n = hist.totalCounts();
    MY_BOOST_CHECK_EIGEN_EQUAL(hn.bins, inline_vector_4<float>(0, 3, 19, 24)/n, tol_f);
    MY_BOOST_CHECK_FLOATS_EQUAL(hn.off_chart, 10.f/n, tol_f);
    MY_BOOST_CHECK_FLOATS_EQUAL(hn.totalCounts(), 1.f, tol_f);
  }
  { Tomographer::Histogram<float, int> hist(0.0f, 1.0f, 4);
    hist.load( inline_vector_4<int>(0, 3, 19, 24), 10 );
    BOOST_CHECK_EQUAL(hist.totalCounts(), hist.bins.sum() + hist.off_chart);
    auto hn = hist.normalizedCounts<double>();
    double n = hist.totalCounts();
    MY_BOOST_CHECK_EIGEN_EQUAL(hn.bins, inline_vector_4<double>(0, 3, 19, 24)/n, tol);
    MY_BOOST_CHECK_FLOATS_EQUAL(hn.off_chart, 10.0/n, tol);
    MY_BOOST_CHECK_FLOATS_EQUAL(hn.totalCounts(), 1.0, tol);
  }
}

BOOST_AUTO_TEST_CASE(copy)
{
  Tomographer::Histogram<float, int> hist(0.0f, 1.0f, 5);
  hist.bins << 0, 3, 19, 24, 8;
  
  auto histtgt = Tomographer::Histogram<float, double>::copy(hist);

  MY_BOOST_CHECK_EIGEN_EQUAL( hist.bins.cast<double>(), histtgt.bins, tol );
}

BOOST_AUTO_TEST_CASE(move)
{
  Tomographer::Histogram<float, int> hist(0.0f, 1.0f, 5);
  hist.bins << 0, 3, 19, 24, 8;

  Tomographer::Histogram<float, int> hist2(std::move(hist));

  BOOST_CHECK_EQUAL( hist2.bins.size(), 5 );
  MY_BOOST_CHECK_FLOATS_EQUAL(hist2.bins.cast<double>()(0), 0., tol );
  MY_BOOST_CHECK_FLOATS_EQUAL(hist2.bins.cast<double>()(1), 3., tol );
  MY_BOOST_CHECK_FLOATS_EQUAL(hist2.bins.cast<double>()(2), 19., tol );
  MY_BOOST_CHECK_FLOATS_EQUAL(hist2.bins.cast<double>()(3), 24., tol );
  MY_BOOST_CHECK_FLOATS_EQUAL(hist2.bins.cast<double>()(4), 8., tol );
}

BOOST_AUTO_TEST_SUITE_END(); // uniform_bins_histogram

// -----------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(uniform_bins_histogram_with_error_bars)

BOOST_AUTO_TEST_CASE(basic)
{
  Tomographer::HistogramWithErrorBars<double, float> hist(-1.0, 1.0, 4);

  hist.record(0.01, 1.2f);
  hist.record(-0.56, 0.1f);

  (hist.delta << 0.1f, 0.f, 0.4f, 0.f).finished();

  Eigen::Array<float,4,1> a;
  a << 0.1f, 0.f, 1.2f, 0.f;
  MY_BOOST_CHECK_EIGEN_EQUAL(hist.bins, a, tol_f);

  int k;

  for (k = 0; k < 4; ++k) {
    BOOST_CHECK_CLOSE(hist.errorBar(k), hist.delta(k), tol_percent);
  }

  hist.reset();

  auto zeros4 = Eigen::Array<float,4,1>::Zero();
  MY_BOOST_CHECK_EIGEN_EQUAL(hist.bins, zeros4, tol_f);
  MY_BOOST_CHECK_EIGEN_EQUAL(hist.delta, zeros4, tol_f);
  BOOST_CHECK_CLOSE(hist.off_chart, 0.f, tol_percent_f);

  typedef Tomographer::HistogramWithErrorBars<double,float> MyHistType;
  BOOST_CHECK( MyHistType::HasErrorBars );
}


BOOST_AUTO_TEST_CASE(load_reset)
{
  Tomographer::HistogramWithErrorBars<double, float> hist(0.0f, 1.0f, 10);
  Tomographer::HistogramWithErrorBars<double, double> hist2(0.0, 1.0, 10);

  Eigen::Matrix<int,10,1> m;
  (m << 0,    1,  4, 30, 95,
        150, 77, 18,  5,  1).finished();
  Eigen::Matrix<float,10,1> m2;
  (m2 << 0,    1,  1, 4, 2,
         5, 7, 1,  3,  0.5).finished();

  hist.load(m, m2, 42);
  MY_BOOST_CHECK_EIGEN_EQUAL(hist.bins, m.cast<float>(), tol);
  MY_BOOST_CHECK_EIGEN_EQUAL(hist.delta, m2, tol);
  BOOST_CHECK_EQUAL(hist.off_chart, 42);

  hist2.load(m, m2, 42);
  MY_BOOST_CHECK_EIGEN_EQUAL(hist2.bins, m.cast<double>(), tol);
  MY_BOOST_CHECK_EIGEN_EQUAL(hist2.delta, m2.cast<double>(), tol);
  BOOST_CHECK_EQUAL(hist2.off_chart, 42);

  hist2.reset();
  auto zeros = Eigen::Array<double,10,1>::Zero();
  MY_BOOST_CHECK_EIGEN_EQUAL(hist2.bins, zeros, tol);
  MY_BOOST_CHECK_EIGEN_EQUAL(hist2.delta, zeros, tol);
  BOOST_CHECK_EQUAL(hist2.off_chart, 0);
}

#if __GNUC__ == 4 && __GNUC_MINOR__ <= 6 && !defined(__clang__)
#define IS_GCC_4_6 1
#else
#define IS_GCC_4_6 0
#endif

BOOST_AUTO_TEST_CASE(normalized)
{
  { Tomographer::HistogramWithErrorBars<float, int> hist(0.0f, 4.0f, 4);
    hist.load( inline_vector_4<int>(0, 3, 19, 24),
               inline_vector_4<float>(0, 1, 4, 3),
               10 );
    auto hn = hist.normalized();
    auto n = hist.normalization();
#if ! IS_GCC_4_6  // gcc 4.6 doesn't like decltype(...) it appears:
    typedef std::is_same<decltype(hn)::Scalar, decltype(hist)::Scalar> test1td;
    TOMO_STATIC_ASSERT_EXPR(test1td::value) ;
    typedef std::is_same<decltype(hn)::CountType, decltype(float(1)+int(1))> test2td;
    TOMO_STATIC_ASSERT_EXPR(test2td::value) ;
#endif
    MY_BOOST_CHECK_EIGEN_EQUAL(hn.bins, inline_vector_4<float>(0, 3, 19, 24)/n, tol_f);
    MY_BOOST_CHECK_EIGEN_EQUAL(hn.delta, inline_vector_4<float>(0, 1, 4, 3)/n, tol_f);
    MY_BOOST_CHECK_FLOATS_EQUAL(hn.off_chart, 10.f/n, tol_f);
    MY_BOOST_CHECK_FLOATS_EQUAL(hn.normalization(), 1.f, tol_f);
  }
  { Tomographer::HistogramWithErrorBars<float, int> hist(0.0f, 1.0f, 4);
    hist.load( inline_vector_4<int>(0, 3, 19, 24),
               inline_vector_4<float>(0, 1, 4, 3),
               10 );
    auto hn = hist.normalized<double>();
    // the x-axis Scalar doesn't change, it's the count type that does.
#if ! IS_GCC_4_6  // gcc 4.6 doesn't like decltype(...) it appears:
    typedef std::is_same<decltype(hn)::Scalar, decltype(hist)::Scalar> td1;
    TOMO_STATIC_ASSERT_EXPR(td1::value) ;
    typedef std::is_same<decltype(hn)::CountType, double> td2;
    TOMO_STATIC_ASSERT_EXPR(td2::value) ;
#endif
    auto n = hist.normalization<double>();
    MY_BOOST_CHECK_EIGEN_EQUAL(hn.bins, inline_vector_4<double>(0, 3, 19, 24)/n, tol);
    MY_BOOST_CHECK_EIGEN_EQUAL(hn.delta, inline_vector_4<double>(0, 1, 4, 3)/n, tol);
    MY_BOOST_CHECK_FLOATS_EQUAL(hn.off_chart, 10.0/n, tol);
    MY_BOOST_CHECK_FLOATS_EQUAL(hn.normalization(), 1.0, tol);
  }
}

BOOST_AUTO_TEST_CASE(normalized_counts)
{
  { Tomographer::HistogramWithErrorBars<float, int> hist(0.0f, 4.0f, 4);
    hist.load( inline_vector_4<int>(0, 3, 19, 24),
               inline_vector_4<float>(0, 1, 4, 3),
               10 );
    auto hn = hist.normalizedCounts();
    float n = hist.totalCounts();
#if ! IS_GCC_4_6  // gcc 4.6 doesn't like decltype(...) it appears:
    typedef std::is_same<decltype(hn)::Scalar, decltype(hist)::Scalar> test1td;
    TOMO_STATIC_ASSERT_EXPR(test1td::value) ;
    typedef std::is_same<decltype(hn)::CountType, decltype(float(1)+int(1))> test2td;
    TOMO_STATIC_ASSERT_EXPR(test2td::value) ;
#endif
    MY_BOOST_CHECK_EIGEN_EQUAL(hn.bins, inline_vector_4<float>(0, 3, 19, 24)/n, tol_f);
    MY_BOOST_CHECK_EIGEN_EQUAL(hn.delta, inline_vector_4<float>(0, 1, 4, 3)/n, tol_f);
    MY_BOOST_CHECK_FLOATS_EQUAL(hn.off_chart, 10.f/n, tol_f);
  }
  { Tomographer::HistogramWithErrorBars<float, int> hist(0.0f, 1.0f, 4);
    hist.load( inline_vector_4<int>(0, 3, 19, 24),
               inline_vector_4<float>(0, 1, 4, 3),
               10 );
    auto hn = hist.normalizedCounts<double>();
    double n = hist.totalCounts();
    // the x-axis Scalar doesn't change, it's the count type that does.
#if ! IS_GCC_4_6  // gcc 4.6 doesn't like decltype(...) it appears:
    typedef std::is_same<decltype(hn)::Scalar, decltype(hist)::Scalar> td1;
    TOMO_STATIC_ASSERT_EXPR(td1::value) ;
    typedef std::is_same<decltype(hn)::CountType, double> td2;
    TOMO_STATIC_ASSERT_EXPR(td2::value) ;
#endif
    MY_BOOST_CHECK_EIGEN_EQUAL(hn.bins, inline_vector_4<double>(0, 3, 19, 24)/n, tol);
    MY_BOOST_CHECK_EIGEN_EQUAL(hn.delta, inline_vector_4<double>(0, 1, 4, 3)/n, tol);
    MY_BOOST_CHECK_FLOATS_EQUAL(hn.off_chart, 10.0/n, tol);
  }
}


BOOST_AUTO_TEST_SUITE_END(); // uniform_bins_histogram_with_error_bars


// -----------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(averaged_histogram);

BOOST_AUTO_TEST_CASE(no_underlying_error_bars)
{
  typedef Tomographer::Histogram<double, int> SimpleHistogramType;
  typedef Tomographer::AveragedHistogram<SimpleHistogramType, float> AvgHistogramType;

  typename SimpleHistogramType::Params p(0.0, 1.0, 4);

  AvgHistogramType avghist;

  avghist.reset(p);

  BOOST_CHECK_SMALL(avghist.params.min, tol);
  BOOST_CHECK_CLOSE(avghist.params.max, 1.0, tol_percent);
  BOOST_CHECK_EQUAL(avghist.numBins(), 4u);

  { SimpleHistogramType hist(p);
    hist.load( inline_vector_4<double>(15, 45, 42, 12) , 36 ); // sum=150
    avghist.addHistogram(hist);
  }
  { SimpleHistogramType hist(p);
    hist.load( inline_vector_4<double>(17, 43, 40, 18) , 32 );
    avghist.addHistogram(hist);
  }
  { SimpleHistogramType hist(p);
    hist.load( inline_vector_4<double>(20, 38, 47, 10) , 35 );
    avghist.addHistogram(hist);
  }
  { SimpleHistogramType hist(p);
    hist.load( inline_vector_4<double>(18, 44, 43, 13) , 32 );
    avghist.addHistogram(hist);
  }

  avghist.finalize();

  BOOST_CHECK_EQUAL(avghist.num_histograms, 4);

  BOOST_MESSAGE(avghist.prettyPrint());

  auto cheap_stddev = [](float x, float y, float z, float w) -> float {
    return (float) std::sqrt( ((x*x+y*y+z*z+w*w)/4.f - std::pow((x+y+z+w)/4.f, 2)) / (4.f - 1) );
  };

  BOOST_CHECK_CLOSE(avghist.bins.sum() + avghist.off_chart, 150.0, tol_percent);
  auto vecbins = inline_vector_4<float>(70/4.f,170/4.f,172/4.f,53/4.f);
  MY_BOOST_CHECK_EIGEN_EQUAL(avghist.bins, vecbins, tol_f);
  auto vecdelta = inline_vector_4<float>(
      cheap_stddev(15, 17, 20, 18),
      cheap_stddev(45, 43, 38, 44),
      cheap_stddev(42, 40, 47, 43),
      cheap_stddev(12, 18, 10, 13)
      );
  MY_BOOST_CHECK_EIGEN_EQUAL(avghist.delta, vecdelta, tol_f);
}


BOOST_AUTO_TEST_CASE(with_underlying_error_bars)
{
  typedef Tomographer::HistogramWithErrorBars<double, float> BaseHistogramType;
  typedef Tomographer::AveragedHistogram<BaseHistogramType, float> AvgHistogramType;

  typename BaseHistogramType::Params p(0.0, 1.0, 4);

  AvgHistogramType avghist;

  avghist.reset(p);

  BOOST_CHECK_SMALL(avghist.params.min, tol);
  BOOST_CHECK_CLOSE(avghist.params.max, 1.0, tol_percent);
  BOOST_CHECK_EQUAL(avghist.numBins(), 4u);

  { BaseHistogramType hist(p);
    hist.load( inline_vector_4<double>(15, 45, 42, 12) ,
               inline_vector_4<float>(1, 1, 1, 1), 36 ); // sum=150
    avghist.addHistogram(hist);
  }
  { BaseHistogramType hist(p);
    hist.load( inline_vector_4<double>(17, 43, 40, 18) ,
               inline_vector_4<float>(2, 2, 5, 2), 32 );
    avghist.addHistogram(hist);
  }
  { BaseHistogramType hist(p);
    hist.load( inline_vector_4<double>(20, 38, 47, 10) ,
               inline_vector_4<float>(1, 2, 13, 4), 35 );
    avghist.addHistogram(hist);
  }
  { BaseHistogramType hist(p);
    hist.load( inline_vector_4<double>(18, 44, 43, 13) ,
               inline_vector_4<float>(2, 1, 24, 3), 32 );
    avghist.addHistogram(hist);
  }

  avghist.finalize();

  BOOST_CHECK_EQUAL(avghist.num_histograms, 4);

  BOOST_TEST_MESSAGE(avghist.prettyPrint());

  BOOST_CHECK_CLOSE(avghist.bins.sum() + avghist.off_chart, 150.f, tol_percent_f);
  auto vecbins = inline_vector_4<float>(70/4.f,170/4.f,172/4.f,53/4.f);
  MY_BOOST_CHECK_EIGEN_EQUAL(avghist.bins, vecbins, tol_f);
  auto vecdelta = inline_vector_4<float>(
      std::sqrt(1.f+4+1+4)/4.f,
      std::sqrt(1.f+4+4+1)/4.f,
      std::sqrt(1.f+5*5+13*13+24*24)/4.f,
      std::sqrt(1.f+4.f+16.f+9.f)/4.f
      );
  MY_BOOST_CHECK_EIGEN_EQUAL(avghist.delta, vecdelta, tol_f);
}


BOOST_AUTO_TEST_SUITE_END(); // averaged_histogram

// -----------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(aggregated_histogram);

BOOST_AUTO_TEST_CASE(simple)
{
  typedef Tomographer::Histogram<double, int> SimpleHistogramType;
  typedef Tomographer::AveragedHistogram<SimpleHistogramType, float> AvgHistogramType;

  typename SimpleHistogramType::Params p(0.0, 1.0, 4);

  SimpleHistogramType hist1(p);
  hist1.load( inline_vector_4<double>(15, 45, 42, 12) , 36 ); // sum=150
  SimpleHistogramType hist2(p);
  hist2.load( inline_vector_4<double>(17, 43, 40, 18) , 32 );
  SimpleHistogramType hist3(p);
  hist3.load( inline_vector_4<double>(20, 38, 47, 10) , 35 );
  SimpleHistogramType hist4(p);
  hist4.load( inline_vector_4<double>(18, 44, 43, 13) , 32 );

  AvgHistogramType avghist;
  avghist.reset(p);
  avghist.addHistogram(hist1);
  avghist.addHistogram(hist2);
  avghist.addHistogram(hist3);
  avghist.addHistogram(hist4);
  avghist.finalize();

  std::vector<const SimpleHistogramType*> hist_list;
  hist_list.push_back(&hist1);
  hist_list.push_back(&hist2);
  hist_list.push_back(&hist3);
  hist_list.push_back(&hist4);

  Tomographer::AggregatedHistogramSimple<SimpleHistogramType, float> aggregated
    = Tomographer::AggregatedHistogramSimple<SimpleHistogramType, float>::aggregate(
        p,
        hist_list,
        [](const SimpleHistogramType * item) -> const SimpleHistogramType& { return *item; }
        );

  MY_BOOST_CHECK_FLOATS_EQUAL(aggregated.final_histogram.params.min, p.min, tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(aggregated.final_histogram.params.max, p.max, tol);
  BOOST_CHECK_EQUAL(aggregated.final_histogram.params.num_bins, p.num_bins);
  MY_BOOST_CHECK_EIGEN_EQUAL(aggregated.final_histogram.bins, avghist.bins, tol_f);
  MY_BOOST_CHECK_EIGEN_EQUAL(aggregated.final_histogram.delta, avghist.delta, tol_f);
}

BOOST_AUTO_TEST_CASE(binning)
{
  typedef Tomographer::HistogramWithErrorBars<double, float> BaseHistogramType;
  typedef Tomographer::Histogram<double, float> SimpleHistogramType;
  typedef Tomographer::AveragedHistogram<BaseHistogramType, float> AvgHistogramType;
  typedef Tomographer::AveragedHistogram<SimpleHistogramType, float>
    SimpleAvgHistogramType;

  typename BaseHistogramType::Params p(0.0, 1.0, 4);

  BaseHistogramType hist1(p);
  hist1.load( inline_vector_4<double>(15, 45, 42, 12) ,
              inline_vector_4<float>(1, 1, 1, 1), 36 ); // sum=150
  BaseHistogramType hist2(p);
  hist2.load( inline_vector_4<double>(17, 43, 40, 18) ,
              inline_vector_4<float>(2, 2, 5, 2), 32 );
  BaseHistogramType hist3(p);
  hist3.load( inline_vector_4<double>(20, 38, 47, 10) ,
              inline_vector_4<float>(1, 2, 13, 4), 35 );
  BaseHistogramType hist4(p);
  hist4.load( inline_vector_4<double>(18, 44, 43, 13) ,
              inline_vector_4<float>(2, 1, 24, 3), 32 );

  AvgHistogramType avghist;
  avghist.reset(p);
  avghist.addHistogram(hist1);
  avghist.addHistogram(hist2);
  avghist.addHistogram(hist3);
  avghist.addHistogram(hist4);
  avghist.finalize();

  SimpleHistogramType shist1(p);
  shist1.load( inline_vector_4<double>(15, 45, 42, 12) , 36 ); // sum=150
  SimpleHistogramType shist2(p);
  shist2.load( inline_vector_4<double>(17, 43, 40, 18) , 32 );
  SimpleHistogramType shist3(p);
  shist3.load( inline_vector_4<double>(20, 38, 47, 10) , 35 );
  SimpleHistogramType shist4(p);
  shist4.load( inline_vector_4<double>(18, 44, 43, 13) , 32 );

  SimpleAvgHistogramType simpleavghist;
  simpleavghist.reset(p);
  simpleavghist.addHistogram(shist1);
  simpleavghist.addHistogram(shist2);
  simpleavghist.addHistogram(shist3);
  simpleavghist.addHistogram(shist4);
  simpleavghist.finalize();

  std::vector<const BaseHistogramType*> hist_list;
  hist_list.push_back(&hist1);
  hist_list.push_back(&hist2);
  hist_list.push_back(&hist3);
  hist_list.push_back(&hist4);

  Tomographer::AggregatedHistogramWithErrorBars<BaseHistogramType, float> aggregated
    = Tomographer::AggregatedHistogramWithErrorBars<BaseHistogramType, float>::aggregate(
        p,
        hist_list,
        [](const BaseHistogramType * item) -> const BaseHistogramType& { return *item; }
        );

  MY_BOOST_CHECK_FLOATS_EQUAL(aggregated.final_histogram.params.min, p.min, tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(aggregated.final_histogram.params.max, p.max, tol);
  BOOST_CHECK_EQUAL(aggregated.final_histogram.params.num_bins, p.num_bins);
  MY_BOOST_CHECK_EIGEN_EQUAL(aggregated.final_histogram.bins, avghist.bins, tol_f);
  MY_BOOST_CHECK_EIGEN_EQUAL(aggregated.final_histogram.delta, avghist.delta, tol_f);

  MY_BOOST_CHECK_FLOATS_EQUAL(aggregated.simple_final_histogram.params.min, p.min, tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(aggregated.simple_final_histogram.params.max, p.max, tol);
  BOOST_CHECK_EQUAL(aggregated.simple_final_histogram.params.num_bins, p.num_bins);
  MY_BOOST_CHECK_EIGEN_EQUAL(aggregated.simple_final_histogram.bins, simpleavghist.bins, tol_f);
  MY_BOOST_CHECK_EIGEN_EQUAL(aggregated.simple_final_histogram.delta, simpleavghist.delta, tol_f);
}

BOOST_AUTO_TEST_SUITE_END()

// -----------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(formatting)

BOOST_AUTO_TEST_SUITE(histogram_pretty_print)

BOOST_AUTO_TEST_CASE(basic)
{
  Tomographer::Histogram<double> hist(0.0, 1.0, 5);
  hist.load( (Eigen::VectorXi(5) << 0, 1, 4, 6, 2).finished() );

  const int max_width = 80;

  const std::string correct_str =
    "0.1000 |                                                                       0\n"
    "0.3000 |***********                                                            1\n"
    "0.5000 |*********************************************                          4\n"
    "0.7000 |******************************************************************     6\n"
    "0.9000 |**********************                                                 2\n"
    ;

  const std::string s = Tomographer::histogramPrettyPrint(hist, max_width);
  BOOST_CHECK_EQUAL(s, correct_str) ;
  const std::string s2 = hist.prettyPrint(max_width);
  BOOST_CHECK_EQUAL(s2, correct_str) ;

  std::ostringstream ss;
  Tomographer::histogramPrettyPrint(ss, hist, max_width);
  BOOST_CHECK_EQUAL(ss.str(), correct_str) ;
}
BOOST_AUTO_TEST_CASE(errbars)
{
  Tomographer::HistogramWithErrorBars<double> hist(0.0, 1.0, 5);
  // make the values small (<1) to make sure that there hasn't been a conversion to int
  // somewhere in the process
  hist.load( 0.01 * (Eigen::VectorXd(5) << 0.0, 1.0, 4.53, 6.5, 2.2).finished(),
             0.01 * (Eigen::VectorXd(5) << 0, 0.4, 0.5, 0.3, 0.15).finished() );

  const int max_width = 80;

  const std::string correct_str =
    "0.1000 ||                                                       0.0000 +- 0.0000\n"
    "0.3000 |*****|-----|                                            0.0100 +- 0.0040\n"
    "0.5000 |********************************|-------|               0.0453 +- 0.0050\n"
    "0.7000 |*************************************************|---|  0.0650 +- 0.0030\n"
    "0.9000 |****************|--|                                    0.0220 +- 0.0015\n"
    ;

  const std::string s = Tomographer::histogramPrettyPrint(hist, max_width);
  BOOST_CHECK_EQUAL(s, correct_str) ;
  const std::string s2 = hist.prettyPrint(max_width);
  BOOST_CHECK_EQUAL(s2, correct_str) ;

  std::ostringstream ss;
  Tomographer::histogramPrettyPrint(ss, hist, max_width);
  BOOST_CHECK_EQUAL(ss.str(), correct_str) ;
}
BOOST_AUTO_TEST_CASE(mednumbers)
{
  Tomographer::HistogramWithErrorBars<double> hist(0.0, 1.0, 5);
  // make the values small (<1) to make sure that there hasn't been a conversion to int
  // somewhere in the process
  hist.load( (Eigen::VectorXd(5) << 0, 181.6, 427.3, 1051.4, 1394.8).finished(),
             (Eigen::VectorXd(5) << 0, 32.069439, 46.908433, 46.468675, 32.718598).finished() );

  const int max_width = 80;

  const std::string correct_str =
    "0.1000 ||                                                             0 +-     0\n"
    "0.3000 |******|-|                                                   182 +-    32\n"
    "0.5000 |***************|---|                                        427 +-    47\n"
    "0.7000 |***************************************|---|               1051 +-    46\n"
    "0.9000 |*****************************************************|-|   1395 +-    33\n"
    ;

  const std::string s = Tomographer::histogramPrettyPrint(hist, max_width);
  BOOST_CHECK_EQUAL(s, correct_str) ;
  const std::string s2 = hist.prettyPrint(max_width);
  BOOST_CHECK_EQUAL(s2, correct_str) ;

  std::ostringstream ss;
  Tomographer::histogramPrettyPrint(ss, hist, max_width);
  BOOST_CHECK_EQUAL(ss.str(), correct_str) ;
}
BOOST_AUTO_TEST_CASE(largenumbers)
{
  Tomographer::HistogramWithErrorBars<double> hist(0.0, 1.0, 5);
  // make the values small (<1) to make sure that there hasn't been a conversion to int
  // somewhere in the process
  hist.load( (Eigen::VectorXd(5) << 0, 100033.931, 4538205.111, 6501842.882, 221045.155).finished(),
             (Eigen::VectorXd(5) << 0, 40342.111, 51044.209, 30114.315, 90104.919).finished() );

  const int max_width = 80;

  const std::string correct_str =
    "0.1000 ||                                                             0 +-     0\n"
    "0.3000 |||                                                       100034 +- 40342\n"
    "0.5000 |*************************************||                 4538205 +- 51044\n"
    "0.7000 |*****************************************************|  6501843 +- 30114\n"
    "0.9000 |*|-|                                                     221045 +- 90105\n"
    ;

  const std::string s = Tomographer::histogramPrettyPrint(hist, max_width);
  BOOST_CHECK_EQUAL(s, correct_str) ;
  const std::string s2 = hist.prettyPrint(max_width);
  BOOST_CHECK_EQUAL(s2, correct_str) ;

  std::ostringstream ss;
  Tomographer::histogramPrettyPrint(ss, hist, max_width);
  BOOST_CHECK_EQUAL(ss.str(), correct_str) ;
}

BOOST_AUTO_TEST_SUITE(nobug)

BOOST_AUTO_TEST_CASE(toolargeerrbar)
{
  Tomographer::HistogramWithErrorBars<double> hist(-2.0, 2.0, 2);
  // make the values small (<1) to make sure that there hasn't been a conversion to int
  // somewhere in the process
  hist.load( (Eigen::VectorXd(2) << 3.0, 4.0).finished() ,
             (Eigen::VectorXd(2) << 4.0, 2.0).finished() );

  const int max_width = 80;

  const std::string correct_str =
    "-1.000 ||--------------------------------------------------------|  3.00 +- 4.00\n"
    " 1.000 |*****************|--------------------------------|         4.00 +- 2.00\n"
    ;

  const std::string s = Tomographer::histogramPrettyPrint(hist, max_width);
  BOOST_CHECK_EQUAL(s, correct_str) ;

  const std::string s2 = hist.prettyPrint(max_width);
  BOOST_CHECK_EQUAL(s2, correct_str) ;

  std::ostringstream ss;
  Tomographer::histogramPrettyPrint(ss, hist, max_width);
  BOOST_CHECK_EQUAL(ss.str(), correct_str) ;
}
BOOST_AUTO_TEST_CASE(withinf)
{
  Tomographer::HistogramWithErrorBars<double> hist(-2.0, 2.0, 2);
  // make the values small (<1) to make sure that there hasn't been a conversion to int
  // somewhere in the process
  hist.load( (Eigen::VectorXd(2) << 3.0, std::numeric_limits<double>::infinity()).finished(),
             (Eigen::VectorXd(2) << 4.0, 1.0).finished() );

  const int max_width = 80;

  const std::string correct_str =
    "-1.000 ||--------------------------------------------------------|  3.00 +- 4.00\n"
    " 1.000 ||                                                            inf +- 1.00\n"
    ;

  const std::string s = Tomographer::histogramPrettyPrint(hist, max_width);
  BOOST_CHECK_EQUAL(s, correct_str) ;
  const std::string s2 = hist.prettyPrint(max_width);
  BOOST_CHECK_EQUAL(s2, correct_str) ;
  std::ostringstream ss;
  Tomographer::histogramPrettyPrint(ss, hist, max_width);
  BOOST_CHECK_EQUAL(ss.str(), correct_str) ;
}
BOOST_AUTO_TEST_CASE(withinf2)
{
  Tomographer::HistogramWithErrorBars<double> hist(-2.0, 2.0, 2);
  // make the values small (<1) to make sure that there hasn't been a conversion to int
  // somewhere in the process
  hist.load( (Eigen::VectorXd(2) << 3.0, 2.0).finished() ,
             (Eigen::VectorXd(2) << 4.0, std::numeric_limits<double>::infinity()).finished() );

  const int max_width = 80;

  const std::string correct_str =
    "-1.000 ||--------------------------------------------------------|  3.00 +- 4.00\n"
    " 1.000 ||                                                           2.00 +-  inf\n"
    ;

  const std::string s = Tomographer::histogramPrettyPrint(hist, max_width);
  BOOST_CHECK_EQUAL("\n"+s, "\n"+correct_str) ;
  const std::string s2 = hist.prettyPrint(max_width);
  BOOST_CHECK_EQUAL(s2, correct_str) ;
  std::ostringstream ss;
  Tomographer::histogramPrettyPrint(ss, hist, max_width);
  BOOST_CHECK_EQUAL(ss.str(), correct_str) ;
}
BOOST_AUTO_TEST_CASE(withnan)
{
  Tomographer::HistogramWithErrorBars<double> hist(-2.0, 2.0, 2);
  // make the values small (<1) to make sure that there hasn't been a conversion to int
  // somewhere in the process
  hist.load( (Eigen::VectorXd(2) << 3.0, std::numeric_limits<double>::quiet_NaN()).finished() ,
             (Eigen::VectorXd(2) << 4.0, 1.0).finished() );

  const int max_width = 80;

  const std::string correct_str =
    "-1.000 ||--------------------------------------------------------|  3.00 +- 4.00\n"
    " 1.000 ||                                                            nan +- 1.00\n"
    ;

  const std::string s = Tomographer::histogramPrettyPrint(hist, max_width);
  BOOST_CHECK_EQUAL("\n"+s, "\n"+correct_str) ;
  const std::string s2 = hist.prettyPrint(max_width);
  BOOST_CHECK_EQUAL(s2, correct_str) ;
  std::ostringstream ss;
  Tomographer::histogramPrettyPrint(ss, hist, max_width);
  BOOST_CHECK_EQUAL(ss.str(), correct_str) ;
}
BOOST_AUTO_TEST_CASE(withnan2)
{
  Tomographer::HistogramWithErrorBars<double> hist(-2.0, 2.0, 2);
  // make the values small (<1) to make sure that there hasn't been a conversion to int
  // somewhere in the process
  hist.load( (Eigen::VectorXd(2) << 3.0, 2.0).finished() ,
             (Eigen::VectorXd(2) << 4.0, std::numeric_limits<double>::quiet_NaN()).finished() );

  const int max_width = 80;

  const std::string correct_str =
    "-1.000 ||--------------------------------------------------------|  3.00 +- 4.00\n"
    " 1.000 ||                                                           2.00 +-  nan\n"
    ;

  const std::string s = Tomographer::histogramPrettyPrint(hist, max_width);
  BOOST_CHECK_EQUAL("\n"+s, "\n"+correct_str) ;
  const std::string s2 = hist.prettyPrint(max_width);
  BOOST_CHECK_EQUAL(s2, correct_str) ;
  std::ostringstream ss;
  Tomographer::histogramPrettyPrint(ss, hist, max_width);
  BOOST_CHECK_EQUAL(ss.str(), correct_str) ;
}

BOOST_AUTO_TEST_SUITE_END() // nobug

BOOST_AUTO_TEST_SUITE_END() // histogram_pretty_print


BOOST_AUTO_TEST_CASE(histogram_short_bar)
{
  Tomographer::HistogramWithErrorBars<double> hist(0.0, 1.0, 5);
  // make the values small (<1) to make sure that there hasn't been a conversion to int
  // somewhere in the process
  hist.load( 0.01*(Eigen::VectorXd(5) << 0.0, 1.0, 4.53, 6.5, 2.2).finished(),
             0.01*(Eigen::VectorXd(5) << 0, 0.4, 0.5, 0.3, 0.15).finished() );

  const int max_width = 80;

  const std::string correct_str = "0| -x#+|1";

  const std::string s = Tomographer::histogramShortBar(hist, false, max_width);
  BOOST_CHECK_EQUAL(s, correct_str) ;

  std::ostringstream ss;
  Tomographer::histogramShortBar(ss, hist, false, max_width);
  BOOST_CHECK_EQUAL(ss.str(), correct_str) ;
}

BOOST_AUTO_TEST_CASE(histogram_short_bar_log)
{
  Tomographer::HistogramWithErrorBars<double> hist(0.0, 1.0, 5);
  hist.load( 0.01*(Eigen::VectorXd(5) << 0.0, 1.0, 4.53, 6.5, 2.2).finished(),
             0.01*(Eigen::VectorXd(5) << 0, 0.4, 0.5, 0.3, 0.15).finished() );

  const int max_width = 80;

  const std::string correct_str = "0| .++-|1";
  
  const std::string s = Tomographer::histogramShortBar(hist, true, max_width);
  BOOST_CHECK_EQUAL(s, correct_str) ;

  std::ostringstream ss;
  Tomographer::histogramShortBar(ss, hist, true, max_width);
  BOOST_CHECK_EQUAL(ss.str(), correct_str) ;
}


BOOST_AUTO_TEST_SUITE_END(); // formatting

// =============================================================================
BOOST_AUTO_TEST_SUITE_END();
