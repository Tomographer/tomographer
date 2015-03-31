
#include <cstdio>
#include <random>
#include <iostream>

#include "lib/matrq.h"
#include "lib/tomoproblem.h"
#include "lib/integrator.h"
#include "lib/dmintegrator.h"


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
  dat.Nx << 1500, 800, 300, 300, 10, 30;

  dat.x_MLE << 1.0, 0, 0, 0; // pure up state


  // now, prepare the integrator.
  std::mt19937 rng(1544554); // seeded random number generator

  SimpleFoutLogger flog(stdout);

  QubitPaulisMatrQ::MatrixType start_T = qmq.initMatrixType();
  start_T << 1.0/sqrt(2.0), 0, 0, 1.0/sqrt(2.0);

  typedef DMStateSpaceRandomWalk<std::mt19937,IndepMeasTomoProblem<QubitPaulisMatrQ>,SimpleFoutLogger>
    MyRandomWalk;
  MyRandomWalk rwalk(1, 20, 50, 0.05, start_T, dat, rng, flog);

  MetropolisWalkerBase<MyRandomWalk>::run(rwalk);
  
  return 0;
}
