
#include <iostream>
#include <iomanip>
#include <stdexcept>

#include "test_tomographer.h"

#include <tomographer/histogram.h>
#include <tomographer/qit/matrq.h>
#include <tomographer/qit/random_unitary.h>
#include <tomographer/qit/pos_semidef_util.h>
#include <tomographer/tomoproblem.h>

#include <Eigen/Core>
#include <Eigen/Eigenvalues>



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

  MY_BOOST_CHECK_EIGEN_EQUAL(hist.bins, a, tol);
  BOOST_CHECK_CLOSE(hist.off_chart, 120.399, tol_percent);
}


BOOST_AUTO_TEST_SUITE_END();

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

BOOST_AUTO_TEST_SUITE_END();


// -----------------------------------------------------------------------------




// =============================================================================
BOOST_AUTO_TEST_SUITE_END();
