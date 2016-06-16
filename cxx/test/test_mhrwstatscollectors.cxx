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

#include <boost/math/constants/constants.hpp>

// definitions for Tomographer test framework -- this must be included before any
// <Eigen/...> or <tomographer2/...> header
#include "test_tomographer.h"

#include <tomographer2/mhrwstatscollectors.h>



// -----------------------------------------------------------------------------
// fixture(s)

//
// stats collector which just remembers callback function calls and stores the given
// arguments
//
struct CheckMHRWStatsCollector
{
  const int id;

  bool init_called;
  bool thermalizing_done_called;
  bool done_called;

  struct ProcessSampleCallData {
    ProcessSampleCallData(int k_, int n_, double fnval_) : k(k_), n(n_), fnval(fnval_) { }
    int k;
    int n;
    double fnval;
  };
  bool process_sample_called;
  ProcessSampleCallData process_sample_call_data;

  struct RawMoveCallData
  {
    RawMoveCallData(int k_, bool is_thermalizing_, bool is_live_iter_, bool accepted_,
                    double a_, double newptval_, double curptval_)
      : k(k_), is_thermalizing(is_thermalizing_), is_live_iter(is_live_iter_), accepted(accepted_),
        a(a_), newptval(newptval_), curptval(curptval_)
    {
    }
    int k;
    bool is_thermalizing;
    bool is_live_iter;
    bool accepted;
    double a;
    double newptval;
    double curptval;
  };
  bool raw_move_called;
  RawMoveCallData raw_move_call_data;

  CheckMHRWStatsCollector(const int id_)
    : id(id_),
      init_called(false), thermalizing_done_called(false), done_called(false),
      process_sample_called(false), process_sample_call_data(-1,-1,0),
      raw_move_called(false), raw_move_call_data(-1,false,false,false,std::numeric_limits<double>::quiet_NaN(),
                                                 std::numeric_limits<double>::quiet_NaN(),
                                                 std::numeric_limits<double>::quiet_NaN())
  {
  }
  void init()
  {
    init_called = true;
  }
  void thermalizing_done()
  {
    thermalizing_done_called = true;
  }
  void done()
  {
    done_called = true;
  }
  template<typename CountIntType, typename PointType, typename FnValueType, typename MHRandomWalk>
  void process_sample(CountIntType k, CountIntType n, const PointType & , FnValueType fnval, MHRandomWalk & )
  {
    process_sample_called = true;
    process_sample_call_data.k = k;
    process_sample_call_data.n = n;
    process_sample_call_data.fnval = fnval;
  }
  template<typename CountIntType, typename PointType, typename FnValueType, typename MHRandomWalk>
  void raw_move(CountIntType k, bool is_thermalizing, bool is_live_iter, bool accepted, double a,
                const PointType & /*newpt*/, FnValueType newptval, const PointType & /*curpt*/,
                FnValueType curptval, MHRandomWalk & /*rw*/)
  {
    raw_move_called = true;
    raw_move_call_data.k = k;
    raw_move_call_data.is_thermalizing = is_thermalizing;
    raw_move_call_data.is_live_iter = is_live_iter;
    raw_move_call_data.accepted = accepted;
    raw_move_call_data.a = a;
    raw_move_call_data.newptval = newptval;
    raw_move_call_data.curptval = curptval;
  }
};


struct test_multmhrwstatscollectors_fixture
{
  CheckMHRWStatsCollector a, b, c, d, e;

  Tomographer::MultipleMHRWStatsCollectors<CheckMHRWStatsCollector,CheckMHRWStatsCollector,
                                           CheckMHRWStatsCollector,CheckMHRWStatsCollector,
                                           CheckMHRWStatsCollector> mult;

  test_multmhrwstatscollectors_fixture()
    : a(0), b(1), c(2), d(3), e(4),
      mult(a, b, c, d, e)
  {
  }
  
};


// -----------------------------------------------------------------------------
// test suites


BOOST_AUTO_TEST_SUITE(test_mhrwstatscollectors)

// =============================================================================

BOOST_FIXTURE_TEST_SUITE(multiplemhrwstatscollectors, test_multmhrwstatscollectors_fixture)

BOOST_AUTO_TEST_CASE(init)
{
  mult.init();
  BOOST_CHECK(a.init_called);
  BOOST_CHECK(b.init_called);
  BOOST_CHECK(c.init_called);
  BOOST_CHECK(d.init_called);
  BOOST_CHECK(e.init_called);
}
BOOST_AUTO_TEST_CASE(thermalizing_done)
{
  mult.thermalizing_done();
  BOOST_CHECK(a.thermalizing_done_called);
  BOOST_CHECK(b.thermalizing_done_called);
  BOOST_CHECK(c.thermalizing_done_called);
  BOOST_CHECK(d.thermalizing_done_called);
  BOOST_CHECK(e.thermalizing_done_called);
}
BOOST_AUTO_TEST_CASE(done)
{
  mult.done();
  BOOST_CHECK(a.done_called);
  BOOST_CHECK(b.done_called);
  BOOST_CHECK(c.done_called);
  BOOST_CHECK(d.done_called);
  BOOST_CHECK(e.done_called);
}

BOOST_AUTO_TEST_CASE(process_sample)
{
  struct PointType { std::string dummy; PointType() : dummy("hello") { } };
  struct DummyMHRW { int x; }; DummyMHRW mhrw;
  mult.process_sample(3243, 1245, PointType(), 1.45, mhrw);
  BOOST_CHECK(a.process_sample_called);
  BOOST_CHECK(b.process_sample_called);
  BOOST_CHECK(c.process_sample_called);
  BOOST_CHECK(d.process_sample_called);
  BOOST_CHECK(e.process_sample_called);
  auto check = [](const CheckMHRWStatsCollector::ProcessSampleCallData& d) {
    BOOST_CHECK(d.k == 3243); BOOST_CHECK(d.n == 1245); MY_BOOST_CHECK_FLOATS_EQUAL(d.fnval, 1.45, tol);
  };
  check(a.process_sample_call_data);
  check(b.process_sample_call_data);
  check(c.process_sample_call_data);
  check(d.process_sample_call_data);
  check(e.process_sample_call_data);
}

BOOST_AUTO_TEST_CASE(raw_move)
{
  struct PointType { std::string dummy; PointType() : dummy("hello") { } };
  struct DummyMHRW { int x; }; DummyMHRW mhrw;
  mult.raw_move(3243, true, false, true, 0.95, PointType(), 1.45, PointType(), 1.33, mhrw);
  BOOST_CHECK(a.raw_move_called);
  BOOST_CHECK(b.raw_move_called);
  BOOST_CHECK(c.raw_move_called);
  BOOST_CHECK(d.raw_move_called);
  BOOST_CHECK(e.raw_move_called);
  auto check = [](const CheckMHRWStatsCollector::RawMoveCallData& d) {
    BOOST_CHECK(d.k == 3243); BOOST_CHECK(d.is_thermalizing == true);
    BOOST_CHECK(d.is_live_iter == false); BOOST_CHECK(d.accepted == true);
    MY_BOOST_CHECK_FLOATS_EQUAL(d.a, 0.95, tol);
    MY_BOOST_CHECK_FLOATS_EQUAL(d.newptval, 1.45, tol);
    MY_BOOST_CHECK_FLOATS_EQUAL(d.curptval, 1.33, tol);
  };
  check(a.raw_move_call_data);
  check(b.raw_move_call_data);
  check(c.raw_move_call_data);
  check(d.raw_move_call_data);
  check(e.raw_move_call_data);
}

BOOST_AUTO_TEST_SUITE_END();

// =============================================================================

BOOST_AUTO_TEST_SUITE(tValueHistogramMHRWStatsCollector)

BOOST_AUTO_TEST_CASE(WRITE_ME)
{
  BOOST_CHECK( false );  // write test for ValueHistogramMHRWStatsCollector
}

BOOST_AUTO_TEST_SUITE_END();

// =============================================================================

BOOST_AUTO_TEST_SUITE(tValueHistogramWithBinningMHRWStatsCollector)

BOOST_AUTO_TEST_CASE(ValueHistogramWithBinningMHRWStatsCollectorParams_WRITE_ME)
{
  BOOST_CHECK( false ); // make sure to test the "Params" object as well .....
}

BOOST_AUTO_TEST_CASE(WRITE_ME)
{
  BOOST_CHECK( false );  // write test for ValueHistogramMHRWStatsCollector
}

BOOST_AUTO_TEST_SUITE_END();

// =============================================================================

BOOST_AUTO_TEST_SUITE(tMHRWStatsCollectorStatus__WITH_SPECIALIZATIONS)

BOOST_AUTO_TEST_CASE(WRITE_ME)
{
  BOOST_CHECK( false );  // write test for ValueHistogramMHRWStatsCollector
}

BOOST_AUTO_TEST_SUITE_END();

// =============================================================================

BOOST_AUTO_TEST_SUITE_END()

