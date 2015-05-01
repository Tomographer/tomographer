
#include <cstdio>
#include <random>
#include <chrono>
#include <iostream>

#include <tomographer/qit/matrq.h>
#include <tomographer/tools/loggers.h>
#include <tomographer/tomoproblem.h>
#include <tomographer/integrator.h>
#include <tomographer/dmllhintegrator.h>

int main()
{
  using namespace Tomographer;

  std::cout << "testing our integrator with Pauli meas. on a qubit ... \n";

  QubitPaulisMatrQ qmq(2);
  
  IndepMeasTomoProblem<QubitPaulisMatrQ> dat(qmq);

  dat.Exn = qmq.initVectorParamListType(6);
  std::cout << "Exn.size = " << dat.Exn.rows() << " x " << dat.Exn.cols() << "\n";
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

  dat.x_MLE << 1.0, 0, 0, 0; // pure up state


  // now, prepare the integrator.
  std::mt19937 rng(std::chrono::system_clock::now().time_since_epoch().count()); // seeded random number generator

  //  std::cout << "about to create a SimpleFoutLogger object...\n";

  SimpleFoutLogger flog(stdout, Logger::LONGDEBUG); // just log normally to STDOUT

  //  std::cout << "about to create a SimpleFoutLogger object... done\n";

  QubitPaulisMatrQ::MatrixType start_T = qmq.initMatrixType();
  start_T << 1.0/sqrt(2.0), 0, 0, 1.0/sqrt(2.0);

  QubitPaulisMatrQ::MatrixType ref_T = qmq.initMatrixType();
  ref_T << 1.0, 0, 0, 0;

  typedef FidelityHistogramMHRWStatsCollector<QubitPaulisMatrQ,double,SimpleFoutLogger>
    OurFidMHRWStatsCollector;

  typedef MultipleMHRWStatsCollectors<OurFidMHRWStatsCollector,OurFidMHRWStatsCollector>
    OurMultiMHRWStatsCollector;

  OurFidMHRWStatsCollector fidstats(0.98, 1.0, 50, ref_T, qmq, flog);
  OurFidMHRWStatsCollector fidstats2(0.96, 0.98, 10, ref_T, qmq, flog);

  OurMultiMHRWStatsCollector multistats(fidstats, fidstats2);

  typedef DMStateSpaceLLHRandomWalk<IndepMeasTomoProblem<QubitPaulisMatrQ>,std::mt19937,
				    OurMultiMHRWStatsCollector,SimpleFoutLogger>
    MyRandomWalk;

  std::cout << "About to create the randomwalk object ...\n";

  //  MyRandomWalk rwalk(20, 300, 5000, 0.05, start_T, dat, rng, fidstats, flog);
  MyRandomWalk rwalk(4, 20, 50, 0.06, Eigen::Matrix2cd::Zero(), dat, rng, multistats, flog);

  std::cout << "About to run the randomwalk object ...\n";

  rwalk.run();

  std::cout << "FINAL HISTOGRAM (1)\n" << fidstats.histogram().pretty_print() << "\n";
  std::cout << "FINAL HISTOGRAM (2)\n" << fidstats2.histogram().pretty_print() << "\n";
  
  return 0;
}
