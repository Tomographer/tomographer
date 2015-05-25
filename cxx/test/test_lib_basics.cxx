
#undef NDEBUG

#include <iostream>
#include <stdexcept>

// we want `eigen_assert()` to raise an `eigen_assert_exception` here
#include <tomographer/tools/eigen_assert_exception.h>

#include <tomographer/qit/matrq.h>
#include <tomographer/tomoproblem.h>

#include <Eigen/Core>
#include <Eigen/Eigenvalues>

#define BOOST_TEST_MODULE test_lib_basics
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>


const double tol = 1e-12;


struct test_matrq_fixture {
  test_matrq_fixture()
  {
  }
  ~test_matrq_fixture()
  {
  }
  template<typename TheMatrQ> void test_matrq(const int dim, const int npovms)
  {
    // has RealScalar and ComplexScalar types
    typename TheMatrQ::RealScalar a = 1.0;
    typename TheMatrQ::ComplexScalar z(3.0, 4.0);
    
    BOOST_CHECK_CLOSE(a, 1.0, tol);
    BOOST_CHECK_CLOSE(std::abs(z), 5.0, tol);
    
    TheMatrQ matq(dim);

    // has dim() property
    BOOST_CHECK_EQUAL(matq.dim(), dim);

    // if has fixed dim, make sure that if we attempt to construct a bad dimension,
    // something explodes
    if (TheMatrQ::FixedDim != Eigen::Dynamic) {
      BOOST_CHECK_THROW(
     	  TheMatrQ badmatq(dim+1),
     	  ::Tomographer::Tools::eigen_assert_exception
     	  );
    }
    
    // matrix type
    BOOST_CHECKPOINT("Testing MatrixType");

    typename TheMatrQ::MatrixType rho = matq.initMatrixType();
    BOOST_CHECK_EQUAL(rho.cols(), dim);
    BOOST_CHECK_EQUAL(rho.rows(), dim);

    // initializes to zero
    BOOST_CHECK_CLOSE(rho.norm(), 0, tol);

    // rho is a superposition of the two first basis states
    rho(0,0) = typename TheMatrQ::ComplexScalar(.5, 0.0);
    rho(0,1) = typename TheMatrQ::ComplexScalar(0.0, .5);
    rho(1,0) = typename TheMatrQ::ComplexScalar(0.0, -.5);
    rho(1,1) = typename TheMatrQ::ComplexScalar(.5, 0.0);
    BOOST_CHECK_CLOSE(rho(0,1).imag(), .5, tol);
    BOOST_CHECK_CLOSE(rho.trace().real(), 1.0, tol);
    BOOST_CHECK_CLOSE(rho.trace().imag(), 0.0, tol);
    BOOST_CHECK_CLOSE(rho.eigenvalues().real().maxCoeff(), 1.0, tol);

    // vector param type
    BOOST_CHECKPOINT("Testing VectorParamType");

    typename TheMatrQ::VectorParamType x = matq.initVectorParamType();
    BOOST_CHECK_EQUAL(x.cols(), 1);
    BOOST_CHECK_EQUAL(x.rows(), dim*dim);
    BOOST_CHECK_CLOSE(x.norm(), 0, tol);
    
    // vector param Ndof type
    BOOST_CHECKPOINT("Testing VectorParamNdofType");
    typename TheMatrQ::VectorParamNdofType x2 = matq.initVectorParamNdofType();
    BOOST_CHECK_EQUAL(x2.cols(), 1);
    BOOST_CHECK_EQUAL(x2.rows(), dim*dim-1);
    BOOST_CHECK_CLOSE(x2.norm(), 0, tol);

    // vector param list type
    BOOST_CHECKPOINT("Testing VectorParamListType");
    typename TheMatrQ::VectorParamListType xl = matq.initVectorParamListType(npovms);
    BOOST_CHECK_EQUAL(xl.cols(), dim*dim);
    BOOST_CHECK_EQUAL(xl.rows(), npovms);
    BOOST_CHECK_CLOSE(xl.norm(), 0, tol);

    // frequency list type
    BOOST_CHECKPOINT("Testing FreqListType");
    typename TheMatrQ::FreqListType fl = matq.initFreqListType(npovms);
    BOOST_CHECK_EQUAL(fl.cols(), 1);
    BOOST_CHECK_EQUAL(fl.rows(), npovms);
    BOOST_CHECK_EQUAL(fl.abs().maxCoeff(), 0); // should be integral type
  }
};

BOOST_FIXTURE_TEST_CASE(default_matrq, test_matrq_fixture)
{
  BOOST_CHECKPOINT("Testing DefaultMatrQ(5,100)");
  test_matrq<Tomographer::DefaultMatrQ>(5, 100);
  BOOST_CHECKPOINT("Testing DefaultMatrQ(2,50)");
  test_matrq<Tomographer::DefaultMatrQ>(2, 50);
  BOOST_CHECKPOINT("Testing QubitParulisMatrQ(2,6)");
  test_matrq<Tomographer::QubitPaulisMatrQ>(2, 6);
}


/*

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

*/
