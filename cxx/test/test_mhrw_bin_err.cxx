
#include <cstdio>
#include <random>
#include <iostream>
#include <vector>

// we want `eigen_assert()` to raise an `eigen_assert_exception` here
#include <tomographer/tools/eigen_assert_exception.h>

#include <tomographer/mhrw_bin_err.h>
#include <tomographer/tools/loggers.h>
#include <tomographer/dmmhrw.h>
#include <tomographer/mhrw.h>
#include <tomographer/tomoproblem.h>
#include <tomographer/qit/matrq.h>

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/test/output_test_stream.hpp>


BOOST_AUTO_TEST_SUITE(test_mhrw_bin_err)

// =============================================================================

BOOST_AUTO_TEST_SUITE(binning_analysis)

Eigen::Array<double,4,1> inline_vector(double a1, double a2, double a3, double a4)
{
  Eigen::Array<double,4,1> v;
  (v << a1, a2, a3, a4).finished();
  return v;
}

BOOST_AUTO_TEST_CASE(basic)
{
  Tomographer::Logger::BufferLogger logger(Tomographer::Logger::LONGDEBUG);
  Tomographer::BinningAnalysis<double, Tomographer::Logger::BufferLogger> bina(4, 2, logger);

  bina.process_new_values(0, inline_vector(0.0, 0.0,     2.0, 1.0));
  bina.process_new_values(1, inline_vector(0.0, 0.0,     2.0, 2.0));
  bina.process_new_values(2, inline_vector(0.0, 2.0,     2.0, 3.0));
  bina.process_new_values(3, inline_vector(0.0, 2.0,     2.0, 4.0));
  bina.process_new_values(4, inline_vector(0.0, 100.0, 200.0, 5.0));
  bina.process_new_values(5, inline_vector(0.0, 100.0, 200.0, 6.0));
  bina.process_new_values(6, inline_vector(0.0, 200.0, 200.0, 7.0));
  bina.process_new_values(7, inline_vector(1.0, 200.0, 200.0, 8.0));
  // these will only partially fill the next bin:
  bina.process_new_values(8, inline_vector(0.0, 101.4,   2.0, 9.0));
  bina.process_new_values(9, inline_vector(1.0, 140.0,  1e20, 10.0));

  BOOST_MESSAGE(logger.get_contents());
  BOOST_CHECK_EQUAL(bina.get_n_flushes(), 2);
  BOOST_MESSAGE(bina.get_bin_means());
  BOOST_MESSAGE(bina.get_bin_sqmeans());
}

BOOST_AUTO_TEST_SUITE_END()


// =============================================================================

BOOST_AUTO_TEST_SUITE_END()
