
#include <cstdio>
#include <random>
#include <iostream>
#include <vector>

// we want `eigen_assert()` to raise an `eigen_assert_exception` here
#include <tomographer/tools/eigen_assert_exception.h>

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

  //  std::cout << "about to create a SimpleFoutLogger object...\n";

  //  SimpleFoutLogger flog(stdout, Logger::INFO); // just log normally to STDOUT
  Tomographer::VacuumLogger flog;

  typedef decltype(flog) OurLogger;

  //  std::cout << "about to create a SimpleFoutLogger object... done\n";

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

BOOST_AUTO_TEST_SUITE_END()

