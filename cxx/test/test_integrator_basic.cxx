
#include <cstdio>
#include <random>
#include <iostream>

#include <tomographer/qit/matrq.h>
#include <tomographer/loggers.h>
#include <tomographer/tomoproblem.h>
#include <tomographer/integrator.h>
#include <tomographer/dmintegrator.h>


int main()
{
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
  std::mt19937 rng(1544554); // seeded random number generator

  SimpleFoutLogger flog(stdout, Logger::LONGDEBUG); // just log normally to STDOUT

  QubitPaulisMatrQ::MatrixType start_T = qmq.initMatrixType();
  start_T << 1.0/sqrt(2.0), 0, 0, 1.0/sqrt(2.0);

  QubitPaulisMatrQ::MatrixType ref_T = qmq.initMatrixType();
  ref_T << 1.0, 0, 0, 0;

  typedef FidelityHistogramStatsCollector<QubitPaulisMatrQ,SimpleFoutLogger>
    OurFidStatsCollector;

  OurFidStatsCollector fidstats(0.98, 1.0, 50, ref_T, qmq, flog);

  typedef DMStateSpaceRandomWalk<std::mt19937,IndepMeasTomoProblem<QubitPaulisMatrQ>,OurFidStatsCollector,SimpleFoutLogger>
    MyRandomWalk;
  MyRandomWalk rwalk(20, 300, 5000, 0.04, start_T, dat, rng, fidstats, flog);

  rwalk.run();
  
  return 0;
}
