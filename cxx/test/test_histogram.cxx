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
  Tomographer::UniformBinsHistogram<float, long> hist(0.0f, 1.0f, 10);
  hist.record(0.42323);
  hist.record(0.933);
  hist.record(0.5);
  hist.record(0.55555232);
  hist.record(0.4999);
  hist.record(0.52);
  hist.record(1.2);

  BOOST_CHECK_EQUAL(hist.num_bins(), 10);
  BOOST_CHECK_SMALL(hist.params.min, tol_f);
  BOOST_CHECK_CLOSE(hist.params.max, 1.0, tol_percent_f);

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

  typedef Tomographer::UniformBinsHistogram<float, long> MyHistogramType;
  BOOST_CHECK( ! MyHistogramType::HasErrorBars );
}


BOOST_AUTO_TEST_CASE(boundaries)
{
  Tomographer::UniformBinsHistogram<float, int> hist(0.0f, 1.f, 10);

  BOOST_CHECK(hist.is_within_bounds(0.43f));
  BOOST_CHECK(!hist.is_within_bounds(-0.01f));
  BOOST_CHECK(!hist.is_within_bounds(1.2f));
  BOOST_CHECK(!hist.is_within_bounds(std::numeric_limits<float>::quiet_NaN()));
  BOOST_CHECK(!hist.is_within_bounds(std::numeric_limits<float>::infinity()));
  BOOST_CHECK(!hist.is_within_bounds(-std::numeric_limits<float>::infinity()));

  BOOST_CHECK_EQUAL(hist.bin_index(0.13f), 1);
  BOOST_CHECK_EQUAL(hist.bin_index(0.99f), 9);
  BOOST_CHECK_EQUAL(hist.bin_index(0.34f), 3);

  BOOST_CHECK_SMALL(hist.bin_lower_value(0), tol_f);
  BOOST_CHECK_CLOSE(hist.bin_lower_value(1), 0.1f, tol_percent_f);
  BOOST_CHECK_CLOSE(hist.bin_lower_value(6), 0.6f, tol_percent_f);

  BOOST_CHECK_CLOSE(hist.bin_upper_value(0), 0.1f, tol_percent_f);
  BOOST_CHECK_CLOSE(hist.bin_upper_value(5), 0.6f, tol_percent_f);
  BOOST_CHECK_CLOSE(hist.bin_upper_value(9), 1.0f, tol_percent_f);

  BOOST_CHECK_CLOSE(hist.bin_center_value(1), 0.15f, tol_percent_f);
  
  BOOST_CHECK_CLOSE(hist.bin_resolution(), 0.1f, tol_percent_f);

  {
    EigenAssertTest::setting_scope settingvariable(true); // eigen_assert() should throw an exception.
    BOOST_CHECK_THROW(
        hist.bin_lower_value(-1),
        ::Tomographer::Tools::eigen_assert_exception
        );
    BOOST_CHECK_THROW(
        hist.bin_lower_value(11),
        ::Tomographer::Tools::eigen_assert_exception
        );
    BOOST_CHECK_THROW(
        hist.bin_center_value(-1),
        ::Tomographer::Tools::eigen_assert_exception
        );
    BOOST_CHECK_THROW(
        hist.bin_upper_value(-1),
        ::Tomographer::Tools::eigen_assert_exception
        );
  }
}

BOOST_AUTO_TEST_CASE(add_load_reset)
{
  Tomographer::UniformBinsHistogram<float, long> hist(0.0f, 1.0f, 10);
  hist.record(0.42323);
  hist.record(0.933);
  hist.record(0.5);
  hist.record(0.55555232);
  hist.record(0.4999);
  hist.record(0.52);
  hist.record(1.2);

  Tomographer::UniformBinsHistogram<double, unsigned int> hist2(0.0, 1.0, 10);
  hist2.add(hist);

  int k;

  for (k = 0; k < 10; ++k) {
    BOOST_CHECK_EQUAL(hist.count(k), hist2.count(k));
  }
  BOOST_CHECK_EQUAL(hist.off_chart, hist2.off_chart);

  hist2.load(Eigen::Matrix<int,10,1>::Constant(80));
  for (k = 0; k < 10; ++k) {
    BOOST_CHECK_EQUAL(hist2.count(k), 80);
  }

  Eigen::Matrix<unsigned int,10,1> m;
  (m << 0,    1,  4, 30, 95,
        150, 77, 18,  5,  1).finished();

  hist2.load(m, 42);

  MY_BOOST_CHECK_EIGEN_EQUAL(hist2.bins, m, tol);
  BOOST_CHECK_EQUAL(hist2.off_chart, 42);

  Eigen::Matrix<unsigned int,10,1> m2;
  (m2 <<  0,  0,  0, 10, 10,
         10, 10, 10,  0,  0).finished();
  hist2.add(m2.array());

  MY_BOOST_CHECK_EIGEN_EQUAL(hist2.bins, m+m2, tol);
  BOOST_CHECK_EQUAL(hist2.off_chart, 42);

  hist2.reset();
  auto zeros = Eigen::Array<unsigned int,10,1>::Zero();
  MY_BOOST_CHECK_EIGEN_EQUAL(hist2.bins, zeros, tol);
  BOOST_CHECK_EQUAL(hist2.off_chart, 0);
}



BOOST_AUTO_TEST_CASE(floatcounttype)
{
  Tomographer::UniformBinsHistogram<float, double> hist(0.0f, 1.0f, 10);

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

  BOOST_MESSAGE(hist.pretty_print(100));

  MY_BOOST_CHECK_EIGEN_EQUAL(hist.bins, a, tol);
  BOOST_CHECK_CLOSE(hist.off_chart, 120.399, tol_percent);
}


BOOST_AUTO_TEST_SUITE_END(); // uniform_bins_histogram

// -----------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(uniform_bins_histogram_with_error_bars)

BOOST_AUTO_TEST_CASE(basic)
{
  Tomographer::UniformBinsHistogramWithErrorBars<double, float> hist(-1.0, 1.0, 4);

  hist.record(0.01, 1.2f);
  hist.record(-0.56, 0.1f);

  (hist.delta << 0.1f, 0.f, 0.4f, 0.f).finished();

  Eigen::Array<float,4,1> a;
  a << 0.1f, 0.f, 1.2f, 0.f;
  MY_BOOST_CHECK_EIGEN_EQUAL(hist.bins, a, tol_f);

  int k;

  for (k = 0; k < 4; ++k) {
    BOOST_CHECK_CLOSE(hist.errorbar(k), hist.delta(k), tol_percent);
  }

  hist.reset();

  auto zeros4 = Eigen::Array<float,4,1>::Zero();
  MY_BOOST_CHECK_EIGEN_EQUAL(hist.bins, zeros4, tol_f);
  MY_BOOST_CHECK_EIGEN_EQUAL(hist.delta, zeros4, tol_f);
  BOOST_CHECK_CLOSE(hist.off_chart, 0.f, tol_percent_f);

  typedef Tomographer::UniformBinsHistogramWithErrorBars<double,float> MyHistType;
  BOOST_CHECK( MyHistType::HasErrorBars );
}

BOOST_AUTO_TEST_SUITE_END(); // uniform_bins_histogram_with_error_bars


// -----------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(averaged_histogram);

BOOST_AUTO_TEST_CASE(no_underlying_error_bars)
{
  typedef Tomographer::UniformBinsHistogram<double, int> SimpleHistogramType;
  typedef Tomographer::AveragedHistogram<SimpleHistogramType, float> AvgHistogramType;

  typename SimpleHistogramType::Params p(0.0, 1.0, 4);

  AvgHistogramType avghist;

  avghist.reset(p);

  BOOST_CHECK_SMALL(avghist.params.min, tol);
  BOOST_CHECK_CLOSE(avghist.params.max, 1.0, tol_percent);
  BOOST_CHECK_EQUAL(avghist.num_bins(), 4);

  { SimpleHistogramType hist(p);
    hist.load( inline_vector_4<double>(15, 45, 42, 12) , 36 ); // sum=150
    avghist.add_histogram(hist);
  }
  { SimpleHistogramType hist(p);
    hist.load( inline_vector_4<double>(17, 43, 40, 18) , 32 );
    avghist.add_histogram(hist);
  }
  { SimpleHistogramType hist(p);
    hist.load( inline_vector_4<double>(20, 38, 47, 10) , 35 );
    avghist.add_histogram(hist);
  }
  { SimpleHistogramType hist(p);
    hist.load( inline_vector_4<double>(18, 44, 43, 13) , 32 );
    avghist.add_histogram(hist);
  }

  avghist.finalize();

  BOOST_CHECK_EQUAL(avghist.num_histograms, 4);

  BOOST_MESSAGE(avghist.pretty_print());

  BOOST_CHECK_CLOSE(avghist.bins.sum() + avghist.off_chart, 150.0, tol_percent);
}


BOOST_AUTO_TEST_CASE(with_underlying_error_bars)
{
  typedef Tomographer::UniformBinsHistogramWithErrorBars<double, float> BaseHistogramType;
  typedef Tomographer::AveragedHistogram<BaseHistogramType, float> AvgHistogramType;

  typename BaseHistogramType::Params p(0.0, 1.0, 4);

  AvgHistogramType avghist;

  avghist.reset(p);

  BOOST_CHECK_SMALL(avghist.params.min, tol);
  BOOST_CHECK_CLOSE(avghist.params.max, 1.0, tol_percent);
  BOOST_CHECK_EQUAL(avghist.num_bins(), 4);

  { BaseHistogramType hist(p);
    hist.load( inline_vector_4<double>(15, 45, 42, 12) , 36 ); // sum=150
    (hist.delta << 1, 1, 1, 1).finished();
    avghist.add_histogram(hist);
  }
  { BaseHistogramType hist(p);
    hist.load( inline_vector_4<double>(17, 43, 40, 18) , 32 );
    (hist.delta << 2, 2, 5, 2).finished();
    avghist.add_histogram(hist);
  }
  { BaseHistogramType hist(p);
    hist.load( inline_vector_4<double>(20, 38, 47, 10) , 35 );
    (hist.delta << 1, 2, 13, 4).finished();
    avghist.add_histogram(hist);
  }
  { BaseHistogramType hist(p);
    hist.load( inline_vector_4<double>(18, 44, 43, 13) , 32 );
    (hist.delta << 2, 1, 24, 3).finished();
    avghist.add_histogram(hist);
  }

  avghist.finalize();

  BOOST_CHECK_EQUAL(avghist.num_histograms, 4);

  BOOST_MESSAGE(avghist.pretty_print());

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

BOOST_AUTO_TEST_SUITE(formatting);

BOOST_AUTO_TEST_CASE(histogram_pretty_print)
{
  Tomographer::UniformBinsHistogram<double> hist(0.0, 1.0, 5);
  hist.load( (Eigen::VectorXi(5) << 0, 1, 4, 6, 2).finished() );

  const int max_width = 80;

  const std::string correct_str =
    "0.1000 |                                                                       0\n"
    "0.3000 |***********                                                            1\n"
    "0.5000 |*********************************************                          4\n"
    "0.7000 |******************************************************************     6\n"
    "0.9000 |**********************                                                 2\n"
    ;

  const std::string s = Tomographer::histogram_pretty_print(hist, max_width);
  BOOST_CHECK_EQUAL(s, correct_str) ;
  const std::string s2 = hist.pretty_print(max_width);
  BOOST_CHECK_EQUAL(s2, correct_str) ;

  std::ostringstream ss;
  Tomographer::histogram_pretty_print(ss, hist, max_width);
  BOOST_CHECK_EQUAL(ss.str(), correct_str) ;
}
BOOST_AUTO_TEST_CASE(histogram_pretty_print_errbars)
{
  Tomographer::UniformBinsHistogramWithErrorBars<double> hist(0.0, 1.0, 5);
  // make the values small (<1) to make sure that there hasn't been a conversion to int
  // somewhere in the process
  hist.load( 0.01 * (Eigen::VectorXd(5) << 0.0, 1.0, 4.53, 6.5, 2.2).finished() );
  hist.delta = 0.01 * (Eigen::VectorXd(5) << 0, 0.4, 0.5, 0.3, 0.15).finished();

  const int max_width = 80;

  const std::string correct_str =
    "0.1000 ||                                                       0.0000 +- 0.0000\n"
    "0.3000 |*****|-----|                                            0.0100 +- 0.0040\n"
    "0.5000 |********************************|-------|               0.0453 +- 0.0050\n"
    "0.7000 |*************************************************|---|  0.0650 +- 0.0030\n"
    "0.9000 |****************|--|                                    0.0220 +- 0.0015\n"
    ;

  const std::string s = Tomographer::histogram_pretty_print(hist, max_width);
  BOOST_CHECK_EQUAL(s, correct_str) ;
  const std::string s2 = hist.pretty_print(max_width);
  BOOST_CHECK_EQUAL(s2, correct_str) ;

  std::ostringstream ss;
  Tomographer::histogram_pretty_print(ss, hist, max_width);
  BOOST_CHECK_EQUAL(ss.str(), correct_str) ;
}


BOOST_AUTO_TEST_CASE(histogram_short_bar)
{
  Tomographer::UniformBinsHistogramWithErrorBars<double> hist(0.0, 1.0, 5);
  // make the values small (<1) to make sure that there hasn't been a conversion to int
  // somewhere in the process
  hist.load( 0.01*(Eigen::VectorXd(5) << 0.0, 1.0, 4.53, 6.5, 2.2).finished() );
  hist.delta = 0.01*(Eigen::VectorXd(5) << 0, 0.4, 0.5, 0.3, 0.15).finished();

  const int max_width = 80;

  const std::string correct_str = "0| -x#+|1";

  const std::string s = Tomographer::histogram_short_bar(hist, false, max_width);
  BOOST_CHECK_EQUAL(s, correct_str) ;

  std::ostringstream ss;
  Tomographer::histogram_short_bar(ss, hist, false, max_width);
  BOOST_CHECK_EQUAL(ss.str(), correct_str) ;
}
BOOST_AUTO_TEST_CASE(histogram_short_bar_log)
{
  Tomographer::UniformBinsHistogramWithErrorBars<double> hist(0.0, 1.0, 5);
  hist.load( 0.01*(Eigen::VectorXd(5) << 0.0, 1.0, 4.53, 6.5, 2.2).finished() );
  hist.delta = 0.01*(Eigen::VectorXd(5) << 0, 0.4, 0.5, 0.3, 0.15).finished();

  const int max_width = 80;

  const std::string correct_str = "0| .++-|1";
  
  const std::string s = Tomographer::histogram_short_bar(hist, true, max_width);
  BOOST_CHECK_EQUAL(s, correct_str) ;

  std::ostringstream ss;
  Tomographer::histogram_short_bar(ss, hist, true, max_width);
  BOOST_CHECK_EQUAL(ss.str(), correct_str) ;
}

BOOST_AUTO_TEST_SUITE_END(); // formatting

// =============================================================================
BOOST_AUTO_TEST_SUITE_END();
