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
#include <algorithm>

#include <boost/math/constants/constants.hpp>

// definitions for Tomographer test framework -- this must be included before any
// <Eigen/...> or <tomographer/...> header
#include "test_tomographer.h"

#include <tomographer/mhrwtasks.h>
#include <tomographer/tools/loggers.h>
#include <tomographer/multiproc.h>
#include <tomographer/densedm/dmtypes.h>
#include <tomographer/densedm/indepmeasllh.h>
#include <tomographer/densedm/tspacefigofmerit.h>
#include <tomographer/densedm/tspacellhwalker.h>

#include <tomographer/tools/boost_test_logger.h>
#include "test_mh_random_walk_common.h"

// -----------------------------------------------------------------------------
// fixture(s)

template<typename UType>
inline constexpr int count_set_bits(UType x) {
  return (x == 0) ? 0 : ((x&0x1) + count_set_bits(x>>1));
}


// -----------------------------------------------------------------------------
// test suites

BOOST_AUTO_TEST_SUITE(test_mhrwtasks)
// =============================================================================

BOOST_AUTO_TEST_SUITE(cheap_perm)

BOOST_AUTO_TEST_CASE(interleave)
{
  unsigned int x = 0x0f;
  unsigned int y = 0x00;

  unsigned int i1 = Tomographer::MHRWTasks::tomo_internal::bits_interleaved<unsigned int>::interleave<4>(x, y);
  unsigned int i2 = Tomographer::MHRWTasks::tomo_internal::bits_interleaved<unsigned int>::interleave<4>(y, x);

  BOOST_TEST_MESSAGE("x=0x"<<std::hex<<x<<", y=0x"<<std::hex<<y<<" : i1=0x"<<std::hex<<i1<<", i2=0x"<<std::hex<<i2) ;

  BOOST_CHECK_EQUAL(i1, 0x55u) ;
  BOOST_CHECK_EQUAL(i2, 0xaau) ;
}

BOOST_AUTO_TEST_CASE(is_perm)
{
  const std::size_t N = sizeof(std::size_t)*CHAR_BIT; // 15; //2048+17;
  std::vector<std::size_t> vs;

  vs.resize(N);
  for (std::size_t k = 0; k < N; ++k) {
    const std::size_t x = std::size_t(1)<<k; // check all individual bits
    const std::size_t y = Tomographer::MHRWTasks::tomo_internal::kinda_random_bits_perm<std::size_t>::apply(x);
    BOOST_TEST_MESSAGE("testing " << x << " = 0x" << std::hex << x << ", y = 0x" << std::hex << y );

    //    const int NBits = 64;
    //    BOOST_TEST_MESSAGE("right half = 0x" << std::hex << (x & ((std::size_t(1)<<(NBits>>1))-1))) ;
    
    // check that y has the same number of set bits as k
    BOOST_CHECK_EQUAL(count_set_bits(y), count_set_bits(x)) ;
    
    vs[k] = y;
  }
  
  // finally check that all vs[k] are different
  { std::sort( vs.begin(), vs.end() );
    auto newend = std::unique( vs.begin(), vs.end() );
    BOOST_CHECK(newend == vs.end());
  }


  // do the same for sequential counts
  const std::size_t N2 = 1024 + 37;
  vs.resize(N2);
  const std::size_t key = 0x11001010u;
  for (std::size_t k = 0; k < N2; ++k) {
    const std::size_t x = key ^ k;
    const std::size_t y = Tomographer::MHRWTasks::tomo_internal::kinda_random_bits_perm<std::size_t>::apply(x);
    BOOST_TEST_MESSAGE("testing " << x << " = 0x" << std::hex << x << ", y = 0x" << std::hex << y );
    // check that y has the same number of set bits as k
    BOOST_CHECK_EQUAL(count_set_bits(y), count_set_bits(x)) ;
    vs[k] = y;
  }
  
  // finally check that all vs[k] are different
  { std::sort( vs.begin(), vs.end() );
    auto newend = std::unique( vs.begin(), vs.end() );
    BOOST_CHECK(newend == vs.end());
  }
 
}

BOOST_AUTO_TEST_SUITE_END() ;

// =============================================================================

BOOST_AUTO_TEST_SUITE(cdatabase)

BOOST_AUTO_TEST_CASE(constr)
{
  Tomographer::MHRWTasks::CDataBase<double> cdata(Tomographer::MHRWParams<double, int>(0.1, 128, 50, 500), 100u);
  BOOST_CHECK_EQUAL(cdata.base_seed, 100u);
  MY_BOOST_CHECK_FLOATS_EQUAL(cdata.mhrw_params.mhwalker_params, 0.1, tol);
  BOOST_CHECK_EQUAL(cdata.mhrw_params.n_sweep, 128);
  BOOST_CHECK_EQUAL(cdata.mhrw_params.n_therm, 50);
  BOOST_CHECK_EQUAL(cdata.mhrw_params.n_run, 500);
}

BOOST_AUTO_TEST_CASE(get_task_inputs)
{
  const Tomographer::MHRWTasks::CDataBase<double,int,unsigned int>
    cdata(Tomographer::MHRWParams<double, int>(0.1, 128, 50, 500), 100);

  BOOST_MESSAGE( cdata.getBasicCDataMHRWInfo() );

  std::vector<unsigned int> inputs;
  inputs.reserve(1024);
  for (int k = 0; k < 1024; ++k) {
    auto input = cdata.getTaskInput(k);
    bool oktype = std::is_same<decltype(input),unsigned int>::value;
    BOOST_CHECK( oktype ) ;
    inputs.push_back(input);
    BOOST_TEST_MESSAGE("Task input #" << k << ": " << input) ;
  }

  // make sure that all the inputs are different
  std::sort(inputs.begin(), inputs.end());
  auto last = std::unique(inputs.begin(), inputs.end());
  BOOST_CHECK(last == inputs.end());
  BOOST_CHECK_EQUAL(inputs.size(), 1024u);
}

BOOST_AUTO_TEST_SUITE_END(); // cdatabase

//-----------------------------------------------

BOOST_AUTO_TEST_SUITE(tMHRandomWalkTaskResult)

struct MyStatsResultType {
  MyStatsResultType() : a(0), b(1) { }
  int a, b;
};
typedef Tomographer::MHRWParams<StepSizeType,int> MHRWParamsType;

BOOST_AUTO_TEST_CASE(instantiate_1)
{
  Tomographer::MHRWTasks::MHRandomWalkTaskResult<MyStatsResultType,MHRWParamsType::CountIntType,
                                                 MHRWParamsType::MHWalkerParams> r(
      MyStatsResultType(),
      MHRWParamsType(0.1, 10, 100, 1000),
      0.85
      );
  BOOST_CHECK_EQUAL(r.stats_results.a, MyStatsResultType().a);
  BOOST_CHECK_EQUAL(r.stats_results.b, MyStatsResultType().b);
  MY_BOOST_CHECK_FLOATS_EQUAL(r.mhrw_params.mhwalker_params.stepsize, 0.1, tol);
  BOOST_CHECK_EQUAL(r.mhrw_params.n_sweep, 10);
  BOOST_CHECK_EQUAL(r.mhrw_params.n_therm, 100);
  BOOST_CHECK_EQUAL(r.mhrw_params.n_run, 1000);
  MY_BOOST_CHECK_FLOATS_EQUAL(r.acceptance_ratio, 0.85, tol);
}
BOOST_AUTO_TEST_CASE(instantiate_1b)
{
  const MyStatsResultType xx;
  Tomographer::MHRWTasks::MHRandomWalkTaskResult<MyStatsResultType,MHRWParamsType::CountIntType,
                                                 MHRWParamsType::MHWalkerParams> r(
      xx,
      MHRWParamsType(0.1, 10, 100, 1000),
      0.85
      );
  BOOST_CHECK_EQUAL(r.stats_results.a, MyStatsResultType().a);
  BOOST_CHECK_EQUAL(r.stats_results.b, MyStatsResultType().b);
  MY_BOOST_CHECK_FLOATS_EQUAL(r.mhrw_params.mhwalker_params.stepsize, 0.1, tol);
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
                                    Tomographer::MHRWNoController,
                                    Tomographer::Logger::BoostTestLogger, int>  MHRandomWalkType;

  Tomographer::TrivialMHRWStatsCollector stats;

  TestLatticeMHRWGaussPeak<int> mhwalker(
	  Eigen::Vector2i::Constant(100),
	  (Eigen::Matrix2i() << 10, -5, 5, 10).finished(), 1,
	  (Eigen::Vector2i() << 40, 50).finished(),
	  rng
      );
  Tomographer::MHRWNoController noctrl;
  MHRandomWalkType rw(MHRWParamsType(StepSizeType(2), 10, 100, 1000), mhwalker, stats, noctrl, rng, logger);

  // the thing we actually want to test:
  Tomographer::MHRWTasks::MHRandomWalkTaskResult<MyStatsResultType,
                                                 MHRWParamsType::CountIntType,
                                                 MHRWParamsType::MHWalkerParams> r(
      MyStatsResultType(),
      rw
      );
  BOOST_CHECK_EQUAL(r.stats_results.a, MyStatsResultType().a);
  BOOST_CHECK_EQUAL(r.stats_results.b, MyStatsResultType().b);
  MY_BOOST_CHECK_FLOATS_EQUAL(r.mhrw_params.mhwalker_params.stepsize, 2, tol);
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
  
  typedef TestMHRWStatsCollectorWithResult::ResultType MHRWStatsResultsType;

  struct TestMHWalker2 : public TestMHWalker
  {
    typedef int WalkerParams; // override, not sure why we had double in base class, but I dont dare change that

    template<typename... Args>
    TestMHWalker2(Args&&... x)
      : TestMHWalker(std::forward<Args>(x)...)
    {
    }
  };


  template<typename Rng, typename LoggerType, typename RunFn>
  void setupRandomWalkAndRun(Rng & rng, LoggerType & logger, RunFn run) const
  {
    logger.debug("MyCData::setupRandomWalkAndRun", "()");

    TestMHWalker2 mhwalker(mhrw_params.n_sweep, mhrw_params.n_therm, mhrw_params.n_run, rng) ;
    TestMHRWStatsCollectorWithResult stats(mhrw_params.n_sweep, mhrw_params.n_therm, mhrw_params.n_run);

    run(mhwalker, stats);
  }

};


BOOST_AUTO_TEST_CASE(base)
{
  Tomographer::Logger::BoostTestLogger logger;

  const int mhwalker_params = 2;
  const int nsweep = 10;
  const int ntherm = 50;
  const int nrun = 100;

  MyCData cdata(Tomographer::MHRWParams<int,int>(mhwalker_params, nsweep, ntherm, nrun));

  const std::size_t NumRuns = 5;

  Tomographer::MultiProc::Sequential::TaskDispatcher<
    Tomographer::MHRWTasks::MHRandomWalkTask<MyCData, std::mt19937>,
    MyCData,
    Tomographer::Logger::BoostTestLogger,
    long
    > dispatcher(&cdata, logger, NumRuns);
  
  // tests are all in here
  dispatcher.run();

  typedef Tomographer::MHRWTasks::MHRandomWalkTaskResult<bool, int, int> TaskResultType;
  std::vector<TaskResultType*> taskresults = dispatcher.collectedTaskResults();

  BOOST_CHECK_EQUAL(taskresults.size(), NumRuns) ;

  const auto * pcdata = &cdata;

  for (std::size_t task_no = 0; task_no < NumRuns; ++task_no) {

    const auto & taskresult = *taskresults[task_no];

    BOOST_MESSAGE("Task: #" << task_no << ": result = " << std::boolalpha << taskresult.stats_results );
    BOOST_CHECK(taskresult.acceptance_ratio >= 0 && taskresult.acceptance_ratio <= 1);
    BOOST_CHECK_EQUAL(taskresult.mhrw_params.mhwalker_params, pcdata->mhrw_params.mhwalker_params);
    BOOST_CHECK_EQUAL(taskresult.mhrw_params.n_sweep, pcdata->mhrw_params.n_sweep);
    BOOST_CHECK_EQUAL(taskresult.mhrw_params.n_therm, pcdata->mhrw_params.n_therm);
    BOOST_CHECK_EQUAL(taskresult.mhrw_params.n_run, pcdata->mhrw_params.n_run);
    BOOST_CHECK(taskresult.stats_results);

  }

}

BOOST_AUTO_TEST_SUITE_END(); // tMHRandomWalkTask

// =============================================================================
BOOST_AUTO_TEST_SUITE_END() ;

