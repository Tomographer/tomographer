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

#include <cstdio>
#include <random>
#include <iostream>
#include <vector>


// definitions for Tomographer test framework -- this must be included before any
// <Eigen/...> or <tomographer/...> header
#include "test_tomographer.h"

#include <tomographer/mhrw_bin_err.h>
#include <tomographer/tools/loggers.h>
#include <tomographer/mhrw.h>
//#include <tomographer/densedm/dmmhrw.h>



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
    newpt = curpt + step_size*Tomographer::Tools::dense_random<PointType>(rng, dist);
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
  typedef Tomographer::BinningAnalysis<Tomographer::BinningAnalysisParams<double>,
                                       Tomographer::Logger::BufferLogger> OurBinningAnalysis;
  OurBinningAnalysis bina(4, 2, logger);

  logger.debug("basic()", "Starting to feed samples to the binning analysis object");

  bina.process_new_values(inline_vector(0.0, 0.0,   1.0, 1.0));
  bina.process_new_values(inline_vector(0.0, 0.0,   0.0, 2.0));
  bina.process_new_values(inline_vector(0.0, 0.0,   1.0, 3.0));
  bina.process_new_values(inline_vector(0.0, 0.0,   0.0, 4.0));
  bina.process_new_values(inline_vector(0.0, 100.0, 0.0, 5.0));
  bina.process_new_values(inline_vector(0.0, 100.0, 1.0, 6.0));
  bina.process_new_values(inline_vector(0.0, 100.0, 2.0, 7.0));
  bina.process_new_values(inline_vector(1.0, 100.0, 0.5, 8.0));
  // these will only partially fill the next bin. This will contribute to the *sum*, but
  // not the *sumsq* which is calculated during the flush:
  bina.process_new_values(inline_vector(.125, 0.0,   0.6875, 0.0));
  bina.process_new_values(inline_vector(.125, 100.0, 0.6875, 9.0));

  Eigen::Array<double,4,1> bin_sum;
  bin_sum = inline_vector(1.25, // (...)/10 == .125
                          5*100.0, // (...)/10 == 50
                          (1+1+1+2+0.5+2*0.6875), // (...)/10 == 0.6875
                          (8*(8+1)/2.0)+9.0); // (...)/10 == 4.5
  Eigen::Array<double,4,1> bin_means;
  bin_means = bin_sum / 10.0;
  
  Eigen::Array<double,4,3> bin_sumsq;
  bin_sumsq.col(0) = inline_vector(1, // (...)/8 == 0.125
                                   4*(100*100), // (...)/8 == 5000
                                   (1+1+1+4+0.25), // (...)/8 == 0.90625
                                   (1+4+9+16+25+36+49+64)); // (...)/8 == 25.5
  bin_sumsq.col(1) = inline_vector((0+0+0+0.5*0.5), // (...)/4 == 0.0625
                                   2*(100*100), // (...)/4 == 5000
                                   (0.5*0.5 + 0.5*0.5 + 0.5*0.5 + pow((2.0+0.5)/2,2)), // (...)/4 == 0.578125
                                   (1.5*1.5 + 3.5*3.5 + 5.5*5.5 + 7.5*7.5)); // (...)/4 == 25.25
  bin_sumsq.col(2) = inline_vector((0+0.25*0.25), // (...)/2 == 0.03125
                                   (100*100), // (...)/2 == 5000
                                   (0.5*0.5 + (3.5/4)*(3.5/4)), // (...)/2 == 0.5078125
                                   (2.5*2.5 + 6.5*6.5)); // (...)/2 == 24.25

  Eigen::Array<double,4,3> bin_sqmeans;
  bin_sqmeans.col(0) = bin_sumsq.col(0) / 8.0;
  bin_sqmeans.col(1) = bin_sumsq.col(1) / 4.0;
  bin_sqmeans.col(2) = bin_sumsq.col(2) / 2.0;

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
      str << "we should obtain: bin_sum = " << bin_sum.transpose() << "\n"
          << "\tbin_sqmeans = \n" << bin_sumsq << "\n"
          << "\tbin_means = " << bin_means.transpose() << "\n"
          << "\tand bin_sqmeans = \n" << bin_sqmeans << "\n";
    });

  logger.debug("test_mhrw_bin_err::binning_analysis::basic", [&](std::ostream & str) {
      str << "Debug: binning analysis uses powers_of_two matrix for normalization: \n"
	  << Tomographer::Tools::replicated<OurBinningAnalysis::NumTrackValuesCTime,1>(
	      Tomographer::Tools::powers_of_two<Eigen::Array<OurBinningAnalysis::ValueType,
							     OurBinningAnalysis::NumLevelsPlusOneCTime,
							     1> >(bina.num_levels()+1)
	      .transpose().reverse(),
	      // replicated by:
	      bina.num_track_values(), 1
	      ) << "\n";
    });

  BOOST_MESSAGE(logger.get_contents());
  BOOST_CHECK_EQUAL(bina.get_n_flushes(), 2);
  MY_BOOST_CHECK_EIGEN_EQUAL(bina.get_bin_sum(), bin_sum, tol);
  MY_BOOST_CHECK_EIGEN_EQUAL(bina.get_bin_sumsq(), bin_sumsq, tol);
  MY_BOOST_CHECK_EIGEN_EQUAL(bina.get_bin_means(), bin_means, tol);
  MY_BOOST_CHECK_EIGEN_EQUAL(bina.get_bin_sqmeans(), bin_sqmeans, tol);

  BOOST_CHECK_EQUAL(bina.num_track_values(), 4);
  BOOST_CHECK_EQUAL(bina.num_levels(), 2);

  Tomographer::BinningAnalysis<Tomographer::BinningAnalysisParams<double>,
                               Tomographer::Logger::BufferLogger>::BinSumSqArray
    error_levels_calc(4,3);
  error_levels_calc = bina.calc_error_levels();

  BOOST_MESSAGE("reported error_levels = \n" << error_levels_calc);

  BOOST_CHECK(OurBinningAnalysis::StoreBinSums) ;
  MY_BOOST_CHECK_EIGEN_EQUAL(error_levels_calc, error_levels, tol);
  MY_BOOST_CHECK_EIGEN_EQUAL(bina.calc_error_lastlevel(), error_levels.col(2), tol);
}


BOOST_AUTO_TEST_CASE(no_bin_means)
{
  Tomographer::Logger::BufferLogger logger(Tomographer::Logger::LONGDEBUG);
  typedef Tomographer::BinningAnalysis<Tomographer::BinningAnalysisParams<double, 4, 1, false /*StoreBinSums_*/>,
                                       Tomographer::Logger::BufferLogger>
    OurBinningAnalysis;
  OurBinningAnalysis bina(4, 1, logger);

  bina.process_new_values(inline_vector(0.0, 0.0, 1.0, 0.0));
  bina.process_new_values(inline_vector(0.0, 0.0, 1.0, 1.0));
  bina.process_new_values(inline_vector(0.0, 1.0, 1.0, 2.0));
  bina.process_new_values(inline_vector(0.0, 0.0, 1.0, 3.0));

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

  Eigen::Array<double,4,2> error_levels;
  error_levels = (bin_sqmeans.array() - (means.cwiseProduct(means)).replicate<1,2>()).cwiseSqrt();
  // divide by sqrt(num-samples)
  error_levels.col(0) /= std::sqrt( bina.get_n_flushes() * 2.0 - 1);
  error_levels.col(1) /= std::sqrt( bina.get_n_flushes() * 1.0 - 1);

  BOOST_CHECK(! OurBinningAnalysis::StoreBinSums) ;
  MY_BOOST_CHECK_EIGEN_EQUAL(bina.calc_error_levels(means), error_levels, tol);
  MY_BOOST_CHECK_EIGEN_EQUAL(bina.calc_error_lastlevel(means), error_levels.col(1), tol);
}

BOOST_AUTO_TEST_SUITE_END()


// =============================================================================


BOOST_AUTO_TEST_SUITE(valuehistogramwithbinning)

BOOST_AUTO_TEST_CASE(simple1)
{
  //<<<<<<<<<<<
  typedef Tomographer::Logger::BufferLogger LoggerType;
  LoggerType buflog(Tomographer::Logger::DEBUG);
  //-----------
  // in this case, comment out BOOST_MESSAGE(... .get_contents()) below
  //  typedef Tomographer::Logger::FileLogger LoggerType;
  //  LoggerType buflog(stderr, Tomographer::Logger::LONGDEBUG) ;
  //>>>>>>>>>>>


  typedef Tomographer::ValueHistogramWithBinningMHRWStatsCollector<
    Tomographer::ValueHistogramWithBinningMHRWStatsCollectorParams<test_norm_value_calculator,
                                                                   int,
                                                                   float>,
    LoggerType
    > ValWBinningMHRWStatsCollectorType;

  typedef ValWBinningMHRWStatsCollectorType::HistogramParams HistogramParams;
  typedef test_hypercube_mhwalker<double, 3 /* Dim */> MHWalkerType;
  typedef Tomographer::MHRandomWalk<std::mt19937, MHWalkerType, ValWBinningMHRWStatsCollectorType,
				    LoggerType, int> MHRandomWalkType;

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

  // Todo: Fixme: Why are the error bar curves going like crazy?
  // ----> ok if we have enough samples it seems.

  // TODO: CHECK THAT THE VALUES ARE CORRECT, ALONG WITH THE ERRORS.

  MY_BOOST_CHECK_EIGEN_EQUAL(result.converged_status,
                             Eigen::ArrayXi::Constant(
                                 vhist.get_binning_analysis().num_track_values(),
                                 ValWBinningMHRWStatsCollectorType::BinningAnalysisType::CONVERGED
                                 ),
                             tol) ;

}



BOOST_AUTO_TEST_SUITE_END()

// =============================================================================

BOOST_AUTO_TEST_SUITE_END()
