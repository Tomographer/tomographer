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
#include <algorithm>

#include <boost/math/constants/constants.hpp>

// definitions for Tomographer test framework -- this must be included before any
// <Eigen/...> or <tomographer2/...> header
#include "test_tomographer.h"

#include <tomographer2/mhrwtasks.h>
#include <tomographer2/tools/loggers.h>
#include <tomographer2/multiproc.h>
#include <tomographer2/densedm/dmtypes.h>
#include <tomographer2/densedm/indepmeasllh.h>
#include <tomographer2/densedm/tspacefigofmerit.h>
#include <tomographer2/densedm/tspacellhwalker.h>

#include <tomographer2/tools/boost_test_logger.h>
#include "test_mh_random_walk_common.h"

// -----------------------------------------------------------------------------
// fixture(s)


// -----------------------------------------------------------------------------
// test suites

BOOST_AUTO_TEST_SUITE(test_mhrwtasks)
// =============================================================================

BOOST_AUTO_TEST_SUITE(cdatabase)

BOOST_AUTO_TEST_CASE(constr)
{
  Tomographer::MHRWTasks::CDataBase<> cdata(Tomographer::MHRWParams<unsigned int, double>(0.1, 128, 50, 500), 100);
  BOOST_CHECK_EQUAL(cdata.base_seed, 100);
  MY_BOOST_CHECK_FLOATS_EQUAL(cdata.mhrw_params.step_size, 0.1, tol);
  BOOST_CHECK_EQUAL(cdata.mhrw_params.n_sweep, 128);
  BOOST_CHECK_EQUAL(cdata.mhrw_params.n_therm, 50);
  BOOST_CHECK_EQUAL(cdata.mhrw_params.n_run, 500);
}

BOOST_AUTO_TEST_CASE(get_task_inputs)
{
  const Tomographer::MHRWTasks::CDataBase<> cdata(Tomographer::MHRWParams<unsigned int, double>(0.1, 128, 50, 500), 100);

  BOOST_MESSAGE( cdata.getBasicCDataMHRWInfo() );

  std::vector<int> inputs;
  inputs.reserve(1024);
  for (int k = 0; k < 1024; ++k) {
    inputs.push_back(cdata.getTaskInput(k));
  }

  // make sure that all the inputs are different
  std::sort(inputs.begin(), inputs.end());
  auto last = std::unique(inputs.begin(), inputs.end());
  BOOST_CHECK(last == inputs.end());
  BOOST_CHECK_EQUAL(inputs.size(), 1024);
}

BOOST_AUTO_TEST_SUITE_END(); // cdatabase

//-----------------------------------------------

BOOST_AUTO_TEST_SUITE(tMHRandomWalkTaskResult)

struct MyStatsResultType {
  MyStatsResultType() : a(0), b(1) { }
  int a, b;
};
typedef Tomographer::MHRWParams<int,double> MHRWParamsType;

BOOST_AUTO_TEST_CASE(instantiate_1)
{
  Tomographer::MHRWTasks::MHRandomWalkTaskResult<MyStatsResultType,MHRWParamsType::CountIntType,
                                                 MHRWParamsType::StepRealType> r(
      MyStatsResultType(),
      MHRWParamsType(0.1, 10, 100, 1000),
      0.85
      );
  BOOST_CHECK_EQUAL(r.stats_collector_result.a, MyStatsResultType().a);
  BOOST_CHECK_EQUAL(r.stats_collector_result.b, MyStatsResultType().b);
  MY_BOOST_CHECK_FLOATS_EQUAL(r.mhrw_params.step_size, 0.1, tol);
  BOOST_CHECK_EQUAL(r.mhrw_params.n_sweep, 10);
  BOOST_CHECK_EQUAL(r.mhrw_params.n_therm, 100);
  BOOST_CHECK_EQUAL(r.mhrw_params.n_run, 1000);
  MY_BOOST_CHECK_FLOATS_EQUAL(r.acceptance_ratio, 0.85, tol);
}
BOOST_AUTO_TEST_CASE(instantiate_1b)
{
  const MyStatsResultType xx;
  Tomographer::MHRWTasks::MHRandomWalkTaskResult<MyStatsResultType,MHRWParamsType::CountIntType,
                                                 MHRWParamsType::StepRealType> r(
      xx,
      MHRWParamsType(0.1, 10, 100, 1000),
      0.85
      );
  BOOST_CHECK_EQUAL(r.stats_collector_result.a, MyStatsResultType().a);
  BOOST_CHECK_EQUAL(r.stats_collector_result.b, MyStatsResultType().b);
  MY_BOOST_CHECK_FLOATS_EQUAL(r.mhrw_params.step_size, 0.1, tol);
  BOOST_CHECK_EQUAL(r.mhrw_params.n_sweep, 10);
  BOOST_CHECK_EQUAL(r.mhrw_params.n_therm, 100);
  BOOST_CHECK_EQUAL(r.mhrw_params.n_run, 1000);
  MY_BOOST_CHECK_FLOATS_EQUAL(r.acceptance_ratio, 0.85, tol);
}
BOOST_AUTO_TEST_CASE(instantiate_2)
{
  Tomographer::Logger::BoostTestLogger logger;

  typedef std::mt19937 Rng;
  Rng rng(0);

  typedef Tomographer::MHRandomWalk<Rng, TestLatticeMHRWGaussPeak<int>,
                                    Tomographer::TrivialMHRWStatsCollector,
                                    Tomographer::Logger::BoostTestLogger, int>  MHRandomWalkType;

  Tomographer::TrivialMHRWStatsCollector stats;

  TestLatticeMHRWGaussPeak<int> mhwalker(
	  Eigen::Vector2i::Constant(100),
	  (Eigen::Matrix2i() << 10, -5, 5, 10).finished(), 1,
	  (Eigen::Vector2i() << 40, 50).finished(),
	  rng
      );
  MHRandomWalkType rw(MHRWParamsType(2, 10, 100, 1000), mhwalker, stats, rng, logger);

  // the thing we actually want to test:
  Tomographer::MHRWTasks::MHRandomWalkTaskResult<MyStatsResultType,
                                                 MHRWParamsType::CountIntType,
                                                 MHRWParamsType::StepRealType> r(
      MyStatsResultType(),
      rw
      );
  BOOST_CHECK_EQUAL(r.stats_collector_result.a, MyStatsResultType().a);
  BOOST_CHECK_EQUAL(r.stats_collector_result.b, MyStatsResultType().b);
  MY_BOOST_CHECK_FLOATS_EQUAL(r.mhrw_params.step_size, 2, tol);
  BOOST_CHECK_EQUAL(r.mhrw_params.n_sweep, 10);
  BOOST_CHECK_EQUAL(r.mhrw_params.n_therm, 100);
  BOOST_CHECK_EQUAL(r.mhrw_params.n_run, 1000);
  BOOST_CHECK(std::isnan(r.acceptance_ratio));
}


BOOST_AUTO_TEST_SUITE_END(); // tMHRandomWalkTaskResult


// -----------------------------------------------

BOOST_AUTO_TEST_SUITE(tMHRandomWalkTask) ;

struct MyCData
  : public Tomographer::MHRWTasks::CDataBase<int,int>
{
  Tomographer::MHRWParams<int,int> mhrw_params;

  MyCData(Tomographer::MHRWParams<int,int> && p)
    : Tomographer::MHRWTasks::CDataBase<int,int>(p, -134567),
      mhrw_params(p)
  {
  }
  
  typedef TestMHRWStatsCollectorWithResult::ResultType MHRWStatsCollectorResultType;

  template<typename LoggerType>
  TestMHRWStatsCollectorWithResult createStatsCollector(LoggerType & logger) const
  {
    logger.debug("MyCData::createStatsCollector", "()");
    return TestMHRWStatsCollectorWithResult(mhrw_params.n_sweep, mhrw_params.n_therm, mhrw_params.n_run);
  }

  struct TestMHWalker2 : public TestMHWalker
  {
    typedef int StepRealType; // override, not sure why we had double in base class, but I dont dare change that

    template<typename... Args>
    TestMHWalker2(Args&&... x)
      : TestMHWalker(std::forward<Args>(x)...)
    {
    }
  };
  
  template<typename Rng, typename LoggerType>
  TestMHWalker2 createMHWalker(Rng & rng, LoggerType & logger) const
  {
    logger.debug("MyCData::createMHWalker", "()");
    return TestMHWalker2(mhrw_params.n_sweep, mhrw_params.n_therm, mhrw_params.n_run, rng) ;
  }

};

struct MyTestResultsCollector {
  int total_runs;
  void init(int, int, const MyCData *)
  {
    total_runs = 0;
  }
  void collectResult(int task_no, Tomographer::MHRWTasks::MHRandomWalkTaskResult<bool, int, int> taskresult,
                     const MyCData * pcdata)
  {
    BOOST_MESSAGE("Task: #" << task_no << ": result = " << std::boolalpha << taskresult.stats_collector_result );
    BOOST_CHECK(taskresult.acceptance_ratio >= 0 && taskresult.acceptance_ratio <= 1);
    BOOST_CHECK_EQUAL(taskresult.mhrw_params.step_size, pcdata->mhrw_params.step_size);
    BOOST_CHECK_EQUAL(taskresult.mhrw_params.n_sweep, pcdata->mhrw_params.n_sweep);
    BOOST_CHECK_EQUAL(taskresult.mhrw_params.n_therm, pcdata->mhrw_params.n_therm);
    BOOST_CHECK_EQUAL(taskresult.mhrw_params.n_run, pcdata->mhrw_params.n_run);
    BOOST_CHECK(taskresult.stats_collector_result);
    ++total_runs;
  }
  void runsFinished(int, const MyCData * )
  {
    BOOST_MESSAGE("total # of runs = " << total_runs) ;
  }
};

BOOST_AUTO_TEST_CASE(base)
{
  Tomographer::Logger::BoostTestLogger logger;

  const int step_size = 2;
  const int nsweep = 10;
  const int ntherm = 50;
  const int nrun = 100;

  MyCData cdata(Tomographer::MHRWParams<int,int>(step_size, nsweep, ntherm, nrun));

  MyTestResultsCollector results;

  Tomographer::MultiProc::Sequential::TaskDispatcher<
    Tomographer::MHRWTasks::MHRandomWalkTask<MyCData, std::mt19937>,
    MyCData,
    MyTestResultsCollector,
    Tomographer::Logger::BoostTestLogger,
    long
    > dispatcher(&cdata, &results, logger, 5);
  
  // tests are all in here
  dispatcher.run();

  BOOST_CHECK_EQUAL(results.total_runs, 5);
}

BOOST_AUTO_TEST_SUITE_END(); // tMHRandomWalkTask

// =============================================================================
BOOST_AUTO_TEST_SUITE_END() ;

