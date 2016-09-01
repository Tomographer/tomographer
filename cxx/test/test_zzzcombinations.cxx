/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
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

#include <cmath>

#include <string>
#include <iostream>
#include <random>

#include <boost/math/constants/constants.hpp>

// definitions for Tomographer test framework -- this must be included before any
// <Eigen/...> or <tomographer2/...> header
#include "test_tomographer.h"

#include <tomographer2/tools/loggers.h>
#include <tomographer2/mhrw.h>
#include <tomographer2/mhrw_bin_err.h>
#include <tomographer2/mhrwstatscollectors.h>
#include <tomographer2/densedm/densellh.h>
#include <tomographer2/densedm/indepmeasllh.h>
#include <tomographer2/densedm/tspacefigofmerit.h>
#include <tomographer2/densedm/tspacellhwalker.h>

#include "boost_test_logger.h"



// -----------------------------------------------------------------------------
// fixture(s)


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
  typedef RealScalar_ StepRealType;
  typedef Eigen::Matrix<RealScalar, Dim, 1> PointType;

  typedef float FnValueType;

  enum { UseFnSyntaxType = Tomographer::MHUseFnValue };

  // random number generator, etc.
  std::mt19937 rng;
  std::uniform_real_distribution<double> dist;

  // constructor
  test_hypercube_mhwalker() : rng(0), dist(-1.0, 1.0) { }

  inline void init() { }

  auto startPoint() -> decltype(PointType::Zero()) { return PointType::Zero(); }

  inline void thermalizingDone() { }
  inline void done() { }
  inline PointType jumpFn(const PointType & curpt, RealScalar step_size)
  {
    PointType newpt;
    newpt = curpt + step_size*Tomographer::Tools::denseRandom<PointType>(rng, dist);
    // do the random walk on the torus, i.e. modulo 1.0
    for (int i = 0; i < Dim; ++i) {
      if (newpt(i) < 0) { newpt(i) += 1.0; }
      if (newpt(i) > 1) { newpt(i) -= 1.0; }
    }
    return newpt;
  }

  inline FnValueType fnVal(const PointType & )
  {
    return 1.0;
  }
};



// -----------------------------------------------------------------------------
// test suites


BOOST_AUTO_TEST_SUITE(test_zzzcombinations)

// =============================================================================

BOOST_AUTO_TEST_SUITE(valuehistogramwithbinning)

BOOST_AUTO_TEST_CASE(simple1)
{
  //<<<<<<<<<<<
  typedef BoostTestLogger LoggerType;
  LoggerType buflog(Tomographer::Logger::DEBUG);
  //-----------
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
  typedef test_hypercube_mhwalker<
    double,
    3 // Dim
    > MHWalkerType;
  typedef Tomographer::MHRandomWalk<std::mt19937, MHWalkerType, ValWBinningMHRWStatsCollectorType,
				    LoggerType, int> MHRandomWalkType;

  test_norm_value_calculator vcalc;

  // N levels -> samples_size = 2^N,  2^10 == 1024
  const int num_levels = 10;
  //const int num_levels = 4;
  ValWBinningMHRWStatsCollectorType vhist(HistogramParams(0.f, 2.0f, 20), vcalc, num_levels, buflog);

  std::mt19937 rng(0); // seeded rng, deterministic results

  MHWalkerType mhwalker;
  MHRandomWalkType rwalk(0.03, 5, 50, 500000, mhwalker, vhist, rng, buflog);

  rwalk.run();

  const ValWBinningMHRWStatsCollectorType::ResultType & result = vhist.getResult();

  // Todo: Fixme: Why are the error bar curves going like crazy?
  // ----> ok if we have enough samples it seems.

  // TODO: CHECK THAT THE VALUES ARE CORRECT, ALONG WITH THE ERRORS.

  MY_BOOST_CHECK_EIGEN_EQUAL(result.converged_status,
                             Eigen::ArrayXi::Constant(
                                 vhist.getBinningAnalysis().numTrackValues(),
                                 ValWBinningMHRWStatsCollectorType::BinningAnalysisType::CONVERGED
                                 ),
                             tol) ;

}

BOOST_AUTO_TEST_SUITE_END()


// =============================================================================


BOOST_AUTO_TEST_SUITE(integrator_tests)

BOOST_AUTO_TEST_CASE(basic1)
{
  typedef Tomographer::DenseDM::DMTypes<2> DMTypes;
  DMTypes dmt;
  
  typedef Tomographer::DenseDM::IndepMeasLLH<DMTypes> DenseLLH;
  DenseLLH llh(dmt);

  DenseLLH::VectorParamListType Exn(6, dmt.dim2());
  Exn <<
    0.5, 0.5,  0.707107,  0,
    0.5, 0.5, -0.707107,  0,
    0.5, 0.5,  0,         0.707107,
    0.5, 0.5,  0,        -0.707107,
    1,   0,    0,         0,
    0,   1,    0,         0
    ;
  DenseLLH::FreqListType Nx(6);
  Nx << 0, 0, 0, 0, 250, 0;

  llh.setMeas(Exn, Nx, false);

  // now, prepare the integrator.
  std::mt19937 rng(0); // seeded random number generator

  typedef BoostTestLogger LoggerType;
  LoggerType logger(Tomographer::Logger::DEBUG);

  DMTypes::MatrixType start_T(dmt.initMatrixType());
  start_T << 1.0/sqrt(2.0), 0,
              0, 1.0/sqrt(2.0);

  typedef Tomographer::DenseDM::TSpace::FidelityToRefCalculator<DMTypes> OurValueCalculator;
  typedef Tomographer::UniformBinsHistogram<typename OurValueCalculator::ValueType> OurHistogramType;
  typedef Tomographer::ValueHistogramMHRWStatsCollector<
    OurValueCalculator,
    LoggerType,
    OurHistogramType
    >
    OurValMHRWStatsCollector;
  typedef Tomographer::MultipleMHRWStatsCollectors<
    OurValMHRWStatsCollector,
    OurValMHRWStatsCollector
    >
    OurMultiMHRWStatsCollector;

  DMTypes::MatrixType ref_T(dmt.initMatrixType());
  ref_T <<
    1, 0,
    0, 0;

  OurValueCalculator fidcalc(ref_T);
  OurValMHRWStatsCollector fidstats(OurHistogramType::Params(0.98, 1.0, 50), fidcalc, logger);
  OurValMHRWStatsCollector fidstats2(OurHistogramType::Params(0.96, 0.98, 10), fidcalc, logger);

  OurMultiMHRWStatsCollector multistats(fidstats, fidstats2);

  typedef Tomographer::DenseDM::TSpace::LLHMHWalker<DenseLLH,std::mt19937,LoggerType>
    MyMHWalker;

  MyMHWalker mhwalker(start_T, llh, rng, logger);

  //  std::cout << "About to create the randomwalk object ...\n";
  Tomographer::MHRandomWalk<std::mt19937,MyMHWalker,OurMultiMHRWStatsCollector,LoggerType,long>
    rwalk(0.05, 20, 300, 5000, mhwalker, multistats, rng, logger);

  //  std::cout << "About to run the randomwalk object ...\n";

  rwalk.run();

  // because we used a seeded RNG, we should get exactly reproducible results, i.e. the
  // exact same histograms.

  const auto & hist1 = fidstats.histogram();
  BOOST_MESSAGE("FINAL HISTOGRAM(1):\n" << hist1.prettyPrint(100));

  boost::test_tools::output_test_stream output1(
      TOMOGRAPHER_TEST_PATTERNS_DIR "test_densedm_tspacellhwalker/hist1.txt",
      true // true = match mode, false = write mode
      );
  dump_histogram_test(output1, hist1);
  BOOST_CHECK(output1.match_pattern());

  const auto & hist2 = fidstats2.histogram();
  BOOST_MESSAGE("FINAL HISTOGRAM(2):\n" << hist2.prettyPrint(100));

  boost::test_tools::output_test_stream output2(
      TOMOGRAPHER_TEST_PATTERNS_DIR "test_densedm_tspacellhwalker/hist2.txt",
      true // true = match mode, false = write mode
      );
  dump_histogram_test(output2, hist2);
  BOOST_CHECK(output2.match_pattern());
}


BOOST_AUTO_TEST_CASE(with_binning_analysis)
{
  typedef Tomographer::DenseDM::DMTypes<2> DMTypes;
  DMTypes dmt;
  typedef Tomographer::DenseDM::IndepMeasLLH<DMTypes> DenseLLH;
  DenseLLH llh(dmt);

  DenseLLH::VectorParamListType Exn(2, dmt.dim2());
  Exn <<
    1,   0,    0,         0,
    0,   1,    0,         0
    ;
  DenseLLH::FreqListType Nx(2);
  Nx << 250, 0;

  llh.setMeas(Exn, Nx);

  // --------

  typedef BoostTestLogger LoggerType;
  LoggerType logger(Tomographer::Logger::DEBUG);

  typedef Tomographer::DenseDM::TSpace::FidelityToRefCalculator<DMTypes> OurValueCalculator;
  typedef Tomographer::ValueHistogramWithBinningMHRWStatsCollector<
    Tomographer::ValueHistogramWithBinningMHRWStatsCollectorParams<OurValueCalculator,
                                                                   int,
                                                                   float>,
    LoggerType
    > ValWBinningMHRWStatsCollectorType;

  typedef ValWBinningMHRWStatsCollectorType::HistogramParams HistogramParams;
  typedef Tomographer::DenseDM::TSpace::LLHMHWalker<DenseLLH,std::mt19937,LoggerType> MHWalkerType;

  DMTypes::MatrixType ref_T(dmt.initMatrixType());
  ref_T << 1, 0, 0, 0;
  OurValueCalculator fidcalc(ref_T);

  // N levels -> samples_size = 2^N
  const int num_levels = 5;

  ValWBinningMHRWStatsCollectorType vhist(HistogramParams(0.98f, 1.0f, 20), fidcalc, num_levels, logger);

  std::mt19937 rng(0); // seeded rng, deterministic results

  DMTypes::MatrixType start_T = dmt.initMatrixType();
  start_T << 1.0/sqrt(2.0), 0, 0, 1.0/sqrt(2.0);

  MHWalkerType mhwalker(start_T, llh, rng, logger);

  //  std::cout << "About to create the randomwalk object ...\n";
  Tomographer::MHRandomWalk<std::mt19937,MHWalkerType,ValWBinningMHRWStatsCollectorType,LoggerType,unsigned long>
    rwalk(0.05, 20, 300, 8192, mhwalker, vhist, rng, logger);

  rwalk.run();

  const ValWBinningMHRWStatsCollectorType::ResultType & result = vhist.getResult();

  // all error bars should have converged with these settings
  MY_BOOST_CHECK_EIGEN_EQUAL(
      result.converged_status,
      Eigen::ArrayXi::Constant(result.hist.numBins(),
			       ValWBinningMHRWStatsCollectorType::BinningAnalysisParamsType::CONVERGED),
      tol_f
      );

  std::string conv_analysis = result.dumpConvergenceAnalysis();
  BOOST_MESSAGE("Convergence Analysis:\n" << conv_analysis);

  boost::test_tools::output_test_stream output_conv_analysis(
      TOMOGRAPHER_TEST_PATTERNS_DIR "test_densedm_tspacellhwalker/binning_convergence_analysis.txt",
      true // true = compare mode, false = write mode
      );
  output_conv_analysis << conv_analysis;
  BOOST_CHECK(output_conv_analysis.match_pattern());

  boost::test_tools::output_test_stream output_error_bars(
      TOMOGRAPHER_TEST_PATTERNS_DIR "test_densedm_tspacellhwalker/binning_error_bars.txt",
      true // true = compare mode, false = write mode
      );
  output_error_bars
    << "--------------------------------------------------------------------------------\n";
  dump_histogram_test(output_error_bars, result.hist, 2);
  output_error_bars
    << "--------------------------------------------------------------------------------\n"
    << "ERROR BARS @ BINNING LEVELS = \n"
    << std::setprecision(2) << std::scientific << std::left << std::setfill('0')
    << result.error_levels
    << "\n";
  BOOST_CHECK(output_error_bars.match_pattern());
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================

BOOST_AUTO_TEST_SUITE_END()

