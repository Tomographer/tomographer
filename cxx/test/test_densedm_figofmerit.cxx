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



// -----------------------------------------------------------------------------
// fixture(s)


#include "test_densedm_distmeasures_common.h"


// -----------------------------------------------------------------------------
// test suites


BOOST_AUTO_TEST_SUITE(test_densedm_densedmtypes)

BOOST_FIXTURE_TEST_CASE(FidelityToRefCalculator_2_d, distmeasures_qubit_fixture<double>)
{
  Tomographer::DenseDM::FidelityToRefCalculator<DMTypes, double> f(T1);

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
  Tomographer::DenseDM::FidelityToRefCalculator<DMTypes, float> f(T1);

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
  Tomographer::DenseDM::FidelityToRefCalculator<DMTypes, float> f(T1);

  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T1), fid_with_1<float>(1), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T2), fid_with_1<float>(2), tol_f);
}


// -----------------


BOOST_FIXTURE_TEST_CASE(PurifDistToRefCalculator_2_d, distmeasures_qubit_fixture<double>)
{
  Tomographer::DenseDM::PurifDistToRefCalculator<DMTypes, double> f(T1);

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
  Tomographer::DenseDM::PurifDistToRefCalculator<DMTypes, float> f(T1);

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
  Tomographer::DenseDM::PurifDistToRefCalculator<DMTypes, float> f(T1);

  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T1), 0, tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T2), std::sqrt(1-std::pow(fid_with_1<float>(2),2)), tol_f);
}


// -----------------


BOOST_FIXTURE_TEST_CASE(TrDistToRefCalculator_2_d, distmeasures_qubit_fixture<double>)
{
  Tomographer::DenseDM::TrDistToRefCalculator<DMTypes, double> f(T1*T1.adjoint());

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
  Tomographer::DenseDM::TrDistToRefCalculator<DMTypes, float> f(T1*T1.adjoint());

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
  Tomographer::DenseDM::TrDistToRefCalculator<DMTypes, float> f(T1*T1.adjoint());

  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T1), trdist_with_1<float>(1), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T2), trdist_with_1<float>(2), tol_f);
}


// -----------------


BOOST_FIXTURE_TEST_CASE(ObservableValueCalculator_2_d, distmeasures_qubit_fixture<double>)
{
  Tomographer::DenseDM::ObservableValueCalculator<DMTypes> f(dmt, rho1);

  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T1), (std::pow(fid_with_1<double>(1),2)), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T2), (std::pow(fid_with_1<double>(2),2)), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T2b), (std::pow(fid_with_1<double>(2),2)), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T3), (std::pow(fid_with_1<double>(3),2)), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T4), (std::pow(fid_with_1<double>(4),2)), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T5), (std::pow(fid_with_1<double>(5),2)), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T6), (std::pow(fid_with_1<double>(6),2)), tol);
}
BOOST_FIXTURE_TEST_CASE(ObservableValueCalculator_2_f, distmeasures_qubit_fixture<float>)
{
  Tomographer::DenseDM::ObservableValueCalculator<DMTypes> f(dmt, Tomographer::DenseDM::ParamX<DMTypes>(dmt).HermToX(rho1));

  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T1), (std::pow(fid_with_1<float>(1),2)), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T2), (std::pow(fid_with_1<float>(2),2)), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T2b), (std::pow(fid_with_1<float>(2),2)), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T3), (std::pow(fid_with_1<float>(3),2)), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T4), (std::pow(fid_with_1<float>(4),2)), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T5), (std::pow(fid_with_1<float>(5),2)), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T6), (std::pow(fid_with_1<float>(6),2)), tol_f);
}
BOOST_FIXTURE_TEST_CASE(ObservableValueCalculator_4_f, distmeasures_qudit4_fixture<float>)
{
  MatrixType A ..............  
  Tomographer::DenseDM::ObservableValueCalculator<DMTypes> f(dmt, Tomographer::DenseDM::ParamX<DMTypes>(dmt).HermToX(rho1));

  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T1), , tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(f.getValue(T2), , tol_f);
}









BOOST_AUTO_TEST_SUITE_END()

