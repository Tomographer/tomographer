
#include <iostream>
#include <iomanip>
#include <stdexcept>

#include "test_tomographer.h"

#include <tomographer/qit/matrq.h>
#include <tomographer/qit/random_unitary.h>
#include <tomographer/qit/pos_semidef_util.h>
#include <tomographer/tomoproblem.h>

#include <Eigen/Core>
#include <Eigen/Eigenvalues>



BOOST_AUTO_TEST_SUITE(test_lib_basics);
// =============================================================================


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
    
    BOOST_CHECK_CLOSE(a, 1.0, tol_percent);
    BOOST_CHECK_CLOSE(std::abs(z), 5.0, tol_percent);
    
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
    BOOST_CHECK_CLOSE(rho.norm(), 0, tol_percent);

    // rho is a superposition of the two first basis states
    rho(0,0) = typename TheMatrQ::ComplexScalar(.5, 0.0);
    rho(0,1) = typename TheMatrQ::ComplexScalar(0.0, .5);
    rho(1,0) = typename TheMatrQ::ComplexScalar(0.0, -.5);
    rho(1,1) = typename TheMatrQ::ComplexScalar(.5, 0.0);
    BOOST_CHECK_CLOSE(rho(0,1).imag(), .5, tol_percent);
    BOOST_CHECK_CLOSE(rho.trace().real(), 1.0, tol_percent);
    BOOST_CHECK_CLOSE(rho.trace().imag(), 0.0, tol_percent);
    BOOST_CHECK_CLOSE(rho.eigenvalues().real().maxCoeff(), 1.0, tol_percent);

    // vector param type
    BOOST_CHECKPOINT("Testing VectorParamType");

    typename TheMatrQ::VectorParamType x = matq.initVectorParamType();
    BOOST_CHECK_EQUAL(x.cols(), 1);
    BOOST_CHECK_EQUAL(x.rows(), dim*dim);
    BOOST_CHECK_CLOSE(x.norm(), 0, tol_percent);
    
    // vector param Ndof type
    BOOST_CHECKPOINT("Testing VectorParamNdofType");
    typename TheMatrQ::VectorParamNdofType x2 = matq.initVectorParamNdofType();
    BOOST_CHECK_EQUAL(x2.cols(), 1);
    BOOST_CHECK_EQUAL(x2.rows(), dim*dim-1);
    BOOST_CHECK_CLOSE(x2.norm(), 0, tol_percent);

    // vector param list type
    BOOST_CHECKPOINT("Testing VectorParamListType");
    typename TheMatrQ::VectorParamListType xl = matq.initVectorParamListType(npovms);
    BOOST_CHECK_EQUAL(xl.cols(), dim*dim);
    BOOST_CHECK_EQUAL(xl.rows(), npovms);
    BOOST_CHECK_CLOSE(xl.norm(), 0, tol_percent);

    // frequency list type
    BOOST_CHECKPOINT("Testing FreqListType");
    typename TheMatrQ::FreqListType fl = matq.initFreqListType(npovms);
    BOOST_CHECK_EQUAL(fl.cols(), 1);
    BOOST_CHECK_EQUAL(fl.rows(), npovms);
    BOOST_CHECK_EQUAL(fl.abs().maxCoeff(), 0); // should be integral type
  }
};

BOOST_FIXTURE_TEST_SUITE(test_impl_matrq, test_matrq_fixture)

BOOST_AUTO_TEST_CASE(default_matrq)
{
  test_matrq<Tomographer::DefaultMatrQ>(5, 100);
}
BOOST_AUTO_TEST_CASE(default_matrq_2)
{
  test_matrq<Tomographer::DefaultMatrQ>(2, 50);
}
BOOST_AUTO_TEST_CASE(qubitpaulis_matrq)
{
  test_matrq<Tomographer::QubitPaulisMatrQ>(2, 6);
}

BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE(test_tomoproblem)

BOOST_AUTO_TEST_CASE(indep_meas_tomo_problem)
{
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
  dat.Nx << 1500, 800, 300, 300, 10, 30;

  Tomographer::QubitPaulisMatrQ::VectorParamType x = qmq.initVectorParamType();
  x << 0.5, 0.5, 0, 0; // maximally mixed state
  
  Tomographer::QubitPaulisMatrQ::RealScalar value = dat.calc_llh(x);
  
  BOOST_CHECK_CLOSE(value, 4075.70542169248, 1e-4);

  //std::cout << "llh @ mixed state = " << std::setprecision(15) << value << "\n";
  
}

BOOST_AUTO_TEST_SUITE_END()

// -----------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(test_random_unitary)

BOOST_AUTO_TEST_CASE(test_random_unitary_basic)
{
  Eigen::MatrixXcd U(7,7);
  
  std::mt19937 rng(43423); // seeded, deterministic random number generator

  Tomographer::random_unitary(U, rng);
  
  // check that the given U is unitary

  BOOST_CHECK_SMALL( (U * U.adjoint() - Eigen::MatrixXcd::Identity(7,7)).norm(), tol);
  BOOST_CHECK_SMALL( (U.adjoint() * U - Eigen::MatrixXcd::Identity(7,7)).norm(), tol);
}

// TODO: check that the unitary is indeed Haar-distributed. For example, if we average
// many unitaries, we should get the identity
BOOST_AUTO_TEST_CASE(test_random_unitary_distr)
{
  BOOST_MESSAGE("TODO: check that the random_unitary is indeed Haar-distributed");
  BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END();


// -----------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(pos_semidef_util);

BOOST_AUTO_TEST_CASE(force_pos_semidef)
{
  Eigen::Matrix4cd rho;
  rho <<
    -0.1, 0, 0, 0,
    0, 0.05, 0, 0,
    0, 0, 0.55, 0,
    0, 0, 0, 0.5;

  BOOST_CHECK_CLOSE(rho.trace().real(), 1.0, tol_percent);

  Eigen::Matrix4cd rhopos;

  rhopos = Tomographer::Tools::force_pos_semidef<Eigen::Matrix4cd>(rho, 0.1); // high tolerance, check our algo

  BOOST_CHECK_CLOSE(rhopos.trace().real(), 1.0, tol_percent);

  Eigen::Matrix4cd rhopos_ref_withtol;
  rhopos_ref_withtol <<
    0.1, 0, 0, 0,
    0, 0.1, 0, 0,
    0, 0, 0.425, 0,
    0, 0, 0, 0.375;
  // the 0.25 "excess trace" is evenly subtracted from all good eigenvalues

  MY_BOOST_CHECK_EIGEN_EQUAL(rhopos, rhopos_ref_withtol, tol);


  // should get the same behavior if we apply some Unitary

  // get some nontrivial unitary (fixed by deterministic seeded rng)
  Eigen::Matrix4cd Unitary;
  std::mt19937 rng(1); // seeded, deterministic random number generator
  Tomographer::random_unitary(Unitary, rng);

  BOOST_MESSAGE("Chose Unitary = \n" << Unitary) ;

  Eigen::Matrix4cd rhoposU;
  rhoposU = Tomographer::Tools::force_pos_semidef<Eigen::Matrix4cd>(Unitary*rho*Unitary.adjoint(), 0.1);

  BOOST_CHECK_CLOSE(rhoposU.trace().real(), 1.0, tol_percent);
  MY_BOOST_CHECK_EIGEN_EQUAL(rhoposU, Unitary*rhopos_ref_withtol*Unitary.adjoint(), tol);
}


BOOST_AUTO_TEST_CASE(safe_ops)
{
  Eigen::Matrix3cd A;
  A <<
    0, 0, 0,
    0, 0, 0,
    0, 0, 1;

  MY_BOOST_CHECK_EIGEN_EQUAL(Tomographer::Tools::safe_operator_sqrt<Eigen::Matrix3cd>(A, 1e-18), A, 2e-9);
  MY_BOOST_CHECK_EIGEN_EQUAL(Tomographer::Tools::safe_operator_inv_sqrt<Eigen::Matrix3cd>(A, 1e-12), A, 2e-6);
}

BOOST_AUTO_TEST_SUITE_END();



// =============================================================================
BOOST_AUTO_TEST_SUITE_END();
