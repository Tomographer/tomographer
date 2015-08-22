
#include <cstdio>
#include <random>
#include <iostream>
#include <vector>

// definitions for Tomographer test framework -- this must be included before any
// <tomographer/...> header
#include "test_tomographer.h"

#include <tomographer/qit/util.h>


BOOST_AUTO_TEST_SUITE(test_misc)

BOOST_AUTO_TEST_CASE(is_positive)
{
  BOOST_CHECK(Tomographer::Tools::is_positive(1u)) ;
  BOOST_CHECK(Tomographer::Tools::is_positive(1)) ;
  BOOST_CHECK(Tomographer::Tools::is_positive(1.f)) ;
  BOOST_CHECK(Tomographer::Tools::is_positive(1.0)) ;
  BOOST_CHECK(!Tomographer::Tools::is_positive(-1)) ;
  BOOST_CHECK(!Tomographer::Tools::is_positive(-1.0)) ;
}

BOOST_AUTO_TEST_SUITE(powers_of_two)

BOOST_AUTO_TEST_CASE(fixed)
{
  Eigen::Array<double,1,9> twopows = Tomographer::powers_of_two<Eigen::Array<double,1,9> >().transpose();
  Eigen::Array<double,1,9> correct_twopows;
  correct_twopows << 1, 2, 4, 8, 16, 32, 64, 128, 256;

  MY_BOOST_CHECK_EIGEN_EQUAL(twopows, correct_twopows, tol);
}

BOOST_AUTO_TEST_CASE(dyn_vector)
{
  Eigen::VectorXd twopows(6);
  twopows = Tomographer::powers_of_two<Eigen::VectorXd>(6);
  BOOST_MESSAGE("twopows = " << twopows);
  Eigen::VectorXd correct_twopows(6);
  (correct_twopows << 1, 2, 4, 8, 16, 32).finished();

  MY_BOOST_CHECK_EIGEN_EQUAL(twopows, correct_twopows, tol);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE_END()
