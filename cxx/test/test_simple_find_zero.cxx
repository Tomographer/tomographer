
#include <cmath>
#include <limits>
#include <iostream>

// we want `eigen_assert()` to raise an `eigen_assert_exception` here
#include <tomographer/tools/eigen_assert_exception.h>

#include <tomographer/tools/loggers.h>
#include <tomographer/qit/simple_find_zero.h>

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>



// -----------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(test_simple_find_zero)

// this is referenced later by f4()
double f1(double x)
{
  // zero at x = 1.2 (by construction)
  return std::exp(0.5*x) - std::exp(0.5*1.2);
};

BOOST_AUTO_TEST_CASE(test_simple_find_zero_1)
{
  const double x1 = -1;
  const double x2 = 10;
  const double tol = 1e-15;

  double pt = Tomographer::Tools::simpleFindZero<double, double>(f1, x1, x2, 50, tol, NULL, NULL);

  BOOST_MESSAGE("Point is = " << pt << " [tol="<<tol<<"]    (known to be = 1.2)");
  BOOST_CHECK_CLOSE(pt, 1.2, 1e-8 /* PERCENT */);
  // no final value returned to check
}

// -----------------------------------------------------------------------------

double f2(double x)
{
  // zero found at x = 0.511578
  return std::exp(0.5*x) + 3 - 6*std::sqrt(std::fabs(x));
}

BOOST_AUTO_TEST_CASE(test_simple_find_zero_2)
{
  // set up x1 and x2 so that the root at 0.511578 is picked up
  const double x1 = 0.1;
  const double x2 = 0.2;
  const double tol = 1e-10;

  double final_value = std::numeric_limits<double>::quiet_NaN();
  int final_iters = -1;

  double pt = Tomographer::Tools::simpleFindZero<double, double>(f2, x1, x2, 50, tol,
								 &final_value, &final_iters);

  BOOST_MESSAGE("Point is = " << pt << ", final_value = " << final_value << " [tol="<<tol<<"]"
		<< "  final_iters="<<final_iters);
  BOOST_CHECK_CLOSE(pt, 0.51157781220155718, 1e-5 /*PERCENT*/);
  BOOST_CHECK_SMALL(final_value, tol /* abs tol */);
}

// -----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_simple_find_zero_3)
{
  auto f3 = [](double x){
    // zero at 0, but over a large interval is *really* small: our algorithm finds a
    // suitable zero at  x = 0.264675
    return exp(-1/(x*x));
  };

  const double x1 = 1;
  const double x2 = .5;
  const double tol = 1e-10;

  double final_value = std::numeric_limits<double>::quiet_NaN();

  double pt = Tomographer::Tools::simpleFindZero<double, double>(f3, x1, x2, 50, tol, &final_value);

  BOOST_MESSAGE("Point is = " << pt << ", final_value = " << final_value);
  // the value of pt won't be of interest, as the function is really really almost zero
  // over a finite interval (don't believe me? run "Plot[Exp[-(1/x^2)], {x, -1, 1}]" in
  // Mathematica)
  //
  // So just check for the final_value, which should be small.
  BOOST_CHECK_SMALL(final_value, tol /* abs tol */);
}

// -----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_simple_find_zero_4)
{
  // recovering from function returning NaN ?
  auto f4 = [](double x) {
    if (x > 1.3) {
      return std::numeric_limits<double>::quiet_NaN();
    }
    return f1(x);
  };

  const double x1 = -1;
  const double x2 = 40;
  const double tol = 1e-15;

  double final_value = std::numeric_limits<double>::quiet_NaN();
  int final_iters = -1;

  Tomographer::Logger::BufferLogger logger(Tomographer::Logger::LONGDEBUG);

  double pt = Tomographer::Tools::simpleFindZero<double, double>(f4, x1, x2, 50, tol,
								 &final_value, &final_iters,
								 logger);

  BOOST_MESSAGE(logger.get_contents());
  BOOST_MESSAGE("Point is = " << pt << ", final_value = " << final_value << " [tol="<<tol<<"]"
		<< "  final_iters="<<final_iters);
  BOOST_CHECK_CLOSE(pt, 1.2, 1e-10 /* PERCENT */);
  BOOST_CHECK_SMALL(final_value, tol);
}

// -----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(test_simple_find_zero_5)
{
  // recovering from function returning NaN, test #2

  const double x1 = 0.1;
  const double x2 = 4;
  const double tol = 1e-8;

  double final_value = std::numeric_limits<double>::quiet_NaN();
  int final_iters = -1;

  Tomographer::Logger::BufferLogger logger(Tomographer::Logger::LONGDEBUG);

  double pt = Tomographer::Tools::simpleFindZero<double, double>(
      [](double x) -> double {
	return pow(x, 0.55);
      },
      x1,
      x2,
      50,
      tol,
      &final_value,
      &final_iters,
      logger
      );

  BOOST_MESSAGE(logger.get_contents());
  BOOST_MESSAGE("Point is = " << pt << ", final_value = " << final_value << " [tol="<<tol<<"]"
		<< "  final_iters="<<final_iters);
  BOOST_CHECK_SMALL(pt, 1e-8);
  BOOST_CHECK_SMALL(final_value, tol);
}


BOOST_AUTO_TEST_SUITE_END()
