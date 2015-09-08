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

#include <vector>
#include <algorithm>

#include "test_tomographer.h"

#include <tomographer/tools/ezmatio.h>


BOOST_AUTO_TEST_SUITE(test_tools_ezmatio);
// =============================================================================

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


BOOST_AUTO_TEST_CASE(eigen)
{
}


BOOST_AUTO_TEST_SUITE_END();


// =============================================================================
BOOST_AUTO_TEST_SUITE_END();
