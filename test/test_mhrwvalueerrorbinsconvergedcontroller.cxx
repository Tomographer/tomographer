/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
 * Copyright (c) 2017 Caltech, Institute for Quantum Information and Matter, Philippe Faist
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
// <Eigen/...> or <tomographer/...> header
#include "test_tomographer.h"

#include <tomographer/mhrwvalueerrorbinsconvergedcontroller.h>
#include <tomographer/tools/boost_test_logger.h>


// -----------------------------------------------------------------------------
// fixture(s)


// fixtures:

struct TestStatsCollectorFixture2
{
  typedef int PointType;

  struct DummyMHRW { int x; };
  DummyMHRW mhrw;

  const Eigen::ArrayXi pt_seq;

  int last_pt;
  int iter_k;
  int coll_samples;

  //
  // The point sequence has been tuned to get just the right properties. DO NOT CHANGE IT!
  //

  TestStatsCollectorFixture2()
    : mhrw(),
      pt_seq((Eigen::ArrayXi(64*2)
              // The test cases depends on this exact point sequences DO NOT CHANGE
              << 0, 1, 2, 1, 3, 4, 5, 4, 5, 3, 7, 9, 8, 7, 5, 4,
              3, 1, 2, 1, 3, 4, 5, 4, 5, 6, 6, 9, 8, 7, 5, 4, // ... 32
              0, 1, 2, 1, 3, 4, 5, 4, 5, 6, 7, 9, 8, 7, 5, 4,
              1, 3, 4, 5, 4, 5, 6, 7, 9, 8, 0, 5, 4, 3, 1, 2, // ... 64
              0, 1, 2, 3, 3, 8, 7, 5, 4, 4, 5, 4, 5, 6, 7, 9,
              3, 1, 2, 1, 3, 4, 0, 4, 5, 5, 7, 8, 8, 7, 5, 4,
              0, 1, 2, 3, 4, 3, 3, 4, 5, 6, 7, 9, 8, 7, 5, 4,
              3, 1, 2, 3, 3, 4, 4, 4, 5, 3, 4, 9, 3, 3, 3, 3
                 ).finished()),
      last_pt(0),
      iter_k(0),
      coll_samples(0)
  {
  }

  template<typename StatsColl>
  void run_dummy_rw_init(StatsColl & statcoll) {
  
    statcoll.init();
    statcoll.rawMove(0, true, false, true, 0.95,
                     PointType(1), 123.4, PointType(0), 123.4, mhrw);
    statcoll.rawMove(1, true, false, true, 1.0,
                     PointType(2), 123.4, PointType(1), 123.4, mhrw);
    statcoll.rawMove(2, true, false, true, 1.0,
                     PointType(0), 123.4, PointType(2), 123.4, mhrw);
    statcoll.rawMove(3, true, false, true, 1.0,
                     PointType(0), 123.4, PointType(2), 123.4, mhrw);

    statcoll.thermalizingDone();

    iter_k = 0;
    last_pt = 0;
    coll_samples = 0;

  }

  //
  // Go through the point sequence once.
  //
  // `step` is the step with which to stride through the point sequence,
  // effectively generating a permutation of the points.
  //
  // WARNING: To this effect we must choose odd `step` in order to be prime with
  //          `pt_seq.size`!
  //
  // WARNING: We have assumed that n_sweep=2 !!
  //
  template<typename StatsColl>
  void run_dummy_rw_runs(StatsColl & statcoll, int step = 1)
  {
    run_dummy_sweeps(statcoll, -1, step);
  }

  //
  // Same as run_dummy_rw_runs(), but stop after exactly num_sweeps
  // sweeps. Sweep size is hard-coded at value == 2.
  //
  template<typename StatsColl>
  void run_dummy_sweeps(StatsColl & statcoll, int num_sweeps, int step = 7)
  {
    for (int k = 0; k < pt_seq.size(); ++k) {
      if (num_sweeps > 0 && k/2 >= num_sweeps) {
        return;
      }
      ++iter_k;
      const auto idx = (k*step)% pt_seq.size();
      statcoll.rawMove(iter_k, false, (iter_k%2==0), true, 1.0,
                       pt_seq(idx), 123.4, last_pt, 123.4, mhrw);
      if (k%2==0) {
        statcoll.processSample(iter_k, iter_k/2-1, pt_seq(idx), 123.4, mhrw); // takes sample here
        ++coll_samples;
      }
      last_pt = pt_seq(idx);
    }
  }
};


struct MeeselfValueCalculator {
  typedef double ValueType;
  MeeselfValueCalculator() { }
  double getValue(int pt) const {
    return (double)pt;
  };
};




// -----------------------------------------------------------------------------
// test suites


BOOST_AUTO_TEST_SUITE(test_mhrwvalueerrorbinsconvergedcontroller)

BOOST_FIXTURE_TEST_CASE(keepsrunning, TestStatsCollectorFixture2)
{
  MeeselfValueCalculator valcalc;
  Tomographer::Logger::BoostTestLogger logger;
  typedef Tomographer::ValueHistogramWithBinningMHRWStatsCollectorParams<MeeselfValueCalculator> VHWBParams;
  typedef Tomographer::ValueHistogramWithBinningMHRWStatsCollector<VHWBParams, Tomographer::Logger::BoostTestLogger>
    MyStatsCollector;

  const int bin_num_levels = 4;

  MyStatsCollector statcoll(MyStatsCollector::HistogramParams(0,10,10),
                            valcalc,
                            bin_num_levels,
                            logger);

  struct DummyMHWalker {};
  DummyMHWalker dmhwalker;
  
  Tomographer::MHRWParams<Tomographer::MHWalkerParamsStepSize<double>,int> p(0.1, 2, 2, 128/2) ;

  typedef Tomographer::MHRWValueErrorBinsConvergedController<MyStatsCollector,int,
                                                             Tomographer::Logger::BoostTestLogger>
    CtrlType;
  CtrlType ctrl(statcoll, logger, 1 /*check_frequency_sweeps*/, 0, 0, 0, -1); // don't stop prematurely

  BOOST_CHECK_EQUAL( (unsigned int) CtrlType::AdjustmentStrategy,
                     (unsigned int) Tomographer::MHRWControllerDoNotAdjust ) ;

  ctrl.init(p, dmhwalker, mhrw);

  run_dummy_rw_init(statcoll);

  BOOST_CHECK( ctrl.allowDoneThermalization(p, dmhwalker, 4, mhrw) ) ; // should never return false
  ctrl.thermalizingDone(p, dmhwalker, mhrw);

  run_dummy_rw_runs(statcoll, 1); // runs

  // the bins should not have converged -- we chose the point sequence for this to happen
  { auto binmeans = statcoll.binMeans();
    auto errlvls = statcoll.getBinningAnalysis().calcErrorLevels(binmeans);
    auto conv_st = statcoll.getBinningAnalysis().determineErrorConvergence(errlvls);
    auto summary = Tomographer::BinningErrorBarConvergenceSummary::fromConvergedStatus(conv_st);
    logger.debug("keepsrunning test case", [&](std::ostream & stream) {
        stream << "Bins convergence : " << summary << "\n";
        stream << "# samples @ last level: " << coll_samples / (1<<bin_num_levels) ;
      });
  
    BOOST_CHECK_EQUAL(summary.n_bins, 10);
    BOOST_CHECK_EQUAL(summary.n_converged, 8);
    BOOST_CHECK_EQUAL(summary.n_unknown, 0);
    BOOST_CHECK_EQUAL(summary.n_unknown_isolated, 0);
    BOOST_CHECK_EQUAL(summary.n_not_converged, 2);
  }

  // our point sequence is designed so that some bins have not converged -- so the
  // controller should prevent the rw from stopping
  BOOST_CHECK( ! ctrl.allowDoneRuns(p, dmhwalker, iter_k, mhrw) ) ;

  run_dummy_rw_runs(statcoll, 17); // more runs -- makes error bars converge

  // now the bins should have converged, so allow rw to end
  BOOST_CHECK( ctrl.allowDoneRuns(p, dmhwalker, iter_k, mhrw) ) ;

  statcoll.done();
  ctrl.done(p, dmhwalker, mhrw);
  
  auto result = statcoll.getResult();

  auto summary = result.errorBarConvergenceSummary();

  BOOST_CHECK_EQUAL(summary.n_bins, 10);
  BOOST_CHECK_EQUAL(summary.n_converged, 10);
  BOOST_CHECK_EQUAL(summary.n_unknown, 0);
  BOOST_CHECK_EQUAL(summary.n_unknown_isolated, 0);
  BOOST_CHECK_EQUAL(summary.n_not_converged, 0);

  logger.debug("keepsrunning test case", [&](std::ostream & stream) {
      result.dumpConvergenceAnalysis(stream);
    }) ;
}

BOOST_FIXTURE_TEST_CASE(stops_prematurely_for_long_runs, TestStatsCollectorFixture2)
{
  MeeselfValueCalculator valcalc;
  Tomographer::Logger::BoostTestLogger baselogger(Tomographer::Logger::LONGDEBUG);
  auto logger = Tomographer::Logger::makeLocalLogger(TOMO_ORIGIN, baselogger) ;
  typedef Tomographer::ValueHistogramWithBinningMHRWStatsCollectorParams<MeeselfValueCalculator> VHWBParams;
  typedef Tomographer::ValueHistogramWithBinningMHRWStatsCollector<VHWBParams, Tomographer::Logger::BoostTestLogger>
    MyStatsCollector;

  const int bin_num_levels = 4;

  MyStatsCollector statcoll(MyStatsCollector::HistogramParams(0,10,10),
                            valcalc,
                            bin_num_levels,
                            logger.parentLogger());

  struct DummyMHWalker {};
  DummyMHWalker dmhwalker;
  
  Tomographer::MHRWParams<Tomographer::MHWalkerParamsStepSize<double>,int> p(0.1, 2, 2, 128/2) ;

  typedef Tomographer::MHRWValueErrorBinsConvergedController<MyStatsCollector,int,
                                                             Tomographer::Logger::BoostTestLogger>
    CtrlType;
  CtrlType ctrl(statcoll, logger.parentLogger(), 1/*check_frequency_sweeps*/,
                0, 0, 0, // require all converged exactly
                1.2); // don't go more that 1.2x number of runs

  ctrl.init(p, dmhwalker, mhrw);

  run_dummy_rw_init(statcoll);

  BOOST_CHECK( ctrl.allowDoneThermalization(p, dmhwalker, 4, mhrw) ) ; // should never return false
  ctrl.thermalizingDone(p, dmhwalker, mhrw);

  logger.longdebug("iter_k=%d", (int)iter_k) ;

  run_dummy_rw_runs(statcoll, 5); // set of runs -- not enough to make bars converge, not
                                  // yet enough to exceed max_add_run_iters

  // the bins should not have converged
  { auto binmeans = statcoll.binMeans();
    auto errlvls = statcoll.getBinningAnalysis().calcErrorLevels(binmeans);
    auto conv_st = statcoll.getBinningAnalysis().determineErrorConvergence(errlvls);
    auto summary = Tomographer::BinningErrorBarConvergenceSummary::fromConvergedStatus(conv_st);
    logger.debug([&](std::ostream & stream) {
        stream << "Bins convergence : " << summary << "\n";
        stream << "# samples @ last level: " << coll_samples / (1<<bin_num_levels) ;
      });
  
    BOOST_CHECK_EQUAL(summary.n_bins, 10);
    BOOST_CHECK_EQUAL(summary.n_converged, 6);
    BOOST_CHECK_EQUAL(summary.n_unknown, 0);
    BOOST_CHECK_EQUAL(summary.n_unknown_isolated, 0);
    BOOST_CHECK_EQUAL(summary.n_not_converged, 4);
  }

  logger.longdebug("********************************************************************************\n"
                   "allowDoneRuns -- first check, iter_k=%d", (int)iter_k) ;

  // the bins have not yet converged, we should not end yet
  BOOST_CHECK( ! ctrl.allowDoneRuns(p, dmhwalker, iter_k, mhrw) ) ;

  run_dummy_sweeps(statcoll, 16, 3); // some more sweeps -- still not enough to make bars
                                     // converge, but enough to exceed max_add_run_iters

  // the bins should not have converged
  { auto binmeans = statcoll.binMeans();
    auto errlvls = statcoll.getBinningAnalysis().calcErrorLevels(binmeans);
    auto conv_st = statcoll.getBinningAnalysis().determineErrorConvergence(errlvls);
    auto summary = Tomographer::BinningErrorBarConvergenceSummary::fromConvergedStatus(conv_st);
    logger.debug([&](std::ostream & stream) {
        stream << "Bins convergence : " << summary << "\n";
        stream << "# samples @ last level: " << coll_samples / (1<<bin_num_levels) ;
      });
  
    BOOST_CHECK_EQUAL(summary.n_bins, 10);
    BOOST_CHECK_EQUAL(summary.n_converged, 7);
    BOOST_CHECK_EQUAL(summary.n_unknown, 2);
    BOOST_CHECK_EQUAL(summary.n_unknown_isolated, 1);
    BOOST_CHECK_EQUAL(summary.n_not_converged, 1);
  }

  logger.longdebug("********************************************************************************\n"
                   "allowDoneRuns -- second check") ;

  // now the bins have not yet converged, but rw should end because we exceeded
  // max_add_run_iters=1.2 (reached 64+16 sweeps vs n_run=64 [with 1.2*64==64+12.8])
  BOOST_CHECK( ctrl.allowDoneRuns(p, dmhwalker, iter_k, mhrw) ) ;

}

BOOST_AUTO_TEST_SUITE_END()

