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

#include <tomographer2/mhrw_valuehist_tasks.h>
#include <tomographer2/multiprocomp.h>

#include "boost_test_logger.h"
#include "test_mh_random_walk_common.h" // our test-case random walk


// -----------------------------------------------------------------------------
// fixture(s)

template<typename ValueType_>
struct NormValueCalculator
{
  typedef ValueType_ ValueType;

  template<typename EigenObject>
  ValueType getValue(EigenObject&& pt) const
  {
    return pt.norm();
  }
};

template<typename CDataBaseType>
struct OurCDataTask : CDataBaseType
{
  template<typename... Args>
  OurCDataTask(Args&&... args)
    : CDataBaseType(std::forward<Args>(args)...)
  {
  }

  // using `int' instead of `double' allows us to ensure that everything is deterministic
  // as there are no floating-point calculations.
  template<typename Rng, typename LoggerType>
  inline TestLatticeMHRWGaussPeak<int,Rng,LoggerType>
  createMHWalker(Rng & rng, LoggerType & logger) const
  {
    // 100x100 lattice
    return TestLatticeMHRWGaussPeak<int,Rng,LoggerType>(
	1000*Eigen::Vector2i::Ones(), Eigen::Matrix2i::Identity(), 200*200, Eigen::Vector2i::Zero(2),
	rng, logger
	);
  }
};




template<bool UseBinningAnalysis, bool FullRun = true>
struct run_test {

  typedef NormValueCalculator<double> ValueCalculator;
    
  typedef BoostTestLogger LoggerType;

  typedef std::mt19937 Rng;
    
  typedef OurCDataTask<
    Tomographer::MHRWTasks::ValueHistogramTasks::CDataBase<ValueCalculator,
							   UseBinningAnalysis>
    > OurCDataSimple;
  typedef typename OurCDataSimple::HistogramParams HistogramParams;

  typedef Tomographer::MHRWTasks::MHRandomWalkTask<OurCDataSimple, Rng>  OurMHRandomWalkTask;
  typedef typename OurCDataSimple::template ResultsCollectorType<LoggerType>::Type OurResultsCollector;
  typedef typename OurCDataSimple::template ResultsCollectorType<LoggerType>::RunTaskResultType
    OurResultsCollectorRunTaskResultType;

  typedef Tomographer::MHRWParams<int,double> MHRWParamsType;
    
  typedef Tomographer::MultiProc::OMP::TaskDispatcher<OurMHRandomWalkTask, OurCDataSimple, OurResultsCollector,
						      LoggerType, int>
    TaskDispatcherType;

  LoggerType logger;
  OurCDataSimple taskcdat;
  OurResultsCollector results;

  TaskDispatcherType tasks;

  static constexpr int NumRepeats = 6;

  TOMOGRAPHER_ENABLED_IF(!UseBinningAnalysis)
  run_test()
    : logger(Tomographer::Logger::DEBUG),
      taskcdat(NormValueCalculator<double>(), HistogramParams(0, 500, 50),
	       MHRWParamsType(2, // step_size
                              FullRun?500:1, // n_sweep
			      FullRun?500:1, // n_therm
			      FullRun?4096:32 // n_run
		   ), 29329),
      results(logger),
      tasks(
	  &taskcdat, // constant data
	  &results, // results collector
	  logger, // the main logger object
	  NumRepeats, // num_repeats
	  1 // n_chunk
	  )
  {
  }
  TOMOGRAPHER_ENABLED_IF(UseBinningAnalysis)
  run_test()
    : logger(Tomographer::Logger::DEBUG),
      taskcdat(NormValueCalculator<double>(), HistogramParams(0, 500, 50), 5,
	       MHRWParamsType(2, // step_size
                              FullRun?500:1, // n_sweep
			      FullRun?500:1, // n_therm
			      FullRun?4096:32 // n_run
		   ), 29329),
      results(logger),
    tasks(
	&taskcdat, // constant data
	&results, // results collector
	logger, // the main logger object
	NumRepeats, // num_repeats
	1 // n_chunk
	)
  {
  }

  virtual ~run_test() { }
};
// static members:
template<bool UseBinningAnalysis, bool FullRun>
constexpr int run_test<UseBinningAnalysis,FullRun>::NumRepeats;



typedef run_test<false, false> run_test_simple_nofullrun;
typedef run_test<true, false> run_test_binning_nofullrun;


// -----------------------------------------------------------------------------
// test suites

BOOST_AUTO_TEST_SUITE(test_mhrw_valuehist_tasks)

BOOST_AUTO_TEST_CASE(REWRITE_ME)
{
  BOOST_CHECK( false && "These test cases rely too much on \"the whole thing must work\": "
               "so TODO: extract useful use cases." ) ;
}

BOOST_FIXTURE_TEST_CASE(simple_runtaskresults, run_test_simple_nofullrun)
{
  BOOST_CHECK(!results.isFinalized());

  tasks.run();

  BOOST_CHECK(results.isFinalized());

  BOOST_CHECK_EQUAL(results.numTasks(), NumRepeats);
  BOOST_CHECK_EQUAL(results.collectedRunTaskResults().size(), NumRepeats);
  auto taskresults = results.collectedRunTaskResults();
  for (std::size_t k = 0; k < (std::size_t)NumRepeats; ++k) {
    const OurResultsCollectorRunTaskResultType r =  * results.collectedRunTaskResult(k);
    BOOST_CHECK_CLOSE(r.histogram.params.min, taskresults[k]->histogram.params.min, tol_percent);
    BOOST_CHECK_CLOSE(r.histogram.params.max, taskresults[k]->histogram.params.max, tol_percent);
    BOOST_CHECK_EQUAL(r.histogram.params.num_bins, taskresults[k]->histogram.params.num_bins);
    BOOST_CHECK_CLOSE(r.acceptance_ratio, taskresults[k]->acceptance_ratio, tol_percent);
    MY_BOOST_CHECK_EIGEN_EQUAL(r.histogram.bins, taskresults[k]->histogram.bins, tol);
  }
  OurResultsCollector::FinalHistogramType finalhistogram = results.finalHistogram();
  BOOST_MESSAGE(Tomographer::histogramPrettyPrint(finalhistogram));
}

BOOST_FIXTURE_TEST_CASE(binning_runtaskresults, run_test_binning_nofullrun)
{
  BOOST_CHECK(!results.isFinalized());

  tasks.run();

  BOOST_CHECK(results.isFinalized());

  BOOST_CHECK_EQUAL(results.numTasks(), NumRepeats);
  BOOST_CHECK_EQUAL(results.collectedRunTaskResults().size(), NumRepeats);
  auto taskresults = results.collectedRunTaskResults();
  for (std::size_t k = 0; k < (std::size_t)NumRepeats; ++k) {
    const OurResultsCollectorRunTaskResultType r =  * results.collectedRunTaskResult(k);
    auto stats_coll_result_hist = r.stats_collector_result.hist;
    MY_BOOST_CHECK_EIGEN_EQUAL(stats_coll_result_hist.bins, taskresults[k]->stats_collector_result.hist.bins, tol);
  }
  OurResultsCollector::FinalHistogramType finalhistogram = results.finalHistogram();
  BOOST_MESSAGE(Tomographer::histogramPrettyPrint(finalhistogram));
}



BOOST_FIXTURE_TEST_CASE(simple_check_pattern, run_test<false>)
{
  tasks.run();
  
  std::string msg;
  { std::ostringstream ss; results.printHistogramCsv(ss, ","); msg = ss.str(); }
  BOOST_MESSAGE( msg ) ;

  const std::string hist = results.finalHistogram().prettyPrint(100);
  BOOST_MESSAGE("FINAL HISTOGRAM:\n" << hist);

  boost::test_tools::output_test_stream output(
      TOMOGRAPHER_TEST_PATTERNS_DIR "test_mhrw_valuehist_tasks/hist_simple.txt",
      true // true = match mode, false = write mode
      );
  dump_histogram_test(output, results.finalHistogram(), 2);
  BOOST_CHECK(output.match_pattern());
}
BOOST_FIXTURE_TEST_CASE(binning_check_pattern, run_test<true>)
{
  // for debugging. Beware, generates *LOTS* of output!  Also set NumRepeats=1.
  //logger.setLevel(Tomographer::Logger::LONGDEBUG);

  tasks.run();
  
  std::string msg;
  { std::ostringstream ss; results.printHistogramCsv(ss, ","); msg = ss.str(); }
  BOOST_MESSAGE( msg ) ;

  const std::string hist = results.finalHistogram().prettyPrint(100);
  BOOST_MESSAGE("FINAL HISTOGRAM:\n" << hist);

  boost::test_tools::output_test_stream output(
      TOMOGRAPHER_TEST_PATTERNS_DIR "test_mhrw_valuehist_tasks/hist_binning.txt",
      true // true = match mode, false = write mode
      );
  dump_histogram_test(output, results.finalHistogram(), 2);
  BOOST_CHECK(output.match_pattern());
}


BOOST_AUTO_TEST_SUITE_END()

