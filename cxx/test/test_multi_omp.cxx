
#include <cstdio>
#include <cstring>
#include <random>
#include <map>
#include <chrono>
#include <algorithm>
#include <string>

#include <omp.h>

#include "test_tomographer.h"

#include <Eigen/Core>

#include <tomographer/qit/matrq.h>
#include <tomographer/tools/loggers.h>
#include <tomographer/tomoproblem.h>
#include <tomographer/mhrwtasks.h>
#include <tomographer/dmmhrw.h>
#include <tomographer/multiprocomp.h>

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/test/output_test_stream.hpp>




// -----------------------------------------------------------------------------


typedef Tomographer::IndepMeasTomoProblem<Tomographer::QubitPaulisMatrQ> OurTomoProblem;

typedef Tomographer::FidelityToRefCalculator<OurTomoProblem> OurValueCalculator;

typedef Tomographer::UniformBinsHistogram<OurValueCalculator::ValueType> OurHistogramType;

struct MyCData : public Tomographer::MHRWTasks::CDataBase<> {
  MyCData(const OurTomoProblem & tomo_)
    : tomo(tomo_), vcalc(tomo_), histogram_params()
  {
  }

  OurTomoProblem tomo;
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
  inline Tomographer::DMStateSpaceLLHMHWalker<OurTomoProblem,Rng,LoggerType>
  createMHWalker(Rng & rng, LoggerType & log) const
  {
    return Tomographer::DMStateSpaceLLHMHWalker<OurTomoProblem,Rng,LoggerType>(
	tomo.matq.initMatrixType(),
	tomo,
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
  inline void collect_result(unsigned int /*task_no*/, const OurHistogramType& taskresult, const MyCData *)
  {
    finalhistogram.add_histogram(taskresult);
  }
  inline void runs_finished(unsigned int, const MyCData *)
  {
    finalhistogram.finalize();
  }
};


BOOST_AUTO_TEST_SUITE(test_multi_omp);

// =============================================================================

BOOST_AUTO_TEST_SUITE(OMPThreadSanitizerLogger);

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


BOOST_AUTO_TEST_SUITE_END();

// -----------------------------------------------------------------------------


BOOST_AUTO_TEST_CASE(dmmhrwtask)
{
  using namespace Tomographer;

  // use a strict logging mechanism (statically discard messages below WARNING). Change
  // this for debugging.
  Logger::BufferLogger buflog(Logger::DEBUG);
  Logger::MinimumSeverityLogger<Logger::BufferLogger, Logger::WARNING> logger(buflog);
  // for debugging, use e.g. this:
  //Logger::BufferLogger buflog(Logger::LONGDEBUG);
  //auto & logger = buflog;

  // some initializations

  logger.info("main()", "testing our integrator with Pauli meas. on a qubit ... ");

  QubitPaulisMatrQ qmq(2);
  
  OurTomoProblem dat(qmq);

  dat.Exn = qmq.initVectorParamListType(6);
  logger.debug("main()", [&](std::ostream& str) {
      str << "Exn.size = " << dat.Exn.rows() << " x " << dat.Exn.cols() << "\n";
    });
  dat.Exn <<
    0.5, 0.5,  0.707107,  0,
    0.5, 0.5, -0.707107,  0,
    0.5, 0.5,  0,         0.707107,
    0.5, 0.5,  0,        -0.707107,
    1,   0,    0,         0,
    0,   1,    0,         0
    ;
  dat.Nx = qmq.initFreqListType(6);

  // try to reproduce the nice "1qubit-test9-pureup-extreme-onlyupmeas" curve
  dat.Nx << 0, 0, 0, 0, 250, 0;

  dat.x_MLE << 1.0, 0, 0, 0; // pure up state
  dat.T_MLE <<
    1.0, 0,
    0,   0;

  // NOW, RUN THE MH TASKS:

  logger.debug("main()", "Starting to log stuff.");

  // ---------------

  MyCData taskcdat(dat);
  // seed for random number generator
  taskcdat.base_seed = 1000; // fixed seed for deterministic results in this test case.
  //taskcdat.base_seed = std::chrono::system_clock::now().time_since_epoch().count();
  // parameters for the fidelity histogram
  taskcdat.histogram_params = OurHistogramType::Params(0.98, 1.0, 50);
  // parameters of the random walk
  taskcdat.n_sweep = 20;
  taskcdat.n_therm = 100;
  taskcdat.n_run = 1000;
  taskcdat.step_size = 0.05;

  MyResultsCollector results;

  typedef Tomographer::MHRWTasks::MHRandomWalkTask<MyCData, std::mt19937> OurMHRWTask;

  MultiProc::OMP::makeTaskDispatcher<OurMHRWTask>(
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
      TOMOGRAPHER_TEST_PATTERNS_DIR "test_multi_omp/hist_dmmhrwtask.txt",
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


