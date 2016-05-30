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

#include <tomographer/densedm/figofmerit.h>


#define SQRT2  boost::math::constants::root_two<double>()
#define INVSQRT2  boost::math::constants::half_root_two<double>()


// -----------------------------------------------------------------------------
// fixture(s)


struct test_valcalc_fixture
{

  // see test_densedm_distmeasures.cxx

  Eigen::Matrix<std::complex<double>,2,2> T1, T2, T2b, T3, T4, T5, T6;

  Eigen::Matrix<std::complex<double>,4,4> rho1_qudit, rho2_qudit;
  Eigen::Matrix<std::complex<double>,4,4> T1_qudit, T2_qudit;

  test_valcalc_fixture()
  {
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

  typedef std::complex<double> CD;
  Eigen::Matrix<CD, Eigen::Dynamic, Eigen::Dynamic> rho1(4, 4);
  Eigen::Matrix<CD, 4, 4> rho2;

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
  
  BOOST_CHECK_CLOSE(Tomographer::DenseDM::fidelity<double>(rho1, rho2), 7.611036198843356e-01, tol_percent);
  BOOST_CHECK_CLOSE(Tomographer::DenseDM::trace_dist<double>(rho1, rho2), 6.208689785356507e-01, tol_percent);
    
  }

  ~test_valcalc_fixture()
  {
  }

  template<typename ValueCalculator,
	   typename DMTypes, typename DCalc>
  void test_valcalc(const DMTypes dmt, DCalc dcalc)
  {

    
  }
};



// -----------------------------------------------------------------------------
// test suites


BOOST_FIXTURE_TEST_SUITE(test_densedm_densedmtypes, test_dmtypes_fixture)

BOOST_AUTO_TEST_CASE(testFidelityToRefCalculator_qubit)
{
  Tomographer::DenseDM::FidelityToRefCalculator f(T1);

  BOOST_CHECK_CLOSE(f.getValue(T1), 1, tol_percent);
  BOOST_CHECK_CLOSE(f.getValue(T2), INVSQRT2, tol_percent);
  BOOST_CHECK_CLOSE(f.getValue(T2b), INVSQRT2, tol_percent);
  BOOST_CHECK_SMALL(f.getValue(T3), tol);
  BOOST_CHECK_CLOSE(f.getValue(T4), INVSQRT2, tol_percent);
  BOOST_CHECK_CLOSE(f.getValue(T5), std::sqrt(0.8), tol_percent);
  BOOST_CHECK_CLOSE(f.getValue(T6), INVSQRT2, tol_percent);
}
BOOST_AUTO_TEST_CASE(testFidelityToRefCalculator_qudit)
{
  typedef std::complex<double> CD;
  Eigen::Matrix<CD, Eigen::Dynamic, Eigen::Dynamic> rho1(4, 4);
  Eigen::Matrix<CD, 4, 4> rho2;

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
  
  BOOST_CHECK_CLOSE(Tomographer::DenseDM::fidelity<double>(rho1, rho2), 7.611036198843356e-01, tol_percent);
  BOOST_CHECK_CLOSE(Tomographer::DenseDM::trace_dist<double>(rho1, rho2), 6.208689785356507e-01, tol_percent);
}


BOOST_AUTO_TEST_CASE(testFidelityToRefCalculator)
{
  // see test_densedm_distmeasures.cxx
  typename DMTypes::MatrixType T1, T2, T2b, T3, T4, T5, T6;
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
  // --
  Tomographer::DenseDM::FidelityToRefCalculator f(T1);

  BOOST_CHECK_CLOSE(f.getValue(T1), 1, tol_percent);
  BOOST_CHECK_CLOSE(f.getValue(T2), INVSQRT2, tol_percent);
  BOOST_CHECK_CLOSE(f.getValue(T2b), INVSQRT2, tol_percent);
  BOOST_CHECK_SMALL(f.getValue(T3), tol);
  BOOST_CHECK_CLOSE(f.getValue(T4), INVSQRT2, tol_percent);
  BOOST_CHECK_CLOSE(f.getValue(T5), std::sqrt(0.8), tol_percent);
  BOOST_CHECK_CLOSE(f.getValue(T6), INVSQRT2, tol_percent);
}

BOOST_AUTO_TEST_SUITE_END()

