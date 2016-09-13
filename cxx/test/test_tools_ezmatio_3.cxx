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

#include <tomographer2/tools/ezmatio.h>

#include <boost/math/constants/constants.hpp>

#include "test_tools_ezmatio_common.h"

BOOST_FIXTURE_TEST_SUITE(test_tools_ezmatio_3, load_mat_test_file);
// =============================================================================

BOOST_AUTO_TEST_SUITE(stdvec_of_eigen);
BOOST_AUTO_TEST_CASE(mu32_3x3)
{
  Tomographer::MAT::Var var = f.var("mu32_3x3");
  typedef Tomographer::Tools::EigenStdVector<Eigen::Matrix<uint32_t,3,3> >::type MyType;
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
  typedef Tomographer::Tools::EigenStdVector<Eigen::Matrix<Cd,2,2> >::type MyType;
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
  typedef Tomographer::Tools::EigenStdVector<Eigen::Matrix<Cd,2,2,Eigen::RowMajor> >::type MyType;
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
  typedef Tomographer::Tools::EigenStdVector<Eigen::Matrix<Cf,2,2,Eigen::RowMajor> >::type MyType;
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
  typedef Tomographer::Tools::EigenStdVector<Eigen::Matrix<Cf,2,3> >::type MyType;
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
  typedef Tomographer::Tools::EigenStdVector<Eigen::Matrix<Cd,2,3> >::type MyType;
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
  typedef Tomographer::Tools::EigenStdVector<Eigen::Matrix<Cd,2,3,Eigen::RowMajor> >::type MyType;
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


BOOST_AUTO_TEST_SUITE(psdeigen);

BOOST_AUTO_TEST_SUITE(psdf_3x3);
BOOST_AUTO_TEST_CASE(eigf)
{
  typedef Eigen::Matrix<float,Eigen::Dynamic,Eigen::Dynamic> MatrixType;
  Tomographer::MAT::EigenPosSemidefMatrixWithSqrt<MatrixType> mpsd =
    Tomographer::MAT::value<Tomographer::MAT::EigenPosSemidefMatrixWithSqrt<MatrixType> >(f.var("psdf_3x3"));
  MatrixType truemat(3,3);
  truemat <<
    1, 0.5, 0.2,
    0.5,   1, 0.1,
    0.2, 0.1,   1;
  MatrixType truesqrt(3,3);
  truesqrt <<
    9.6135908e-01,   2.5755921e-01,   9.7219139e-02,
    2.5755921e-01,   9.6550506e-01,   3.8244441e-02,
    9.7219139e-02,   3.8244441e-02,   9.9452764e-01;

  MY_BOOST_CHECK_EIGEN_EQUAL(mpsd.mat, truemat, 1e-3);
  MY_BOOST_CHECK_EIGEN_EQUAL(mpsd.sqrt, truesqrt, 1e-3);
}
BOOST_AUTO_TEST_CASE(eigf_st3)
{
  typedef Eigen::Matrix<float,3,3> MatrixType;
  Tomographer::MAT::EigenPosSemidefMatrixWithSqrt<MatrixType> mpsd =
    Tomographer::MAT::value<Tomographer::MAT::EigenPosSemidefMatrixWithSqrt<MatrixType> >(f.var("psdf_3x3"));
  MatrixType truemat;
  truemat <<
    1, 0.5, 0.2,
    0.5,   1, 0.1,
    0.2, 0.1,   1;
  MatrixType truesqrt;
  truesqrt <<
    9.6135908e-01,   2.5755921e-01,   9.7219139e-02,
    2.5755921e-01,   9.6550506e-01,   3.8244441e-02,
    9.7219139e-02,   3.8244441e-02,   9.9452764e-01;

  MY_BOOST_CHECK_EIGEN_EQUAL(mpsd.mat, truemat, 1e-3);
  MY_BOOST_CHECK_EIGEN_EQUAL(mpsd.sqrt, truesqrt, 1e-3);
}
BOOST_AUTO_TEST_CASE(eigd)
{
  typedef Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic> MatrixType;
  Tomographer::MAT::EigenPosSemidefMatrixWithSqrt<MatrixType> mpsd =
    Tomographer::MAT::value<Tomographer::MAT::EigenPosSemidefMatrixWithSqrt<MatrixType> >(f.var("psdf_3x3"));
  MatrixType truemat(3,3);
  truemat <<
    1, 0.5, 0.2,
    0.5,   1, 0.1,
    0.2, 0.1,   1;
  MatrixType truesqrt(3,3);
  truesqrt <<
    9.6135908e-01,   2.5755921e-01,   9.7219139e-02,
    2.5755921e-01,   9.6550506e-01,   3.8244441e-02,
    9.7219139e-02,   3.8244441e-02,   9.9452764e-01;

  MY_BOOST_CHECK_EIGEN_EQUAL(mpsd.mat, truemat, 1e-8);
  MY_BOOST_CHECK_EIGEN_EQUAL(mpsd.sqrt, truesqrt, 1e-5);
}
BOOST_AUTO_TEST_CASE(eigd_p)
{
  typedef Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic> MatrixType;
  Tomographer::MAT::EigenPosSemidefMatrixWithSqrt<MatrixType> mpsd =
    Tomographer::MAT::value<Tomographer::MAT::EigenPosSemidefMatrixWithSqrt<MatrixType> >(f.var("psdf_3x3"), 1e-6);
  MatrixType truemat(3,3);
  truemat <<
    1, 0.5, 0.2,
    0.5,   1, 0.1,
    0.2, 0.1,   1;
  MatrixType truesqrt(3,3);
  truesqrt <<
    9.6135908e-01,   2.5755921e-01,   9.7219139e-02,
    2.5755921e-01,   9.6550506e-01,   3.8244441e-02,
    9.7219139e-02,   3.8244441e-02,   9.9452764e-01;

  MY_BOOST_CHECK_EIGEN_EQUAL(mpsd.mat, truemat, 1e-8);
  MY_BOOST_CHECK_EIGEN_EQUAL(mpsd.sqrt, truesqrt, 1e-3);
}
BOOST_AUTO_TEST_SUITE_END(); // psdf_3x3

BOOST_AUTO_TEST_SUITE(psdd_2x2);
BOOST_AUTO_TEST_CASE(eigd)
{
  typedef Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic> MatrixType;
  Tomographer::MAT::EigenPosSemidefMatrixWithSqrt<MatrixType> mpsd =
    Tomographer::MAT::value<Tomographer::MAT::EigenPosSemidefMatrixWithSqrt<MatrixType> >(f.var("psdd_2x2"));
  auto SQRT2 = boost::math::constants::root_two<double>();

  // reasonable default for tolerance
  auto tolerance = Tomographer::MAT::EigenPosSemidefMatrixWithSqrt<MatrixType>::Params().tolerance;
  BOOST_MESSAGE("tolerance is " << std::scientific << tolerance) ;
  BOOST_CHECK(tolerance < 1e-8 && tolerance > 1e-15);

  MatrixType truemat(2,2);
  truemat <<
    0.5*SQRT2,    0.5*SQRT2,
    0.5*SQRT2,    0.5*SQRT2;
  MatrixType truesqrt(2,2);
  auto ssqrt2 = std::sqrt(SQRT2);
  truesqrt <<
    0.5*ssqrt2,    0.5*ssqrt2,
    0.5*ssqrt2,    0.5*ssqrt2;

  MY_BOOST_CHECK_EIGEN_EQUAL(mpsd.mat, truemat, 2e-8);
  MY_BOOST_CHECK_EIGEN_EQUAL(mpsd.sqrt, truesqrt, 2e-4);

  auto v = mpsd.eig.eigenvalues();
  auto v1 = std::min(v(0), v(1));
  auto v2 = std::max(v(0), v(1));
  MY_BOOST_CHECK_FLOATS_EQUAL(v1, 0.0, 1e-12);
  MY_BOOST_CHECK_FLOATS_EQUAL(v2, boost::math::constants::root_two<double>(), 1e-12);
}
BOOST_AUTO_TEST_CASE(eigf)
{
  typedef Eigen::Matrix<float,Eigen::Dynamic,Eigen::Dynamic> MatrixType;
  Tomographer::MAT::EigenPosSemidefMatrixWithSqrt<MatrixType> mpsd =
    Tomographer::MAT::value<Tomographer::MAT::EigenPosSemidefMatrixWithSqrt<MatrixType> >(f.var("psdd_2x2"));
  MatrixType truemat(2,2);
  truemat <<
    boost::math::constants::half_root_two<float>(), boost::math::constants::half_root_two<float>(),
    boost::math::constants::half_root_two<float>(), boost::math::constants::half_root_two<float>();
  MatrixType truesqrt(2,2);
  truesqrt << 0.5, 0.5, 0.5, 0.5;
  truesqrt *= std::sqrt(boost::math::constants::root_two<float>());

  MY_BOOST_CHECK_EIGEN_EQUAL(mpsd.mat, truemat, 1e-4f);
  MY_BOOST_CHECK_EIGEN_EQUAL(mpsd.sqrt, truesqrt, 5e-3f);

  auto v = mpsd.eig.eigenvalues();
  auto v1 = std::min(v(0), v(1));
  auto v2 = std::max(v(0), v(1));
  MY_BOOST_CHECK_FLOATS_EQUAL(v1, 0.0f, 1e-5f);
  MY_BOOST_CHECK_FLOATS_EQUAL(v2, boost::math::constants::root_two<float>(), 1e-5f);
}
BOOST_AUTO_TEST_CASE(eigd_testp)
{
  typedef Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic> MatrixType;

  double tolerance = 0.1;

  Tomographer::MAT::EigenPosSemidefMatrixWithSqrt<MatrixType> mpsd =
    Tomographer::MAT::value<Tomographer::MAT::EigenPosSemidefMatrixWithSqrt<MatrixType> >(f.var("psdd_2x2"), tolerance);

  auto SQRT2 = boost::math::constants::root_two<double>();

  MatrixType truemat(2,2);
  truemat <<
    0.5*SQRT2,    0.5*(SQRT2-2*tolerance),
    0.5*(SQRT2-2*tolerance),    0.5*SQRT2;
  
  MatrixType truesqrt(2,2);
  auto aa = std::sqrt(SQRT2 - tolerance);
  auto bb = std::sqrt(tolerance);
  truesqrt <<
    0.5*(aa+bb), 0.5*(aa-bb),
    0.5*(aa-bb), 0.5*(aa+bb);

  MY_BOOST_CHECK_EIGEN_EQUAL(mpsd.mat, truemat, tol);
  MY_BOOST_CHECK_EIGEN_EQUAL(mpsd.sqrt, truesqrt, tol);

  auto v = mpsd.eig.eigenvalues();
  auto v1 = std::min(v(0), v(1));
  auto v2 = std::max(v(0), v(1));

  MY_BOOST_CHECK_FLOATS_EQUAL(v1, 0.0, 1e-12);
  MY_BOOST_CHECK_FLOATS_EQUAL(v2, boost::math::constants::root_two<double>(), 1e-12);

  Eigen::SelfAdjointEigenSolver<MatrixType> eigx(mpsd.mat);
  auto vv = eigx.eigenvalues();
  auto vv1 = std::min(vv(0), vv(1));
  auto vv2 = std::max(vv(0), vv(1));

  MY_BOOST_CHECK_FLOATS_EQUAL(vv1, 0.1, 1e-12);
  MY_BOOST_CHECK_FLOATS_EQUAL(vv2, boost::math::constants::root_two<double>()-0.1, 1e-12);

  Eigen::SelfAdjointEigenSolver<MatrixType> eigy(mpsd.sqrt);
  auto vy = eigy.eigenvalues();
  auto vy1 = std::min(vy(0), vy(1));
  auto vy2 = std::max(vy(0), vy(1));

  MY_BOOST_CHECK_FLOATS_EQUAL(vy1, std::sqrt(0.1), 1e-12);
  MY_BOOST_CHECK_FLOATS_EQUAL(vy2, std::sqrt(boost::math::constants::root_two<double>()-0.1), 1e-12);
}
BOOST_AUTO_TEST_SUITE_END(); // psdd_2x2

BOOST_AUTO_TEST_SUITE(psdcf_2x2);
BOOST_AUTO_TEST_CASE(eigf)
{
  typedef Eigen::Matrix<std::complex<float>,Eigen::Dynamic,Eigen::Dynamic> MatrixType;
  Tomographer::MAT::EigenPosSemidefMatrixWithSqrt<MatrixType> mpsd =
    Tomographer::MAT::value<Tomographer::MAT::EigenPosSemidefMatrixWithSqrt<MatrixType> >(f.var("psdcf_2x2"));

  // reasonable default for tolerance
  auto tolerance = Tomographer::MAT::EigenPosSemidefMatrixWithSqrt<MatrixType>::Params().tolerance;
  BOOST_MESSAGE("tolerance is " << std::scientific << tolerance) ;
  BOOST_CHECK(tolerance < 1e-3 && tolerance > 1e-7);

  MatrixType truemat(2,2);
  truemat <<
    0.5f,    std::complex<float>(0,0.5f),
    std::complex<float>(0,-0.5f),    0.5f;

  MY_BOOST_CHECK_EIGEN_EQUAL(mpsd.mat, truemat, 2e-3);
  MY_BOOST_CHECK_EIGEN_EQUAL(mpsd.sqrt, truemat, 2e-2);

  auto v = mpsd.eig.eigenvalues();
  auto v1 = std::min(v(0), v(1));
  auto v2 = std::max(v(0), v(1));
  MY_BOOST_CHECK_FLOATS_EQUAL(v1, 0.0, tolerance);
  MY_BOOST_CHECK_FLOATS_EQUAL(v2, 1, tolerance);
}
BOOST_AUTO_TEST_SUITE_END(); // psdcf_2x2

BOOST_AUTO_TEST_SUITE(psdcd_2x2);
BOOST_AUTO_TEST_CASE(eigd)
{
  typedef Eigen::Matrix<std::complex<double>,Eigen::Dynamic,Eigen::Dynamic> MatrixType;
  Tomographer::MAT::EigenPosSemidefMatrixWithSqrt<MatrixType> mpsd =
    Tomographer::MAT::value<Tomographer::MAT::EigenPosSemidefMatrixWithSqrt<MatrixType> >(f.var("psdcd_2x2"));
  auto SQRT2 = boost::math::constants::root_two<double>();

  // reasonable default for tolerance
  auto tolerance = Tomographer::MAT::EigenPosSemidefMatrixWithSqrt<MatrixType>::Params().tolerance;
  BOOST_MESSAGE("tolerance is " << std::scientific << tolerance) ;
  BOOST_CHECK(tolerance < 1e-8 && tolerance > 1e-15);

  MatrixType truemat(2,2);
  truemat <<
    0.5*SQRT2,     std::complex<double>(0,0.5*SQRT2),
    std::complex<double>(0,-0.5*SQRT2),    0.5*SQRT2;
  MatrixType truesqrt(2,2);
  auto ssqrt2 = std::sqrt(SQRT2);
  truesqrt <<
    0.5*ssqrt2,     std::complex<double>(0,0.5*ssqrt2),
    std::complex<double>(0,-0.5*ssqrt2),    0.5*ssqrt2;

  MY_BOOST_CHECK_EIGEN_EQUAL(mpsd.mat, truemat, 2e-8);
  MY_BOOST_CHECK_EIGEN_EQUAL(mpsd.sqrt, truesqrt, 2e-4);

  auto v = mpsd.eig.eigenvalues();
  auto v1 = std::min(v(0), v(1));
  auto v2 = std::max(v(0), v(1));
  MY_BOOST_CHECK_FLOATS_EQUAL(v1, 0.0, 1e-12);
  MY_BOOST_CHECK_FLOATS_EQUAL(v2, boost::math::constants::root_two<double>(), 1e-12);
}
BOOST_AUTO_TEST_SUITE_END(); // psdcd_2x2

BOOST_AUTO_TEST_SUITE_END(); // psdeigen

BOOST_AUTO_TEST_SUITE_END(); // test_tools_ezmatio_3
