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

#include <tomographer/mhrw_valuehist_tools.h>
#include <tomographer/multiproc.h>

#include <tomographer/tools/boost_test_logger.h>

#include "test_mh_random_walk_common.h" // our test-case random walk


#define MY_BOOSTCHK( ... )                                   \
  { bool my_boost_check_condition_value = ( __VA_ARGS__ );      \
    BOOST_TEST_MESSAGE( "Testing ... " << #__VA_ARGS__ );       \
    BOOST_CHECK( my_boost_check_condition_value ) ; }

// -----------------------------------------------------------------------------
// fixture(s)

struct IdentValueCalculator
{
  IdentValueCalculator(int member_ = -1) : member(member_) { }

  int member;

  typedef double ValueType;

  template<typename ScalarType>
  inline double getValue(ScalarType x) const
  {
    return (double) x;
  }
};



template<bool UseBinningAnalysis>
struct check_cdata_types
{
  typedef Tomographer::MHRWTasks::ValueHistogramTools::CDataBase<
    IdentValueCalculator,
    UseBinningAnalysis,
    Tomographer::MHWalkerParamsStepSize<float>,
    /* IterCountIntType_ = */ unsigned long,
    /* CountRealType_ = */ long double,
    /* HistCountIntType_ = */ long
    >
    CDataBase;

  typedef typename CDataBase::MHRWParamsType CDataBaseMHRWParamsType;

  typedef Tomographer::MHWalkerParamsStepSize<float> OurMHWalkerParamsType;

  void check_basic_types()
  {
    BOOST_CHECK_EQUAL( CDataBase::UseBinningAnalysis, UseBinningAnalysis );

    MY_BOOSTCHK( std::is_same<typename CDataBase::MHWalkerParams,
                              Tomographer::MHWalkerParamsStepSize<float> >::value ) ;

    MY_BOOSTCHK( std::is_same<typename CDataBase::IterCountIntType, unsigned long>::value ) ;
    MY_BOOSTCHK( std::is_same<typename CDataBase::CountRealType, long double>::value ) ;
    MY_BOOSTCHK( std::is_same<typename CDataBase::HistCountIntType, long>::value ) ;

    MY_BOOSTCHK( std::is_same<typename CDataBase::ValueCalculator, IdentValueCalculator>::value ) ;
    
    MY_BOOSTCHK( std::is_same<typename CDataBase::MHRWParamsType,
                              Tomographer::MHRWParams<Tomographer::MHWalkerParamsStepSize<float>,
                                                      typename CDataBase::IterCountIntType>  >::value ) ;

    BOOST_CHECK_EQUAL( CDataBase::HistogramType::HasErrorBars , UseBinningAnalysis ) ;

    check_basic_types_specifics();
  }

  TOMOGRAPHER_ENABLED_IF(!UseBinningAnalysis)
  void check_basic_types_specifics()
  {
    // no binning analysis

    MY_BOOSTCHK(
        std::is_same<
          typename CDataBase::AggregatedHistogramType,
          Tomographer::AggregatedHistogramSimple<
            Tomographer::Histogram<typename CDataBase::HistogramType::Scalar,
                                   typename CDataBase::CountRealType>,
            typename CDataBase::CountRealType
          >
        >::value
        ) ;

    MY_BOOSTCHK(
        std::is_same<typename CDataBase::ValueStatsCollectorResultType,
                     typename CDataBase::HistogramType>::value
          ) ;

    MY_BOOSTCHK(
        std::is_same<typename CDataBase::MHRWStatsResultsBaseType,
                     Tomographer::MHRWTasks::ValueHistogramTools::MHRWStatsResultsBaseSimple<
                       typename CDataBase::HistogramType,
                       Tomographer::Histogram<
                         typename CDataBase::HistogramType::Scalar,
                         typename CDataBase::CountRealType
                         > > >::value
          )
  }

  TOMOGRAPHER_ENABLED_IF(UseBinningAnalysis)
  void check_basic_types_specifics()
  {
    // with binning analysis

    MY_BOOSTCHK( std::is_same<typename CDataBase::AggregatedHistogramType,
                              Tomographer::AggregatedHistogramWithErrorBars<
                                typename CDataBase::HistogramType,
                                typename CDataBase::CountRealType
                              > >::value ) ;

    MY_BOOSTCHK(
        std::is_same<typename CDataBase::ValueStatsCollectorResultType,
                     typename Tomographer::ValueHistogramWithBinningMHRWStatsCollectorParams<
                       typename CDataBase::ValueCalculator,
                       typename CDataBase::HistCountIntType,
                       typename CDataBase::CountRealType,
                       Eigen::Dynamic,
                       Eigen::Dynamic
                       >::Result>::value
          );

    MY_BOOSTCHK(
        std::is_same<typename CDataBase::MHRWStatsResultsBaseType,
                     typename Tomographer::ValueHistogramWithBinningMHRWStatsCollectorParams<
                       typename CDataBase::ValueCalculator,
                       typename CDataBase::HistCountIntType,
                       typename CDataBase::CountRealType,
                       Eigen::Dynamic,
                       Eigen::Dynamic
                       >::Result>::value
          );

  }
};


// -----------------------------------------------------------------------------
// test suites


BOOST_AUTO_TEST_SUITE(test_mhrw_valuehist_tools)
// =============================================================================

BOOST_AUTO_TEST_SUITE(tMHRWStatsResultsBaseSimple)

BOOST_AUTO_TEST_CASE(works)
{
  typedef Tomographer::Histogram<double,int> RawHistType;
  RawHistType h(0, 1, 2);
  h.bins << 9, 8;
  h.off_chart = 3;

  Tomographer::MHRWTasks::ValueHistogramTools::MHRWStatsResultsBaseSimple<
    RawHistType,
    Tomographer::Histogram<double,float>
    >
    r(std::move(h));
  
  MY_BOOST_CHECK_FLOATS_EQUAL(r.raw_histogram.params.min, 0.0, tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(r.raw_histogram.params.max, 1.0, tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(r.histogram.params.min, 0.0, tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(r.histogram.params.max, 1.0, tol);
  BOOST_CHECK_EQUAL(r.raw_histogram.params.num_bins, 2u);
  BOOST_CHECK_EQUAL(r.histogram.params.num_bins, 2u);
  BOOST_CHECK_EQUAL(r.raw_histogram.bins(0), 9);
  BOOST_CHECK_EQUAL(r.raw_histogram.bins(1), 8);
  BOOST_CHECK_EQUAL(r.raw_histogram.off_chart, 3);
  MY_BOOST_CHECK_FLOATS_EQUAL(r.histogram.bins(0), 9.f/20.f, tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(r.histogram.bins(1), 8.f/20.f, tol_f);
  MY_BOOST_CHECK_FLOATS_EQUAL(r.histogram.off_chart, 3.f/20.f, tol_f);
}

BOOST_AUTO_TEST_SUITE_END() ;

// -----------------------------------------------

BOOST_AUTO_TEST_SUITE(tCDataBase)

BOOST_FIXTURE_TEST_CASE(simple, check_cdata_types<false>)
{
  check_basic_types();

  IdentValueCalculator valcalc(123);
  Tomographer::HistogramParams<double> histparams(0,1,2);
  CDataBaseMHRWParamsType mhrw_params(0.1f, 1024, 500, 32768);

  CDataBase cdat(valcalc, histparams, mhrw_params, 999);

  BOOST_CHECK_EQUAL(cdat.valcalc.member, 123);

  MY_BOOST_CHECK_FLOATS_EQUAL(cdat.histogram_params.min, 0.0, tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(cdat.histogram_params.max, 1.0, tol);
  BOOST_CHECK_EQUAL(cdat.histogram_params.num_bins, 2u);

  MY_BOOST_CHECK_FLOATS_EQUAL(cdat.mhrw_params.mhwalker_params.step_size, 0.1f, tol_f);
  BOOST_CHECK_EQUAL(cdat.mhrw_params.n_sweep, 1024);
  BOOST_CHECK_EQUAL(cdat.mhrw_params.n_therm, 500);
  BOOST_CHECK_EQUAL(cdat.mhrw_params.n_run, 32768);

  BOOST_CHECK_EQUAL(cdat.base_seed, 999u);

  // aggregated histograms

  typename CDataBase::HistogramType h1(0, 1, 2);
  h1.bins << 9, 8;
  h1.off_chart = 3;
  typename CDataBase::HistogramType h2(0, 1, 2);
  h2.bins << 7, 8;
  h2.off_chart = 5;

  typedef typename CDataBase::MHRWStatsResultsBaseType MHRWStatsResultsBaseType;
  typedef Tomographer::MHRWTasks::MHRandomWalkTaskResult<MHRWStatsResultsBaseType,
                                                         typename CDataBase::IterCountIntType,
                                                         OurMHWalkerParamsType
                                                         > TaskResultType;
  std::vector<TaskResultType*> task_results;
  task_results.push_back(new TaskResultType(MHRWStatsResultsBaseType(std::move(h1)), cdat.mhrw_params, 0.25));
  task_results.push_back(new TaskResultType(MHRWStatsResultsBaseType(std::move(h2)), cdat.mhrw_params, 0.28));
  
  const std::vector<TaskResultType*> & task_results_cr = task_results;

  typename CDataBase::AggregatedHistogramType aggregated_histogram
    = cdat.aggregateResultHistograms( task_results_cr ) ;
  
  const long double tol_l = (long double)tol;

  MY_BOOST_CHECK_FLOATS_EQUAL( aggregated_histogram.final_histogram.params.min, 0.0, tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( aggregated_histogram.final_histogram.params.max, 1.0, tol );
  BOOST_CHECK_EQUAL( aggregated_histogram.final_histogram.params.num_bins, 2 );
  MY_BOOST_CHECK_FLOATS_EQUAL( aggregated_histogram.final_histogram.bins(0), 8.0l/20.0l, tol_l );
  MY_BOOST_CHECK_FLOATS_EQUAL( aggregated_histogram.final_histogram.bins(1), 8.0l/20.0l, tol_l );
  MY_BOOST_CHECK_FLOATS_EQUAL( aggregated_histogram.final_histogram.delta(0), 0.05l, tol_l );
  MY_BOOST_CHECK_FLOATS_EQUAL( aggregated_histogram.final_histogram.delta(1), 0.0l, tol_l );
  MY_BOOST_CHECK_FLOATS_EQUAL( aggregated_histogram.final_histogram.off_chart, 4.0/20.0l, tol_l );

  delete task_results[0];
  delete task_results[1];
}

BOOST_FIXTURE_TEST_CASE(binning_types, check_cdata_types<true>)
{
  check_basic_types();

  IdentValueCalculator valcalc(123);
  Tomographer::HistogramParams<double> histparams(0,1,2);
  CDataBaseMHRWParamsType mhrw_params(0.1f, 1024, 500, 32768);

  CDataBase cdat(valcalc, histparams, 12 /*num binning levels*/, mhrw_params, 999);

  BOOST_CHECK_EQUAL(cdat.valcalc.member, 123);

  MY_BOOST_CHECK_FLOATS_EQUAL(cdat.histogram_params.min, 0.0, tol);
  MY_BOOST_CHECK_FLOATS_EQUAL(cdat.histogram_params.max, 1.0, tol);
  BOOST_CHECK_EQUAL(cdat.histogram_params.num_bins, 2);

  BOOST_CHECK_EQUAL(cdat.binningNumLevels.value, 12) ;

  MY_BOOST_CHECK_FLOATS_EQUAL(cdat.mhrw_params.mhwalker_params.step_size, 0.1f, tol_f);
  BOOST_CHECK_EQUAL(cdat.mhrw_params.n_sweep, 1024);
  BOOST_CHECK_EQUAL(cdat.mhrw_params.n_therm, 500);
  BOOST_CHECK_EQUAL(cdat.mhrw_params.n_run, 32768);

  BOOST_CHECK_EQUAL(cdat.base_seed, 999);

  // aggregated histograms

  typedef typename CDataBase::HistogramType HistogramType;
  HistogramType h1(0, 1, 2);
  h1.bins << 9.0/20.0, 8.0/20.0;
  h1.delta << 0.3/20.0, 0.4/20.0;
  h1.off_chart = 3/20.0;
  HistogramType h2(0, 1, 2);
  h2.bins << 7.0/20.0, 8.0/20.0;
  h2.delta << 0.1/20.0, 0.5/20.0;
  h2.off_chart = 5/20.0;

  struct MHRWStatsResultsBaseType {
    MHRWStatsResultsBaseType(HistogramType && val) : histogram(std::move(val)) { }
    HistogramType histogram;
  };

  typedef Tomographer::MHRWTasks::MHRandomWalkTaskResult<MHRWStatsResultsBaseType,
                                                         typename CDataBase::IterCountIntType,
                                                         OurMHWalkerParamsType
                                                         > TaskResultType;
  std::vector<TaskResultType*> task_results;
  task_results.push_back(new TaskResultType(MHRWStatsResultsBaseType(std::move(h1)), cdat.mhrw_params, 0.25));
  task_results.push_back(new TaskResultType(MHRWStatsResultsBaseType(std::move(h2)), cdat.mhrw_params, 0.28));
  
  const std::vector<TaskResultType*> & task_results_cr = task_results;

  typename CDataBase::AggregatedHistogramType aggregated_histogram
    = cdat.aggregateResultHistograms( task_results_cr ) ;
  
  const long double tol_l = (long double)tol;

  MY_BOOST_CHECK_FLOATS_EQUAL( aggregated_histogram.final_histogram.params.min, 0.0, tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( aggregated_histogram.final_histogram.params.max, 1.0, tol );
  BOOST_CHECK_EQUAL( aggregated_histogram.final_histogram.params.num_bins, 2 );
  MY_BOOST_CHECK_FLOATS_EQUAL( aggregated_histogram.final_histogram.bins(0), 8.0/20.0l, tol_l );
  MY_BOOST_CHECK_FLOATS_EQUAL( aggregated_histogram.final_histogram.bins(1), 8.0/20.0l, tol_l );
  MY_BOOST_CHECK_FLOATS_EQUAL( aggregated_histogram.final_histogram.delta(0), 0.0079l, 1e-2l );
  MY_BOOST_CHECK_FLOATS_EQUAL( aggregated_histogram.final_histogram.delta(1), 0.016l, 1e-2l );
  MY_BOOST_CHECK_FLOATS_EQUAL( aggregated_histogram.final_histogram.off_chart, 4.0/20.0l, tol_l );
  MY_BOOST_CHECK_FLOATS_EQUAL( aggregated_histogram.simple_final_histogram.params.min, 0.0, tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( aggregated_histogram.simple_final_histogram.params.max, 1.0, tol );
  BOOST_CHECK_EQUAL( aggregated_histogram.simple_final_histogram.params.num_bins, 2 );
  MY_BOOST_CHECK_FLOATS_EQUAL( aggregated_histogram.simple_final_histogram.bins(0), 8.0/20.0l, tol_l );
  MY_BOOST_CHECK_FLOATS_EQUAL( aggregated_histogram.simple_final_histogram.bins(1), 8.0/20.0l, tol_l );
  MY_BOOST_CHECK_FLOATS_EQUAL( aggregated_histogram.simple_final_histogram.delta(0), 0.05l, tol_l);
  MY_BOOST_CHECK_FLOATS_EQUAL( aggregated_histogram.simple_final_histogram.delta(1), 0.0l, tol_l );
  MY_BOOST_CHECK_FLOATS_EQUAL( aggregated_histogram.simple_final_histogram.off_chart, 4.0/20.0l, tol_l );

  delete task_results[0];
  delete task_results[1];
}

BOOST_AUTO_TEST_SUITE_END() ;

// =============================================================================
BOOST_AUTO_TEST_SUITE_END()

