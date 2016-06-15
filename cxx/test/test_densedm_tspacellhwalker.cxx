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
#include <sstream>
#include <random>

#include <boost/math/constants/constants.hpp>

// include before <Eigen/*> !
#include "test_tomographer.h"

#include <tomographer2/densedm/tspacellhwalker.h>

#include <tomographer2/densedm/densellh.h>
#include <tomographer2/densedm/indepmeasllh.h>
#include <tomographer2/densedm/tspacefigofmerit.h>
#include <tomographer2/mhrw.h>
#include <tomographer2/mhrwstatscollectors.h>


// -----------------------------------------------------------------------------
// fixture(s)


// -----------------------------------------------------------------------------
// test suites


BOOST_AUTO_TEST_SUITE(test_densedm_dmmhrw)

BOOST_AUTO_TEST_CASE(tspacellhmhwalker)
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
  Nx << 1500, 800, 300, 300, 10, 30;

  llh.setMeas(Exn, Nx, false);

  typedef Tomographer::Logger::BufferLogger LoggerType;
  LoggerType buflog(Tomographer::Logger::DEBUG);

  std::mt19937 rng(0); // seeded rng, deterministic results
  
  Tomographer::DenseDM::TSpace::LLHMHWalker<DenseLLH, std::mt19937, LoggerType>
    dmmhrw(DMTypes::MatrixType::Zero(), llh, rng, buflog);

  DMTypes::VectorParamType x(dmt.initVectorParamType());
  x << 0.5, 0.5, 0, 0; // maximally mixed state.
  DMTypes::MatrixType T(dmt.initMatrixType());
  T << boost::math::constants::half_root_two<double>(), 0,
       0, boost::math::constants::half_root_two<double>() ; // maximally mixed state.

  dmmhrw.init();
  dmmhrw.thermalizing_done();

  const DMTypes::MatrixType Tconst(T); // make sure that fnlogval() accepts const argument
  BOOST_CHECK_CLOSE(dmmhrw.fnlogval(Tconst), llh.logLikelihoodX(x), tol_percent);

  DMTypes::MatrixType newT = dmmhrw.jump_fn(T, 0.2);
  BOOST_CHECK_CLOSE(newT.norm(), 1.0, tol_percent);

  // not sure how to check that the jump distribution is symmetric ???
  // ..........

  dmmhrw.done();
  BOOST_MESSAGE(buflog.get_contents());
}



// ===============================================

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

  typedef Tomographer::Logger::BufferLogger LoggerType;
  LoggerType flog(Tomographer::Logger::DEBUG);

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
  ref_T << 1, 0,
    /* */  0, 0;

  OurValueCalculator fidcalc(ref_T);
  OurValMHRWStatsCollector fidstats(OurHistogramType::Params(0.98, 1.0, 50), fidcalc, flog);
  OurValMHRWStatsCollector fidstats2(OurHistogramType::Params(0.96, 0.98, 10), fidcalc, flog);

  OurMultiMHRWStatsCollector multistats(fidstats, fidstats2);

  typedef Tomographer::DenseDM::TSpace::LLHMHWalker<DenseLLH,std::mt19937,LoggerType>
    MyMHWalker;

  MyMHWalker mhwalker(start_T, llh, rng, flog);

  //  std::cout << "About to create the randomwalk object ...\n";
  Tomographer::MHRandomWalk<std::mt19937,MyMHWalker,OurMultiMHRWStatsCollector,LoggerType,long>
    rwalk(20, 0.05, 300, 5000, mhwalker, multistats, rng, flog);

  //  std::cout << "About to run the randomwalk object ...\n";

  rwalk.run();

  BOOST_MESSAGE(flog.get_contents()) ;

  // because we used a seeded RNG, we should get exactly reproducible results, i.e. the
  // exact same histograms.

  const auto & hist1 = fidstats.histogram();
  BOOST_MESSAGE("FINAL HISTOGRAM(1):\n" << hist1.pretty_print(100));

  boost::test_tools::output_test_stream output1(
      TOMOGRAPHER_TEST_PATTERNS_DIR "test_densedm_tspacellhwalker/hist1.txt",
      true // true = match mode, false = write mode
      );
  dump_histogram_test(output1, hist1);
  BOOST_CHECK(output1.match_pattern());

  const auto & hist2 = fidstats2.histogram();
  BOOST_MESSAGE("FINAL HISTOGRAM(2):\n" << hist2.pretty_print(100));

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

  typedef Tomographer::Logger::BufferLogger LoggerType;
  LoggerType buflog(Tomographer::Logger::DEBUG);

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

  ValWBinningMHRWStatsCollectorType vhist(HistogramParams(0.98f, 1.0f, 20), fidcalc, num_levels, buflog);

  std::mt19937 rng(0); // seeded rng, deterministic results

  DMTypes::MatrixType start_T = dmt.initMatrixType();
  start_T << 1.0/sqrt(2.0), 0, 0, 1.0/sqrt(2.0);

  MHWalkerType mhwalker(start_T, llh, rng, buflog);

  //  std::cout << "About to create the randomwalk object ...\n";
  Tomographer::MHRandomWalk<std::mt19937,MHWalkerType,ValWBinningMHRWStatsCollectorType,LoggerType,unsigned long>
    rwalk(20, 0.05, 300, 8192, mhwalker, vhist, rng, buflog);

  rwalk.run();

  BOOST_MESSAGE(buflog.get_contents());

  const ValWBinningMHRWStatsCollectorType::Result & result = vhist.getResult();

  // all error bars should have converged with these settings
  MY_BOOST_CHECK_EIGEN_EQUAL(
      result.converged_status,
      Eigen::ArrayXi::Constant(result.hist.num_bins(),
			       ValWBinningMHRWStatsCollectorType::BinningAnalysisParamsType::CONVERGED),
      tol_f
      );

  std::string conv_analysis = result.dump_convergence_analysis();
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



BOOST_AUTO_TEST_SUITE_END()

