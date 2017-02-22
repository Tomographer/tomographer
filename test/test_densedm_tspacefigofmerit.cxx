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

#include <cmath>

#include <string>
#include <sstream>
#include <random>

#include <boost/math/constants/constants.hpp>

// include before <Eigen/*> !
#include "test_tomographer.h"

#include <tomographer/densedm/tspacefigofmerit.h>



// -----------------------------------------------------------------------------
// fixture(s)


#include "test_densedm_distmeasures_common.h"


// -----------------------------------------------------------------------------
// test suites


BOOST_AUTO_TEST_SUITE(test_densedm_densedmtypes)

BOOST_FIXTURE_TEST_CASE(FidelityToRefCalculator_2_d, distmeasures_qubit_fixture<double>)
{
  Tomographer::DenseDM::TSpace::FidelityToRefCalculator<DMTypes, double> f(T1);

  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T1), fid_with_1<double>(1), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T2), fid_with_1<double>(2), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T2b), fid_with_1<double>(2), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T3), fid_with_1<double>(3), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T4), fid_with_1<double>(4), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T5), fid_with_1<double>(5), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T6), fid_with_1<double>(6), tol);
}
BOOST_FIXTURE_TEST_CASE(FidelityToRefCalculator_2_f, distmeasures_qubit_fixture<float>)
{
  Tomographer::DenseDM::TSpace::FidelityToRefCalculator<DMTypes, float> f(T1);

  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T1), fid_with_1<float>(1), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T2), fid_with_1<float>(2), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T2b), fid_with_1<float>(2), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T3), fid_with_1<float>(3), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T4), fid_with_1<float>(4), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T5), fid_with_1<float>(5), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T6), fid_with_1<float>(6), tol_f);
}
BOOST_FIXTURE_TEST_CASE(FidelityToRefCalculator_4_f, distmeasures_qudit4_fixture<float>)
{
  Tomographer::DenseDM::TSpace::FidelityToRefCalculator<DMTypes, float> f(T1);

  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T1), fid_with_1<float>(1), 32*tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T2), fid_with_1<float>(2), 32*tol_f);
}


// -----------------


BOOST_FIXTURE_TEST_CASE(PurifDistToRefCalculator_2_d, distmeasures_qubit_fixture<double>)
{
  Tomographer::DenseDM::TSpace::PurifDistToRefCalculator<DMTypes, double> f(T1);

  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T1), std::sqrt(1-std::pow(fid_with_1<double>(1),2)), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T2), std::sqrt(1-std::pow(fid_with_1<double>(2),2)), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T2b), std::sqrt(1-std::pow(fid_with_1<double>(2),2)), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T3), std::sqrt(1-std::pow(fid_with_1<double>(3),2)), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T4), std::sqrt(1-std::pow(fid_with_1<double>(4),2)), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T5), std::sqrt(1-std::pow(fid_with_1<double>(5),2)), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T6), std::sqrt(1-std::pow(fid_with_1<double>(6),2)), tol);
}
BOOST_FIXTURE_TEST_CASE(PurifDistToRefCalculator_2_f, distmeasures_qubit_fixture<float>)
{
  Tomographer::DenseDM::TSpace::PurifDistToRefCalculator<DMTypes, float> f(T1);

  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T1), std::sqrt(1-std::pow(fid_with_1<float>(1),2)), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T2), std::sqrt(1-std::pow(fid_with_1<float>(2),2)), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T2b), std::sqrt(1-std::pow(fid_with_1<float>(2),2)), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T3), std::sqrt(1-std::pow(fid_with_1<float>(3),2)), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T4), std::sqrt(1-std::pow(fid_with_1<float>(4),2)), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T5), std::sqrt(1-std::pow(fid_with_1<float>(5),2)), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T6), std::sqrt(1-std::pow(fid_with_1<float>(6),2)), tol_f);
}
BOOST_FIXTURE_TEST_CASE(PurifDistToRefCalculator_4_f, distmeasures_qudit4_fixture<float>)
{
  Tomographer::DenseDM::TSpace::PurifDistToRefCalculator<DMTypes, float> f(T1);

  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T1), 0.0f, 1e-2f); // Because error diverges at @f=1 -> purif_dist=0
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T2), std::sqrt(1-std::pow(fid_with_1<float>(2),2)), 32*tol_f);
}


// -----------------


BOOST_FIXTURE_TEST_CASE(TrDistToRefCalculator_2_d, distmeasures_qubit_fixture<double>)
{
  Tomographer::DenseDM::TSpace::TrDistToRefCalculator<DMTypes, double> f(T1*T1.adjoint());

  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T1), trdist_with_1<double>(1), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T2), trdist_with_1<double>(2), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T2b), trdist_with_1<double>(2), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T3), trdist_with_1<double>(3), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T4), trdist_with_1<double>(4), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T5), trdist_with_1<double>(5), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T6), trdist_with_1<double>(6), tol);
}
BOOST_FIXTURE_TEST_CASE(TrDistToRefCalculator_2_f, distmeasures_qubit_fixture<float>)
{
  Tomographer::DenseDM::TSpace::TrDistToRefCalculator<DMTypes, float> f(T1*T1.adjoint());

  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T1), trdist_with_1<float>(1), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T2), trdist_with_1<float>(2), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T2b), trdist_with_1<float>(2), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T3), trdist_with_1<float>(3), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T4), trdist_with_1<float>(4), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T5), trdist_with_1<float>(5), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T6), trdist_with_1<float>(6), tol_f);
}
BOOST_FIXTURE_TEST_CASE(TrDistToRefCalculator_4_f, distmeasures_qudit4_fixture<float>)
{
  Tomographer::DenseDM::TSpace::TrDistToRefCalculator<DMTypes, float> f(T1*T1.adjoint());

  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T1), trdist_with_1<float>(1), 32*tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T2), trdist_with_1<float>(2), 32*tol_f);
}


// -----------------


BOOST_FIXTURE_TEST_CASE(ObservableValueCalculator_2_d, distmeasures_qubit_fixture<double>)
{
  typedef DMTypes::ComplexScalar Cplx;
  MatrixType A;
  A <<
    Cplx( 6.528329762670850e-01, + 0.000000000000000e+00),    Cplx(-2.828700628152467e-02, - 4.752282889738084e-01),
    Cplx(-2.828700628152467e-02, + 4.752282889738084e-01),    Cplx( 3.471670237329149e-01, + 0.000000000000000e+00) ;
  Tomographer::DenseDM::TSpace::ObservableValueCalculator<DMTypes> f(dmt, A);

  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T1), (A*rho1).real().trace(), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T2), (A*rho2).real().trace(), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T2b), (A*rho2).real().trace(), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T3), (A*rho3).real().trace(), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T4), (A*rho4).real().trace(), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T5), (A*rho5).real().trace(), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T6), (A*rho6).real().trace(), tol);
}
BOOST_FIXTURE_TEST_CASE(ObservableValueCalculator_2_f, distmeasures_qubit_fixture<float>)
{
  //  typedef DMTypes::ComplexScalar Cplx;
  auto Cplx = [](double re, double im) -> typename DMTypes::ComplexScalar {
    return typename DMTypes::ComplexScalar(typename DMTypes::RealScalar(re), typename DMTypes::RealScalar(im));
  };
  
  MatrixType A;
  A<<
    Cplx(5.769321416135639e-01, + 0.000000000000000e+00),      Cplx(3.720330764264117e-01, + 3.250735849487777e-01),
    Cplx(3.720330764264117e-01, - 3.250735849487777e-01),      Cplx(4.230678583864360e-01, + 0.000000000000000e+00);
  Tomographer::DenseDM::TSpace::ObservableValueCalculator<DMTypes> f(dmt, A);

  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T1), (A*rho1).real().trace(), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T2), (A*rho2).real().trace(), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T2b), (A*rho2).real().trace(), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T3), (A*rho3).real().trace(), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T4), (A*rho4).real().trace(), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T5), (A*rho5).real().trace(), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T6), (A*rho6).real().trace(), tol_f);
}
BOOST_FIXTURE_TEST_CASE(ObservableValueCalculator_4_f, distmeasures_qudit4_fixture<float>)
{
  // typedef DMTypes::ComplexScalar Cplx;
  auto Cplx = [](double re, double im) -> typename DMTypes::ComplexScalar {
    return typename DMTypes::ComplexScalar(typename DMTypes::RealScalar(re), typename DMTypes::RealScalar(im));
  };
  
  MatrixType A;
  A <<
    Cplx( 1.668309209270559e+00, - 2.775557561562891e-17),     Cplx(-7.439487529186134e-01, - 5.855920232909842e-02),
    Cplx(-4.537721235745775e-01, + 4.228424897118110e-03),     Cplx( 1.799138697503067e-02, + 2.706936451208813e-02),
    
    Cplx(-7.439487529186133e-01, + 5.855920232909838e-02),     Cplx( 9.029493599494294e-01, + 5.551115123125783e-17),
    Cplx(-2.149554776206607e-01, + 6.525886338252280e-01),     Cplx( 3.469407736574681e-01, + 3.089648739998575e-01),
    
    Cplx(-4.537721235745775e-01, - 4.228424897118249e-03),     Cplx(-2.149554776206607e-01, - 6.525886338252280e-01),
    Cplx( 1.411783000531320e+00, + 5.551115123125783e-17),     Cplx( 6.555785447953847e-01, - 5.371466754457879e-01),
    
    Cplx( 1.799138697503067e-02, - 2.706936451208811e-02),     Cplx( 3.469407736574681e-01, - 3.089648739998575e-01),
    Cplx( 6.555785447953847e-01, + 5.371466754457879e-01),     Cplx( 2.016958430248692e+00, + 0.000000000000000e+00) ;

  Tomographer::DenseDM::TSpace::ObservableValueCalculator<DMTypes> f(dmt, A);

  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T1), (A*rho1).real().trace(), 32*tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T2), (A*rho2).real().trace(), 32*tol_f);
}









BOOST_AUTO_TEST_SUITE_END()

