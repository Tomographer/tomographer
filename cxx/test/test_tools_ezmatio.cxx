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


BOOST_AUTO_TEST_SUITE(test_tools_ezmatio);
// =============================================================================

BOOST_AUTO_TEST_SUITE(DimList);
BOOST_AUTO_TEST_CASE(basic)
{
  typedef Tomographer::MAT::DimList DimList;
  DimList dims{3, 4, 5};
  const std::vector<int> ok{3, 4, 5};
  BOOST_CHECK_EQUAL(dims.size(), 3);
  BOOST_CHECK_EQUAL(dims.ndims(), 3);
  BOOST_CHECK_EQUAL(dims.numel(), 3*4*5);
  BOOST_CHECK(dims == ok);
  BOOST_CHECK(dims.matchesWanted(DimList{3, 4, 5}));
  BOOST_CHECK(!dims.matchesWanted(DimList{2, 4, 5}));
  BOOST_CHECK(!dims.matchesWanted(DimList{3, 4}));
  BOOST_CHECK(dims.matchesWanted(DimList{-1, -1, -1}));
  BOOST_CHECK(dims.matchesWanted(DimList{-1, 4, -1}));
  BOOST_CHECK(dims.matchesWanted(DimList{3, 4, -1}));
  BOOST_CHECK(!dims.matchesWanted(DimList{3, -1, 3}));
}
BOOST_AUTO_TEST_SUITE_END();

BOOST_AUTO_TEST_SUITE(IndexList)

// IndexList<> defaults to column-major
TOMO_STATIC_ASSERT_EXPR(Tomographer::MAT::IndexList<>::IsRowMajor == false);

BOOST_AUTO_TEST_CASE(constr1)
{
  Tomographer::MAT::DimList dims{3, 4, 5};
  BOOST_MESSAGE("dims = " << dims) ;
  Tomographer::MAT::IndexList<false> il{dims};
  BOOST_CHECK(il.dims() == dims);

  il.setLinearIndex(23);
  BOOST_MESSAGE("il == " << il) ;
  const std::vector<int> ok{2, 3, 1};
  BOOST_CHECK(il.index() == ok);

  BOOST_CHECK_EQUAL(il.linearIndex(), 23);

  // can use as std::vector<int> (base class):
  BOOST_CHECK((const std::vector<int>&)(il) == ok);
}
BOOST_AUTO_TEST_CASE(constr2)
{
  Tomographer::MAT::DimList dims{3, 4, 5};
  Tomographer::MAT::IndexList<> il{dims, 23};
  BOOST_CHECK(il.dims() == dims);
  BOOST_CHECK_EQUAL(il.linearIndex(), 23);
  const std::vector<int> ok{2, 3, 1};
  BOOST_CHECK(il.index() == ok);
}
BOOST_AUTO_TEST_CASE(constr1_rowmaj)
{
  Tomographer::MAT::DimList dims{3, 4, 5};
  Tomographer::MAT::IndexList<true> il{dims};
  BOOST_CHECK(il.dims() == dims);
  il.setLinearIndex(23);
  const std::vector<int> ok{1, 0, 3};
  BOOST_CHECK(il.index() == ok);
  // can use as std::vector<int>:
  BOOST_CHECK((const std::vector<int>&)(il) == ok);
}
BOOST_AUTO_TEST_CASE(rvalref_index)
{
  Tomographer::MAT::DimList dims{3, 4, 5};
  const std::vector<int> ok{1, 0, 3};
  {
    Tomographer::MAT::IndexList<true> il{dims, 23};
    Tomographer::MAT::IndexList<true> && ilref = std::move(il).index(); // && rvalue-ref
    BOOST_CHECK(std::vector<int>(ilref) == ok);
  }
  {
    Tomographer::MAT::IndexList<true> il{dims, 23};
    const Tomographer::MAT::IndexList<true> & ilref = il.index(); // const&
    BOOST_CHECK(std::vector<int>(ilref) == ok);
  }
}

BOOST_AUTO_TEST_SUITE_END();


BOOST_AUTO_TEST_SUITE(IndexListIterator);

BOOST_AUTO_TEST_CASE(IndexListIterator1)
{
  Tomographer::MAT::DimList dims;
  dims << 3 << 1 << 2;
  
  Tomographer::MAT::IndexListIterator<false> it(dims);
  BOOST_CHECK_EQUAL(it.linearIndex(), 0);
  MY_BOOST_CHECK_STD_VECTOR_EQUAL(it.index(), Tomographer::MAT::IndexList<>() << 0 << 0 << 0, tol);
  BOOST_CHECK(it.valid());

  std::vector<int> oldstate = it.index();

  for (std::size_t j = 0; j < (std::size_t)it.numel(); ++j) {
    BOOST_CHECK(it.valid());
    BOOST_CHECK_EQUAL(it.linearIndex(), j);

    BOOST_MESSAGE("j = " << j << ", index=" << it) ;

    std::vector<int> indlist(dims.size());
    std::size_t jj = j;
    for (int k = 0; k < (int)dims.size(); ++k) {
      indlist[k] = jj % dims[k];
      jj /= dims[k];
    }
    MY_BOOST_CHECK_STD_VECTOR_EQUAL(it.index(), indlist, tol);

    ++it;
  }
}

BOOST_AUTO_TEST_CASE(IndexListIterator2)
{
  Tomographer::MAT::DimList dims;
  dims << 3 << 1 << 2;
  
  Tomographer::MAT::IndexListIterator<true> it(dims);
  BOOST_CHECK_EQUAL(it.linearIndex(), 0);
  MY_BOOST_CHECK_STD_VECTOR_EQUAL(it.index(), Tomographer::MAT::IndexList<>() << 0 << 0 << 0, tol);
  BOOST_CHECK(it.valid());

  std::vector<int> oldstate = it.index();

  for (std::size_t j = 0; j < (std::size_t)it.numel(); ++j) {
    BOOST_CHECK(it.valid());
    BOOST_CHECK_EQUAL(it.linearIndex(), j);

    std::vector<int> indlist(dims.size());
    std::size_t jj = j;
    for (int k = dims.size()-1; k >= 0; --k) {
      indlist[k] = jj % dims[k];
      jj /= dims[k];
    }
    MY_BOOST_CHECK_STD_VECTOR_EQUAL(it.index(), indlist, tol);

    ++it;
  }
}

BOOST_AUTO_TEST_SUITE_END();



// -----------------------------------------------------------------------------

struct load_mat_test_file
{
  Tomographer::MAT::File f;

  load_mat_test_file()
    : f(TOMOGRAPHER_TEST_DATA_DIR "test_tools_ezmatio/test_tools_ezmatio_data.mat")
  {
  }

  ~load_mat_test_file()
  {
  }
};

const char * good_varlist[] = {
  "d",
  "f",
  "i8",
  "i16",
  "i32",
  "i64",
  "u8",
  "u16",
  "u32",
  "u64",
  "md_4x3",
  "rvd_5",
  "vd_5",
  "mcd_4x3",
  "rvcd_5",
  "vcd_5",
  "mf_4x3",
  "rvf_5",
  "vf_5",
  "mi8_3x3",
  "mi32_3x3",
  "mu32_3x3",
  "rvc",
  "c_3x2",
  "s",
  NULL
};
std::size_t good_varlist_len() {
  std::size_t j;
  for (j = 0; good_varlist[j] != NULL; ++j)
    ;
  return j;
} 

BOOST_FIXTURE_TEST_SUITE(matfile, load_mat_test_file);

BOOST_AUTO_TEST_CASE(basics)
{
  BOOST_CHECK(f.getMatPtr() != NULL);

  mat_t * fptr = f.getMatPtr();
  // move constructor
  Tomographer::MAT::File f2(std::move(f));
  BOOST_CHECK(fptr == f2.getMatPtr()); // check: stole MAT file resource
  BOOST_CHECK(f.getMatPtr() == NULL); // and didn't leave a dangling pointer

}



BOOST_AUTO_TEST_CASE(varlist)
{
  std::vector<Tomographer::MAT::Var> varinfo = f.getVarInfoList();

  std::size_t good_varlist_len_ = good_varlist_len();
  
  BOOST_MESSAGE("good_varlist_len = " << good_varlist_len_);
  BOOST_CHECK_EQUAL(varinfo.size(), good_varlist_len_);
  std::vector<std::string> varnames;
  varnames.reserve(varinfo.size());
  for (std::size_t j = 0; j < varinfo.size(); ++j) {
    varnames.push_back(varinfo[j].varName());
  }

  std::sort(varnames.begin(), varnames.end());

  std::vector<std::string> goodvarnames(good_varlist, good_varlist + good_varlist_len_);
  std::sort(goodvarnames.begin(), goodvarnames.end());

  BOOST_CHECK_EQUAL(varnames.size(), goodvarnames.size());
  for (std::size_t i = 0; i < varnames.size(); ++i) {
    BOOST_CHECK_EQUAL(varnames[i], goodvarnames[i]);
  }
}


BOOST_AUTO_TEST_CASE(var)
{
  {
    Tomographer::MAT::Var var(f, "i8");
    BOOST_CHECK_EQUAL(var.value<int>(), 42);
  }
  {
    Tomographer::MAT::Var var = f.var("i8");
    BOOST_CHECK_EQUAL(var.value<long>(), 42l);
  }
  {
    Tomographer::MAT::Var var = f.var("i8");
    BOOST_CHECK_EQUAL(Tomographer::MAT::value<unsigned int>(var), 42u);
  }
}
BOOST_AUTO_TEST_CASE(scalars_conv)
{
  Tomographer::MAT::Var var = f.var("d");
  {
    double a = var.value<double>();
    BOOST_CHECK_CLOSE(a, 3.14, tol_percent);
  }
  {
    float a = var.value<float>();
    BOOST_CHECK_CLOSE(a, 3.14f, tol_percent_f);
  }
  {
    int a = var.value<int>();
    BOOST_CHECK_EQUAL(a, 3);
  }
  {
    uint8_t a = var.value<uint8_t>();
    BOOST_CHECK_EQUAL(a, 3u);
  }
  {
    unsigned long a = var.value<unsigned long>();
    BOOST_CHECK_EQUAL(a, 3lu);
  }
  {
    std::complex<double> c = var.value<std::complex<double> >();
    BOOST_CHECK_SMALL(std::abs(c - std::complex<double>(3.14, 0)), tol);
  }
  {
    std::complex<float> c = var.value<std::complex<float> >();
    BOOST_CHECK_SMALL(std::abs(c - std::complex<float>(3.14f, 0)), tol_f);
  }
}

BOOST_AUTO_TEST_CASE(scalars)
{
  {
    Tomographer::MAT::Var var = f.var("d");
    BOOST_CHECK_CLOSE(var.value<double>(), 3.14, tol_percent);
  }
  {
    Tomographer::MAT::Var var = f.var("f");
    BOOST_CHECK_CLOSE(var.value<float>(), 2.718f, tol_percent_f);
  }
  {
    Tomographer::MAT::Var var = f.var("i8");
    BOOST_CHECK_EQUAL(var.value<int8_t>(), 42);
  }
  {
    Tomographer::MAT::Var var = f.var("i16");
    BOOST_CHECK_EQUAL(var.value<int16_t>(), -32768L);
  }
  {
    Tomographer::MAT::Var var = f.var("i32");
    BOOST_CHECK_EQUAL(var.value<int32_t>(), 2147483647L);
  }
  {
    Tomographer::MAT::Var var = f.var("i64");
    BOOST_CHECK_EQUAL(var.value<int64_t>(), std::numeric_limits<int64_t>::min());//-9223372036854775808LL);
  }
  {
    Tomographer::MAT::Var var = f.var("u8");
    BOOST_CHECK_EQUAL(var.value<uint8_t>(), 42u);
  }
  {
    Tomographer::MAT::Var var = f.var("u16");
    BOOST_CHECK_EQUAL(var.value<uint16_t>(), 65535u);
  }
  {
    Tomographer::MAT::Var var = f.var("u32");
    BOOST_CHECK_EQUAL(var.value<uint32_t>(), 4294967295LU);
  }
  {
    Tomographer::MAT::Var var = f.var("u64");
    BOOST_CHECK_EQUAL(var.value<uint64_t>(), 18446744073709551615LLU);
  }
}



BOOST_AUTO_TEST_CASE(getstdvector_conv)
{
  {
    Tomographer::MAT::Var var = f.var("md_4x3");
    std::vector<double> v = var.value<Tomographer::MAT::GetStdVector<double, true> >();
    static const double ok[] = {
      1.0, 2.0, 3.0, 1.5, 3, 4.5, 100.0, 200.0, 300.0, 0.0, 0.0, 1.0
    };
    MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
  }
  {
    Tomographer::MAT::Var var = f.var("md_4x3");
    std::vector<double> v = var.value<Tomographer::MAT::GetStdVector<double, false> >();
    static const double ok[] = {
      1.0, 1.5, 100, 0, 2, 3, 200, 0, 3, 4.5, 300, 1
    };
    MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
  }
  {
    Tomographer::MAT::Var var = f.var("md_4x3");
    std::vector<float> v = var.value<Tomographer::MAT::GetStdVector<float, false> >();
    static const float ok[] = {
      1.0, 1.5, 100, 0, 2, 3, 200, 0, 3, 4.5, 300, 1
    };
    MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
  }
  {
    Tomographer::MAT::Var var = f.var("md_4x3");
    std::vector<std::complex<float> > v = var.value<Tomographer::MAT::GetStdVector<std::complex<float>, false> >();
    static const float ok[] = {
      1.0, 1.5, 100, 0, 2, 3, 200, 0, 3, 4.5, 300, 1
    };
    MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
  }
  {
    Tomographer::MAT::Var var = f.var("md_4x3");
    std::vector<std::complex<double> > v = var.value<Tomographer::MAT::GetStdVector<std::complex<double>, true> >();
    static const double ok[] = {
      1.0, 2.0, 3.0, 1.5, 3, 4.5, 100.0, 200.0, 300.0, 0.0, 0.0, 1.0
    };
    MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
  }
}


BOOST_AUTO_TEST_CASE(getstdvector)
{
  {
    Tomographer::MAT::Var var = f.var("md_4x3");
    std::vector<double> v = var.value<Tomographer::MAT::GetStdVector<double, true> >();
    static const double ok[] = {
      1.0, 2.0, 3.0, 1.5, 3, 4.5, 100.0, 200.0, 300.0, 0.0, 0.0, 1.0
    };
    MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
  }
  {
    Tomographer::MAT::Var var = f.var("rvd_5");
    std::vector<double> v = var.value<Tomographer::MAT::GetStdVector<double, true> >();
    static const double ok[] = {
      1.0, 2.0, -3.0, 4.0, -193.223
    };
    MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
  }
  {
    Tomographer::MAT::Var var = f.var("vd_5");
    std::vector<double> v = var.value<Tomographer::MAT::GetStdVector<double, true> >();
    static const double ok[] = {
      1.0, 2.0, -3.0, 4.0, -193.223
    };
    MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
  }
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
  {
    Tomographer::MAT::Var var = f.var("rvf_5");
    std::vector<float> v = var.value<Tomographer::MAT::GetStdVector<float, true> >();
    const float ok[] = {
      1.0f, 2.0f, -3.0f, 4.0f, -193.223f
    };
    MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
  }
  {
    Tomographer::MAT::Var var = f.var("vf_5");
    std::vector<float> v = var.value<Tomographer::MAT::GetStdVector<float, true> >();
    const float ok[] = {
      1.0f, 2.0f, -3.0f, 4.0f, -193.223f
    };
    MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
  }

  {
    Tomographer::MAT::Var var = f.var("mi8_3x3");
    std::vector<int8_t> v = var.value<Tomographer::MAT::GetStdVector<int8_t, true> >();
    const std::vector<int8_t> ok{
      1, 1, 1, 2, 2, 2, 127, 0, -128
    };
    MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
  }
  {
    Tomographer::MAT::Var var = f.var("mi32_3x3");
    std::vector<int32_t> v = var.value<Tomographer::MAT::GetStdVector<int32_t, true> >();
    const std::vector<int32_t> ok{
      1, 1, 1, 2, 2, 2, 2147483647l, 0, -2147483648l
    };
    MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
  }
  {
    Tomographer::MAT::Var var = f.var("mu32_3x3");
    std::vector<uint32_t> v = var.value<Tomographer::MAT::GetStdVector<uint32_t, true> >();
    const std::vector<uint32_t> ok{
      1, 1, 1, 2, 2, 2, 4294967295lu, 0, 0
    };
    MY_BOOST_CHECK_STD_VECTOR_EQUAL(v, ok, tol);
  }
}



BOOST_AUTO_TEST_CASE(eigen_conv)
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

BOOST_AUTO_TEST_CASE(eigen)
{
  {
    Tomographer::MAT::Var var = f.var("md_4x3");
    Eigen::MatrixXd m = var.value<Eigen::MatrixXd>();
    Eigen::Matrix<double,4,3> ok;
    ok <<  1.0, 2.0, 3.0, 1.5, 3.0, 4.5, 100.0, 200.0, 300.0, 0.0, 0.0, 1.0 ;
    MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
  }
  {
    Tomographer::MAT::Var var = f.var("rvd_5");
    Eigen::RowVectorXd m = var.value<Eigen::RowVectorXd>();
    Eigen::RowVectorXd ok(5);
    ok <<  1.0, 2.0, -3.0, 4.0, -193.223 ;
    MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
  }
  {
    Tomographer::MAT::Var var = f.var("vd_5");
    Eigen::VectorXd m = var.value<Eigen::VectorXd>();
    Eigen::VectorXd ok(5);
    ok <<  1.0, 2.0, -3.0, 4.0, -193.223 ;
    MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
  }
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
  {
    Tomographer::MAT::Var var = f.var("rvf_5");
    Eigen::Matrix<float,1,5> m = var.value<Eigen::Matrix<float,1,5> >();
    Eigen::Matrix<float,1,5> ok;
    ok <<
      1.0f, 2.0f, -3.0f, 4.0f, -193.223f
      ;
    MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
  }
  {
    Tomographer::MAT::Var var = f.var("vf_5");
    Eigen::Matrix<float,5,1> m = var.value<Eigen::Matrix<float,5,1> >();
    Eigen::Matrix<float,5,1> ok;
    ok <<
      1.0f, 2.0f, -3.0f, 4.0f, -193.223f
      ;
    MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
  }

  {
    Tomographer::MAT::Var var = f.var("mi8_3x3");
    Eigen::Matrix<int8_t,Eigen::Dynamic,3> m = var.value<Eigen::Matrix<int8_t,Eigen::Dynamic,3> >();
    Eigen::Matrix<int8_t,3,3> ok;
    ok <<
      1, 1, 1, 2, 2, 2, 127, 0, -128
      ;
    MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
  }
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
  {
    Tomographer::MAT::Var var = f.var("mu32_3x3");
    Eigen::Matrix<uint32_t,3,3> m = var.value<Eigen::Matrix<uint32_t,3,3> >();
    Eigen::Matrix<uint32_t,3,3> ok;
    ok <<
      1, 1, 1, 2, 2, 2, 4294967295lu, 0, 0
      ;
    MY_BOOST_CHECK_EIGEN_EQUAL(m, ok, tol);
  }
}




BOOST_AUTO_TEST_CASE(stdvec_of_eigen)
{
  
}

BOOST_AUTO_TEST_SUITE_END();


// =============================================================================
BOOST_AUTO_TEST_SUITE_END();
