

#include <Eigen/unsupported/MatrixFunctions>


template<typename RealScalar = double>
struct distmeasures_qubit_fixture {

  typedef Tomographer::DenseDM::DMTypes<2, RealScalar>  DMTypes;

  typedef DMTypes::MatrixType  MatrixType;

  MatrixType  rho1, rho2, rho3, rho4, rho5, rho6;
  MatrixType  T1, T2, T2b, T3, T4, T5, T6;

  static constexpr RealScalar INVSQRT2 = boost::math::constants::half_root_two<RealScalar>();

  distmeasures_qubit_fixture()
  {
    rho1 << 1, 0,
      0, 0;
    rho2 << 0.5, 0.5,
      0.5, 0.5;
    rho3 << 0, 0,
      0, 1;
    rho4 << 0.5, std::complex<RealScalar>(0, -0.5),
      std::complex<RealScalar>(0, 0.5), 0.5;
    rho5 << 0.8, 0,
      0, 0.2;
    rho6 << 0.5, 0,
      0, 0.5;

    T1 << 1, 0,
      0, 0;
    T2 << INVSQRT2, 0,
      INVSQRT2, 0;
    T2b = rho2; // rho2 is pure, so sqrt(rho2)==rho2
    T3 << 0, 0,
      0, 1;
    T4 = rho4; // rho4 is pure, so sqrt(rho4)==rho4
    T5 << std::sqrt(0.8), 0,
      0, std::sqrt(0.2);
    T6 << INVSQRT2, 0,
      0, INVSQRT2;
  }
  ~distmeasures_qubit_fixture()
  {
  }


  inline void internal_test_fixture() {
    // internal test checks:
    MY_BOOST_CHECK_EIGEN_EQUAL(T1*T1.adjoint(), rho1, tol);
    MY_BOOST_CHECK_EIGEN_EQUAL(T2*T2.adjoint(), rho2, tol);
    MY_BOOST_CHECK_EIGEN_EQUAL(T2b*T2b.adjoint(), rho2, tol);
    MY_BOOST_CHECK_EIGEN_EQUAL(T3*T3.adjoint(), rho3, tol);
    MY_BOOST_CHECK_EIGEN_EQUAL(T4*T4.adjoint(), rho4, tol);
    MY_BOOST_CHECK_EIGEN_EQUAL(T5*T5.adjoint(), rho5, tol);
    MY_BOOST_CHECK_EIGEN_EQUAL(T6*T6.adjoint(), rho6, tol);
  }


  inline RealScalar fid_with_1(int which) const {
    switch (which) {
    case 1:
      return 1;
    case 2:
      return INVSQRT2;
    case 3:
      return 0;
    case 4:
      return INVSQRT2;
    case 5:
      return std::sqrt(0.8);
    case 6:
      return INVSQRT2;
    default:
      fprintf(stderr, "INVALID 'which' for test fixture fid_with_1: %d", which);
      assert(false);
    }
  }

  inline RealScalar trdist_with_1(int which) const {
    switch (which) {
    case 1:
      return 0;
    case 2:
      return INVSQRT2;
    case 3:
      return 1;
    case 4:
      return INVSQRT2;
    case 5:
      return 0.2;
    case 6:
      return 0.5;
    default:
      fprintf(stderr, "INVALID 'which' for test fixture trdist_with_1: %d", which);
      assert(false);
    }
  }
};



template<typename RealScalar = double>
struct distmeasures_qudit4_fixture
{

  typedef Tomographer::DenseDM::DMTypes<4, RealScalar>  DMTypes;

  typedef DMTypes::MatrixType  MatrixType;

  MatrixType  rho1, rho2;
  MatrixType  T1, T2;

  distmeasures_qudit4_fixture() {
    rho1 <<
      CD(1.895222898432606e-01,  + 0.000000000000000e+00),      CD(1.084025272341251e-01,  + 1.516096020672695e-02),
      CD(8.314826089318567e-02,  - 1.441693960987760e-01),     CD(-4.849903197599588e-02,  - 9.894562194279641e-02),
      
      CD(1.084025272341251e-01,  - 1.516096020672695e-02),      CD(1.403975159107966e-01,  + 0.000000000000000e+00),
      CD(9.189478772453549e-02,  - 1.113002628282837e-01),     CD(-3.963271236943127e-02,  - 8.342253473747827e-02),
      
      CD(8.314826089318567e-02,  + 1.441693960987760e-01),      CD(9.189478772453549e-02,  + 1.113002628282837e-01),
      CD(3.468111374375993e-01,  + 0.000000000000000e+00),      CD(3.926673263985917e-02,  - 8.857048139726613e-02),
      
      CD(-4.849903197599588e-02,  + 9.894562194279641e-02),     CD(-3.963271236943127e-02,  + 8.342253473747827e-02),
      CD(3.926673263985917e-02,  + 8.857048139726613e-02),      CD(3.232690568083436e-01,  + 0.000000000000000e+00) ;
    
    rho2 <<
      CD(1.156724759647584e-01,  + 0.000000000000000e+00),      CD(2.120616131342336e-01,  + 1.333496382385370e-01),
      CD(7.008776286076293e-02,  - 9.028470691907955e-03),      CD(3.730567277668716e-02,  - 8.832584001201396e-02),
      
      CD(2.120616131342336e-01,  - 1.333496382385370e-01),      CD(6.702321505951183e-01,  + 0.000000000000000e+00),
      CD(1.087831860504907e-01,  - 7.738062875525148e-02),     CD(-5.761735204119786e-02,  - 2.701304922505648e-01),
      
      CD(7.008776286076293e-02,  + 9.028470691907955e-03),      CD(1.087831860504907e-01,  + 7.738062875525148e-02),
      CD(7.310740563562612e-02,  + 0.000000000000000e+00),      CD(3.427023484653953e-02,  - 5.397779491330748e-02),
      
      CD(3.730567277668716e-02,  + 8.832584001201396e-02),     CD(-5.761735204119786e-02,  + 2.701304922505648e-01),
      CD(3.427023484653953e-02,  + 5.397779491330748e-02),      CD(1.409879678044973e-01,  + 0.000000000000000e+00) ;

    T1 = rho1.sqrt();
    T2 = rho2.sqrt();
  }
    
  
  inline RealScalar fid_with_1(int which) const {
    switch (which) {
    case 1:
      return 1;
    case 2:
      return 7.611036198843356e-01;
    default:
      fprintf(stderr, "INVALID 'which' for test fixture fid_with_1: %d", which);
      assert(false);
    }
  }
  inline RealScalar trdist_with_1(int which) const {
    switch (which) {
    case 1:
      return 1;
    case 2:
      return 6.208689785356507e-01;
    default:
      fprintf(stderr, "INVALID 'which' for test fixture fid_with_1: %d", which);
      assert(false);
    }
  }

};
