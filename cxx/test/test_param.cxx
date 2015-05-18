
#include <iostream>

#include <tomographer/qit/matrq.h>
#include <tomographer/qit/param_herm_x.h>
#include <tomographer/qit/param_rho_a.h>

#include <Eigen/Core>


template<typename MatrQ>
void test_params(MatrQ matq, const typename MatrQ::MatrixType & rho)
{
  std::cout << "-----------------------------------------------\n";
  std::cout << "rho = \n" << rho << "\n";
  // test param herm rho
  {
    typename MatrQ::VectorParamType x = matq.initVectorParamType();
    Tomographer::param_herm_to_x(x, rho);
    std::cout << " --> x = " << x.transpose() << "\n";
    // and convert back to rho
    typename MatrQ::MatrixType rhoback1 = matq.initMatrixType();
    Tomographer::param_x_to_herm(rhoback1, x);
    std::cout << " --> and back to rho = \n" << rhoback1 << "\n";
  }
  // test param rho a
  {
    Tomographer::ParamRhoA<MatrQ> param(matq);
    // display generalized Gell-Mann matrices
    for (std::size_t l = 0; l < matq.ndof(); ++l) {
      std::cout << "\tlambda["<<l<<"] = \n" << param.getLambda(l) << "\n";
    }
    // do checks that all HS inner products are correct
    Eigen::MatrixXcd inner_prods = Eigen::MatrixXcd::Zero(matq.ndof(), matq.ndof());
    for (std::size_t i = 0; i < matq.ndof(); ++i) {
      assert( (param.getLambda(i).adjoint() - param.getLambda(i)).norm() < 1e-8 );
      for (std::size_t j = 0; j < matq.ndof(); ++j) {
	inner_prods(i,j) = (param.getLambda(i).adjoint() * param.getLambda(j)).trace();
      }
    }
    std::cout << "\tMatrix of inner products tr(A'*B) = \n" << inner_prods << "\n";
    // ---
    typename MatrQ::VectorParamNdofType a = matq.initVectorParamNdofType();
    param.rhoToA(a, rho);
    std::cout << " --> a = " << a.transpose() << "\n";
    // and convert back to rho
    typename MatrQ::MatrixType rhoback2 = matq.initMatrixType();
    param.aToRho(rhoback2, a);
    std::cout << " --> and back to rho = \n" << rhoback2 << "\n";
  }
}


int main()
{
  std::cout << "testing param_herm_rho ...\n";

  // qubit
  {
    typedef Tomographer::QubitPaulisMatrQ OurMatrQ;
    OurMatrQ matq(2);
    OurMatrQ::MatrixType rho = matq.initMatrixType();
    rho = OurMatrQ::MatrixType::Identity(matq.dim(), matq.dim()) / 2;
    test_params(matq, rho);
  }
  
  // 3-level system --> "normal" Gell-Mann matrices
  {
    typedef Tomographer::DefaultMatrQ OurMatrQ;
    OurMatrQ matq(3);
    OurMatrQ::MatrixType rho = matq.initMatrixType();
    typedef OurMatrQ::ComplexScalar CD;
    rho.setZero();
    rho(0,0) = 0.2;
    rho(0,1) = CD(0.1, 0.1);
    rho(1,0) = CD(0.1, -0.1);
    rho(1,1) = 0.1;
    rho(2,2) = 0.7;
    test_params(matq, rho);
  }

  return 0;
}
