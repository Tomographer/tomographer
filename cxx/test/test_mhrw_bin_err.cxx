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
// <Eigen/...> or <tomographer2/...> header
#include "test_tomographer.h"

#include <tomographer2/mhrw_bin_err.h>
#include <tomographer2/mhrwstatscollectors.h>
#include <tomographer2/tools/loggers.h>
#include <tomographer2/mhrw.h>

#include <tomographer2/tools/boost_test_logger.h>


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
  BoostTestLogger logger(Tomographer::Logger::LONGDEBUG);
  typedef Tomographer::BinningAnalysis<Tomographer::BinningAnalysisParams<double>,
                                       BoostTestLogger> OurBinningAnalysis;
  OurBinningAnalysis bina(4, 2, logger);

  logger.debug("basic()", "Starting to feed samples to the binning analysis object");

  bina.processNewValues(inline_vector(0.0, 0.0,   1.0, 1.0));
  bina.processNewValues(inline_vector(0.0, 0.0,   0.0, 2.0));
  bina.processNewValues(inline_vector(0.0, 0.0,   1.0, 3.0));
  bina.processNewValues(inline_vector(0.0, 0.0,   0.0, 4.0));
  bina.processNewValues(inline_vector(0.0, 100.0, 0.0, 5.0));
  bina.processNewValues(inline_vector(0.0, 100.0, 1.0, 6.0));
  bina.processNewValues(inline_vector(0.0, 100.0, 2.0, 7.0));
  bina.processNewValues(inline_vector(1.0, 100.0, 0.5, 8.0));
  // these will only partially fill the next bin. This will contribute to the *sum*, but
  // not the *sumsq* which is calculated during the flush:
  bina.processNewValues(inline_vector(.125, 0.0,   0.6875, 0.0));
  bina.processNewValues(inline_vector(.125, 100.0, 0.6875, 9.0));

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
      str << "Debug: binning analysis uses powersOfTwo matrix for normalization: \n"
	  << Tomographer::Tools::replicated<OurBinningAnalysis::NumTrackValuesCTime,1>(
	      Tomographer::Tools::powersOfTwo<Eigen::Array<OurBinningAnalysis::ValueType,
                                                           OurBinningAnalysis::NumLevelsPlusOneCTime,
							   1> >(bina.numLevels()+1)
	      .transpose().reverse(),
	      // replicated by:
	      bina.numTrackValues(), 1
	      ) << "\n";
    });

  BOOST_CHECK_EQUAL(bina.getNumFlushes(), 2);
  MY_BOOST_CHECK_EIGEN_EQUAL(bina.getBinSum(), bin_sum, tol);
  MY_BOOST_CHECK_EIGEN_EQUAL(bina.getBinSumsq(), bin_sumsq, tol);
  MY_BOOST_CHECK_EIGEN_EQUAL(bina.getBinMeans(), bin_means, tol);
  MY_BOOST_CHECK_EIGEN_EQUAL(bina.getBinSqmeans(), bin_sqmeans, tol);

  BOOST_CHECK_EQUAL(bina.numTrackValues(), 4);
  BOOST_CHECK_EQUAL(bina.numLevels(), 2);

  Tomographer::BinningAnalysis<Tomographer::BinningAnalysisParams<double>,
                               BoostTestLogger>::BinSumSqArray
    error_levels_calc(4,3);
  error_levels_calc = bina.calcErrorLevels();

  BOOST_MESSAGE("reported error_levels = \n" << error_levels_calc);

  BOOST_CHECK(OurBinningAnalysis::StoreBinSums) ;
  MY_BOOST_CHECK_EIGEN_EQUAL(error_levels_calc, error_levels, tol);
  MY_BOOST_CHECK_EIGEN_EQUAL(bina.calcErrorLastLevel(), error_levels.col(2), tol);
}


BOOST_AUTO_TEST_CASE(no_bin_means)
{
  BoostTestLogger logger(Tomographer::Logger::LONGDEBUG);
  typedef Tomographer::BinningAnalysis<Tomographer::BinningAnalysisParams<double, 4, 1, false /*StoreBinSums_*/>,
                                       BoostTestLogger>
    OurBinningAnalysis;
  OurBinningAnalysis bina(4, 1, logger);

  bina.processNewValues(inline_vector(0.0, 0.0, 1.0, 0.0));
  bina.processNewValues(inline_vector(0.0, 0.0, 1.0, 1.0));
  bina.processNewValues(inline_vector(0.0, 1.0, 1.0, 2.0));
  bina.processNewValues(inline_vector(0.0, 0.0, 1.0, 3.0));

  BOOST_CHECK_EQUAL(bina.getNumFlushes(), 2);

  Eigen::Matrix<double, 4, 2> bin_sqmeans;
  bin_sqmeans <<
    0, 0,
    0.25, 0.125,
    1, 1,
    (1*1+2*2+3*3)/4.0, (0.5*0.5 + 2.5*2.5)/2.0
    ;

  MY_BOOST_CHECK_EIGEN_EQUAL(bina.getBinSqmeans(), bin_sqmeans, tol);

  Eigen::Array<double,4,1> means;
  means << 0, 0.25, 1, (3*4/2)/4.0;

  Eigen::Array<double,4,2> error_levels;
  error_levels = (bin_sqmeans.array() - (means.cwiseProduct(means)).replicate<1,2>()).cwiseSqrt();
  // divide by sqrt(num-samples)
  error_levels.col(0) /= std::sqrt( bina.getNumFlushes() * 2.0 - 1);
  error_levels.col(1) /= std::sqrt( bina.getNumFlushes() * 1.0 - 1);

  BOOST_CHECK(! OurBinningAnalysis::StoreBinSums) ;
  MY_BOOST_CHECK_EIGEN_EQUAL(bina.calcErrorLevels(means), error_levels, tol);
  MY_BOOST_CHECK_EIGEN_EQUAL(bina.calcErrorLastLevel(means), error_levels.col(1), tol);
}

BOOST_AUTO_TEST_SUITE_END()


// =============================================================================

BOOST_AUTO_TEST_SUITE_END()
