
#include <iostream>
#include <iomanip>
#include <stdexcept>

#include "test_tomographer.h"

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
  Tomographer::UniformBinsHistogram<float,int> hist(0.0, 1.0, 10);
  hist.record(0.42323);
  hist.record(0.933);
  hist.record(0.5);
  hist.record(0.55555232);
  hist.record(0.4999);
  hist.record(0.52);
  hist.record(1.2);

  BOOST_CHECK_EQUAL(hist.num_bins(), 10);
  BOOST_CHECK_SMALL(hist.min(), tol);
  BOOST_CHECK_CLOSE(hist.max(), 1.0, tol_percent);

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
}

BOOST_AUTO_TEST_SUITE_END();


// =============================================================================
BOOST_AUTO_TEST_SUITE_END();
