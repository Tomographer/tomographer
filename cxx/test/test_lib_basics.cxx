
#include <iostream>

#include <tomographer/qit/matrq.h>
#include <tomographer/tomoproblem.h>

int main()
{
  using namespace Tomographer;

  {
    std::cout << "testing DefaultMatrQ ...\n";
    
    DefaultMatrQ::RealScalar a = 1.0;
    std::cout << "a = " << a << "\n";
    
    DefaultMatrQ matrq(5);
    
    DefaultMatrQ::MatrixType rho = matrq.initMatrixType();
    
    rho(1,4) = DefaultMatrQ::ComplexScalar(0.0, 1.0);
    
    std::cout << "rho = \n" << rho << "\n";
  }

  // -------------------------------------------------------

  {
    std::cout << "testing QubitPaulisMatrQ ...\n";
    
    QubitPaulisMatrQ qmq(2);
    
    QubitPaulisMatrQ::VectorParamType a = qmq.initVectorParamType();

    std::cout << "a = " << a << "\n";
  
  }

  // -------------------------------------------------------

  {
    std::cout << "testing IndepMeasTomoProblem ... \n";

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

    QubitPaulisMatrQ::VectorParamType x = qmq.initVectorParamType();
    x << 0.5, 0.5, 0, 0; // maximally mixed state

    QubitPaulisMatrQ::RealScalar value = dat.calc_llh(x);

    std::cout << "llh @ mixed state = " << value << "\n";

  }

  return 0;
}
