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
#include <cstdlib>
#include <cmath>

#include <vector>
#include <algorithm>

#include "test_tomographer.h"

#include <tomographer/tools/ezmatio.h>

#include "test_tools_ezmatio_common.h"

BOOST_FIXTURE_TEST_SUITE(test_tools_ezmatio_2, load_mat_test_file);
// =============================================================================


BOOST_AUTO_TEST_SUITE(getstdvector_conv);
BOOST_AUTO_TEST_CASE(rowmaj)
{
  Tomographer::MAT::Var var = f.var("md_4x3");
  std::vector<double> v = var.value<Tomographer::MAT::GetStdVector<double, true> >();
  static const double ok[] = {
    1.0, 2.0, 3.0, 1.5, 3, 4.5, 100.0, 200.0, 300.0, 0.0, 0.0, 1.0
  };
  MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
}
BOOST_AUTO_TEST_CASE(colmaj)
{
  Tomographer::MAT::Var var = f.var("md_4x3");
  std::vector<double> v = var.value<Tomographer::MAT::GetStdVector<double, false> >();
  static const double ok[] = {
    1.0, 1.5, 100, 0, 2, 3, 200, 0, 3, 4.5, 300, 1
  };
  MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
}
BOOST_AUTO_TEST_CASE(conv_d_f)
{
  Tomographer::MAT::Var var = f.var("md_4x3");
  std::vector<float> v = var.value<Tomographer::MAT::GetStdVector<float, false> >();
  static const float ok[] = {
    1.0, 1.5, 100, 0, 2, 3, 200, 0, 3, 4.5, 300, 1
  };
  MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
}
BOOST_AUTO_TEST_CASE(conv_d_cf)
{
  Tomographer::MAT::Var var = f.var("md_4x3");
  std::vector<std::complex<float> > v = var.value<Tomographer::MAT::GetStdVector<std::complex<float>, false> >();
  static const float ok[] = {
    1.0, 1.5, 100, 0, 2, 3, 200, 0, 3, 4.5, 300, 1
  };
  MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
}
BOOST_AUTO_TEST_CASE(conv_d_cd_rowmaj)
{
  Tomographer::MAT::Var var = f.var("md_4x3");
  std::vector<std::complex<double> > v = var.value<Tomographer::MAT::GetStdVector<std::complex<double>, true> >();
  static const double ok[] = {
    1.0, 2.0, 3.0, 1.5, 3, 4.5, 100.0, 200.0, 300.0, 0.0, 0.0, 1.0
  };
  MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
}
BOOST_AUTO_TEST_SUITE_END(); // getstdvector_conv


BOOST_AUTO_TEST_SUITE(getstdvector);
BOOST_AUTO_TEST_CASE(md_4x3)
{
  Tomographer::MAT::Var var = f.var("md_4x3");
  std::vector<double> v = var.value<Tomographer::MAT::GetStdVector<double, true> >();
  static const double ok[] = {
    1.0, 2.0, 3.0, 1.5, 3, 4.5, 100.0, 200.0, 300.0, 0.0, 0.0, 1.0
  };
  MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
}
BOOST_AUTO_TEST_CASE(rvd_5)
{
  Tomographer::MAT::Var var = f.var("rvd_5");
  std::vector<double> v = var.value<Tomographer::MAT::GetStdVector<double, true> >();
  static const double ok[] = {
    1.0, 2.0, -3.0, 4.0, -193.223
  };
  MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
}
BOOST_AUTO_TEST_CASE(vd_5)
{
  Tomographer::MAT::Var var = f.var("vd_5");
  std::vector<double> v = var.value<Tomographer::MAT::GetStdVector<double, true> >();
  static const double ok[] = {
    1.0, 2.0, -3.0, 4.0, -193.223
  };
  MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
}
BOOST_AUTO_TEST_CASE(mcd_4x3)
{
  Tomographer::MAT::Var var = f.var("mcd_4x3");
  typedef std::complex<double> Cd;
  std::vector<Cd> v = var.value<Tomographer::MAT::GetStdVector<Cd, true> >();
  const std::vector<Cd> ok{ // row major
    Cd(1,1),
      Cd(0,2),
      Cd(0,3),
      Cd(0,1.5),
      Cd(1,3),
      Cd(0,4.5),
      Cd(0,100),
      Cd(0,200),
      Cd(1,300),
      Cd(0,0),
      Cd(0,0),
      Cd(0,1)
      };
  MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
}
BOOST_AUTO_TEST_CASE(rvcd_5)
{
  Tomographer::MAT::Var var = f.var("rvcd_5");
  typedef std::complex<double> Cd;
  std::vector<Cd> v = var.value<Tomographer::MAT::GetStdVector<Cd> >();
  const std::vector<Cd> ok{ // row major
    Cd(1,1),
      Cd(2,2.5),
      Cd(-3,0),
      Cd(4,0),
      Cd(-193.223,0),
      };
  MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
}
BOOST_AUTO_TEST_CASE(vcd_5)
{
  Tomographer::MAT::Var var = f.var("vcd_5");
  typedef std::complex<double> Cd;
  std::vector<Cd> v = var.value<Tomographer::MAT::GetStdVector<Cd> >();
  const std::vector<Cd> ok{ // row major
    Cd(1,1),
      Cd(2,-2.5),
      Cd(-3,0),
      Cd(4,0),
      Cd(-193.223,0),
      };
  MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
}
BOOST_AUTO_TEST_CASE(mf_4x3)
{
  Tomographer::MAT::Var var = f.var("mf_4x3");
  std::vector<float> v = var.value<Tomographer::MAT::GetStdVector<float, true> >();
  const float ok[] = {
    1.0f, 2.0f, 3.0f,
    1.5f, 3.f, 4.5f,
    100.0f, 200.0f, 300.0f,
    0.0f, 0.0f, 1.0f
  };
  MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
}
BOOST_AUTO_TEST_CASE(rvf_5)
{
  Tomographer::MAT::Var var = f.var("rvf_5");
  std::vector<float> v = var.value<Tomographer::MAT::GetStdVector<float, true> >();
  const float ok[] = {
    1.0f, 2.0f, -3.0f, 4.0f, -193.223f
  };
  MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
}
BOOST_AUTO_TEST_CASE(vf_5)
{
  Tomographer::MAT::Var var = f.var("vf_5");
  std::vector<float> v = var.value<Tomographer::MAT::GetStdVector<float, true> >();
  const float ok[] = {
    1.0f, 2.0f, -3.0f, 4.0f, -193.223f
  };
  MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
}
BOOST_AUTO_TEST_CASE(mi8_3x3)
{
  Tomographer::MAT::Var var = f.var("mi8_3x3");
  std::vector<int8_t> v = var.value<Tomographer::MAT::GetStdVector<int8_t, true> >();
  const std::vector<int8_t> ok{
    1, 1, 1, 2, 2, 2, 127, 0, -128
      };
  MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
}
BOOST_AUTO_TEST_CASE(mi32_3x3)
{
  Tomographer::MAT::Var var = f.var("mi32_3x3");
  std::vector<int32_t> v = var.value<Tomographer::MAT::GetStdVector<int32_t, true> >();
  const std::vector<int32_t> ok{
    1, 1, 1, 2, 2, 2, 2147483647l, 0, -2147483648l
      };
  MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
}
BOOST_AUTO_TEST_CASE(mu32_3x3)
{
  Tomographer::MAT::Var var = f.var("mu32_3x3");
  std::vector<uint32_t> v = var.value<Tomographer::MAT::GetStdVector<uint32_t, true> >();
  const std::vector<uint32_t> ok{
    1, 1, 1, 2, 2, 2, 4294967295lu, 0, 0
      };
  MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
}
BOOST_AUTO_TEST_SUITE_END(); // getstdvector


// -----------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(eigen);
BOOST_AUTO_TEST_CASE(conv)
{
  {
    Tomographer::MAT::Var var = f.var("md_4x3");
    Eigen::MatrixXd m = var.value<Eigen::MatrixXd>();
    BOOST_CHECK_EQUAL(m.rows(), 4);
    BOOST_CHECK_EQUAL(m.cols(), 3);
    Eigen::Matrix<double,4,3> ok;
    ok <<  1.0, 2.0, 3.0, 1.5, 3, 4.5, 100.0, 200.0, 300.0, 0.0, 0.0, 1.0 ;
    MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
  }
  {
    Tomographer::MAT::Var var = f.var("md_4x3");
    typedef Eigen::Matrix<double,4,Eigen::Dynamic,Eigen::RowMajor> MatrixType;
    MatrixType m = var.value<MatrixType>();
    BOOST_CHECK_EQUAL(m.rows(), 4);
    BOOST_CHECK_EQUAL(m.cols(), 3);
    Eigen::Matrix<double,4,3> ok;
    // still same matrix, even if m is stored in row-major format.
    ok <<  1.0, 2.0, 3.0, 1.5, 3, 4.5, 100.0, 200.0, 300.0, 0.0, 0.0, 1.0 ;
    MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
  }
  {
    Tomographer::MAT::Var var = f.var("md_4x3");
    typedef Eigen::Matrix<float,Eigen::Dynamic,3,Eigen::RowMajor> MatrixType;
    MatrixType m = var.value<MatrixType>();
    BOOST_CHECK_EQUAL(m.rows(), 4);
    BOOST_CHECK_EQUAL(m.cols(), 3);
    Eigen::Matrix<float,4,3> ok;
    ok <<  1.0f, 2.0f, 3.0f, 1.5f, 3.f, 4.5f, 100.0f, 200.0f, 300.0f, 0.0f, 0.0f, 1.0f ;
    MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
  }
  {
    Tomographer::MAT::Var var = f.var("md_4x3");
    typedef Eigen::Matrix<float,4,3> MatrixType;
    MatrixType m = var.value<MatrixType>();
    BOOST_CHECK_EQUAL(m.rows(), 4);
    BOOST_CHECK_EQUAL(m.cols(), 3);
    Eigen::Matrix<float,4,3> ok;
    ok <<  1.0f, 2.0f, 3.0f, 1.5f, 3.f, 4.5f, 100.0f, 200.0f, 300.0f, 0.0f, 0.0f, 1.0f ;
    MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
  }
  {
    Tomographer::MAT::Var var = f.var("md_4x3");
    typedef Eigen::Matrix<int,Eigen::Dynamic,Eigen::Dynamic> MatrixType;
    MatrixType m = var.value<MatrixType>();
    BOOST_CHECK_EQUAL(m.rows(), 4);
    BOOST_CHECK_EQUAL(m.cols(), 3);
    Eigen::Matrix<int,4,3> ok;
    ok <<  1, 2, 3, 1, 3, 4, 100, 200, 300, 0, 0, 1 ;
    MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
  }
  {
    Tomographer::MAT::Var var = f.var("rvd_5");
    Eigen::MatrixXd m = var.value<Eigen::MatrixXd>();
    Eigen::RowVectorXd ok(5);
    ok <<  1.0, 2.0, -3.0, 4.0, -193.223 ;
    MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
  }
}
BOOST_AUTO_TEST_CASE(conv_err)
{
  // check exceptions are raised for mismatched shapes
  {
    Tomographer::MAT::Var var = f.var("md_4x3");
    Eigen::MatrixXd m;
    auto t = [&]() { return var.value<Eigen::Matrix<double,43,17> >(); };
    BOOST_CHECK_THROW(m = t(), Tomographer::MAT::VarTypeError);
  }
  {
    Tomographer::MAT::Var var = f.var("md_4x3");
    Eigen::MatrixXd m;
    auto t = [&]() { return var.value<Eigen::Matrix<double,Eigen::Dynamic,17> >(); };
    BOOST_CHECK_THROW(m = t(), Tomographer::MAT::VarTypeError);
  }
  {
    Tomographer::MAT::Var var = f.var("md_4x3");
    Eigen::MatrixXd m;
    auto t = [&]() { return var.value<Eigen::Matrix<double,17,Eigen::Dynamic> >(); };
    BOOST_CHECK_THROW(m = t(), Tomographer::MAT::VarTypeError);
  }
  {
    Tomographer::MAT::Var var = f.var("rvd_5");
    Eigen::MatrixXd m;
    auto t = [&]() { return var.value<Eigen::Matrix<double,5,1> >(); } ;
    BOOST_CHECK_THROW(m = t(), Tomographer::MAT::VarTypeError);
  }
}
BOOST_AUTO_TEST_CASE(md_4x3)
{
  Tomographer::MAT::Var var = f.var("md_4x3");
  Eigen::MatrixXd m = var.value<Eigen::MatrixXd>();
  Eigen::Matrix<double,4,3> ok;
  ok <<  1.0, 2.0, 3.0, 1.5, 3.0, 4.5, 100.0, 200.0, 300.0, 0.0, 0.0, 1.0 ;
  MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
}
BOOST_AUTO_TEST_CASE(rvd_5)
{
  Tomographer::MAT::Var var = f.var("rvd_5");
  Eigen::RowVectorXd m = var.value<Eigen::RowVectorXd>();
  Eigen::RowVectorXd ok(5);
  ok <<  1.0, 2.0, -3.0, 4.0, -193.223 ;
  MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
}
BOOST_AUTO_TEST_CASE(vd_5)
{
  Tomographer::MAT::Var var = f.var("vd_5");
  Eigen::VectorXd m = var.value<Eigen::VectorXd>();
  Eigen::VectorXd ok(5);
  ok <<  1.0, 2.0, -3.0, 4.0, -193.223 ;
  MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
}
BOOST_AUTO_TEST_CASE(mcd_4x3)
{
  Tomographer::MAT::Var var = f.var("mcd_4x3");
  Eigen::MatrixXcd m = var.value<Eigen::MatrixXcd>();
  Eigen::MatrixXcd ok(4,3);
  typedef std::complex<double> Cd;
  ok <<
    Cd(1,1),
    Cd(0,2),
    Cd(0,3),
    Cd(0,1.5),
    Cd(1,3),
    Cd(0,4.5),
    Cd(0,100),
    Cd(0,200),
    Cd(1,300),
    Cd(0,0),
    Cd(0,0),
    Cd(0,1) ;
  MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
}
BOOST_AUTO_TEST_CASE(rvcd_5)
{
  Tomographer::MAT::Var var = f.var("rvcd_5");
  Eigen::MatrixXcd m = var.value<Eigen::MatrixXcd>();
  Eigen::MatrixXcd ok(1,5);
  typedef std::complex<double> Cd;
  ok <<
    Cd(1,1),
    Cd(2,2.5),
    Cd(-3,0),
    Cd(4,0),
    Cd(-193.223,0) ;
  MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
}
BOOST_AUTO_TEST_CASE(vcd_5)
{
  Tomographer::MAT::Var var = f.var("vcd_5");
  Eigen::VectorXcd m = var.value<Eigen::VectorXcd>();
  Eigen::VectorXcd ok(5);
  typedef std::complex<double> Cd;
  ok <<
    Cd(1,1),
    Cd(2,-2.5),
    Cd(-3,0),
    Cd(4,0),
    Cd(-193.223,0) ;
  MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
}
BOOST_AUTO_TEST_CASE(mf_4x3)
{
  Tomographer::MAT::Var var = f.var("mf_4x3");
  Eigen::MatrixXf m = var.value<Eigen::MatrixXf>();
  Eigen::Matrix<float,4,3> ok;
  ok <<
    1.0f, 2.0f, 3.0f,
    1.5f, 3.f, 4.5f,
    100.0f, 200.0f, 300.0f,
    0.0f, 0.0f, 1.0f
    ;
  MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
}
BOOST_AUTO_TEST_CASE(rvf_5)
{
  Tomographer::MAT::Var var = f.var("rvf_5");
  Eigen::Matrix<float,1,5> m = var.value<Eigen::Matrix<float,1,5> >();
  Eigen::Matrix<float,1,5> ok;
  ok <<
    1.0f, 2.0f, -3.0f, 4.0f, -193.223f
    ;
  MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
}
BOOST_AUTO_TEST_CASE(vf_5)
{
  Tomographer::MAT::Var var = f.var("vf_5");
  Eigen::Matrix<float,5,1> m = var.value<Eigen::Matrix<float,5,1> >();
  Eigen::Matrix<float,5,1> ok;
  ok <<
    1.0f, 2.0f, -3.0f, 4.0f, -193.223f
    ;
  MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
}
BOOST_AUTO_TEST_CASE(mi8_3x3)
{
  Tomographer::MAT::Var var = f.var("mi8_3x3");
  Eigen::Matrix<int8_t,Eigen::Dynamic,3> m = var.value<Eigen::Matrix<int8_t,Eigen::Dynamic,3> >();
  Eigen::Matrix<int8_t,3,3> ok;
  ok <<
    1, 1, 1, 2, 2, 2, 127, 0, -128
    ;
  MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
}
BOOST_AUTO_TEST_CASE(mi32_3x3)
{
  Tomographer::MAT::Var var = f.var("mi32_3x3");
  Eigen::Matrix<int32_t,Eigen::Dynamic,Eigen::Dynamic> m
    = var.value<Eigen::Matrix<int32_t,Eigen::Dynamic,Eigen::Dynamic> >();
  Eigen::Matrix<int32_t,3,3> ok;
  ok <<
    1, 1, 1, 2, 2, 2, 2147483647l, 0, -2147483648l
    ;
  MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
}
BOOST_AUTO_TEST_CASE(mu32_3x3)
{
  Tomographer::MAT::Var var = f.var("mu32_3x3");
  Eigen::Matrix<uint32_t,3,3> m = var.value<Eigen::Matrix<uint32_t,3,3> >();
  Eigen::Matrix<uint32_t,3,3> ok;
  ok <<
    1, 1, 1, 2, 2, 2, 4294967295lu, 0, 0
    ;
  MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
}
BOOST_AUTO_TEST_CASE(mcd_2x2x3)
{
  Tomographer::MAT::Var var = f.var("mcd_2x2x3");
  Eigen::MatrixXcd m = var.value<Eigen::MatrixXcd>();
  typedef std::complex<double> Cd;
  Eigen::Matrix<Cd,4,3> ok;
  ok.col(0) = (Eigen::Matrix<Cd,4,1>() << Cd(0), Cd(1), Cd(1), Cd(0)).finished();
  ok.col(1) = (Eigen::Matrix<Cd,4,1>() << Cd(0), Cd(0,1), Cd(0,-1), Cd(0)).finished();
  ok.col(2) = (Eigen::Matrix<Cd,4,1>() << Cd(1), Cd(0), Cd(0), Cd(-1)).finished();
  MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
}
BOOST_AUTO_TEST_CASE(mcd_2x2x3_fix)
{
  Tomographer::MAT::Var var = f.var("mcd_2x2x3");
  typedef std::complex<double> Cd;
  Eigen::Matrix<Cd,4,3> m = var.value<Eigen::Matrix<Cd,4,3> >();
  Eigen::Matrix<Cd,4,3> ok;
  ok.col(0) = (Eigen::Matrix<Cd,4,1>() << Cd(0), Cd(1), Cd(1), Cd(0)).finished();
  ok.col(1) = (Eigen::Matrix<Cd,4,1>() << Cd(0), Cd(0,1), Cd(0,-1), Cd(0)).finished();
  ok.col(2) = (Eigen::Matrix<Cd,4,1>() << Cd(1), Cd(0), Cd(0), Cd(-1)).finished();
  MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
}
BOOST_AUTO_TEST_CASE(mcf_2x2x3)
{
  Tomographer::MAT::Var var = f.var("mcf_2x2x3");
  typedef std::complex<float> Cf;
  Eigen::Matrix<Cf,4,Eigen::Dynamic> m = var.value<Eigen::Matrix<Cf,4,Eigen::Dynamic> >();
  Eigen::Matrix<Cf,4,3> ok;
  ok.col(0) = (Eigen::Matrix<Cf,4,1>() << Cf(0), Cf(1), Cf(1), Cf(0)).finished();
  ok.col(1) = (Eigen::Matrix<Cf,4,1>() << Cf(0), Cf(0,1), Cf(0,-1), Cf(0)).finished();
  ok.col(2) = (Eigen::Matrix<Cf,4,1>() << Cf(1), Cf(0), Cf(0), Cf(-1)).finished();
  MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
}
BOOST_AUTO_TEST_CASE(mf_2x3x2)
{
  Tomographer::MAT::Var var = f.var("mf_2x3x2");
  Eigen::MatrixXf m = var.value<Eigen::MatrixXf>();
  Eigen::Matrix<float,6,2> ok;
  ok.col(0) = (Eigen::Matrix<float,6,1>() << 1, 1.0, 4, 1.5, -2.5, -1e4).finished();
  ok.col(1) = (Eigen::Matrix<float,6,1>() << 0, 1, 0, -2, 0, -3).finished();
  MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
}
BOOST_AUTO_TEST_CASE(mcd_2x2x3x2)
{
  Tomographer::MAT::Var var = f.var("mcd_2x3x2x2");
  Eigen::MatrixXcd m = var.value<Eigen::MatrixXcd>();
  typedef std::complex<double> Cd;
  Eigen::Matrix<Cd,12,2> ok;
  ok.col(0) = (Eigen::Matrix<Cd,12,1>()
               << 1, 1, Cd(0,1), Cd(0,1.5), Cd(0,-1), Cd(-1e4,1e3),
               0, Cd(0,1), 0, Cd(0,-2), 0, Cd(0,-3)).finished();
  ok.col(1) = (Eigen::Matrix<Cd,12,1>()
               << 1, 0, 0, 1, 0, 0, 0, Cd(0,1), 0, Cd(0,-2), 0, Cd(0,-3)).finished();
  MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
}
BOOST_AUTO_TEST_SUITE_END(); // eigen



// =============================================================================
BOOST_AUTO_TEST_SUITE_END(); // test_tools_ezmatio_2

