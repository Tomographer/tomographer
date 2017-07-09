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

#include <cstdio>
#include <random>
#include <iostream>
#include <vector>

#include "test_tomographer.h"

#include <tomographer/mhrw.h>
#include <tomographer/mhrwstatscollectors.h>

#include "test_mh_random_walk_common.h" // our test-case random walk


// -----------------------------------------------------------------------------
// fixtures
// -----------------------------------------------------------------------------


template<unsigned int AdjustmentStrategyFlags>
struct mhrandomwalk_control_fixture
{
  void go()
  {
    typedef Tomographer::Logger::VacuumLogger LoggerType;
    LoggerType logger;

    typedef std::mt19937 Rng;
    // rng for the random walk engine itself
    Rng rng(3040); // fixed seed

    typedef Tomographer::MHRandomWalk<Rng, TestLatticeMHRWGaussPeak<int>,
                                      Tomographer::TrivialMHRWStatsCollector,
                                      TestMHRWController<AdjustmentStrategyFlags>,
                                      LoggerType, int>
      MHRandomWalkType;

    const int ntherm = 50;
    const int nrun = 100;
    const int nsweep = 10;

    // rng for the mhwalker
    Rng rng2(414367); // seed, fixed -> deterministic

    TestLatticeMHRWGaussPeak<int> mhwalker(
        Eigen::Vector2i::Constant(100),
        (Eigen::Matrix2i() << 10, -5, 5, 10).finished(), 1,
        (Eigen::Vector2i() << 40, 50).finished(),
        rng
        );
    Tomographer::TrivialMHRWStatsCollector stats;

    // our test controller
    TestMHRWController<AdjustmentStrategyFlags> ctrl;

    MHRandomWalkType rw(2, nsweep, ntherm, nrun, mhwalker, stats, ctrl, rng, logger);

    rw.run();

    // make sure that the done() callback of the controller was called, because that's
    // where all the tests are!
    BOOST_CHECK( ctrl.done_called );
  }
};


// -----------------------------------------------------------------------------
// test cases
// -----------------------------------------------------------------------------



BOOST_AUTO_TEST_SUITE(test_mhrw)
// -----------------------------------------------------------------------------


BOOST_AUTO_TEST_CASE(mhrandomwalksetup)
{
  typedef Tomographer::Logger::VacuumLogger LoggerType;
  LoggerType logger;

  typedef std::mt19937 Rng;
  Rng rng(3040); // fixed seed

  typedef Tomographer::MHRandomWalk<Rng, TestMHWalker, TestMHRWStatsCollector,
                                    Tomographer::MHRWNoController, LoggerType, int>
    MHRandomWalkType;

  const int ntherm = 50;
  const int nrun = 100;
  const int nsweep = 10;

  // rng for the mhwalker
  Rng rng2(414367); // seed, fixed -> deterministic

  TestMHWalker mhwalker(nsweep, ntherm, nrun, rng2);
  TestMHRWStatsCollector stats(nsweep, ntherm, nrun);
  Tomographer::MHRWNoController noctrl;
  MHRandomWalkType rw(Tomographer::MHRWParams<StepSizeType,int>(2, nsweep, ntherm, nrun),
                      mhwalker, stats, noctrl, rng, logger);

  BOOST_CHECK_EQUAL(rw.nSweep(), nsweep);
  BOOST_CHECK_EQUAL(rw.nTherm(), ntherm);
  BOOST_CHECK_EQUAL(rw.nRun(), nrun);

  BOOST_CHECK(!rw.hasAcceptanceRatio());
}



BOOST_AUTO_TEST_CASE(mhrandomwalk)
{
  typedef Tomographer::Logger::VacuumLogger LoggerType;
  LoggerType logger;

  typedef std::mt19937 Rng;
  // rng for the random walk engine itself
  Rng rng(3040); // fixed seed

  typedef Tomographer::MHRandomWalk<Rng, TestMHWalker, TestMHRWStatsCollector, Tomographer::MHRWNoController,
                                    LoggerType, int>
    MHRandomWalkType;

  const int ntherm = 50;
  const int nrun = 100;
  const int nsweep = 10;

  // rng for the mhwalker
  Rng rng2(414367); // seed, fixed -> deterministic

  TestMHWalker mhwalker(nsweep, ntherm, nrun, rng2);
  TestMHRWStatsCollector stats(nsweep, ntherm, nrun);
  Tomographer::MHRWNoController noctrl;
  MHRandomWalkType rw(2, nsweep, ntherm, nrun, mhwalker, stats, noctrl, rng, logger);

  BOOST_CHECK_EQUAL(rw.nSweep(), nsweep);
  BOOST_CHECK_EQUAL(rw.nTherm(), ntherm);
  BOOST_CHECK_EQUAL(rw.nRun(), nrun);

  BOOST_CHECK(!rw.hasAcceptanceRatio());

  rw.run();

  BOOST_CHECK(rw.hasAcceptanceRatio());
  BOOST_MESSAGE("Accept ratio = " << rw.acceptanceRatio());
}


BOOST_AUTO_TEST_CASE(mhrandomwalk_overflow_protection)
{
  typedef Tomographer::Logger::VacuumLogger LoggerType;
  LoggerType logger;

  typedef std::mt19937 Rng;
  Rng rng(0);

  typedef Tomographer::MHRandomWalk<Rng, TestMHWalker, Tomographer::MultipleMHRWStatsCollectors<>,
                                    Tomographer::MHRWNoController,
                                    LoggerType, int32_t>
    MHRandomWalkType;

  {
    const int32_t ntherm = 1;
    const int32_t nrun = std::numeric_limits<int32_t>::max() / 128; // oops!
    const int32_t nsweep = 256; // oops!

    TestMHWalker mhwalker(nsweep, ntherm, nrun, rng);
    Tomographer::MultipleMHRWStatsCollectors<> nostats;
    Tomographer::MHRWNoController noctrl;
    MHRandomWalkType rw(2, nsweep, ntherm, nrun, mhwalker, nostats, noctrl, rng, logger);

    BOOST_CHECK_THROW( rw.run() , std::exception );
  }

  {
    const int32_t ntherm = std::numeric_limits<int32_t>::max() / 128; // oops!
    const int32_t nrun = 1;
    const int32_t nsweep = 256; // oops!

    TestMHWalker mhwalker(nsweep, ntherm, nrun, rng);
    Tomographer::MultipleMHRWStatsCollectors<> nostats;
    Tomographer::MHRWNoController noctrl;
    MHRandomWalkType rw(2, nsweep, ntherm, nrun, mhwalker, nostats, noctrl, rng, logger);

    BOOST_CHECK_THROW( rw.run() , std::exception );
  }
}

BOOST_AUTO_TEST_SUITE(mhrandomwalk_control_iface)

BOOST_FIXTURE_TEST_CASE(ThRnIt,
                        mhrandomwalk_control_fixture<
                        Tomographer::MHRWControllerAdjustEveryIterationAlways
                        >)
{
  go();
}
BOOST_FIXTURE_TEST_CASE(ThRnItSm,
                        mhrandomwalk_control_fixture<
                        Tomographer::MHRWControllerAdjustEveryIterationAlways|
                        Tomographer::MHRWControllerAdjustEverySample
                        >)
{
  go();
}
BOOST_FIXTURE_TEST_CASE(ThIt,
                        mhrandomwalk_control_fixture<
                        Tomographer::MHRWControllerAdjustWhileThermalizing|
                        Tomographer::MHRWControllerAdjustEveryIteration
                        >)
{
  go();
}
BOOST_FIXTURE_TEST_CASE(RnSm,
                        mhrandomwalk_control_fixture<
                        Tomographer::MHRWControllerAdjustWhileRunning|
                        Tomographer::MHRWControllerAdjustEverySample
                        >)
{
  go();
}
BOOST_FIXTURE_TEST_CASE(ThRnSm,
                        mhrandomwalk_control_fixture<
                        Tomographer::MHRWControllerAdjustWhileThermalizingAndRunning|
                        Tomographer::MHRWControllerAdjustEverySample
                        >)
{
  go();
}
BOOST_FIXTURE_TEST_CASE(RnItSm,
                        mhrandomwalk_control_fixture<
                        Tomographer::MHRWControllerAdjustWhileRunning|
                        Tomographer::MHRWControllerAdjustEveryIteration|
                        Tomographer::MHRWControllerAdjustEverySample
                        >)
{
  go();
}

BOOST_AUTO_TEST_SUITE_END() ; // mhrandomwalk_control_iface


// -----------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE_END()

