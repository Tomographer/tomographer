/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
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

#include <cstdint>

#include <vector>
#include <algorithm>

#include "test_tomographer.h"

#include <tomographer/tools/ezmatio.h>

#include "test_tools_ezmatio_common.h"

BOOST_FIXTURE_TEST_SUITE(test_tools_ezmatio_3, load_mat_test_file);
// =============================================================================

BOOST_AUTO_TEST_SUITE(stdvec_of_eigen);
BOOST_AUTO_TEST_CASE(mu32_3x3)
{
  Tomographer::MAT::Var var = f.var("mu32_3x3");
  typedef Tomographer::Tools::eigen_std_vector<Eigen::Matrix<uint32_t,3,3> >::type MyType;
  MyType m = var.value<MyType>();
  BOOST_CHECK_EQUAL(m.size(), 1);
  Eigen::Matrix<uint32_t,3,3> ok;
  ok << 1, 1, 1, 2, 2, 2, 4294967295lu, 0, 0 ;
  MY_BOOST_CHECK_EIGEN_EQUAL(m[0], ok, tol);
}
BOOST_AUTO_TEST_CASE(mcd_2x2x3)
{
  Tomographer::MAT::Var var = f.var("mcd_2x2x3");
  typedef std::complex<double> Cd;
  typedef Tomographer::Tools::eigen_std_vector<Eigen::Matrix<Cd,2,2> >::type MyType;
  MyType m = var.value<MyType>();
  BOOST_CHECK_EQUAL(m.size(),3);
  Eigen::Matrix<Cd,2,2> ok1;
  ok1 <<
    Cd(0), Cd(1),
    Cd(1), Cd(0);
  MY_BOOST_CHECK_EIGEN_EQUAL(m[0], ok1, tol);
  Eigen::Matrix<Cd,2,2> ok2;
  ok2 <<
    Cd(0), Cd(0,-1),
    Cd(0,1), Cd(0);
  MY_BOOST_CHECK_EIGEN_EQUAL(m[1], ok2, tol);
  Eigen::Matrix<Cd,2,2> ok3;
  ok3 <<
    Cd(1), Cd(0),
    Cd(0), Cd(-1);
  MY_BOOST_CHECK_EIGEN_EQUAL(m[2], ok3, tol);
}
BOOST_AUTO_TEST_CASE(mcd_2x2x3_rowmaj)
{
  Tomographer::MAT::Var var = f.var("mcd_2x2x3");
  typedef std::complex<double> Cd;
  typedef Tomographer::Tools::eigen_std_vector<Eigen::Matrix<Cd,2,2,Eigen::RowMajor> >::type MyType;
  MyType m = var.value<MyType>();
  BOOST_CHECK_EQUAL(m.size(),3);
  Eigen::Matrix<Cd,2,2> ok1;
  ok1 <<
    Cd(0), Cd(1),
    Cd(1), Cd(0);
  MY_BOOST_CHECK_EIGEN_EQUAL(m[0], ok1, tol);
  Eigen::Matrix<Cd,2,2> ok2;
  ok2 <<
    Cd(0), Cd(0,-1),
    Cd(0,1), Cd(0);
  MY_BOOST_CHECK_EIGEN_EQUAL(m[1], ok2, tol);
  Eigen::Matrix<Cd,2,2> ok3;
  ok3 <<
    Cd(1), Cd(0),
    Cd(0), Cd(-1);
  MY_BOOST_CHECK_EIGEN_EQUAL(m[2], ok3, tol);
}
BOOST_AUTO_TEST_CASE(mcf_2x2x3)
{
  Tomographer::MAT::Var var = f.var("mcf_2x2x3");
  typedef std::complex<float> Cf;
  typedef Tomographer::Tools::eigen_std_vector<Eigen::Matrix<Cf,2,2,Eigen::RowMajor> >::type MyType;
  MyType m = var.value<MyType>();
  BOOST_CHECK_EQUAL(m.size(),3);
  Eigen::Matrix<Cf,2,2> ok1;
  ok1 <<
    Cf(0), Cf(1),
    Cf(1), Cf(0);
  MY_BOOST_CHECK_EIGEN_EQUAL(m[0], ok1, tol);
  Eigen::Matrix<Cf,2,2> ok2;
  ok2 <<
    Cf(0), Cf(0,-1),
    Cf(0,1), Cf(0);
  MY_BOOST_CHECK_EIGEN_EQUAL(m[1], ok2, tol);
  Eigen::Matrix<Cf,2,2> ok3;
  ok3 <<
    Cf(1), Cf(0),
    Cf(0), Cf(-1);
  MY_BOOST_CHECK_EIGEN_EQUAL(m[2], ok3, tol);
}
BOOST_AUTO_TEST_CASE(mf_2x3x2) 
{
  Tomographer::MAT::Var var = f.var("mf_2x3x2");
  typedef std::complex<float> Cf;
  typedef Tomographer::Tools::eigen_std_vector<Eigen::Matrix<Cf,2,3> >::type MyType;
  MyType m = var.value<MyType>();
  BOOST_CHECK_EQUAL(m.size(), 2);
  Eigen::Matrix<Cf,2,3> ok0;
  ok0 <<
    1.f,  4.f,   -2.5f,
    1.f,  1.5f,  -1e4f;
  MY_BOOST_CHECK_EIGEN_EQUAL(m[0], ok0, tol);
  Eigen::Matrix<Cf,2,3> ok1;
  ok1 <<
    0, 0, 0,
    1, -2, -3 ;
  MY_BOOST_CHECK_EIGEN_EQUAL(m[1], ok1, tol);
}
BOOST_AUTO_TEST_CASE(mcd_2x3x2x2) 
{
  Tomographer::MAT::Var var = f.var("mcd_2x3x2x2");
  typedef std::complex<double> Cd;
  typedef Tomographer::Tools::eigen_std_vector<Eigen::Matrix<Cd,2,3> >::type MyType;
  MyType m = var.value<MyType>();
  BOOST_CHECK_EQUAL(m.size(),4);
  Eigen::Matrix<Cd,2,3> ok0;
  ok0 <<
    1, Cd(0,1),   Cd(0,-1),
    1, Cd(0,1.5), Cd(-1e4,1e3);
  MY_BOOST_CHECK_EIGEN_EQUAL(m[0], ok0, tol);
  Eigen::Matrix<Cd,2,3> ok1;
  ok1 <<
    0,       0,        0,
    Cd(0,1), Cd(0,-2), Cd(0,-3);
  MY_BOOST_CHECK_EIGEN_EQUAL(m[1], ok1, tol);
  Eigen::Matrix<Cd,2,3> ok2;
  ok2 <<
    1, 0, 0,
    0, 1, 0;
  MY_BOOST_CHECK_EIGEN_EQUAL(m[2], ok2, tol);
  Eigen::Matrix<Cd,2,3> ok3;
  ok3 <<
    0,       0,        0,
    Cd(0,1), Cd(0,-2), Cd(0,-3);
  MY_BOOST_CHECK_EIGEN_EQUAL(m[3], ok3, tol);
}
BOOST_AUTO_TEST_CASE(mcd_2x3x2x2_rowmaj)
{
  Tomographer::MAT::Var var = f.var("mcd_2x3x2x2");
  typedef std::complex<double> Cd;
  typedef Tomographer::Tools::eigen_std_vector<Eigen::Matrix<Cd,2,3,Eigen::RowMajor> >::type MyType;
  MyType m = var.value<MyType>();
  BOOST_CHECK_EQUAL(m.size(),4);
  Eigen::Matrix<Cd,2,3> ok0;
  ok0 <<
    1, Cd(0,1),   Cd(0,-1),
    1, Cd(0,1.5), Cd(-1e4,1e3);
  MY_BOOST_CHECK_EIGEN_EQUAL(m[0], ok0, tol);
  Eigen::Matrix<Cd,2,3> ok1;
  ok1 <<
    0,       0,        0,
    Cd(0,1), Cd(0,-2), Cd(0,-3);
  MY_BOOST_CHECK_EIGEN_EQUAL(m[1], ok1, tol);
  Eigen::Matrix<Cd,2,3> ok2;
  ok2 <<
    1, 0, 0,
    0, 1, 0;
  MY_BOOST_CHECK_EIGEN_EQUAL(m[2], ok2, tol);
  Eigen::Matrix<Cd,2,3> ok3;
  ok3 <<
    0,       0,        0,
    Cd(0,1), Cd(0,-2), Cd(0,-3);
  MY_BOOST_CHECK_EIGEN_EQUAL(m[3], ok3, tol);
}
BOOST_AUTO_TEST_SUITE_END(); // stdvec_of_eigen


BOOST_AUTO_TEST_SUITE_END(); // test_tools_ezmatio_3
