/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
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

#include <cmath>

#include <string>
#include <sstream>
#include <random>

#include <boost/math/constants/constants.hpp>

// include before <Eigen/*> !
#include "test_tomographer.h"

#include <tomographer/densedm/indepmeasllh.h>


// -----------------------------------------------------------------------------
// fixture(s)


// -----------------------------------------------------------------------------
// test suites


BOOST_AUTO_TEST_SUITE(test_densedm_indeapmeasllh)

BOOST_AUTO_TEST_CASE(basic)
{
  typedef Tomographer::DenseDM::DMTypes<2> DMTypes;
  DMTypes dmt;
  
  typedef Tomographer::DenseDM::IndepMeasLLH<DMTypes> IndepMeasLLH;
  IndepMeasLLH dat(dmt);

  IndepMeasLLH::VectorParamListType Exn(6, dmt.dim2());
  Exn <<
    0.5, 0.5,  1./std::sqrt(2.0),  0,
    0.5, 0.5, -1./std::sqrt(2.0),  0,
    0.5, 0.5,  0,         1./std::sqrt(2.0),
    0.5, 0.5,  0,        -1./std::sqrt(2.0),
    1,   0,    0,         0,
    0,   1,    0,         0
    ;
  IndepMeasLLH::FreqListType Nx(6);
  Nx << 1500, 800, 300, 300, 10, 30;

  dat.setMeas(Exn, Nx);

  DMTypes::VectorParamType x(dmt.initVectorParamType());
  x << 0.5, 0.5, 0, 0; // maximally mixed state
  
  DMTypes::RealScalar value = -2*dat.logLikelihoodX(x);
  
  BOOST_CHECK_CLOSE(value, 4075.70542169248, 1e-4);
  //std::cout << "llh @ mixed state = " << std::setprecision(15) << value << "\n";
}

BOOST_AUTO_TEST_CASE(basic_dyn)
{
  typedef Tomographer::DenseDM::DMTypes<Eigen::Dynamic> DMTypes;
  DMTypes dmt(2);
  
  typedef Tomographer::DenseDM::IndepMeasLLH<DMTypes> IndepMeasLLH;
  IndepMeasLLH dat(dmt);

  IndepMeasLLH::VectorParamListType Exn(6, dmt.dim2());
  Exn <<
    0.5, 0.5,  1./std::sqrt(2.0),  0,
    0.5, 0.5, -1./std::sqrt(2.0),  0,
    0.5, 0.5,  0,         1./std::sqrt(2.0),
    0.5, 0.5,  0,        -1./std::sqrt(2.0),
    1,   0,    0,         0,
    0,   1,    0,         0
    ;
  IndepMeasLLH::FreqListType Nx(6);
  Nx << 1500, 800, 300, 300, 10, 30;

  dat.setMeas(Exn, Nx);

  DMTypes::VectorParamType x(dmt.initVectorParamType());
  x << 0.5, 0.5, 0, 0; // maximally mixed state
  
  const DMTypes::VectorParamType xconst(x); // make sure logLikelihoodX() accepts const argument
  DMTypes::RealScalar value = -2*dat.logLikelihoodX(xconst);
  
  BOOST_CHECK_CLOSE(value, 4075.70542169248, 1e-4);
  //std::cout << "llh @ mixed state = " << std::setprecision(15) << value << "\n";
}

BOOST_AUTO_TEST_CASE(reset_meas)
{
  typedef Tomographer::DenseDM::DMTypes<2> DMTypes;
  DMTypes dmt(2);
  
  typedef Tomographer::DenseDM::IndepMeasLLH<DMTypes> IndepMeasLLH;
  IndepMeasLLH dat(dmt);

  IndepMeasLLH::VectorParamListType Exn(2,4);
  Exn <<
    1, 0, 0, 0,
    0, 1, 0, 0
    ;
  IndepMeasLLH::FreqListType Nx(2);
  Nx << 50, 50;
  dat.setMeas(Exn, Nx);

  BOOST_CHECK_EQUAL(dat.numEffects(), 2);
  BOOST_CHECK_EQUAL(dat.Exn().rows(), 2);
  { DMTypes::VectorParamType x; x << 0.0,1,0,0; MY_BOOST_CHECK_EIGEN_EQUAL(dat.Exn(1), x, tol); }
  BOOST_CHECK_EQUAL(dat.Nx(0), 50);
  BOOST_CHECK_EQUAL(dat.Nx(1), 50);

  // test resetMeas()
  dat.resetMeas();

  BOOST_CHECK_EQUAL(dat.numEffects(), 0);
  BOOST_CHECK_EQUAL(dat.Exn().rows(), 0);
  BOOST_CHECK_EQUAL(dat.Nx().rows(), 0);
}

BOOST_AUTO_TEST_CASE(add_meas)
{
  typedef Tomographer::DenseDM::DMTypes<2> DMTypes;
  DMTypes dmt(2);
  typedef DMTypes::VectorParamType VectorParamType;
  typedef DMTypes::MatrixType MatrixType;
  
  typedef Tomographer::DenseDM::IndepMeasLLH<DMTypes> IndepMeasLLH;
  IndepMeasLLH dat(dmt);

  IndepMeasLLH::VectorParamListType Exn(2,4);
  Exn <<
    1, 0, 0, 0,
    0, 1, 0, 0
    ;
  IndepMeasLLH::FreqListType Nx(2);
  Nx << 50, 50;
  dat.setMeas(Exn, Nx);

  // test addMeasEffect(MatrixType)
  dat.addMeasEffect(0.5*MatrixType::Identity(), 75);

  BOOST_CHECK_EQUAL(dat.numEffects(), 3);
  { VectorParamType x; x<<0.5,0.5,0,0; MY_BOOST_CHECK_EIGEN_EQUAL(dat.Exn().row(2), x.transpose(), tol); }
  BOOST_CHECK_EQUAL(dat.Nx().rows(), 3);
  BOOST_CHECK_EQUAL(dat.Nx(2), 75);
  BOOST_CHECK_EQUAL(dat.Nx()(2), 75);

  // test addMeasEffect(VectorParamType)
  dat.addMeasEffect((VectorParamType()<<0.5,0.5,-1./std::sqrt(2),0).finished(), 1175);

  BOOST_CHECK_EQUAL(dat.numEffects(), 4);
  { VectorParamType x; x <<0.5,0.5,-1./std::sqrt(2),0; MY_BOOST_CHECK_EIGEN_EQUAL(dat.Exn().row(3), x.transpose(), tol); }
  BOOST_CHECK_EQUAL(dat.Nx().rows(), 4);
  BOOST_CHECK_EQUAL(dat.Nx(3), 1175);
  BOOST_CHECK_EQUAL(dat.Nx()(3), 1175);

  // and check that the second addMeasEffect() didn't affect the previous data
  { VectorParamType x; x<<0.5,0.5,0,0;  MY_BOOST_CHECK_EIGEN_EQUAL(dat.Exn().row(2), x.transpose(), tol); }
  BOOST_CHECK_EQUAL(dat.Nx(2), 75);
  BOOST_CHECK_EQUAL(dat.Nx()(2), 75);
}


BOOST_AUTO_TEST_CASE(add_meas_checkmeas)
{
  typedef Tomographer::DenseDM::DMTypes<2> DMTypes;
  DMTypes dmt(2);
  //typedef DMTypes::VectorParamType VectorParamType;
  typedef DMTypes::MatrixType MatrixType;
  
  typedef Tomographer::DenseDM::IndepMeasLLH<DMTypes> IndepMeasLLH;
  IndepMeasLLH dat(dmt);

  IndepMeasLLH::VectorParamListType Exn(2,4);
  Exn <<
    1, 0, 0, 0,
    0, 1, 0, 0
    ;
  IndepMeasLLH::FreqListType Nx(2);
  Nx << 50, 50;
  dat.setMeas(Exn, Nx);
  
  // test addMeasEffect(MatrixType) --> must raise exception because matrix is not Hermitian
  {
    MatrixType Ebad(dmt.initMatrixType());
    Ebad << 1, 0.5,
            0, 1;
    auto call = [&Ebad,&dat]() { dat.addMeasEffect(Ebad, 500); };
    BOOST_CHECK_THROW(call(), Tomographer::DenseDM::InvalidMeasData);
  }

  // test addMeasEffect(MatrixType) --> must raise exception because matrix is not pos semidef
  {
    MatrixType Ebad(dmt.initMatrixType());
    Ebad << 0, 1e-2,
      1e-2, 1;
    auto call = [&Ebad,&dat]() { dat.addMeasEffect(Ebad, 500); };
    BOOST_CHECK_THROW(call(), Tomographer::DenseDM::InvalidMeasData);
  }

  // test addMeasEffect(MatrixType) --> must raise exception because matrix is zero
  {
    MatrixType Ebad(dmt.initMatrixType());
    Ebad << 0, 1e-12,
      1e-12, 1e-15;
    auto call = [&Ebad,&dat]() { dat.addMeasEffect(Ebad, 500); };
    BOOST_CHECK_THROW(call(), Tomographer::DenseDM::InvalidMeasData);
  }
  
}

BOOST_AUTO_TEST_SUITE_END()

