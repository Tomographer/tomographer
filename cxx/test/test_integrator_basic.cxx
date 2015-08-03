
#include <cstdio>
#include <random>
#include <iostream>
#include <vector>

#include "test_tomographer.h"

#include <tomographer/qit/matrq.h>
#include <tomographer/tools/loggers.h>
#include <tomographer/tomoproblem.h>
#include <tomographer/mhrw.h>
#include <tomographer/mhrwtasks.h>
#include <tomographer/dmmhrw.h>

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include <boost/test/output_test_stream.hpp>

//#include <boost/algorithm/string.hpp>



BOOST_AUTO_TEST_SUITE(test_integrator_basic)

BOOST_AUTO_TEST_CASE(test_integrator_basic1)
{
  //std::cout << "testing our integrator with Pauli meas. on a qubit ... \n";

  Tomographer::QubitPaulisMatrQ qmq(2);
  
  Tomographer::IndepMeasTomoProblem<Tomographer::QubitPaulisMatrQ> dat(qmq);

  dat.Exn = qmq.initVectorParamListType(6);
  //std::cout << "Exn.size = " << dat.Exn.rows() << " x " << dat.Exn.cols() << "\n";
  dat.Exn <<
    0.5, 0.5,  0.707107,  0,
    0.5, 0.5, -0.707107,  0,
    0.5, 0.5,  0,         0.707107,
    0.5, 0.5,  0,        -0.707107,
    1,   0,    0,         0,
    0,   1,    0,         0
    ;
  dat.Nx = qmq.initFreqListType(6);
  //  dat.Nx << 1500, 800, 300, 300, 10, 30;

  // try to reproduce the nice "1qubit-test9-pureup-extreme-onlyupmeas" curve
  dat.Nx << 0, 0, 0, 0, 250, 0;

  dat.rho_MLE << 1.0, 0, 0, 0;
  dat.T_MLE << 1.0, 0, 0, 0;
  dat.x_MLE << 1.0, 0, 0, 0; // pure up state

  // now, prepare the integrator.
  std::mt19937 rng(0); // seeded random number generator

  //  std::cout << "about to create a FileLogger object...\n";

  //  FileLogger flog(stdout, Logger::INFO); // just log normally to STDOUT
  Tomographer::Logger::VacuumLogger flog;

  typedef decltype(flog) OurLogger;

  //  std::cout << "about to create a FileLogger object... done\n";

  Tomographer::QubitPaulisMatrQ::MatrixType start_T = qmq.initMatrixType();
  start_T << 1.0/sqrt(2.0), 0, 0, 1.0/sqrt(2.0);

  //  QubitPaulisMatrQ::MatrixType ref_T = qmq.initMatrixType();
  //  ref_T << 1.0, 0, 0, 0;

  typedef Tomographer::IndepMeasTomoProblem<Tomographer::QubitPaulisMatrQ> OurTomoProblem;

  typedef Tomographer::FidelityToRefCalculator<OurTomoProblem> OurValueCalculator;
  typedef Tomographer::UniformBinsHistogram<typename OurValueCalculator::ValueType> OurHistogramType;
  typedef Tomographer::ValueHistogramMHRWStatsCollector<
    OurValueCalculator,
    OurLogger,
    OurHistogramType
    >
    OurValMHRWStatsCollector;

  typedef Tomographer::MultipleMHRWStatsCollectors<
    OurValMHRWStatsCollector,
    OurValMHRWStatsCollector
    >
    OurMultiMHRWStatsCollector;

  Tomographer::FidelityToRefCalculator<OurTomoProblem> fidcalc(dat);
  OurValMHRWStatsCollector fidstats(OurHistogramType::Params(0.98, 1.0, 50), fidcalc, flog);
  OurValMHRWStatsCollector fidstats2(OurHistogramType::Params(0.96, 0.98, 10), fidcalc, flog);

  OurMultiMHRWStatsCollector multistats(fidstats, fidstats2);

  typedef Tomographer::DMStateSpaceLLHMHWalker<OurTomoProblem,std::mt19937,OurLogger>
    MyMHWalker;

  MyMHWalker mhwalker(start_T, dat, rng, flog);

  //  std::cout << "About to create the randomwalk object ...\n";
  Tomographer::MHRandomWalk<std::mt19937,MyMHWalker,OurMultiMHRWStatsCollector,OurLogger,long>
    rwalk(20, 300, 5000, 0.05, mhwalker, multistats, rng, flog);

  //  std::cout << "About to run the randomwalk object ...\n";

  rwalk.run();

  // because we used a seeded RNG, we should get exactly reproducible results, i.e. the
  // exact same histograms.

  std::string hist1 = fidstats.histogram().pretty_print(80);
  BOOST_MESSAGE("FINAL HISTOGRAM(1):\n" << hist1);

  boost::test_tools::output_test_stream output1(TOMOGRAPHER_TEST_PATTERNS_DIR "test_integrator_basic/hist1.txt", true);
  output1 << hist1;
  BOOST_CHECK(output1.match_pattern());

  std::string hist2 = fidstats2.histogram().pretty_print(80);
  BOOST_MESSAGE("FINAL HISTOGRAM(2):\n" << hist2);

  boost::test_tools::output_test_stream output2(TOMOGRAPHER_TEST_PATTERNS_DIR "test_integrator_basic/hist2.txt", true);
  output2 << hist2;
  BOOST_CHECK(output2.match_pattern());
}





BOOST_AUTO_TEST_CASE(binning_analysis)
{
  typedef Tomographer::IndepMeasTomoProblem<Tomographer::QubitPaulisMatrQ> OurTomoProblem;

  typedef Tomographer::Logger::BufferLogger LoggerType;
  //typedef Tomographer::Logger::FileLogger LoggerType;

  // -----

  Tomographer::QubitPaulisMatrQ qmq(2);
  
  OurTomoProblem dat(qmq);

  dat.Exn = qmq.initVectorParamListType(2);
  dat.Exn <<
    1,   0,    0,         0,
    0,   1,    0,         0
    ;
  dat.Nx = qmq.initFreqListType(2);

  dat.Nx << 250, 0;

  dat.rho_MLE << 1.0, 0, 0, 0;
  dat.T_MLE << 1.0, 0, 0, 0;
  dat.x_MLE << 1.0, 0, 0, 0; // pure up state

  // --------

  LoggerType buflog(Tomographer::Logger::DEBUG);
  //LoggerType buflog(stderr, Tomographer::Logger::LONGDEBUG) ;

  typedef Tomographer::FidelityToRefCalculator<OurTomoProblem> OurValueCalculator;
  typedef Tomographer::ValueHistogramWithBinningMHRWStatsCollector<
    Tomographer::ValueHistogramWithBinningMHRWStatsCollectorParams<OurValueCalculator,
                                                                   int,
                                                                   float>,
    LoggerType
    > ValWBinningMHRWStatsCollectorType;

  typedef ValWBinningMHRWStatsCollectorType::HistogramParams HistogramParams;
  typedef Tomographer::DMStateSpaceLLHMHWalker<OurTomoProblem,std::mt19937,LoggerType> MHWalkerType;
  //  typedef Tomographer::MHRandomWalk<std::mt19937, MHWalkerType, ValWBinningMHRWStatsCollectorType,
  //				    LoggerType, int> MHRandomWalkType;
  OurValueCalculator fidcalc(dat);

  // N levels -> samples_size = 2^N
  const int num_levels = 5;

  ValWBinningMHRWStatsCollectorType vhist(HistogramParams(0.98f, 1.0f, 20), fidcalc, num_levels, buflog);

  std::mt19937 rng(0); // seeded rng, deterministic results

  Tomographer::QubitPaulisMatrQ::MatrixType start_T = qmq.initMatrixType();
  start_T << 1.0/sqrt(2.0), 0, 0, 1.0/sqrt(2.0);

  MHWalkerType mhwalker(start_T, dat, rng, buflog);

  //  std::cout << "About to create the randomwalk object ...\n";
  Tomographer::MHRandomWalk<std::mt19937,MHWalkerType,ValWBinningMHRWStatsCollectorType,LoggerType,unsigned long>
    rwalk(20, 300, 8192, 0.05, mhwalker, vhist, rng, buflog);

  rwalk.run();

  BOOST_MESSAGE(buflog.get_contents());

  const ValWBinningMHRWStatsCollectorType::Result & result = vhist.getResult();

  // all error bars should have converged with these settings
  MY_BOOST_CHECK_EIGEN_EQUAL(
      result.converged_status,
      Eigen::ArrayXi::Constant(result.hist.num_bins(),
			       ValWBinningMHRWStatsCollectorType::BinningAnalysisParamsType::CONVERGED),
      tol_f
      );

  std::string conv_analysis = result.dump_convergence_analysis();
  BOOST_MESSAGE("Convergence Analysis:\n" << conv_analysis);

  boost::test_tools::output_test_stream output_conv_analysis(
      TOMOGRAPHER_TEST_PATTERNS_DIR "test_integrator_basic/binning_convergence_analysis.txt",
      true // true = compare mode, false = write mode
      );
  output_conv_analysis << conv_analysis;
  BOOST_CHECK(output_conv_analysis.match_pattern());
}



BOOST_AUTO_TEST_SUITE_END()

