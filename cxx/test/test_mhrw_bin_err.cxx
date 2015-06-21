
#include <cstdio>
#include <random>
#include <iostream>
#include <vector>

#define TOMOGRAPHER_TEST_EIGEN_ASSERT_ASSERT // throw assert() failure for debugging.
// definitions for Tomographer test framework -- this must be included before any
// <tomographer/...> header
#include "test_tomographer.h"

#include <tomographer/mhrw_bin_err.h>
#include <tomographer/tools/loggers.h>
#include <tomographer/dmmhrw.h>
#include <tomographer/mhrw.h>
#include <tomographer/tomoproblem.h>
#include <tomographer/qit/matrq.h>



template<typename T>
struct has_get_bin_means_method
{
  template<typename U, std::size_t (U::*)() const> struct SFINAE {};
  template<typename U> static inline char Test(SFINAE<U, &U::get_bin_means> *) { return 0; }
  template<typename U> static inline long Test(int) { return 0; }
  enum {
    value = (sizeof(Test<T>(0)) == sizeof(char))
  };
};



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

  bina.process_new_values(0, inline_vector(0.0, 0.0,   1.0, 1.0));
  bina.process_new_values(1, inline_vector(0.0, 0.0,   0.0, 2.0));
  bina.process_new_values(2, inline_vector(0.0, 0.0,   1.0, 3.0));
  bina.process_new_values(3, inline_vector(0.0, 0.0,   0.0, 4.0));
  bina.process_new_values(4, inline_vector(0.0, 100.0, 0.0, 5.0));
  bina.process_new_values(5, inline_vector(0.0, 100.0, 1.0, 6.0));
  bina.process_new_values(6, inline_vector(0.0, 100.0, 2.0, 7.0));
  bina.process_new_values(7, inline_vector(1.0, 100.0, 0.5, 8.0));
  // these will only partially fill the next bin, and thus not be included in the
  // analysis:
  bina.process_new_values(8, inline_vector(0.5, 101.4,   2.0, 9.0));
  bina.process_new_values(9, inline_vector(1.0, 140.0,  1e20, 10.0));

  Eigen::Array<double,4,1> bin_means;
  bin_means = inline_vector(1.0/8, 4*100.0/8, (1+1+1+2+0.5)/8.0, (8*(8+1)/2.0)/8.0);
  Eigen::Array<double,4,2> bin_sqmeans;
  bin_sqmeans.col(0) = inline_vector(1/8.0,
                                     4*(100*100)/8.0,
                                     (1+1+1+4+0.25)/8.0,
                                     (1+4+9+16+25+36+49+64)/8.0);
  bin_sqmeans.col(1) = inline_vector((0+0+0+0.5*0.5)/4.0,
                                     2*(100*100)/4.0,
                                     (0.5*0.5 + 0.5*0.5 + 0.5*0.5 + pow((2.0+0.5)/2,2))/4.0,
                                     (1.5*1.5 + 3.5*3.5 + 5.5*5.5 + 7.5*7.5)/4.0);

  logger.debug("test_mhrw_bin_err::binning_analysis::basic", [&](std::ostream & str) {
      str << "we should obtain bin_means = \n" << bin_means << "\n"
          << "\tand bin_sqmeans = \n" << bin_sqmeans << "\n";
    });

  BOOST_MESSAGE(logger.get_contents());
  BOOST_CHECK_EQUAL(bina.get_n_flushes(), 2);
  MY_BOOST_CHECK_EIGEN_EQUAL(bina.get_bin_means(), bin_means, tol);
  MY_BOOST_CHECK_EIGEN_EQUAL(bina.get_bin_sqmeans(), bin_sqmeans, tol);

  BOOST_CHECK_EQUAL(bina.num_track_values(), 4);
  BOOST_CHECK_EQUAL(bina.num_levels(), 2);

  Eigen::Array<double,4,2> stddev_levels;
  stddev_levels = (bin_sqmeans - (bin_means.cwiseProduct(bin_means)).replicate<1,2>()).cwiseSqrt();

  std::cout << "stddev_levels = \n" << stddev_levels << "\n";

  Tomographer::BinningAnalysis<double, Tomographer::Logger::BufferLogger>::BinSqMeansArray
    stddev_levels_calc(4,2);
  stddev_levels_calc = bina.calc_stddev_levels();

  std::cout << "stddev_levels_calc = \n" << stddev_levels_calc << "\n";

  MY_BOOST_CHECK_EIGEN_EQUAL(stddev_levels_calc, stddev_levels, tol);
}


BOOST_AUTO_TEST_CASE(no_bin_means)
{
  Tomographer::Logger::BufferLogger logger(Tomographer::Logger::LONGDEBUG);
  Tomographer::BinningAnalysis<double, Tomographer::Logger::BufferLogger, 4, 2, false /*StoreBinMeans_*/>
    bina(4, 2, logger);

  bina.process_new_values(0, inline_vector(0.0, 0.0, 1.0, 0.0));
  bina.process_new_values(1, inline_vector(0.0, 0.0, 1.0, 1.0));
  bina.process_new_values(2, inline_vector(0.0, 1.0, 1.0, 2.0));
  bina.process_new_values(3, inline_vector(0.0, 0.0, 1.0, 3.0));

  BOOST_MESSAGE(logger.get_contents());
  BOOST_CHECK_EQUAL(bina.get_n_flushes(), 2);

  Eigen::Matrix<double, 4, 2> bin_sqmeans;
  bin_sqmeans <<
    0, 0,
    0.25, 0.125,
    1, 1,
    (1*1+2*2+3*3)/4.0, (0.5*0.5 + 2.5*2.5)/2.0
    ;

  MY_BOOST_CHECK_EIGEN_EQUAL(bina.get_bin_sqmeans(), bin_sqmeans, tol);

  Eigen::Array<double,4,1> means;
  means << 0, 0.25, 1, (3*4/2)/4.0;

  Eigen::Array<double,4,2> stddev_levels;
  stddev_levels = (bin_sqmeans.array() - (means.cwiseProduct(means)).replicate<1,2>()).cwiseSqrt();

  BOOST_CHECK(!has_get_bin_means_method<decltype(bina)>::value) ;
}

BOOST_AUTO_TEST_SUITE_END()


// =============================================================================

BOOST_AUTO_TEST_SUITE_END()
