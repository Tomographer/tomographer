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


// include after the relevant include files.

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

static const char * good_varlist[] = {
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
  "mcd_2x2x3",
  "mcf_2x2x3",
  "mf_2x3x2",
  "mcd_2x3x2x2",
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

inline std::size_t good_varlist_len()
{
  std::size_t j;
  for (j = 0; good_varlist[j] != NULL; ++j)
    ;
  return j;
} 
