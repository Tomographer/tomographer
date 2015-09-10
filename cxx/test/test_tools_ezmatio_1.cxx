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

BOOST_FIXTURE_TEST_SUITE(test_tools_ezmatio_1, load_mat_test_file);
// =============================================================================

BOOST_AUTO_TEST_SUITE(DimList);
BOOST_AUTO_TEST_CASE(basic)
{
  typedef Tomographer::MAT::DimList DimList;
  DimList dims{{3, 4, 5}};
  const std::vector<int> ok{{3, 4, 5}};
  BOOST_CHECK_EQUAL(dims.size(), 3);
  BOOST_CHECK_EQUAL(dims.ndims(), 3);
  BOOST_CHECK_EQUAL(dims.numel(), 3*4*5);
  BOOST_CHECK(dims == ok);
  BOOST_CHECK(dims.matchesWanted(DimList{{3, 4, 5}}));
  BOOST_CHECK(!dims.matchesWanted(DimList{{2, 4, 5}}));
  BOOST_CHECK(!dims.matchesWanted(DimList{{3, 4}}));
  BOOST_CHECK(dims.matchesWanted(DimList{{-1, -1, -1}}));
  BOOST_CHECK(dims.matchesWanted(DimList{{-1, 4, -1}}));
  BOOST_CHECK(dims.matchesWanted(DimList{{3, 4, -1}}));
  BOOST_CHECK(!dims.matchesWanted(DimList{{3, -1, 3}}));
}
BOOST_AUTO_TEST_SUITE_END(); // DimList

BOOST_AUTO_TEST_SUITE(IndexList)

// IndexList<> defaults to column-major
TOMO_STATIC_ASSERT_EXPR(Tomographer::MAT::IndexList<>::IsRowMajor == false);

BOOST_AUTO_TEST_CASE(constr1)
{
  Tomographer::MAT::DimList dims{{3, 4, 5}};
  BOOST_MESSAGE("dims = " << dims) ;
  Tomographer::MAT::IndexList<false> il{dims};
  BOOST_CHECK(il.dims() == dims);

  il.setLinearIndex(23);
  BOOST_MESSAGE("il == " << il) ;
  const std::vector<int> ok{{2, 3, 1}};
  BOOST_CHECK(il.index() == ok);

  BOOST_CHECK_EQUAL(il.linearIndex(), 23);

  // can use as std::vector<int> (base class):
  BOOST_CHECK((const std::vector<int>&)(il) == ok);
}
BOOST_AUTO_TEST_CASE(constr1b)
{
  Tomographer::MAT::DimList dims{{3, 4, 5}};
  // constructor with indices
  Tomographer::MAT::IndexList<false> il{dims, std::vector<int>{{0, 2, 4}}};
  const std::vector<int> ok{{0, 2, 4}};
  BOOST_CHECK(il.index() == ok);
}
BOOST_AUTO_TEST_CASE(constr2)
{
  Tomographer::MAT::DimList dims{{3, 4, 5}};
  Tomographer::MAT::IndexList<> il{dims, 23};
  BOOST_CHECK(il.dims() == dims);
  BOOST_CHECK_EQUAL(il.linearIndex(), 23);
  const std::vector<int> ok{{2, 3, 1}};
  BOOST_CHECK(il.index() == ok);
}
BOOST_AUTO_TEST_CASE(constr1_rowmaj)
{
  Tomographer::MAT::DimList dims{{3, 4, 5}};
  Tomographer::MAT::IndexList<true> il{dims};
  BOOST_CHECK(il.dims() == dims);
  il.setLinearIndex(23);
  const std::vector<int> ok{{1, 0, 3}};
  BOOST_CHECK(il.index() == ok);
  // can use as std::vector<int>:
  BOOST_CHECK((const std::vector<int>&)(il) == ok);
}
BOOST_AUTO_TEST_CASE(rvalref_index)
{
  Tomographer::MAT::DimList dims{{3, 4, 5}};
  const std::vector<int> ok{{1, 0, 3}};
  /* had to remove rvalue-ref qualified methods for g++ 4.6 ... :(
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
  */
  {
    Tomographer::MAT::IndexList<true> il{dims, 23};
    std::vector<int> index{std::move(il.index())};
    BOOST_CHECK(index == ok);
  }
}

BOOST_AUTO_TEST_SUITE_END(); // IndexList


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

BOOST_AUTO_TEST_SUITE_END(); // IndexListIterator




BOOST_AUTO_TEST_SUITE(matfile);
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
BOOST_AUTO_TEST_SUITE_END(); // matfile


BOOST_AUTO_TEST_SUITE(var);
BOOST_AUTO_TEST_CASE(i8)
{
  Tomographer::MAT::Var var(f, "i8");
  BOOST_CHECK_EQUAL(var.value<int>(), 42);
}
BOOST_AUTO_TEST_CASE(i8_2)
{
  Tomographer::MAT::Var var = f.var("i8");
  BOOST_CHECK_EQUAL(var.value<long>(), 42l);
}
BOOST_AUTO_TEST_CASE(i8_3)
{
  Tomographer::MAT::Var var = f.var("i8");
  BOOST_CHECK_EQUAL(Tomographer::MAT::value<unsigned int>(var), 42u);
}
BOOST_AUTO_TEST_SUITE_END(); // var

BOOST_AUTO_TEST_SUITE(scalars);
BOOST_AUTO_TEST_CASE(conv)
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
BOOST_AUTO_TEST_CASE(d)
{
  Tomographer::MAT::Var var = f.var("d");
  BOOST_CHECK_CLOSE(var.value<double>(), 3.14, tol_percent);
}
BOOST_AUTO_TEST_CASE(f_)
{
  Tomographer::MAT::Var var = f.var("f");
  BOOST_CHECK_CLOSE(var.value<float>(), 2.718f, tol_percent_f);
}
BOOST_AUTO_TEST_CASE(i8)
{
  Tomographer::MAT::Var var = f.var("i8");
  BOOST_CHECK_EQUAL(var.value<int8_t>(), 42);
}
BOOST_AUTO_TEST_CASE(i16)
{
  Tomographer::MAT::Var var = f.var("i16");
  BOOST_CHECK_EQUAL(var.value<int16_t>(), -32768L);
}
BOOST_AUTO_TEST_CASE(i32)
{
  Tomographer::MAT::Var var = f.var("i32");
  BOOST_CHECK_EQUAL(var.value<int32_t>(), 2147483647L);
}
BOOST_AUTO_TEST_CASE(i64)
{
  Tomographer::MAT::Var var = f.var("i64");
  BOOST_CHECK_EQUAL(var.value<int64_t>(), std::numeric_limits<int64_t>::min());//-9223372036854775808LL);
}
BOOST_AUTO_TEST_CASE(u8)
{
  Tomographer::MAT::Var var = f.var("u8");
  BOOST_CHECK_EQUAL(var.value<uint8_t>(), 42u);
}
BOOST_AUTO_TEST_CASE(u16)
{
  Tomographer::MAT::Var var = f.var("u16");
  BOOST_CHECK_EQUAL(var.value<uint16_t>(), 65535u);
}
BOOST_AUTO_TEST_CASE(u32)
{
  Tomographer::MAT::Var var = f.var("u32");
  BOOST_CHECK_EQUAL(var.value<uint32_t>(), 4294967295LU);
}
BOOST_AUTO_TEST_CASE(u64)
{
  Tomographer::MAT::Var var = f.var("u64");
  BOOST_CHECK_EQUAL(var.value<uint64_t>(), 18446744073709551615LLU);
}

BOOST_AUTO_TEST_SUITE_END(); // scalars


// =============================================================================
BOOST_AUTO_TEST_SUITE_END(); // test_tools_ezmatio_1
