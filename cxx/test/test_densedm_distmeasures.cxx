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

#include <tomographer/densedm/distmeasures.h>
#include <tomographer/densedm/dmtypes.h>




// -----------------------------------------------------------------------------
// fixture(s)

#include "test_densedm_distmeasures_common.h"


// -----------------------------------------------------------------------------
// test suites


BOOST_AUTO_TEST_SUITE(test_densedm_distmeasures)

BOOST_FIXTURE_TEST_SUITE(qubit_d, distmeasures_qubit_fixture<double>)

BOOST_AUTO_TEST_CASE(internal_test_fixture)
{
  internal_test_fixture();
}

BOOST_AUTO_TEST_CASE(trace_dist)
{
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::trace_dist<RealScalar>(rho1, rho1), trdist_with_1(1), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::trace_dist<RealScalar>(rho1, rho2), trdist_with_1(2), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::trace_dist<RealScalar>(rho1, rho3), trdist_with_1(3), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::trace_dist<RealScalar>(rho1, rho4), trdist_with_1(4), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::trace_dist<RealScalar>(rho1, rho5), trdist_with_1(5), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::trace_dist<RealScalar>(rho1, rho6), trdist_with_1(6), tol);
}

BOOST_AUTO_TEST_CASE(fidelity)
{
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity<RealScalar>(rho1, rho1), fid_with_1(1), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity<RealScalar>(rho1, rho2), fid_with_1(2), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity<RealScalar>(rho1, rho3), fid_with_1(3), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity<RealScalar>(rho1, rho4), fid_with_1(4), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity<RealScalar>(rho1, rho5), fid_with_1(5), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity<RealScalar>(rho1, rho6), fid_with_1(6), tol);
}

BOOST_AUTO_TEST_CASE(fidelity_T)
{
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity_T<RealScalar>(T1, T1), fid_with_1(1), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity_T<RealScalar>(T1, T2), fid_with_1(2), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity_T<RealScalar>(T1, T2b), fid_with_1(2), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity_T<RealScalar>(T1, T3), fid_with_1(3), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity_T<RealScalar>(T1, T4), fid_with_1(4), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity_T<RealScalar>(T1, T5), fid_with_1(5), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity_T<RealScalar>(T1, T6), fid_with_1(6), tol);
}

BOOST_AUTO_TEST_SUITE_END()

// -----------------------------------------------

BOOST_FIXTURE_TEST_SUITE(qubit_f, distmeasures_qubit_fixture<float>)

BOOST_AUTO_TEST_CASE(internal_test_fixture)
{
  internal_test_fixture();
}

BOOST_AUTO_TEST_CASE(trace_dist)
{
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::trace_dist<RealScalar>(rho1, rho1), trdist_with_1(1), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::trace_dist<RealScalar>(rho1, rho2), trdist_with_1(2), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::trace_dist<RealScalar>(rho1, rho3), trdist_with_1(3), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::trace_dist<RealScalar>(rho1, rho4), trdist_with_1(4), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::trace_dist<RealScalar>(rho1, rho5), trdist_with_1(5), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::trace_dist<RealScalar>(rho1, rho6), trdist_with_1(6), tol_f);
}

BOOST_AUTO_TEST_CASE(fidelity)
{
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity<RealScalar>(rho1, rho1), fid_with_1(1), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity<RealScalar>(rho1, rho2), fid_with_1(2), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity<RealScalar>(rho1, rho3), fid_with_1(3), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity<RealScalar>(rho1, rho4), fid_with_1(4), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity<RealScalar>(rho1, rho5), fid_with_1(5), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity<RealScalar>(rho1, rho6), fid_with_1(6), tol_f);
}

BOOST_AUTO_TEST_CASE(fidelity_T)
{
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity_T<RealScalar>(T1, T1), fid_with_1(1), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity_T<RealScalar>(T1, T2), fid_with_1(2), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity_T<RealScalar>(T1, T2b), fid_with_1(2), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity_T<RealScalar>(T1, T3), fid_with_1(3), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity_T<RealScalar>(T1, T4), fid_with_1(4), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity_T<RealScalar>(T1, T5), fid_with_1(5), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity_T<RealScalar>(T1, T6), fid_with_1(6), tol_f);
}

BOOST_AUTO_TEST_SUITE_END()

// -----------------------------------------------

BOOST_FIXTURE_TEST_SUITE(qudit4_d, distmeasures_qudit4_fixture<double>)

BOOST_AUTO_TEST_CASE(trace_dist)
{
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::trace_dist<RealScalar>(rho1, rho1), trdist_with_1(1), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::trace_dist<RealScalar>(rho1, rho2), trdist_with_1(2), tol);
}
BOOST_AUTO_TEST_CASE(fidelity)
{
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity<RealScalar>(rho1, rho1), fid_with_1(1), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity<RealScalar>(rho1, rho2), fid_with_1(2), tol);
}
BOOST_AUTO_TEST_CASE(fidelity_T)
{
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity_T<RealScalar>(T1, T1), fid_with_1(1), tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity_T<RealScalar>(T1, T2), fid_with_1(2), tol);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(qudit4_f, distmeasures_qudit4_fixture<float>)

BOOST_AUTO_TEST_CASE(trace_dist)
{
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::trace_dist<RealScalar>(rho1, rho1), trdist_with_1(1), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::trace_dist<RealScalar>(rho1, rho2), trdist_with_1(2), tol_f);
}
BOOST_AUTO_TEST_CASE(fidelity)
{
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity<RealScalar>(rho1, rho1), fid_with_1(1), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity<RealScalar>(rho1, rho2), fid_with_1(2), tol_f);
}
BOOST_AUTO_TEST_CASE(fidelity_T)
{
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity_T<RealScalar>(T1, T1), fid_with_1(1), tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(Tomographer::DenseDM::fidelity_T<RealScalar>(T1, T2), fid_with_1(2), tol_f);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
