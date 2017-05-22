/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
 * Copyright (c) 2017 Caltech, Institute for Quantum Information and Matter, Philippe Faist
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include <tomographer/densedm/dmtypes.h>
#include <tomographer/mathtools/random_unitary.h>


template<typename DMTypes_>
struct TestParamsFixture
{
  typedef DMTypes_ DMTypes;

  TestParamsFixture() { }
  ~TestParamsFixture() { }

  void test_param_x(DMTypes dmt, const typename DMTypes::MatrixType & rho)
  {
    BOOST_MESSAGE("test_param_x(), rho = \n" << rho);
    //std::cout << "-----------------------------------------------\n";
    //std::cout << "rho = \n" << rho << "\n";

    Tomographer::DenseDM::ParamX<DMTypes> px(dmt);

    typename DMTypes::VectorParamType x;
    x = px.HermToX(rho);
    //std::cout << " --> x = " << x.transpose() << "\n";
    BOOST_CHECK_SMALL((x.block(0,0,dmt.dim(),1) - rho.real().diagonal()).norm(), tol);
    // and convert back to rho
    typename DMTypes::MatrixType rhoback1(dmt.initMatrixType());
    rhoback1 = px.XToHerm(x);
    //std::cout << " --> and back to rho = \n" << rhoback1 << "\n";
    BOOST_CHECK_SMALL((rho - rhoback1).norm(), tol);
  }

  void test_param_a(DMTypes dmt, const typename DMTypes::MatrixType & rho)
  {
    BOOST_MESSAGE("test_param_a(), rho = \n" << rho);

    Tomographer::DenseDM::ParamA<DMTypes> param(dmt);
    // display generalized Gell-Mann matrices
    for (Eigen::Index l = 0; l < dmt.ndof(); ++l) {
      BOOST_MESSAGE("\tlambda[" << l << "] = \n" << param.getLambda((std::size_t)l));
      BOOST_CHECK_SMALL((param.getLambda((std::size_t)l).adjoint() - param.getLambda((std::size_t)l)).norm(),
                        tol_percent);
    }
    // do checks that all HS inner products are correct
    Eigen::MatrixXcd inner_prods = Eigen::MatrixXcd::Zero(dmt.ndof(), dmt.ndof());
    for (Eigen::Index i = 0; i < dmt.ndof(); ++i) {
      for (Eigen::Index j = 0; j < dmt.ndof(); ++j) {
	inner_prods(i,j) = (param.getLambda((std::size_t)i).adjoint() * param.getLambda((std::size_t)j)).trace();
      }
    }
    BOOST_MESSAGE("Matrix of inner products [expected == 2*Ident]: tr(A'*B) = \n" << inner_prods);
    BOOST_CHECK_SMALL((inner_prods - 2*Eigen::MatrixXcd::Identity(dmt.ndof(), dmt.ndof())).norm(), tol_percent);

    // ---
    typename DMTypes::VectorParamNdofType a(dmt.initVectorParamNdofType());
    a = param.rhoToA(rho);
    //std::cout << " --> a = " << a.transpose() << "\n";
    // and convert back to rho
    typename DMTypes::MatrixType rhoback2(dmt.initMatrixType());
    rhoback2 = param.aToRho(a);
    //std::cout << " --> and back to rho = \n" << rhoback2 << "\n";

    BOOST_CHECK_SMALL((rho - rhoback2).norm(), tol_percent);
  }

};



struct TestFixtureQubitIdent : public TestParamsFixture<Tomographer::DenseDM::DMTypes<2> > {
  typedef typename TestParamsFixture<Tomographer::DenseDM::DMTypes<2> >::DMTypes DMTypes;

  DMTypes dmt;
  typename DMTypes::MatrixType rho;

  TestFixtureQubitIdent()
    : dmt(2), rho(dmt.initMatrixType())
  {
    rho << 0.5, 0,
           0, 0.5;
  }
};

struct TestFixtureTritExample : public TestParamsFixture<Tomographer::DenseDM::DMTypes<3,double> > {
  typedef typename TestParamsFixture<Tomographer::DenseDM::DMTypes<3,double> >::DMTypes DMTypes;

  DMTypes dmt;
  typename DMTypes::MatrixType rho;

  TestFixtureTritExample()
    : dmt(3), rho(dmt.initMatrixType())
  {
    typedef typename DMTypes::ComplexScalar CD;
    rho.setZero();
    rho(0,0) = 0.2;
    rho(0,1) = CD(0.1, 0.1);
    rho(1,0) = CD(0.1, -0.1);
    rho(1,1) = 0.1;
    rho(2,2) = 0.7;
  }
};

template<int Dim>
struct TestFixtureQuditPure0 : public TestParamsFixture<Tomographer::DenseDM::DMTypes<Dim> > {
  typedef typename TestParamsFixture<Tomographer::DenseDM::DMTypes<Dim> >::DMTypes DMTypes;

  DMTypes dmt;
  typename DMTypes::MatrixType rho;

  TestFixtureQuditPure0()
    : dmt(Dim), rho(dmt.initMatrixType())
  {
    //    typedef typename DMTypes::ComplexScalar CD; // unused
    rho.setZero();
    rho(0,0) = 1.0;
  }
};

template<int Dim, int RandSeed = 123450>
struct TestFixtureQuditRand : public TestParamsFixture<Tomographer::DenseDM::DMTypes<Dim> > {
  typedef typename TestParamsFixture<Tomographer::DenseDM::DMTypes<Dim> >::DMTypes DMTypes;

  DMTypes dmt;
  typename DMTypes::MatrixType rho;

  TestFixtureQuditRand()
    : dmt(Dim), rho(dmt.initMatrixType())
  {
    typename DMTypes::MatrixType U(dmt.initMatrixType());

    std::mt19937 rng(RandSeed); // seeded, deterministic random number generator

    Tomographer::MathTools::randomUnitary(U, rng);

    //    BOOST_MESSAGE("U = \n" << U);

    rho.setZero();
    for (int k = 0; k < Dim; ++k) {
      rho(k,k) = 1.0 / (1+k);
    }
    rho /= rho.real().trace(); // normalize

    //    BOOST_MESSAGE("rho = \n" << rho);

    // and turn in some new crazy basis.
    rho = U * rho * U.adjoint();
    
    //    BOOST_MESSAGE("rho = \n" << rho);
  }
};

