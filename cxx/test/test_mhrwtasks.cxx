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
#include <iostream>
#include <random>
#include <algorithm>

#include <boost/math/constants/constants.hpp>

// definitions for Tomographer test framework -- this must be included before any
// <Eigen/...> or <tomographer2/...> header
#include "test_tomographer.h"

#include <tomographer2/mhrwtasks.h>
#include <tomographer2/tools/loggers.h>
#include <tomographer2/multiproc.h>
#include <tomographer2/densedm/dmtypes.h>
#include <tomographer2/densedm/indepmeasllh.h>
#include <tomographer2/densedm/tspacefigofmerit.h>
#include <tomographer2/densedm/tspacellhwalker.h>


// -----------------------------------------------------------------------------
// fixture(s)


// -----------------------------------------------------------------------------
// test suites

BOOST_AUTO_TEST_SUITE(test_mhrwtasks)
// =============================================================================

BOOST_AUTO_TEST_SUITE(cdatabase)

BOOST_AUTO_TEST_CASE(constr)
{
  Tomographer::MHRWTasks::CDataBase<> cdata(Tomographer::MHRWParams<unsigned int, double>(0.1, 128, 50, 500), 100);
  BOOST_CHECK_EQUAL(cdata.base_seed, 100);
  MY_BOOST_CHECK_FLOATS_EQUAL(cdata.mhrw_params.step_size, 0.1, tol);
  BOOST_CHECK_EQUAL(cdata.mhrw_params.n_sweep, 128);
  BOOST_CHECK_EQUAL(cdata.mhrw_params.n_therm, 50);
  BOOST_CHECK_EQUAL(cdata.mhrw_params.n_run, 500);
}

BOOST_AUTO_TEST_CASE(get_task_inputs)
{
  const Tomographer::MHRWTasks::CDataBase<> cdata(Tomographer::MHRWParams<unsigned int, double>(0.1, 128, 50, 500), 100);

  BOOST_MESSAGE( cdata.getBasicCDataMHRWInfo() );

  std::vector<int> inputs;
  inputs.reserve(1024);
  for (int k = 0; k < 1024; ++k) {
    inputs.push_back(cdata.getTaskInput(k));
  }

  // make sure that all the inputs are different
  std::sort(inputs.begin(), inputs.end());
  auto last = std::unique(inputs.begin(), inputs.end());
  BOOST_CHECK(last == inputs.end());
  BOOST_CHECK_EQUAL(inputs.size(), 1024);
}

BOOST_AUTO_TEST_SUITE_END(); // cdatabase

//-----------------------------------------------

BOOST_AUTO_TEST_SUITE(tMHRandomWalkTaskResult)

BOOST_AUTO_TEST_CASE(write_me)
{
  BOOST_CHECK( false ) ;
}


BOOST_AUTO_TEST_SUITE_END(); // tMHRandomWalkTaskResult



// =============================================================================
BOOST_AUTO_TEST_SUITE_END() ;

