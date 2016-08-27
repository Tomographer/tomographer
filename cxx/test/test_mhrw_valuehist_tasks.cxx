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

#include <tomographer2/mhrw_valuehist_tasks.h>
#include <tomographer2/multiprocomp.h>

#include "boost_test_logger.h"
#include "test_mh_random_walk_common.h" // our test-case random walk


// -----------------------------------------------------------------------------
// fixture(s)

struct IdentValueCalculator
{
  typedef double ValueType;

  template<typename ScalarType>
  inline double getValue(ScalarType x) const
  {
    return (double) x;
  }
};



// -----------------------------------------------------------------------------
// test suites


BOOST_AUTO_TEST_SUITE(test_mhrw_valuehist_tasks)
// =============================================================================

BOOST_AUTO_TEST_SUITE(results_collector_simple)

BOOST_AUTO_TEST_CASE(types)
{
  typedef Tomographer::MHRWTasks::ValueHistogramTasks::ResultsCollectorSimple<
    Tomographer::MHRWTasks::ValueHistogramTasks::CDataBase<IdentValueCalculator, false>,
    BoostTestLogger> ResultsCollectorSimple;

  BOOST_CHECK( ! ResultsCollectorSimple::HistogramType::HasErrorBars ) ;
  BOOST_CHECK( ! ResultsCollectorSimple::NormalizedHistogramType::HasErrorBars ) ;
  BOOST_CHECK( ResultsCollectorSimple::FinalHistogramType::HasErrorBars ) ;
}

BOOST_AUTO_TEST_CASE(collects_histograms)
{
  typedef Tomographer::MHRWTasks::ValueHistogramTasks::CDataBase<IdentValueCalculator, false>  CDataType;
  typedef Tomographer::MHRWTasks::ValueHistogramTasks::ResultsCollectorSimple<
    CDataType, BoostTestLogger
    > ResultsCollectorSimple;

  BoostTestLogger logger;

  IdentValueCalculator valcalc;
  CDataType::HistogramParams hp(0, 10, 10);
  CDataType::MHRWParamsType mhrwparams(0.1, 512, 256, 1024);
  CDataType cdata(valcalc, hp, mhrwparams);
  
  ResultsCollectorSimple res(logger);

  BOOST_CHECK( ! res.isFinalized() );
  {
    EigenAssertTest::setting_scope settingvariable(true);
    // these should generate assertion failures.
    BOOST_CHECK_THROW( res.finalHistogram(), Tomographer::Tools::EigenAssertException) ;
    BOOST_CHECK_THROW( res.numTasks(), Tomographer::Tools::EigenAssertException) ;
    BOOST_CHECK_THROW( res.collectedRunTaskResults(), Tomographer::Tools::EigenAssertException ) ;
    BOOST_CHECK_THROW( res.collectedRunTaskResult(0), Tomographer::Tools::EigenAssertException ) ;
  };

  res.init(4, 1, &cdata) ;

  BOOST_CHECK( ! res.isFinalized() );
  {
    EigenAssertTest::setting_scope settingvariable(true);
    // these should generate assertion failures.
    BOOST_CHECK_THROW( res.finalHistogram(), Tomographer::Tools::EigenAssertException) ;
    BOOST_CHECK_THROW( res.numTasks(), Tomographer::Tools::EigenAssertException) ;
    BOOST_CHECK_THROW( res.collectedRunTaskResults(), Tomographer::Tools::EigenAssertException ) ;
    BOOST_CHECK_THROW( res.collectedRunTaskResult(0), Tomographer::Tools::EigenAssertException ) ;
  };

  typedef Tomographer::MHRWTasks::MHRandomWalkTaskResult<
    ResultsCollectorSimple::MHRWStatsCollectorResultType, int, double
    >  TaskResultType;

  CDataType::HistogramType h0(hp);
  h0.bins << 0 , 4 , 24 , 153 , 382 , 260 , 152 , 36 , 13 , 0 ;
  TaskResultType r0(h0, mhrwparams, 0.35);

  CDataType::HistogramType h1(hp);
  h1.bins << 0 , 10 , 26 , 147 , 380 , 258 , 154 , 31 , 17 , 1 ;
  TaskResultType r1(h1, mhrwparams, 0.41);

  CDataType::HistogramType h2(hp);
  h2.bins << 0 , 8 , 20 , 151 , 384 , 262 , 150 , 35 , 14 , 0 ;
  TaskResultType r2(h2, mhrwparams, 0.33);

  CDataType::HistogramType h3(hp);
  h3.bins << 0 , 5 , 23 , 155 , 372 , 258 , 168 , 30 , 12 , 1 ;
  TaskResultType r3(h3, mhrwparams, 0.32);

  res.collectResult(2, r2, &cdata);
  res.collectResult(1, r1, &cdata);
  res.collectResult(0, r0, &cdata);
  res.collectResult(3, r3, &cdata);

  BOOST_CHECK( ! res.isFinalized() );
  {
    EigenAssertTest::setting_scope settingvariable(true);
    // these should generate assertion failures.
    BOOST_CHECK_THROW( res.finalHistogram(), Tomographer::Tools::EigenAssertException) ;
    BOOST_CHECK_THROW( res.numTasks(), Tomographer::Tools::EigenAssertException) ;
    BOOST_CHECK_THROW( res.collectedRunTaskResults(), Tomographer::Tools::EigenAssertException ) ;
    BOOST_CHECK_THROW( res.collectedRunTaskResult(0), Tomographer::Tools::EigenAssertException ) ;
  };

  res.runsFinished(4, &cdata);
  
  BOOST_CHECK( res.isFinalized() );
  BOOST_CHECK_EQUAL( res.numTasks(), 4 );
  auto fhist = res.finalHistogram();
  BOOST_CHECK_EQUAL( fhist.numBins(), 10 );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.count(0) , 0.0 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.count(1) , 0.006591796875 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.count(2) , 0.022705078125 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.count(3) , 0.14794921875 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.count(4) , 0.37060546875 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.count(5) , 0.25341796875 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.count(6) , 0.15234375 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.count(7) , 0.0322265625 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.count(8) , 0.013671875 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.count(9) , 0.00048828125 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.errorBar(0) , 0.0 , 1e-6 );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.errorBar(1) , 0.00134462171565 , 1e-6 );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.errorBar(2) , 0.001220703125 , 1e-6 );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.errorBar(3) , 0.00166779797623 , 1e-6 );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.errorBar(4) , 0.00256831605437 , 1e-6 );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.errorBar(5) , 0.000934987409918 , 1e-6 );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.errorBar(6) , 0.00398679971156 , 1e-6 );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.errorBar(7) , 0.0014374610785 , 1e-6 );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.errorBar(8) , 0.00105480805638 , 1e-6 );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.errorBar(9) , 0.000281909311128 , 1e-6 );
}

BOOST_AUTO_TEST_SUITE_END()

// -----------------------------------------------

BOOST_AUTO_TEST_SUITE(results_collector_binning)

BOOST_AUTO_TEST_CASE(types)
{
  typedef Tomographer::MHRWTasks::ValueHistogramTasks::ResultsCollectorWithBinningAnalysis<
    Tomographer::MHRWTasks::ValueHistogramTasks::CDataBase<IdentValueCalculator, true>,
    BoostTestLogger> ResultsCollectorBinning;

  BOOST_CHECK( ResultsCollectorBinning::HistogramType::HasErrorBars ) ;
  BOOST_CHECK( ! ResultsCollectorBinning::SimpleNormalizedHistogramType::HasErrorBars ) ;
  BOOST_CHECK( ResultsCollectorBinning::SimpleFinalHistogramType::HasErrorBars ) ;
  BOOST_CHECK( ResultsCollectorBinning::FinalHistogramType::HasErrorBars ) ;
}

BOOST_AUTO_TEST_CASE(collects_histograms)
{
  typedef Tomographer::MHRWTasks::ValueHistogramTasks::CDataBase<IdentValueCalculator, true>  CDataType;
  typedef Tomographer::MHRWTasks::ValueHistogramTasks::ResultsCollectorWithBinningAnalysis<
    CDataType, BoostTestLogger
    > ResultsCollectorBinning;

  BoostTestLogger logger;

  IdentValueCalculator valcalc;
  CDataType::HistogramParams hp(0, 10, 10);
  CDataType::MHRWParamsType mhrwparams(0.1, 512, 256, 1024);
  CDataType cdata(valcalc, hp, 7 /* num binning levels */, mhrwparams);
  
  ResultsCollectorBinning res(logger);

  BOOST_CHECK( ! res.isFinalized() );
  {
    EigenAssertTest::setting_scope settingvariable(true);
    // these should generate assertion failures.
    BOOST_CHECK_THROW( res.finalHistogram(), Tomographer::Tools::EigenAssertException) ;
    BOOST_CHECK_THROW( res.simpleFinalHistogram(), Tomographer::Tools::EigenAssertException) ;
    BOOST_CHECK_THROW( res.numTasks(), Tomographer::Tools::EigenAssertException) ;
    BOOST_CHECK_THROW( res.collectedRunTaskResults(), Tomographer::Tools::EigenAssertException ) ;
    BOOST_CHECK_THROW( res.collectedRunTaskResult(0), Tomographer::Tools::EigenAssertException ) ;
  };

  res.init(4, 1, &cdata) ;

  BOOST_CHECK( ! res.isFinalized() );
  {
    EigenAssertTest::setting_scope settingvariable(true);
    // these should generate assertion failures.
    BOOST_CHECK_THROW( res.finalHistogram(), Tomographer::Tools::EigenAssertException) ;
    BOOST_CHECK_THROW( res.simpleFinalHistogram(), Tomographer::Tools::EigenAssertException) ;
    BOOST_CHECK_THROW( res.numTasks(), Tomographer::Tools::EigenAssertException) ;
    BOOST_CHECK_THROW( res.collectedRunTaskResults(), Tomographer::Tools::EigenAssertException ) ;
    BOOST_CHECK_THROW( res.collectedRunTaskResult(0), Tomographer::Tools::EigenAssertException ) ;
  };

  typedef ResultsCollectorBinning::MHRWStatsCollectorResultType VhResType;
  typedef Tomographer::MHRWTasks::MHRandomWalkTaskResult<VhResType, int, double>  TaskResultType;

  auto ALL_CONV = Eigen::ArrayXi::Constant(10, ResultsCollectorBinning::BinningAnalysisParamsType::CONVERGED);

  CDataType::HistogramType h0(hp);
  h0.bins  << 0 ,  4 , 24 , 153 , 382 , 260 , 152 , 36 , 13 , 0 ;  h0.bins /= 1024.0;
  h0.delta << 0., 0.1, 0.8,  3.4,  8.2,  4.1,  3.2, 2.0, 1.3, 0 ;  h0.delta /= 1024.0;
  VhResType rr0; rr0.converged_status = ALL_CONV; rr0.hist = h0;
  TaskResultType r0(rr0, mhrwparams, 0.35);

  CDataType::HistogramType h1(hp);
  h1.bins  << 0 , 10 , 26 , 147 , 380 , 258 , 154 , 31 , 17 , 1 ;  h1.bins /= 1024.0;
  h1.delta << 0., 0.2, 1.0,  3.0,  8.8,  3.2,  3.7, 4.0, 2.3, 0 ;  h1.delta /= 1024.0;
  VhResType rr1; rr1.converged_status = ALL_CONV; rr1.hist = h1;
  TaskResultType r1(rr1, mhrwparams, 0.41);

  CDataType::HistogramType h2(hp);
  h2.bins  << 0 ,  8 , 20 , 151 , 384 , 262 , 150 , 35 , 14 , 0 ;  h2.bins /= 1024.0;
  h2.delta << 0., 0.1, 1.2,  2.7, 10.1,  3.2,  3.3, 2.3, 1.8, 0 ;  h2.delta /= 1024.0;
  VhResType rr2; rr2.converged_status = ALL_CONV; rr2.hist = h2;
  TaskResultType r2(rr2, mhrwparams, 0.33);

  CDataType::HistogramType h3(hp);
  h3.bins  << 0 ,  5 , 23 , 155 , 372 , 258 , 168 , 30 , 12 , 1 ;  h3.bins /= 1024.0;
  h3.delta << 0., 0.6, 1.4,  3.0,  8.0,  3.9,  2.9, 3.0, 1.8, 1 ;  h3.delta /= 1024.0;
  VhResType rr3; rr3.converged_status = ALL_CONV; rr3.hist = h3;
  TaskResultType r3(rr3, mhrwparams, 0.32);

  res.collectResult(2, r2, &cdata);
  res.collectResult(1, r1, &cdata);
  res.collectResult(0, r0, &cdata);
  res.collectResult(3, r3, &cdata);

  BOOST_CHECK( ! res.isFinalized() );
  {
    EigenAssertTest::setting_scope settingvariable(true);
    // these should generate assertion failures.
    BOOST_CHECK_THROW( res.finalHistogram(), Tomographer::Tools::EigenAssertException) ;
    BOOST_CHECK_THROW( res.simpleFinalHistogram(), Tomographer::Tools::EigenAssertException) ;
    BOOST_CHECK_THROW( res.numTasks(), Tomographer::Tools::EigenAssertException) ;
    BOOST_CHECK_THROW( res.collectedRunTaskResults(), Tomographer::Tools::EigenAssertException ) ;
    BOOST_CHECK_THROW( res.collectedRunTaskResult(0), Tomographer::Tools::EigenAssertException ) ;
  };

  res.runsFinished(4, &cdata);
  
  BOOST_CHECK( res.isFinalized() );
  BOOST_CHECK_EQUAL( res.numTasks(), 4 );
  auto fhist = res.finalHistogram();
  BOOST_CHECK_EQUAL( fhist.numBins(), 10 );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.count(0) , 0.0 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.count(1) , 0.006591796875 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.count(2) , 0.022705078125 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.count(3) , 0.14794921875 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.count(4) , 0.37060546875 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.count(5) , 0.25341796875 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.count(6) , 0.15234375 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.count(7) , 0.0322265625 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.count(8) , 0.013671875 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.count(9) , 0.00048828125 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.errorBar(0) , 0.0 , 1e-6 );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.errorBar(1) , 0.00015822120845722314 , 1e-6 );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.errorBar(2) , 0.00054809434376571408 , 1e-6 );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.errorBar(3) , 0.0014820361533961122 , 1e-6 );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.errorBar(4) , 0.0043033204720617909 , 1e-6 );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.errorBar(5) , 0.0017689668879625781 , 1e-6 );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.errorBar(6) , 0.0016052130056911876 , 1e-6 );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.errorBar(7) , 0.00142963046601146 , 1e-6 );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.errorBar(8) , 0.00089570001600801472 , 1e-6 );
  MY_BOOST_CHECK_FLOATS_EQUAL( fhist.errorBar(9) , 0.000244140625 , 1e-6 );

  // these values are the same as the test case without binning analysis
  auto sfhist = res.simpleFinalHistogram();
  BOOST_CHECK_EQUAL( sfhist.numBins(), 10 );
  MY_BOOST_CHECK_FLOATS_EQUAL( sfhist.count(0) , 0.0 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( sfhist.count(1) , 0.006591796875 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( sfhist.count(2) , 0.022705078125 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( sfhist.count(3) , 0.14794921875 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( sfhist.count(4) , 0.37060546875 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( sfhist.count(5) , 0.25341796875 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( sfhist.count(6) , 0.15234375 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( sfhist.count(7) , 0.0322265625 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( sfhist.count(8) , 0.013671875 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( sfhist.count(9) , 0.00048828125 , tol );
  MY_BOOST_CHECK_FLOATS_EQUAL( sfhist.errorBar(0) , 0.0 , 1e-6 );
  MY_BOOST_CHECK_FLOATS_EQUAL( sfhist.errorBar(1) , 0.00134462171565 , 1e-6 );
  MY_BOOST_CHECK_FLOATS_EQUAL( sfhist.errorBar(2) , 0.001220703125 , 1e-6 );
  MY_BOOST_CHECK_FLOATS_EQUAL( sfhist.errorBar(3) , 0.00166779797623 , 1e-6 );
  MY_BOOST_CHECK_FLOATS_EQUAL( sfhist.errorBar(4) , 0.00256831605437 , 1e-6 );
  MY_BOOST_CHECK_FLOATS_EQUAL( sfhist.errorBar(5) , 0.000934987409918 , 1e-6 );
  MY_BOOST_CHECK_FLOATS_EQUAL( sfhist.errorBar(6) , 0.00398679971156 , 1e-6 );
  MY_BOOST_CHECK_FLOATS_EQUAL( sfhist.errorBar(7) , 0.0014374610785 , 1e-6 );
  MY_BOOST_CHECK_FLOATS_EQUAL( sfhist.errorBar(8) , 0.00105480805638 , 1e-6 );
  MY_BOOST_CHECK_FLOATS_EQUAL( sfhist.errorBar(9) , 0.000281909311128 , 1e-6 );

}

BOOST_AUTO_TEST_SUITE_END()

// -----------------------------------------------

BOOST_AUTO_TEST_SUITE(cdata_base)

BOOST_AUTO_TEST_CASE(types_simple)
{
  typedef Tomographer::MHRWTasks::ValueHistogramTasks::CDataBase<IdentValueCalculator, false>  CDataType;

  BOOST_CHECK( ! CDataType::UseBinningAnalysis );
  BOOST_CHECK_EQUAL(std::string(typeid(CDataType::MHRWStatsCollectorResultType).name()),
                    std::string(typeid(CDataType::HistogramType).name()));
}

BOOST_AUTO_TEST_CASE(types_binning)
{
  typedef Tomographer::MHRWTasks::ValueHistogramTasks::CDataBase<IdentValueCalculator, true>  CDataType;

  BOOST_CHECK( CDataType::UseBinningAnalysis );
  BOOST_CHECK_EQUAL(std::string(typeid(CDataType::MHRWStatsCollectorResultType).name()),
                    std::string(typeid(Tomographer::ValueHistogramWithBinningMHRWStatsCollectorParams<
                                       IdentValueCalculator
                                       >::Result).name()));
}

BOOST_AUTO_TEST_CASE(constr_simple)
{
  typedef Tomographer::MHRWTasks::ValueHistogramTasks::CDataBase<IdentValueCalculator, false>  CDataType;

  IdentValueCalculator valcalc;
  CDataType::HistogramParams hp(0, 10, 10);
  CDataType::MHRWParamsType mhrwparams(0.1, 512, 256, 1024);

  CDataType cdata(valcalc, hp, mhrwparams);
}

BOOST_AUTO_TEST_CASE(constr_binning)
{
  typedef Tomographer::MHRWTasks::ValueHistogramTasks::CDataBase<IdentValueCalculator, true>  CDataType;

  IdentValueCalculator valcalc;
  CDataType::HistogramParams hp(0, 10, 10);
  CDataType::MHRWParamsType mhrwparams(0.1, 512, 256, 1024);

  // need to specify also binning level
  CDataType cdata(valcalc, hp, 7, mhrwparams);
}

BOOST_AUTO_TEST_CASE(createstatscoll_simple)
{
  typedef Tomographer::MHRWTasks::ValueHistogramTasks::CDataBase<IdentValueCalculator, false>  CDataType;

  IdentValueCalculator valcalc;
  CDataType::HistogramParams hp(0, 10, 10);
  CDataType::MHRWParamsType mhrwparams(0.1, 512, 256, 1024);

  CDataType cdata(valcalc, hp, mhrwparams);

  BoostTestLogger logger;
  auto stcoll = cdata.createStatsCollector(logger);

  BOOST_CHECK_EQUAL(std::string(typeid(decltype(stcoll)).name()),
                    std::string(typeid(Tomographer::ValueHistogramMHRWStatsCollector<
                                       IdentValueCalculator,BoostTestLogger
                                       >).name()));
}

BOOST_AUTO_TEST_CASE(createstatscoll_binning)
{
  typedef Tomographer::MHRWTasks::ValueHistogramTasks::CDataBase<IdentValueCalculator, true>  CDataType;

  IdentValueCalculator valcalc;
  CDataType::HistogramParams hp(0, 10, 10);
  CDataType::MHRWParamsType mhrwparams(0.1, 512, 256, 1024);

  // need to specify also binning level
  CDataType cdata(valcalc, hp, 7, mhrwparams);

  BoostTestLogger logger;
  auto stcoll = cdata.createStatsCollector(logger);

  BOOST_CHECK_EQUAL(std::string(typeid(decltype(stcoll)).name()),
                    std::string(typeid(Tomographer::ValueHistogramWithBinningMHRWStatsCollector<
                                       Tomographer::MHRWTasks::ValueHistogramTasks::tomo_internal
                                       ::histogram_types<CDataType, true>
                                       ::BinningMHRWStatsCollectorParams,
                                       BoostTestLogger>).name()));
}

BOOST_AUTO_TEST_SUITE_END()

// =============================================================================
BOOST_AUTO_TEST_SUITE_END()

