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
  inline TestLatticeMHRWGaussPeak<int,Rng>  createMHWalker(Rng & rng, LoggerType & /*logger*/) const
  {
    // 100x100 lattice
    return TestLatticeMHRWGaussPeak<int,Rng>(
	100*Eigen::Vector2i::Ones(), Eigen::Matrix2i::Identity(), 30, Eigen::Vector2i::Zero(2),
	rng()
	);
  }
};

// -----------------------------------------------------------------------------
// test suites


BOOST_AUTO_TEST_SUITE(test_mhrw_valuehist_tasks)

BOOST_AUTO_TEST_CASE(simple)
{
  typedef NormValueCalculator ValueCalculator;

  typedef Tomographer::Logger::BufferLogger LoggerType;
  LoggerType logger(Tomographer::Logger::DEBUG);

  typedef std::mt19937 Rng;

  const int ntherm = 500;
  const int nrun = 10000;
  const int nsweep = 100;
  const int step_size = 2;

  typedef OurCDataTask<Tomographer::MHRWTasks::ValueHistogramTasks::CDataBase<ValueCalculator, false> > OurCDataSimple;
  typedef OurCDataSimple::HistogramParams HistogramParams;
  OurCDataSimple taskcdat(NormValueCalculator(), HistogramParams(0, 100, 100));
  taskcdat.n_therm = ntherm;
  taskcdat.n_sweep = nsweep;
  taskcdat.n_run = nrun;
  taskcdat.step_size = step_size;
  taskcdat.base_seed = 29329;

  typedef Tomographer::MHRWTasks::MHRandomWalkTask<OurCDataSimple, Rng>  OurMHRandomWalkTask;

  OurCDataSimple::ResultsCollectorType<LoggerType>::type results(logger);

  const int n_repeat = 2;

  auto tasks = Tomographer::MultiProc::OMP::makeTaskDispatcher<OurMHRandomWalkTask>(
      &taskcdat, // constant data
      &results, // results collector
      logger, // the main logger object
      n_repeat, // num_repeats
      1 // n_chunk
      );

  tasks.run();

  BOOST_MESSAGE( logger.get_contents() );

  std::string msg;
  { std::ostringstream ss; results.print_histogram_csv(ss); msg = ss.str(); }
  BOOST_MESSAGE( msg ) ;

  //  ........ perform a check ! ............
  BOOST_CHECK( false ) ; // TODO: WRITE ME
}
BOOST_AUTO_TEST_CASE(binning)
{
}

BOOST_AUTO_TEST_SUITE_END()

