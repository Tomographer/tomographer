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

#include "test_mh_random_walk_common.h" // our test-case random walk


// -----------------------------------------------------------------------------
// fixture(s)

struct NormValueCalculator
{
  typedef double ValueType;

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

  template<typename Rng, typename LoggerType>
  inline TestLatticeMHRWGaussPeak<double,Rng>  createMHWalker(Rng & rng, LoggerType & /*logger*/) const
  {
    // 100x100 lattice
    return TestLatticeMHRWGaussPeak<double,Rng>(
	100*Eigen::Vector2i::Ones(), Eigen::Matrix2d::Identity(), 20*20, Eigen::Vector2d::Zero(2),
	rng()
	);
  }
};




template<bool UseBinningAnalysis>
struct run_test {

  typedef NormValueCalculator ValueCalculator;
    
  typedef Tomographer::Logger::BufferLogger LoggerType;

  typedef std::mt19937 Rng;
    
  typedef OurCDataTask<Tomographer::MHRWTasks::ValueHistogramTasks::CDataBase<ValueCalculator,
									      UseBinningAnalysis> > OurCDataSimple;
  typedef typename OurCDataSimple::HistogramParams HistogramParams;

  typedef Tomographer::MHRWTasks::MHRandomWalkTask<OurCDataSimple, Rng>  OurMHRandomWalkTask;
  typedef typename OurCDataSimple::template ResultsCollectorType<LoggerType>::type OurResultsCollector;

  typedef Tomographer::MHRWParams<int,double> MHRWParamsType;
    
  typedef Tomographer::MultiProc::OMP::TaskDispatcher<OurMHRandomWalkTask, OurCDataSimple, OurResultsCollector,
						      LoggerType, int>
    TaskDispatcherType;

  LoggerType logger;
  OurCDataSimple taskcdat;
  OurResultsCollector results;

  TaskDispatcherType tasks;

  TOMOGRAPHER_ENABLED_IF(!UseBinningAnalysis)
  run_test()
    : logger(Tomographer::Logger::DEBUG),
      taskcdat(NormValueCalculator(), HistogramParams(0, 100, 100),
	       MHRWParamsType(50, // n_sweep
			      2, // step_size
			      500, // n_therm
			      25000 // n_run
		   ), 29329),
      results(logger),
      tasks(Tomographer::MultiProc::OMP::makeTaskDispatcher<OurMHRandomWalkTask>(
	&taskcdat, // constant data
	&results, // results collector
	logger, // the main logger object
	6, // num_repeats
	1 // n_chunk
		))
  {
  }
  TOMOGRAPHER_ENABLED_IF(UseBinningAnalysis)
  run_test()
    : logger(Tomographer::Logger::DEBUG),
      taskcdat(NormValueCalculator(), HistogramParams(0, 100, 100), 5,
	       MHRWParamsType(50, // n_sweep
			      2, // step_size
			      500, // n_therm
			      25000 // n_run
		   ), 29329),
      results(logger),
      tasks(Tomographer::MultiProc::OMP::makeTaskDispatcher<OurMHRandomWalkTask>(
	&taskcdat, // constant data
	&results, // results collector
	logger, // the main logger object
	6, // num_repeats
	1 // n_chunk
		))
  {
  }

  virtual ~run_test() { }
};





// -----------------------------------------------------------------------------
// test suites

BOOST_AUTO_TEST_SUITE(test_mhrw_valuehist_tasks)

BOOST_FIXTURE_TEST_CASE(simple, run_test<false>)
{
  tasks.run();
  
  BOOST_MESSAGE( logger.get_contents() );

  std::string msg;
  { std::ostringstream ss; results.print_histogram_csv(ss); msg = ss.str(); }
  BOOST_MESSAGE( msg ) ;
}
BOOST_FIXTURE_TEST_CASE(binning, run_test<true>)
{
  //  tasks.run();
  
  //  BOOST_MESSAGE( logger.get_contents() );

  //  std::string msg;
  //  { std::ostringstream ss; results.print_histogram_csv(ss); msg = ss.str(); }
  //  BOOST_MESSAGE( msg ) ;
}

BOOST_AUTO_TEST_SUITE_END()

