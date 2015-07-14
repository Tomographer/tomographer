
#include <cstdio>
#include <random>
#include <iostream>
#include <vector>

#define TOMOGRAPHER_TEST_EIGEN_ASSERT_ASSERT
// definitions for Tomographer test framework -- this must be included before any
// <tomographer/...> header
#include "test_tomographer.h"

#include <tomographer/mhrw_bin_err.h>
#include <tomographer/tools/loggers.h>
#include <tomographer/dmmhrw.h>
#include <tomographer/mhrw.h>
#include <tomographer/tomoproblem.h>
#include <tomographer/qit/matrq.h>




struct test_norm_value_calculator
{
  typedef double ValueType;

  template<typename Derived>
  inline ValueType getValue(const Eigen::MatrixBase<Derived> & pt)
  {
    return pt.norm();
  }
};

template<typename RealScalar_, int Dim = 4>
struct test_hypercube_mhwalker
{
  typedef RealScalar_ RealScalar;
  typedef Eigen::Matrix<RealScalar, Dim, 1> PointType;

  typedef float FnValueType;

  enum { UseFnSyntaxType = Tomographer::MHUseFnValue };

  // random number generator, etc.
  std::mt19937 rng;
  std::uniform_real_distribution<double> dist;

  // constructor
  test_hypercube_mhwalker() : rng(0), dist(-1.0, 1.0) { }

  inline void init() { }

  auto startpoint() -> decltype(PointType::Zero()) { return PointType::Zero(); }

  inline void thermalizing_done() { }
  inline void done() { }
  inline PointType jump_fn(const PointType & curpt, RealScalar step_size)
  {
    PointType newpt;
    newpt = curpt + step_size*Tomographer::dense_random<PointType>(rng, dist);
    // do the random walk on the torus, i.e. modulo 1.0
    for (int i = 0; i < Dim; ++i) {
      if (newpt(i) < 0) { newpt(i) += 1.0; }
      if (newpt(i) > 1) { newpt(i) -= 1.0; }
    }
    return newpt;
  }

  inline FnValueType fnval(const PointType & /*curpt*/)
  {
    return 1.0;
  }
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
  typedef Tomographer::BinningAnalysis<double, Tomographer::Logger::BufferLogger> OurBinningAnalysis;
  OurBinningAnalysis bina(4, 2, logger);

  logger.debug("basic()", "Starting to feed samples to the binning analysis object");

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
  bin_means = inline_vector(1.0/8, // == 0.125
			    4*100.0/8, // == 50
			    (1+1+1+2+0.5)/8.0, // == 0.6875
			    (8*(8+1)/2.0)/8.0); // == 4.5
  Eigen::Array<double,4,3> bin_sqmeans;
  bin_sqmeans.col(0) = inline_vector(1/8.0, // == 0.125
                                     4*(100*100)/8.0, // == 5000
                                     (1+1+1+4+0.25)/8.0, // == 0.90625
                                     (1+4+9+16+25+36+49+64)/8.0); // == 25.5
  bin_sqmeans.col(1) = inline_vector((0+0+0+0.5*0.5)/4.0, // == 0.0625
                                     2*(100*100)/4.0, // == 5000
                                     (0.5*0.5 + 0.5*0.5 + 0.5*0.5 + pow((2.0+0.5)/2,2))/4.0, // == 0.578125
                                     (1.5*1.5 + 3.5*3.5 + 5.5*5.5 + 7.5*7.5)/4.0); // == 25.25
  bin_sqmeans.col(2) = inline_vector((0+0.25*0.25)/2.0, // == 0.03125
                                     (100*100)/2.0, // == 5000
                                     (0.5*0.5 + (3.5/4)*(3.5/4))/2.0, // == 0.5078125
                                     (2.5*2.5 + 6.5*6.5)/2.0); // == 24.25

  Eigen::Array<double,4,3> error_levels;
  // binning analysis: don't forget to divide by sqrt(num_samples_seen_by_this_bin_level - 1)
  // See http://arxiv.org/abs/0906.0943
  error_levels <<
    (0.125 - 0.125*0.125)/(2*4-1),       (0.0625-0.125*0.125)/(2*2-1),   (0.03125 - 0.125*0.125)/1,
    (5000.-50*50)/(2*4-1),                      (5000.-50*50)/(2*2-1),   (5000.-50*50)/1,
    (0.90625-0.6875*0.6875)/(2*4-1), (0.578125-0.6875*0.6875)/(2*2-1),   (0.5078125-0.6875*0.6875)/1,
    (25.5-4.5*4.5)/(2*4-1),                   (25.25-4.5*4.5)/(2*2-1),   (24.25-4.5*4.5)/1 ;
  error_levels = error_levels.cwiseSqrt();
 // = (bin_sqmeans - (bin_means.cwiseProduct(bin_means)).replicate<1,3>()).cwiseSqrt();
 //  error_levels.col(0) /= std::sqrt( bina.get_n_flushes() * 4.0 );
 //  error_levels.col(1) /= std::sqrt( bina.get_n_flushes() * 2.0 );
 //  error_levels.col(2) /= std::sqrt( bina.get_n_flushes() * 1.0 );

  logger.debug("test_mhrw_bin_err::binning_analysis::basic", [&](std::ostream & str) {
      str << "we should obtain bin_means = \n" << bin_means << "\n"
          << "\tand bin_sqmeans = \n" << bin_sqmeans << "\n";
    });

  logger.debug("test_mhrw_bin_err::binning_analysis::basic", [&](std::ostream & str) {
      str << "Debug: binning analysis uses powers_of_two matrix for normalization: \n"
	  << Tomographer::replicated<OurBinningAnalysis::NumTrackValuesCTime,1>(
	      Tomographer::powers_of_two<Eigen::Array<OurBinningAnalysis::ValueType,
						      OurBinningAnalysis::NumLevelsPlusOneCTime,
						      1> >(bina.num_levels()+1)
	      .transpose().reverse(),
	      // replicated by:
	      bina.num_track_values(), 1
	      ) << "\n";
    });

  BOOST_MESSAGE(logger.get_contents());
  BOOST_CHECK_EQUAL(bina.get_n_flushes(), 2);
  MY_BOOST_CHECK_EIGEN_EQUAL(bina.get_bin_means(), bin_means, tol);
  MY_BOOST_CHECK_EIGEN_EQUAL(bina.get_bin_sqmeans(), bin_sqmeans, tol);

  BOOST_CHECK_EQUAL(bina.num_track_values(), 4);
  BOOST_CHECK_EQUAL(bina.num_levels(), 2);

  Tomographer::BinningAnalysis<double, Tomographer::Logger::BufferLogger>::BinSqMeansArray
    error_levels_calc(4,3);
  error_levels_calc = bina.calc_error_levels();

  BOOST_MESSAGE("reported error_levels = \n" << error_levels_calc);

  BOOST_CHECK(OurBinningAnalysis::StoreBinMeans) ;
  MY_BOOST_CHECK_EIGEN_EQUAL(error_levels_calc, error_levels, tol);
  MY_BOOST_CHECK_EIGEN_EQUAL(bina.calc_error_lastlevel(), error_levels.col(2), tol);
}


BOOST_AUTO_TEST_CASE(no_bin_means)
{
  Tomographer::Logger::BufferLogger logger(Tomographer::Logger::LONGDEBUG);
  typedef Tomographer::BinningAnalysis<double, Tomographer::Logger::BufferLogger, 4, 2, false /*StoreBinMeans_*/>
    OurBinningAnalysis;
  OurBinningAnalysis bina(4, 2, logger);

  bina.process_new_values(0, inline_vector(0.0, 0.0, 1.0, 0.0));
  bina.process_new_values(1, inline_vector(0.0, 0.0, 1.0, 1.0));
  bina.process_new_values(2, inline_vector(0.0, 1.0, 1.0, 2.0));
  bina.process_new_values(3, inline_vector(0.0, 0.0, 1.0, 3.0));

  BOOST_MESSAGE(logger.get_contents());
  BOOST_CHECK_EQUAL(bina.get_n_flushes(), 1);

  Eigen::Matrix<double, 4, 3> bin_sqmeans;
  bin_sqmeans <<
    0, 0, 0,
    0.25, 0.125, 0.25*0.25,
    1, 1, 1,
    (1*1+2*2+3*3)/4.0, (0.5*0.5 + 2.5*2.5)/2.0, 1.5*1.5
    ;

  MY_BOOST_CHECK_EIGEN_EQUAL(bina.get_bin_sqmeans(), bin_sqmeans, tol);

  Eigen::Array<double,4,1> means;
  means << 0, 0.25, 1, (3*4/2)/4.0;

  Eigen::Array<double,4,3> error_levels;
  error_levels = (bin_sqmeans.array() - (means.cwiseProduct(means)).replicate<1,3>()).cwiseSqrt();
  // divide by sqrt(num-samples)
  error_levels.col(0) /= std::sqrt( bina.get_n_flushes() * 4.0 - 1);
  error_levels.col(1) /= std::sqrt( bina.get_n_flushes() * 2.0 - 1);
  error_levels.col(2) /= std::sqrt( bina.get_n_flushes() * 1.0 - 1);

  BOOST_CHECK(! OurBinningAnalysis::StoreBinMeans) ;
  MY_BOOST_CHECK_EIGEN_EQUAL(bina.calc_error_levels(means), error_levels, tol);
  MY_BOOST_CHECK_EIGEN_EQUAL(bina.calc_error_lastlevel(means), error_levels.col(2), tol);
}

BOOST_AUTO_TEST_SUITE_END()


// =============================================================================


BOOST_AUTO_TEST_SUITE(valuehistogramwithbinning)

BOOST_AUTO_TEST_CASE(simple)
{
  typedef Tomographer::Logger::BufferLogger LoggerType;
  //typedef Tomographer::Logger::FileLogger LoggerType;
  typedef Tomographer::ValueHistogramWithBinningMHRWStatsCollector<
    test_norm_value_calculator,
    LoggerType,
    int,
    float> ValWBinningMHRWStatsCollectorType;

  typedef ValWBinningMHRWStatsCollectorType::HistogramParams HistogramParams;
  typedef test_hypercube_mhwalker<double, 3 /* Dim */> MHWalkerType;
  typedef Tomographer::MHRandomWalk<std::mt19937, MHWalkerType, ValWBinningMHRWStatsCollectorType,
				    LoggerType, int> MHRandomWalkType;

  LoggerType buflog(Tomographer::Logger::DEBUG);
  //LoggerType buflog(stderr, Tomographer::Logger::LONGDEBUG) ;

  test_norm_value_calculator vcalc;

  // N levels -> samples_size = 2^N,  2^10 == 1024
  const int num_levels = 10;
  //const int num_levels = 4;
  ValWBinningMHRWStatsCollectorType vhist(HistogramParams(0.f, 2.0f, 20), vcalc, num_levels, buflog);

  std::mt19937 rng(0); // seeded rng, deterministic results

  MHWalkerType mhwalker;
  MHRandomWalkType rwalk(5, 50, 500000, 0.03, mhwalker, vhist, rng, buflog);

  rwalk.run();

  BOOST_MESSAGE(buflog.get_contents());

  const ValWBinningMHRWStatsCollectorType::Result & result = vhist.getResult();

  // TODO: FIXME: WHY ARE THE ERROR BAR CURVES GOING LIKE CRAZY?
}



BOOST_AUTO_TEST_SUITE_END()

// =============================================================================

BOOST_AUTO_TEST_SUITE_END()
