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

BOOST_AUTO_TEST_SUITE(utils);


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
}
/*
% double, 4x3 matrix
d.md_4x3 = double([1.0, 2.0, 3.0; 1.5, 3, 4.5; 100.0, 200.0, 300.0; 0.0 0.0 1.0]);
% double row vector
d.rvd_5 = double([1.0, 2.0, -3.0, 4.0, -193.223]);
% double column vector
d.vd_5 = double([1.0; 2.0; -3.0; 4.0; -193.223]);

% double, 4x3 matrix
d.mcd_4x3 = [1 0 0; 0 1 0; 0 0 1; 0 0 0] + 1i*double([1.0, 2.0, 3.0; 1.5, 3, 4.5; 100.0, 200.0, 300.0; 0.0 0.0 1.0]);
% double row vector
d.rvcd_5 = double([1.0+1i, 2.0+2.5i, -3.0, 4.0, -193.223]);
% double column vector
d.vcd_5 = double([1.0+1i; 2.0; -3.0; 4.0; -193.223]);

% single, 4x3 matrix
d.mf_4x3 = single([1.0, 2.0, 3.0; 1.5, 3, 4.5; 100.0, 200.0, 300.0]);
% single row vector
d.rvf_5 = single([1.0, 2.0, -3.0, 4.0, -193.223]);
% single column vector
d.vf_5 = single([1.0; 2.0; -3.0; 4.0; -193.223]);

% int8, 3x3 matrix
d.mi8_3x3 = int8([1 1 1; 2 2 2; 127 0 -128]);
% int32, 3x3 matrix
d.mi32_3x3 = int32([1 1 1; 2 2 2; 2147483647 0 -2147483648]);
% uint32, 3x3 matrix
d.mu32_3x3 = uint32([1 1 1; 2 2 2; 4294967295 0 0]);
*/


BOOST_AUTO_TEST_SUITE_END();


// =============================================================================
BOOST_AUTO_TEST_SUITE_END();
