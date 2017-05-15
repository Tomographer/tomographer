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

#include "test_mh_random_walk_common.h" // our test-case random walk


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
  MHRandomWalkType rw(Tomographer::MHRWParams<StepSizeType,int>(2, nsweep, ntherm, nrun),
                      mhwalker, stats, rng, logger);

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
  MHRandomWalkType rw(2, nsweep, ntherm, nrun, mhwalker, stats, rng, logger);

  BOOST_CHECK_EQUAL(rw.nSweep(), nsweep);
  BOOST_CHECK_EQUAL(rw.nTherm(), ntherm);
  BOOST_CHECK_EQUAL(rw.nRun(), nrun);

  BOOST_CHECK(!rw.hasAcceptanceRatio());

  rw.run();

  BOOST_CHECK(rw.hasAcceptanceRatio());
  BOOST_MESSAGE("Accept ratio = " << rw.acceptanceRatio());
}


BOOST_AUTO_TEST_CASE(mhrandomwalk_adjuster_iface)
{
  BOOST_CHECK( false ) ; // test cases not yet written
}


BOOST_AUTO_TEST_CASE(mhrw_status_report)
{
  BOOST_CHECK( false ) ; // test cases not yet written -- import them as appropriate from mhrwtasks
}

// -----------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE_END()

