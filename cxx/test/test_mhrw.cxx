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

#include "test_tomographer.h"

#include <tomographer2/mhrw.h>

#include "test_mh_random_walk_common.h" // our test-case random walk


BOOST_AUTO_TEST_SUITE(test_mhrw)
// -----------------------------------------------------------------------------

struct test_mhrandomwalk_fixture
{
  struct TestMHWalker : public TestLatticeMHRWGaussPeak<int> {
    typedef TestLatticeMHRWGaussPeak<int> Base;

    int count_jump;
    
    int Nthermchk;
    int Nrunchk;
    int Nsweepchk;
    
    TestMHWalker(int sweep_size, int check_n_therm, int check_n_run)
      : TestLatticeMHRWGaussPeak(
	  Eigen::Vector2i::Constant(100),
	  (Eigen::Matrix2i() << 10, -5, 5, 10).finished(), 1,
	  (Eigen::Vector2i() << 40, 50).finished(),
	  414367 // seed, fixed -> deterministic
	  ),
	count_jump(0),
	Nthermchk(check_n_therm),
	Nrunchk(check_n_run),
	Nsweepchk(sweep_size)
    {
    }
    
    inline void init()
    {
      Base::init();
      BOOST_CHECK_EQUAL(count_jump, 0);
      count_jump = 0;
    }
    
    inline PointType startpoint()
    {
      BOOST_CHECK_EQUAL(count_jump, 0);
      return PointType::Zero(latticeDims.size());
    }
    
    inline void thermalizing_done()
    {
      BOOST_CHECK_EQUAL(count_jump, Nthermchk*Nsweepchk);
    }
    
    inline void done()
    {
      BOOST_CHECK_EQUAL(count_jump, Nthermchk*Nsweepchk + Nrunchk*Nsweepchk);
    }
    
    template<typename PT>
    inline PointType jump_fn(PT&& curpt, const StepRealType step_size)
    {
      ++count_jump;
      return Base::jump_fn(std::forward<PointType>(curpt), step_size);
    }
  };

  struct TestMHRWStatsCollector
  {
    int count_rawmoves;
    int count_samples;
    
    int Nthermchk;
    int Nrunchk;
    int Nsweepchk;
    
    TestMHRWStatsCollector(int sweep_size, int check_n_therm, int check_n_run)
      : count_rawmoves(0),
	count_samples(0),
	Nthermchk(check_n_therm),
	Nrunchk(check_n_run),
	Nsweepchk(sweep_size)
    {
    }
    void init()
    {
      BOOST_CHECK_EQUAL(count_rawmoves, 0);
      BOOST_CHECK_EQUAL(count_samples, 0);
    }
    void thermalizing_done()
    {
      BOOST_CHECK_EQUAL(count_rawmoves, Nthermchk*Nsweepchk);
      BOOST_CHECK_EQUAL(count_samples, 0);
    }
    void done()
    {
      BOOST_CHECK_EQUAL(count_rawmoves, Nthermchk*Nsweepchk+Nrunchk*Nsweepchk);
      BOOST_CHECK_EQUAL(count_samples, Nrunchk);
    }
    template<typename... Args>
    void process_sample(Args&&... /*a*/)
    //CountIntType k, CountIntType n, const PointType & pt, FnValueType fnval, MHRandomWalk & rw)
    {
      ++count_samples;
    }
    template<typename CountIntType, typename PointType, typename FnValueType, typename MHRandomWalk>
    void raw_move(CountIntType k, bool is_thermalizing, bool is_live_iter, bool accepted, double a,
		  const PointType & /*newpt*/, FnValueType /*newptval*/, const PointType & /*curpt*/,
		  FnValueType /*curptval*/, MHRandomWalk & /*rw*/)
    {
      if (count_rawmoves < Nthermchk*Nsweepchk) {
	BOOST_CHECK_EQUAL(k, count_rawmoves);
      } else {
	BOOST_CHECK_EQUAL(k, count_rawmoves-Nthermchk*Nsweepchk);
      }
      BOOST_CHECK_EQUAL((count_rawmoves < Nthermchk*Nsweepchk), is_thermalizing);
      BOOST_CHECK_EQUAL(is_live_iter, !is_thermalizing && ((k+1)%Nsweepchk == 0));

      BOOST_MESSAGE("a = " << a);
      if (a + std::numeric_limits<double>::epsilon() >= 1.0) {
	BOOST_CHECK(accepted);
      }

      ++count_rawmoves;
    }
  };
  
};


BOOST_FIXTURE_TEST_CASE(mhrandomwalk, test_mhrandomwalk_fixture)
{
  typedef Tomographer::Logger::VacuumLogger LoggerType;
  LoggerType logger;

  typedef std::mt19937 Rng;
  Rng rng(3040); // fixed seed

  typedef Tomographer::MHRandomWalk<Rng, TestMHWalker, TestMHRWStatsCollector, LoggerType, int>
    MHRandomWalkType;

  const int ntherm = 50;
  const int nrun = 100;
  const int nsweep = 10;

  TestMHWalker mhwalker(nsweep, ntherm, nrun);
  TestMHRWStatsCollector stats(nsweep, ntherm, nrun);
  MHRandomWalkType rw(nsweep, 2, ntherm, nrun, mhwalker, stats, rng, logger);

  BOOST_CHECK_EQUAL(rw.nSweep(), nsweep);
  BOOST_CHECK_EQUAL(rw.nTherm(), ntherm);
  BOOST_CHECK_EQUAL(rw.nRun(), nrun);

  BOOST_CHECK(!rw.hasAcceptanceRatio());

  rw.run();

}


BOOST_FIXTURE_TEST_CASE(mhrandomwalksetup, test_mhrandomwalk_fixture)
{
  typedef Tomographer::Logger::VacuumLogger LoggerType;
  LoggerType logger;

  typedef std::mt19937 Rng;
  Rng rng(3040); // fixed seed

  typedef Tomographer::MHRandomWalk<Rng, TestMHWalker, TestMHRWStatsCollector, LoggerType, int>
    MHRandomWalkType;

  const int ntherm = 50;
  const int nrun = 100;
  const int nsweep = 10;

  TestMHWalker mhwalker(nsweep, ntherm, nrun);
  TestMHRWStatsCollector stats(nsweep, ntherm, nrun);
  MHRandomWalkType rw(Tomographer::MHRWParams<int,double>(nsweep, 2, ntherm, nrun), mhwalker, stats, rng, logger);

  BOOST_CHECK_EQUAL(rw.nSweep(), nsweep);
  BOOST_CHECK_EQUAL(rw.nTherm(), ntherm);
  BOOST_CHECK_EQUAL(rw.nRun(), nrun);

  BOOST_CHECK(!rw.hasAcceptanceRatio());
}


// -----------------------------------------------------------------------------
BOOST_AUTO_TEST_SUITE_END()

