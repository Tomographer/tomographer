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

#include <cstdio>
#include <cstring>

#include <random>
#include <map>
#include <chrono>
#include <algorithm>
#include <string>

#include <boost/math/constants/constants.hpp>

// definitions for Tomographer test framework -- this must be included before any
// <Eigen/...> or <tomographer2/...> header
#include "test_tomographer.h"

#include <tomographer2/multiprocomp.h>
#include <tomographer2/tools/loggers.h>
#include <tomographer2/mhrwtasks.h>
#include <tomographer2/densedm/dmtypes.h>
#include <tomographer2/densedm/indepmeasllh.h>
#include <tomographer2/densedm/tspacefigofmerit.h>
#include <tomographer2/densedm/tspacellhwalker.h>


// -----------------------------------------------------------------------------

typedef Tomographer::DenseDM::DMTypes<2> DMTypes;
typedef Tomographer::DenseDM::IndepMeasLLH<DMTypes> DenseLLH;
typedef Tomographer::DenseDM::TSpace::FidelityToRefCalculator<DMTypes> OurValueCalculator;

typedef Tomographer::UniformBinsHistogram<OurValueCalculator::ValueType> OurHistogramType;

struct MyCData : public Tomographer::MHRWTasks::CDataBase<> {
  MyCData(const DenseLLH & llh_, const typename DMTypes::MatrixType & ref_T,
	  OurHistogramType::Params p, MHRWParamsType mhrwp, int base_seed)
    : Tomographer::MHRWTasks::CDataBase<>(mhrwp, base_seed),
      llh(llh_), vcalc(ref_T), histogram_params(p)
  {
  }

  DenseLLH llh;
  OurValueCalculator vcalc;

  OurHistogramType::Params histogram_params;

  typedef OurHistogramType MHRWStatsCollectorResultType;

  template<typename LoggerType>
  inline Tomographer::ValueHistogramMHRWStatsCollector<OurValueCalculator,LoggerType,OurHistogramType>
  createStatsCollector(LoggerType & logger) const
  {
    return Tomographer::ValueHistogramMHRWStatsCollector<OurValueCalculator,LoggerType,OurHistogramType>(
	histogram_params,
	vcalc,
	logger
	);
  }

  template<typename Rng, typename LoggerType>
  inline Tomographer::DenseDM::TSpace::LLHMHWalker<DenseLLH,Rng,LoggerType>
  createMHWalker(Rng & rng, LoggerType & log) const
  {
    return Tomographer::DenseDM::TSpace::LLHMHWalker<DenseLLH,Rng,LoggerType>(
	llh.dmt.initMatrixType(),
	llh,
	rng,
	log
	);
  }

};

struct MyResultsCollector {
  Tomographer::AveragedHistogram<OurHistogramType, double> finalhistogram;

  MyResultsCollector()
    : finalhistogram(OurHistogramType::Params())
  {
  }

  inline void init(unsigned int /*num_total_runs*/, unsigned int /*n_chunk*/,
		   const MyCData * pcdata)
  {
    finalhistogram.reset(pcdata->histogram_params);
  }
  template<typename MyStatsCollectorResultType, typename IntType>
  inline void collect_result(unsigned int /*task_no*/,
			     const Tomographer::MHRWTasks::MHRandomWalkTaskResult<MyStatsCollectorResultType,
										  IntType, double> &
			     taskresult,
			     const MyCData *)
  {
    finalhistogram.add_histogram(taskresult.stats_collector_result);
  }
  inline void runs_finished(unsigned int, const MyCData *)
  {
    finalhistogram.finalize();
  }
};




BOOST_AUTO_TEST_SUITE(test_multiprocomp)

// =============================================================================

BOOST_AUTO_TEST_SUITE(OMPThreadSanitizerLogger)

BOOST_AUTO_TEST_CASE(relays_logs)
{
  Tomographer::Logger::BufferLogger buflog(Tomographer::Logger::DEBUG);
  Tomographer::MultiProc::OMP::ThreadSanitizerLogger<Tomographer::Logger::BufferLogger> testtasklogger(buflog);
  testtasklogger.longdebug("origin", "longdebug level");
  testtasklogger.debug("origin", "debug level");
  testtasklogger.info("origin", "info level");
  testtasklogger.warning("origin", "warning level");
  testtasklogger.error("origin", "error level");
  
  BOOST_CHECK_EQUAL(
      buflog.get_contents(),
      "[origin] debug level\n"
      "[origin] info level\n"
      "[origin] warning level\n"
      "[origin] error level\n"
      );
}

BOOST_AUTO_TEST_CASE(fixes_level)
{
  Tomographer::Logger::BufferLogger buflog(Tomographer::Logger::LONGDEBUG);

  Tomographer::MultiProc::OMP::ThreadSanitizerLogger<Tomographer::Logger::BufferLogger> testtasklogger(buflog);

  // this should NOT have any effect for testtasklogger, because OMP::ThreadSanitizerLogger
  // should fix the level at construction time for thread-safety/consistency reasons.
  buflog.setLevel(Tomographer::Logger::WARNING);

  testtasklogger.longdebug("origin", "test message");
  
  BOOST_CHECK_EQUAL(buflog.get_contents(), "[origin] test message\n");
}

BOOST_AUTO_TEST_CASE(parallel)
{
  // 
  // Make sure that the output of the log is not mangled. We sort the lines because of
  // course the order is undefined, but each line should be intact (thanks to
  // OMP::ThreadSanitizerLogger's wrapping into "#pragma omp critical" sections).
  //

  Tomographer::Logger::BufferLogger buflog(Tomographer::Logger::LONGDEBUG);
  
#pragma omp parallel shared(buflog)
  {
    Tomographer::MultiProc::OMP::ThreadSanitizerLogger<Tomographer::Logger::BufferLogger> testtasklogger(buflog);
    testtasklogger.longdebug("main()", "test task logger from core #%06d of %06d",
			     omp_get_thread_num(), omp_get_num_threads());
  }

  std::string buflog_str = buflog.get_contents();

  BOOST_MESSAGE("buflog contents: \n" << buflog_str);
  BOOST_CHECK(buflog_str.size());

  std::vector<std::string> lines;
  std::stringstream ss(buflog_str);
  std::string line;
  while (std::getline(ss, line, '\n')) {
    lines.push_back(line);
  }

  std::sort(lines.begin(), lines.end()); // std::string's operator< does lexicographical comparision

  std::ostringstream sorted_stream;
  std::copy(lines.begin(), lines.end(), std::ostream_iterator<std::string>(sorted_stream, "\n"));
  std::string sorted = sorted_stream.str();


  std::string reference_str;
  for (int k = 0; k < (int)lines.size(); ++k) {
    reference_str += Tomographer::Tools::fmts("[main()] test task logger from core #%06d of %06d\n",
					      k, (int)lines.size());
  }

  BOOST_CHECK_EQUAL(sorted, reference_str);
}


BOOST_AUTO_TEST_SUITE_END()


// -----------------------------------------------------------------------------


BOOST_AUTO_TEST_CASE(dmmhrwtask)
{
  // use a strict logging mechanism (statically discard messages below WARNING). Change
  // this for debugging.
  Tomographer::Logger::BufferLogger buflog(Tomographer::Logger::DEBUG);
  Tomographer::Logger::MinimumSeverityLogger<Tomographer::Logger::BufferLogger,
					     Tomographer::Logger::WARNING>  logger(buflog);
  // for debugging, use e.g. this:
  //Tomographer::Logger::BufferLogger buflog(Tomographer::Logger::LONGDEBUG);
  //auto & logger = buflog;

  // some initializations

  logger.info("main()", "testing our integrator with Pauli meas. on a qubit ... ");

  DMTypes dmt;
  DenseLLH llh(dmt);

  typename DenseLLH::VectorParamListType Exn(6,dmt.dim2());
  Exn <<
    0.5, 0.5,  0.707107,  0,
    0.5, 0.5, -0.707107,  0,
    0.5, 0.5,  0,         0.707107,
    0.5, 0.5,  0,        -0.707107,
    1,   0,    0,         0,
    0,   1,    0,         0
    ;
  typename DenseLLH::FreqListType Nx(6);
  // try to reproduce the nice "1qubit-test9-pureup-extreme-onlyupmeas" curve
  Nx << 0, 0, 0, 0, 250, 0;

  llh.setMeas(Exn, Nx);

  // NOW, RUN THE MH TASKS:

  logger.debug("main()", "Starting to log stuff.");

  // ---------------

  DMTypes::MatrixType ref_T(dmt.initMatrixType());
  ref_T <<
    1.0, 0,
    0,   0;

  // seed for random number generator
  int base_seed = 1000; // fixed seed for deterministic results in this test case.
  //taskcdat.base_seed = std::chrono::system_clock::now().time_since_epoch().count();

  // parameters of the random walk
  MyCData taskcdat(llh, ref_T,
		   OurHistogramType::Params(0.98, 1.0, 50),  // parameters for the fidelity histogram
		   MyCData::MHRWParamsType(20, 0.05, 100, 1000),
		   base_seed);

  MyResultsCollector results;

  typedef Tomographer::MHRWTasks::MHRandomWalkTask<MyCData, std::mt19937> OurMHRWTask;

  Tomographer::MultiProc::OMP::makeTaskDispatcher<OurMHRWTask>(
      &taskcdat, &results, logger, 64 /* num_runs */, 1 /* n_chunk */
      ).run();

  logger.longdebug("main()", [&](std::ostream& str) {
      str << "Integration finished.";
    });

  logger.info("main()", [&](std::ostream & str) {
      str << "FINAL HISTOGRAM\n" << results.finalhistogram.pretty_print(120) << "\n";
    });

  BOOST_MESSAGE(buflog.get_contents());

  const std::string hist = results.finalhistogram.pretty_print(100);
  BOOST_MESSAGE("FINAL HISTOGRAM:\n" << hist);

  boost::test_tools::output_test_stream output(
      TOMOGRAPHER_TEST_PATTERNS_DIR "test_multiprocomp/hist_dmmhrwtask.txt",
      true // true = match mode, false = write mode
      );
  dump_histogram_test(output, results.finalhistogram, 2);
  // output
  //   << "BINS = \n"
  //   << std::setprecision(3)
  //   << results.finalhistogram.bins
  //   << "\n"
  //   << "ERROR BARS = \n"
  //   << std::setprecision(3)
  //   << results.finalhistogram.delta
  //   << "\n";
  BOOST_CHECK(output.match_pattern());
}



// =============================================================================

BOOST_AUTO_TEST_SUITE_END();


