/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
 * Copyright (c) 2017 Caltech, Institute for Quantum Information and Matter, Philippe Faist
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
// <Eigen/...> or <tomographer/...> header
#include "test_tomographer.h"

#include <tomographer/mhrwstatscollectors.h>

#include <tomographer/tools/boost_test_logger.h>


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
  void thermalizingDone()
  {
    thermalizing_done_called = true;
  }
  void done()
  {
    done_called = true;
  }
  template<typename CountIntType, typename PointType, typename FnValueType, typename MHRandomWalk>
  void processSample(CountIntType k, CountIntType n, const PointType & , FnValueType fnval, MHRandomWalk & )
  {
    process_sample_called = true;
    process_sample_call_data.k = k;
    process_sample_call_data.n = n;
    process_sample_call_data.fnval = fnval;
  }
  template<typename CountIntType, typename PointType, typename FnValueType, typename MHRandomWalk>
  void rawMove(CountIntType k, bool is_thermalizing, bool is_live_iter, bool accepted, double a,
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

struct CheckMHRWStatsCollectorWResult : public CheckMHRWStatsCollector
{
  CheckMHRWStatsCollectorWResult(const int id) : CheckMHRWStatsCollector(id) { }

  struct ResultType {
    ResultType(int id_) : id(id_), timestwo(id_*2), timesthree(id_*3) { }
    ResultType(ResultType && ) = default;

    int id;
    int timestwo;
    int timesthree;

    // make sure implementations don't rely on these
    ResultType(const ResultType & ) = delete;
    ResultType& operator=(const ResultType & ) = delete;
  };

  ResultType getResult() const { return ResultType(id); }
  // here nothing special to do, same as getResult()
  ResultType stealResult() const { return ResultType(id); }
};

struct CheckMHRWStatsCollectorWCopyableResult : public CheckMHRWStatsCollector
{
  CheckMHRWStatsCollectorWCopyableResult(const int id) : CheckMHRWStatsCollector(id) { }

  struct ResultType {
    ResultType(int id_) : id(id_), timestwo(id_*2), timesthree(id_*3) { }
    ResultType(const ResultType & ) = default; // also copyable
    ResultType(ResultType && ) = default;

    int id;
    int timestwo;
    int timesthree;

    // make sure implementations don't rely on these
    ResultType& operator=(const ResultType & ) = delete;
  };

  ResultType getResult() const { return ResultType(id); }
  // here nothing special to do, same as getResult()
  ResultType stealResult() const { return ResultType(id); }
};


struct test_multmhrwstatscollectors_fixture
{
  CheckMHRWStatsCollectorWResult a;
  CheckMHRWStatsCollector b;
  CheckMHRWStatsCollectorWResult c;
  CheckMHRWStatsCollector d;
  CheckMHRWStatsCollector e;

  Tomographer::MultipleMHRWStatsCollectors<CheckMHRWStatsCollectorWResult,CheckMHRWStatsCollector,
                                           CheckMHRWStatsCollectorWResult,CheckMHRWStatsCollector,
                                           CheckMHRWStatsCollector> mult;

  test_multmhrwstatscollectors_fixture()
    : a(0), b(1), c(2), d(3), e(4),
      mult(a, b, c, d, e)
  {
  }
  
};






struct SimpleDummyStatsCollector {
  std::string dummy_status_message;
  SimpleDummyStatsCollector(std::string msg) : dummy_status_message(msg) { }
};

namespace Tomographer { namespace Tools {
template<>
struct StatusProvider<SimpleDummyStatsCollector>
{
  static constexpr bool CanProvideStatusLine = true;

  static inline std::string getStatusLine(const SimpleDummyStatsCollector * stats)
  {
    return stats->dummy_status_message;
  }
};
} } // namespaces


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
  mult.thermalizingDone();
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
  mult.processSample(3243, 1245, PointType(), 1.45, mhrw);
  BOOST_CHECK(a.process_sample_called);
  BOOST_CHECK(b.process_sample_called);
  BOOST_CHECK(c.process_sample_called);
  BOOST_CHECK(d.process_sample_called);
  BOOST_CHECK(e.process_sample_called);
  auto check = [](const CheckMHRWStatsCollector::ProcessSampleCallData& d) {
    BOOST_CHECK_EQUAL(d.k ,  3243); BOOST_CHECK_EQUAL(d.n ,  1245); MY_BOOST_CHECK_FLOATS_EQUAL(d.fnval, 1.45, tol);
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
  mult.rawMove(3243, true, false, true, 0.95, PointType(), 1.45, PointType(), 1.33, mhrw);
  BOOST_CHECK(a.raw_move_called);
  BOOST_CHECK(b.raw_move_called);
  BOOST_CHECK(c.raw_move_called);
  BOOST_CHECK(d.raw_move_called);
  BOOST_CHECK(e.raw_move_called);
  auto check = [](const CheckMHRWStatsCollector::RawMoveCallData& d) {
    BOOST_CHECK_EQUAL(d.k ,  3243); BOOST_CHECK_EQUAL(d.is_thermalizing ,  true);
    BOOST_CHECK_EQUAL(d.is_live_iter ,  false); BOOST_CHECK_EQUAL(d.accepted ,  true);
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


BOOST_AUTO_TEST_CASE(mkfunc)
{
  auto mult2 = Tomographer::mkMultipleMHRWStatsCollectors(a, b, c, d, e);
  mult2.init();
  BOOST_CHECK(a.init_called);
  BOOST_CHECK(b.init_called);
  BOOST_CHECK(c.init_called);
  BOOST_CHECK(d.init_called);
  BOOST_CHECK(e.init_called);
}


BOOST_AUTO_TEST_CASE(results)
{
  typedef std::tuple<CheckMHRWStatsCollectorWResult::ResultType,Tomographer::MHRWStatsCollectorNoResult,
                     CheckMHRWStatsCollectorWResult::ResultType,Tomographer::MHRWStatsCollectorNoResult,
                     Tomographer::MHRWStatsCollectorNoResult>  FullResultType;

  FullResultType res(mult.stealResult());

  BOOST_CHECK_EQUAL(std::get<0>(res).id, 0) ;
  BOOST_CHECK_EQUAL(std::get<0>(res).timestwo, 0) ;
  BOOST_CHECK_EQUAL(std::get<0>(res).timesthree, 0) ;
  BOOST_CHECK_EQUAL(std::get<2>(res).id, 2) ;
  BOOST_CHECK_EQUAL(std::get<2>(res).timestwo, 4) ;
  BOOST_CHECK_EQUAL(std::get<2>(res).timesthree, 6) ;
}

BOOST_AUTO_TEST_CASE(results_copyable)
{
  typedef std::tuple<CheckMHRWStatsCollectorWCopyableResult::ResultType,Tomographer::MHRWStatsCollectorNoResult,
                     CheckMHRWStatsCollectorWCopyableResult::ResultType,Tomographer::MHRWStatsCollectorNoResult,
                     Tomographer::MHRWStatsCollectorNoResult>  FullResultType;

  CheckMHRWStatsCollectorWCopyableResult a(0);
  CheckMHRWStatsCollector b(1);
  CheckMHRWStatsCollectorWCopyableResult c(2);
  CheckMHRWStatsCollector d(3);
  CheckMHRWStatsCollector e(4);

  Tomographer::MultipleMHRWStatsCollectors<CheckMHRWStatsCollectorWCopyableResult,CheckMHRWStatsCollector,
                                           CheckMHRWStatsCollectorWCopyableResult,CheckMHRWStatsCollector,
                                           CheckMHRWStatsCollector> mult(a, b, c, d, e);

  FullResultType res(mult.getResult());

  BOOST_CHECK_EQUAL(std::get<0>(res).id, 0) ;
  BOOST_CHECK_EQUAL(std::get<0>(res).timestwo, 0) ;
  BOOST_CHECK_EQUAL(std::get<0>(res).timesthree, 0) ;
  BOOST_CHECK_EQUAL(std::get<2>(res).id, 2) ;
  BOOST_CHECK_EQUAL(std::get<2>(res).timestwo, 4) ;
  BOOST_CHECK_EQUAL(std::get<2>(res).timesthree, 6) ;
}


BOOST_AUTO_TEST_SUITE_END();

// =============================================================================

// fixtures:

struct TestStatsCollectorFixture
{
  typedef int PointType;

  struct DummyMHRW { int x; };
  DummyMHRW mhrw;

  const unsigned int num_samples;

  const Eigen::ArrayXi pt_seq;

  TestStatsCollectorFixture()
    : mhrw(),
      num_samples(16),
      pt_seq((Eigen::ArrayXi(32) << 0, 1, 2, 1, 3, 4, 5, 4, 5, 6, 7, 9, 8, 7, 5, 4,
              /* */                 3, 1, 2, 1, 3, 4, 5, 4, 5, 6, 7, 9, 8, 7, 5, 4).finished())
  {
  }

  template<typename StatsColl>
  void run_dummy_rw(StatsColl & statcoll) {
  
    statcoll.init();
    statcoll.rawMove(0, true, false, true, 0.95,
                     PointType(1), 123.4, PointType(0), 123.4, mhrw);
    statcoll.rawMove(1, true, false, true, 1.0,
                     PointType(2), 123.4, PointType(1), 123.4, mhrw);
    statcoll.rawMove(2, true, false, true, 1.0,
                     PointType(0), 123.4, PointType(2), 123.4, mhrw);

    statcoll.thermalizingDone();

    int last_pt = 0;
    for (int k = 0; k < pt_seq.size(); ++k) {
      statcoll.rawMove(k, false, (k%2==0), true, 1.0,
                       pt_seq(k), 123.4, last_pt, 123.4, mhrw);
      if (k%2==0) {
        statcoll.processSample(k, k/2-1, pt_seq(k), 123.4, mhrw); // takes sample here
      }
      last_pt = pt_seq(k);
    }
    statcoll.done();
  
  }
};

struct MyMinimalistValueCalculator {
  typedef double ValueType;
  MyMinimalistValueCalculator() { }
  double getValue(int pt) const {
    return std::sqrt(double(pt));
  };
};

// tests:

BOOST_AUTO_TEST_SUITE(tValueHistogramMHRWStatsCollector)

BOOST_FIXTURE_TEST_CASE(simple, TestStatsCollectorFixture)
{
  MyMinimalistValueCalculator valcalc;
  Tomographer::Logger::BoostTestLogger logger;
  typedef Tomographer::ValueHistogramMHRWStatsCollector<MyMinimalistValueCalculator, Tomographer::Logger::BoostTestLogger>
    MyValueHistogramMHRWStatsCollector;

  MyValueHistogramMHRWStatsCollector statcoll(MyValueHistogramMHRWStatsCollector::HistogramParams(0,4,4),
                                              valcalc, logger);

  run_dummy_rw(statcoll);

  // for ValueHistogramMHRWStatsCollector, the ResultType is the histogram itself, simply
  BOOST_CHECK( (statcoll.histogram().bins == statcoll.getResult().bins).all() ) ;
  // check the histogram
  BOOST_MESSAGE("The collected histogram is:\n" << statcoll.histogram().prettyPrint()) ;
  BOOST_CHECK_EQUAL( (int)statcoll.histogram().bins.sum(), (int)num_samples) ;
  BOOST_CHECK_EQUAL( statcoll.histogram().bins(0) ,  1) ; // [0,1[
  BOOST_CHECK_EQUAL( statcoll.histogram().bins(1) ,  5) ; // [1,2[
  BOOST_CHECK_EQUAL( statcoll.histogram().bins(2) ,  10); // [2,3[
  BOOST_CHECK_EQUAL( statcoll.histogram().bins(3) ,  0) ; // [3,4[
}

BOOST_FIXTURE_TEST_CASE(mkfunc, TestStatsCollectorFixture)
{
  MyMinimalistValueCalculator valcalc;
  Tomographer::Logger::BoostTestLogger logger;

  auto statcoll = Tomographer::mkValueHistogramMHRWStatsCollector(
      Tomographer::HistogramParams<>(0,4,4), valcalc, logger
      );

  run_dummy_rw(statcoll);

  // for ValueHistogramMHRWStatsCollector, the ResultType is the histogram itself, simply
  BOOST_CHECK( (statcoll.histogram().bins == statcoll.getResult().bins).all() ) ;
  // check the histogram
  BOOST_MESSAGE("The collected histogram is:\n" << statcoll.histogram().prettyPrint()) ;
  BOOST_CHECK_EQUAL( (int)statcoll.histogram().bins.sum(), (int)num_samples) ;
  BOOST_CHECK_EQUAL( statcoll.histogram().bins(0) ,  1) ; // [0,1[
  BOOST_CHECK_EQUAL( statcoll.histogram().bins(1) ,  5) ; // [1,2[
  BOOST_CHECK_EQUAL( statcoll.histogram().bins(2) ,  10); // [2,3[
  BOOST_CHECK_EQUAL( statcoll.histogram().bins(3) ,  0) ; // [3,4[
}

// BOOST_FIXTURE_TEST_CASE(customhistogramtype, TestStatsCollectorFixture)
// {
//   // checks necessary?
// }

BOOST_AUTO_TEST_SUITE_END();


BOOST_AUTO_TEST_SUITE(tValueHistogramWithBinningMHRWStatsCollector)

BOOST_FIXTURE_TEST_CASE(simple, TestStatsCollectorFixture)
{
  MyMinimalistValueCalculator valcalc;
  Tomographer::Logger::BoostTestLogger logger;
  typedef Tomographer::ValueHistogramWithBinningMHRWStatsCollectorParams<MyMinimalistValueCalculator> VHWBParams;
  typedef Tomographer::ValueHistogramWithBinningMHRWStatsCollector<VHWBParams, Tomographer::Logger::BoostTestLogger>
    MyStatsCollector;

  MyStatsCollector statcoll(MyStatsCollector::HistogramParams(0,4,4),
                            valcalc,
                            2, // number of binning levels
                            logger);

  run_dummy_rw(statcoll);

  // check the base histogram
  BOOST_MESSAGE("The collected histogram is:\n" << statcoll.histogram().prettyPrint()) ;
  BOOST_CHECK_EQUAL( (int)statcoll.histogram().bins.sum(), (int)num_samples) ;
  BOOST_CHECK_EQUAL( statcoll.histogram().bins(0) ,  1) ; // [0,1[
  BOOST_CHECK_EQUAL( statcoll.histogram().bins(1) ,  5) ; // [1,2[
  BOOST_CHECK_EQUAL( statcoll.histogram().bins(2) ,  10); // [2,3[
  BOOST_CHECK_EQUAL( statcoll.histogram().bins(3) ,  0) ; // [3,4[
  // check the histogram with error bars
  const auto & fhist = statcoll.getResult().histogram;
  BOOST_MESSAGE("The full histogram is:\n" << fhist.prettyPrint()) ;
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.bins(0) ,  1 / 16.0 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.bins(1) ,  5 / 16.0 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.bins(2) ,  10 / 16.0 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.bins(3) ,  0 / 16.0 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.delta(0) ,  0.0625 , 0.001 );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.delta(1) ,  0.1875 , 0.001 );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.delta(2) ,  0.2165 , 0.001 );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.delta(3) ,  0.0000 , 0.001 );

  // binning error analysis information
  auto result = statcoll.stealResult();
  BOOST_MESSAGE( "error_levels = \n" << result.error_levels ) ;
  // just make sure the info is there with correct shape; the values themselves should be
  // tested when testing our binning analysis mechanism (test_mhrw_bin_err.cxx)
  BOOST_CHECK_EQUAL(result.error_levels.rows(), 4);
  BOOST_CHECK_EQUAL(result.error_levels.cols(), 3); // two levels of binning

  // Info about convergence of error bars
  BOOST_MESSAGE( "converged_status = \n" << result.converged_status );
  // just make sure the info is there with correct shape; the values themselves should be
  // tested when testing our binning analysis mechanism (test_mhrw_bin_err.cxx)
  BOOST_CHECK_EQUAL(result.converged_status.rows(), 4);
  BOOST_CHECK_EQUAL(result.converged_status.cols(), 1);
  //  // here, the values are actually simple: the convergence is unknown because there are too few samples:
  //  BOOST_CHECK( (result.converged_status ==
  //                Eigen::ArrayXi::Constant(4, MyStatsCollector::BinningAnalysisParamsType::UNKNOWN_CONVERGENCE)).all() ) ;
}

BOOST_FIXTURE_TEST_CASE(simple_mkfunc, TestStatsCollectorFixture)
{
  MyMinimalistValueCalculator valcalc;
  Tomographer::Logger::BoostTestLogger logger;

  auto statcoll = Tomographer::mkValueHistogramWithBinningMHRWStatsCollector(
      Tomographer::HistogramParams<>(0,4,4),
      valcalc,
      2, // number of binning levels
      logger);

  run_dummy_rw(statcoll);

  // check the base histogram
  BOOST_MESSAGE("The collected histogram is:\n" << statcoll.histogram().prettyPrint()) ;
  BOOST_CHECK_EQUAL( (int)statcoll.histogram().bins.sum(), (int)num_samples) ;
  BOOST_CHECK_EQUAL( statcoll.histogram().bins(0) ,  1) ; // [0,1[
  BOOST_CHECK_EQUAL( statcoll.histogram().bins(1) ,  5) ; // [1,2[
  BOOST_CHECK_EQUAL( statcoll.histogram().bins(2) ,  10); // [2,3[
  BOOST_CHECK_EQUAL( statcoll.histogram().bins(3) ,  0) ; // [3,4[
  // check the histogram with error bars
  const auto & fhist = statcoll.getResult().histogram;
  BOOST_MESSAGE("The full histogram is:\n" << fhist.prettyPrint()) ;
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.bins(0) ,  1 / 16.0 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.bins(1) ,  5 / 16.0 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.bins(2) ,  10 / 16.0 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.bins(3) ,  0 / 16.0 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.delta(0) ,  0.0625 , 0.001 );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.delta(1) ,  0.1875 , 0.001 );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.delta(2) ,  0.2165 , 0.001 );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.delta(3) ,  0.0000 , 0.001 );

  // binning error analysis information
  auto result = statcoll.stealResult();
  BOOST_MESSAGE( "error_levels = \n" << result.error_levels ) ;
  // just make sure the info is there with correct shape; the values themselves should be
  // tested when testing our binning analysis mechanism (test_mhrw_bin_err.cxx)
  BOOST_CHECK_EQUAL(result.error_levels.rows(), 4);
  BOOST_CHECK_EQUAL(result.error_levels.cols(), 3); // two levels of binning
}



BOOST_AUTO_TEST_CASE(convergence_summary)
{
  typedef Tomographer::ValueHistogramWithBinningMHRWStatsCollectorParams<MyMinimalistValueCalculator>
    VHWBParams;

  const auto CONVERGED = Tomographer::BINNING_CONVERGED;
  const auto UNKNOWN_CONVERGENCE = Tomographer::BINNING_UNKNOWN_CONVERGENCE;
  const auto NOT_CONVERGED = Tomographer::BINNING_NOT_CONVERGED;

  // some artificial converged_status vector
  VHWBParams::Result r;
  r.converged_status = Eigen::ArrayXi(16);
  r.converged_status << 
    CONVERGED,
    NOT_CONVERGED,
    CONVERGED,
    UNKNOWN_CONVERGENCE,
    UNKNOWN_CONVERGENCE,
    CONVERGED,
    UNKNOWN_CONVERGENCE,
    CONVERGED,
    UNKNOWN_CONVERGENCE,
    NOT_CONVERGED,
    CONVERGED,
    CONVERGED,
    UNKNOWN_CONVERGENCE,
    CONVERGED,
    CONVERGED,
    UNKNOWN_CONVERGENCE ;

  // Summary of convergence status
  Tomographer::BinningErrorBarConvergenceSummary summary = r.errorBarConvergenceSummary();
  BOOST_MESSAGE( "summary of error bar convergence:  " << summary.n_converged << " converged / "
                 << summary.n_unknown << " maybe (" << summary.n_unknown_isolated << " isolated) / "
                 << summary.n_not_converged << " not converged" );

  BOOST_CHECK_EQUAL(summary.n_converged + summary.n_unknown + summary.n_not_converged, summary.n_bins);
  BOOST_CHECK_LE(summary.n_unknown_isolated, summary.n_unknown);

  BOOST_CHECK_EQUAL(summary.n_bins, 16);
  BOOST_CHECK_EQUAL(summary.n_converged, 8);
  BOOST_CHECK_EQUAL(summary.n_unknown, 6);
  BOOST_CHECK_EQUAL(summary.n_unknown_isolated, 3);
  BOOST_CHECK_EQUAL(summary.n_not_converged, 2);
}

BOOST_AUTO_TEST_SUITE_END();

// =============================================================================

BOOST_AUTO_TEST_SUITE(tMHRWStatsCollector_status)

BOOST_AUTO_TEST_CASE(base_multi_status)
{
  SimpleDummyStatsCollector sc1("status - 1");
  SimpleDummyStatsCollector sc2("status - 2");
  typedef Tomographer::MultipleMHRWStatsCollectors<SimpleDummyStatsCollector, SimpleDummyStatsCollector> MultiStatsColl;
  MultiStatsColl msc(sc1, sc2);

  TOMO_STATIC_ASSERT_EXPR( Tomographer::Tools::StatusQuery<MultiStatsColl>::CanProvideStatusLine );

  BOOST_CHECK_EQUAL( Tomographer::Tools::StatusQuery<MultiStatsColl>::getStatusLine(&msc) , "status - 1\nstatus - 2" );
}

BOOST_FIXTURE_TEST_CASE(status_for_valuehistogram, TestStatsCollectorFixture)
{
  MyMinimalistValueCalculator valcalc;
  Tomographer::Logger::BoostTestLogger logger;
  typedef Tomographer::ValueHistogramMHRWStatsCollector<MyMinimalistValueCalculator, Tomographer::Logger::BoostTestLogger>
    MyValueHistogramMHRWStatsCollector;

  MyValueHistogramMHRWStatsCollector statcoll(MyValueHistogramMHRWStatsCollector::HistogramParams(0,4,4),
                                              valcalc, logger);

  run_dummy_rw(statcoll);

  TOMO_STATIC_ASSERT_EXPR(
      Tomographer::Tools::StatusQuery<MyValueHistogramMHRWStatsCollector>::CanProvideStatusLine
      );
  BOOST_CHECK_EQUAL( Tomographer::Tools::StatusQuery<MyValueHistogramMHRWStatsCollector>::getStatusLine(&statcoll),
      "Histogram: 0|.x# |4");
}

BOOST_FIXTURE_TEST_CASE(status_for_valuehistogrambinning, TestStatsCollectorFixture)
{
  MyMinimalistValueCalculator valcalc;
  Tomographer::Logger::BoostTestLogger logger;
  typedef Tomographer::ValueHistogramWithBinningMHRWStatsCollectorParams<MyMinimalistValueCalculator> VHWBParams;
  typedef Tomographer::ValueHistogramWithBinningMHRWStatsCollector<VHWBParams, Tomographer::Logger::BoostTestLogger>
    MyStatsCollector;

  MyStatsCollector statcoll(MyStatsCollector::HistogramParams(0,4,4),
                            valcalc,
                            2, // number of binning levels
                            logger);

  run_dummy_rw(statcoll);

  TOMO_STATIC_ASSERT_EXPR(
      Tomographer::Tools::StatusQuery<MyStatsCollector>::CanProvideStatusLine
      );
  BOOST_CHECK_EQUAL( Tomographer::Tools::StatusQuery<MyStatsCollector>::getStatusLine(&statcoll),
      "0|.x# |4   err(cnvg/?/x): 0/4/0");
}

BOOST_AUTO_TEST_SUITE_END();

// =============================================================================

BOOST_AUTO_TEST_SUITE_END()

