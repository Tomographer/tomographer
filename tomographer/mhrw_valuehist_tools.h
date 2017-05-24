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

#ifndef TOMOGRAPHER_MHRW_VALUEHIST_TOOLS_H
#define TOMOGRAPHER_MHRW_VALUEHIST_TOOLS_H


#include <iostream>
#include <iomanip>

#include <tomographer/tools/loggers.h>
#include <tomographer/tools/cxxutil.h> // tomographer_assert()
#include <tomographer/tools/needownoperatornew.h>
#include <tomographer/mhrwstatscollectors.h>
#include <tomographer/mhrwtasks.h>

#include <boost/math/constants/constants.hpp>


/** \file mhrw_valuehist_tools.h
 *
 * \brief Tools for collecting a histogram of values during a Metropolis-Hastings random
 *        walk using the tools of \ref Tomographer::MHRWTasks
 *
 * See \ref Tomographer::MHRWTasks::ValueHistogramTools .
 *
 * \since This file was added in %Tomographer 5.0.
 */


namespace Tomographer {
namespace MHRWTasks {
namespace ValueHistogramTools {




/** \brief Stores the result of the value histogram stats collector (version without
 *         binning analysis)
 *
 * You shouldn't have to use this class directly.  Use \ref
 * CDataBase::MHRWStatsResultsBaseType instead.
 */
template<typename RawHistogramType_, typename ScaledHistogramType_>
struct TOMOGRAPHER_EXPORT MHRWStatsResultsBaseSimple
{
  typedef RawHistogramType_ RawHistogramType;
  typedef ScaledHistogramType_ ScaledHistogramType;

  MHRWStatsResultsBaseSimple(RawHistogramType && val)
    : raw_histogram(std::move(val)),
      histogram(raw_histogram.params)
  {
    typedef typename ScaledHistogramType::CountType  CountRealType;
    CountRealType ncounts = raw_histogram.totalCounts();
    histogram.load(raw_histogram.bins.template cast<CountRealType>() / ncounts,
                   raw_histogram.off_chart / ncounts);
  }

  RawHistogramType raw_histogram;

  ScaledHistogramType histogram;
};






// -----------------------------------------------


namespace tomo_internal {
//
// version WITHOUT binning analysis:
//
template<typename CDataBaseType, bool UseBinningAnalysis>
struct valuehist_types
{
  typedef Histogram<typename CDataBaseType::ValueCalculator::ValueType,
                    typename CDataBaseType::HistCountIntType> HistogramType;

  // useful types

  // we know that ValueHistogramMHRWStatsCollector<ValueCalculator,...,HistogramType>::ResultType is HistogramType
  typedef HistogramType ValueStatsCollectorResultType;
  typedef typename HistogramType::Params HistogramParams;
  typedef typename CDataBaseType::CountRealType CountRealType;
  typedef Histogram<typename HistogramType::Scalar, CountRealType> ScaledHistogramType;

  // base type for user cdata to use as MHRWStatsResults member
  typedef MHRWStatsResultsBaseSimple<ValueStatsCollectorResultType, ScaledHistogramType>
    MHRWStatsResultsBaseType;

  // the correct histogram aggregator type
  typedef AggregatedHistogramSimple<ScaledHistogramType, CountRealType> AggregatedHistogramType;
};
//
// version WITH binning analysis:
//
template<typename CDataBaseType>
struct valuehist_types<CDataBaseType, true>
{
  // useful types

  typedef typename CDataBaseType::CountRealType CountRealType;

  /// the \ref ValueHistogramWithBinningMHRWStatsCollectorParams...
  typedef ValueHistogramWithBinningMHRWStatsCollectorParams<
    typename CDataBaseType::ValueCalculator,
    typename CDataBaseType::HistCountIntType,
    CountRealType,
    Eigen::Dynamic,
    Eigen::Dynamic
    >  BinningMHRWStatsCollectorParams;

  typedef typename BinningMHRWStatsCollectorParams::Result ValueStatsCollectorResultType;
  typedef typename BinningMHRWStatsCollectorParams::HistogramType HistogramType;
  typedef typename BinningMHRWStatsCollectorParams::HistogramParams HistogramParams;

  // base type for user cdata to use as MHRWStatsResults member
  typedef ValueStatsCollectorResultType MHRWStatsResultsBaseType;

  // the correct histogram aggregator type
  typedef AggregatedHistogramWithErrorBars<HistogramType, CountRealType> AggregatedHistogramType;
};
} // namespace tomo_internal


// ------------------------------------------------



/** \brief Constant data structure for MH random walk tasks with a value histogram stats
 *         collector
 *
 * You should define your \a TaskCData class to inherit this class, and be compliant with
 * the \ref pageInterfaceMHRandomWalkTaskCData.  This class already inherits \ref
 * MHRWTasks::CDataBase, so you only have to inherit this class and provide a \a
 * createMHWalker() method (see \ref pageInterfaceMHRandomWalkTaskCData).  (Note that your
 * class will automatically be compliant with the \ref pageInterfaceTaskCData in this
 * way.)
 *
 * Look at the file <code>test/minimal_tomorun.cxx</code> for an example of you to define
 * your cdata class.
 *
 * We provide some useful typedefs, as well as a \ref createValueStatsCollector() which
 * should be used by subclasses in the \a setupRandomWalkAndRun() method of their \ref
 * pageInterfaceMHRandomWalkTaskCData implementation.
 *
 * \since Changed in %Tomographer 5.0: Removed \a StepRealType template parameter, added
 *        \a MHWalkerParams, beware the new order!
 *
 * \tparam ValueCalculator the value calculator type you wish to use; defining which value
 *         you are interested in collecting a histogram of during the random walk;
 *
 * \tparam UseBinningAnalysis whether or not to use a binning analysis to obtain reliable
 *         error bars during the random walk.
 *
 * \tparam MHWalkerParams The MHWalkerParams required for our \ref pageInterfaceMHWalker
 *         "MHWalker" (for instance a \ref MHWalkerParamsStepSize)
 *
 * \tparam RngSeedType The type used to store the seed of the pseudo random number
 *         generator. See \ref MHRWTasks::CDataBase.
 *
 * \tparam IterCountIntType the integer type to use for counting iterations.
 *
 * \tparam CountRealType the real type to use when calculating the scaled histogram with
 *         error bars.
 *
 * \tparam HistCountIntType the integer type to record the histogram counts in the
 *         underlying raw histogram.
 *
 * Note: if your subclass also takes a template parameter to allow or not the use of a
 * binning analysis, then you'll have to define two conditionally enabled constructors to
 * cover both cases.  For instance (see also the code in
 * <code>tomorun/tomorun_dispatch.h</code> for another example):
 * \code
 * struct MyCData
 *   : public Tomographer::MHRWTasks::ValueHistogramTools::CDataBase< ... >
 * {
 *   typedef Tomographer::MHRWTasks::ValueHistogramTools::CDataBase< ... > Base;
 *
 *   TOMOGRAPHER_ENABLED_IF(!BinningAnalysisEnabled)
 *   MyCData(ValueCalculator valcalc, RngSeedType base_seed, ...)
 *     : Base(valcalc,
 *            typename Base::HistogramParams(...),
 *            typename Base::MHRWParamsType(...),
 *            base_seed)
 *   { ... }
 *
 *   TOMOGRAPHER_ENABLED_IF(BinningAnalysisEnabled)
 *   MyCData(ValueCalculator valcalc, RngSeedType base_seed, int binning_analysis_num_levels, ...)
 *     : Base(valcalc,
 *            typename Base::HistogramParams(...),
 *            binning_analysis_num_levels,
 *            typename Base::MHRWParamsType(...),
 *            base_seed)
 *   { ... }
 *   ...
 * };
 * \endcode
 */
template<typename ValueCalculator_,
         bool UseBinningAnalysis_ = true,
         typename MHWalkerParams_ = MHWalkerParamsStepSize<double>,
         typename RngSeedType_ = std::mt19937::result_type,
         typename IterCountIntType_ = int,
	 typename CountRealType_ = double,
         typename HistCountIntType_ = IterCountIntType_>
struct TOMOGRAPHER_EXPORT CDataBase
  : public MHRWTasks::CDataBase<MHWalkerParams_, IterCountIntType_, RngSeedType_>,
    public virtual Tools::NeedOwnOperatorNew<ValueCalculator_>::ProviderType
{
  //! The MHRWTasks::CDataBase base class
  typedef MHRWTasks::CDataBase<MHWalkerParams_, IterCountIntType_, RngSeedType_> Base;

  //! The MHWalkerParams required for our MHWalker (for instance a \ref MHWalkerParamsStepSize)
  typedef typename Base::MHWalkerParams MHWalkerParams;

  //! Type of the seed for the pseudo-random number generator
  typedef typename Base::RngSeedType RngSeedType;

  //! The integer type which serves to count the number of iterations (see \ref MHRWParams)
  typedef typename Base::IterCountIntType IterCountIntType;

  //! The integer counting type in our underlying raw histogram type
  typedef HistCountIntType_ HistCountIntType;

  /** \brief The class which calculates the value we are collecting a histogram of (\ref
   *         pageInterfaceValueCalculator compliant)
   */
  typedef ValueCalculator_ ValueCalculator;
  //! The real type which serves to average histogram counts (typically \c double)
  typedef CountRealType_ CountRealType;

  //! Whether or not we are to use a binning analysis for calculating the error bars
  static constexpr bool UseBinningAnalysis = UseBinningAnalysis_;

  /** \brief The result type of our stats collector
   *
   * This is either:
   *
   *  - (without binning analysis): a simple histogram type, without error bars
   *
   *  - (with binning analysis): a structure containing a histogram with error bars, along
   *    with a convergence analysis of the errors.  It is a \ref
   *    ValueHistogramWithBinningMHRWStatsCollectorResult type.
   */
  typedef typename tomo_internal::valuehist_types<CDataBase,UseBinningAnalysis>::ValueStatsCollectorResultType
    ValueStatsCollectorResultType;

  /** \brief Stores result of the stats collector. May serve as base class for your own
   *         MHRWStatsResults class.
   *
   * Depending on \a UseBinningAnalysis, this is either \ref MHRWStatsResultsBaseSimple, or
   * directly the corresponding \ref ValueHistogramWithBinningMHRWStatsCollectorResult
   * type.
   *
   * Remember, this class takes a single argument in the constructor which is a
   * rvalue-reference to the result of the value stats collector.
   */
  typedef typename tomo_internal::valuehist_types<CDataBase,UseBinningAnalysis>::MHRWStatsResultsBaseType
    MHRWStatsResultsBaseType;

  /** \brief The histogram type reported by the task
   *
   * Depending on whether we are using a binning analysis, this histogram type will have
   * error bars.
   *
   * It is either:
   *
   *   - (no binning analysis): a Histogram of appropriate parameters type
   *
   *   - (with binning analysis): a HistogramWithErrorBars of appropriate
   *     parameters types.  This is in fact \ref
   *     ValueHistogramWithBinningMHRWStatsCollectorParams::HistogramType.
   */
  typedef typename tomo_internal::valuehist_types<CDataBase,UseBinningAnalysis>::HistogramType
    HistogramType;

  /** \brief The appropriate parameters type for the histogram reported by the task
   */
  typedef typename tomo_internal::valuehist_types<CDataBase,UseBinningAnalysis>::HistogramParams
    HistogramParams;

  //! Type for the parameters of the random walk.
  typedef MHRWParams<MHWalkerParams, IterCountIntType> MHRWParamsType;
 

  //! Constructor (use only without binning analysis)
  TOMOGRAPHER_ENABLED_IF(!UseBinningAnalysis)
  CDataBase(const ValueCalculator & valcalc_, HistogramParams histogram_params_,
	    MHRWParamsType p, RngSeedType base_seed = 0)
    : Base(std::move(p), base_seed), valcalc(valcalc_), histogram_params(histogram_params_),
      binningNumLevels()
  {
  }

  //! Constructor (use only with binning analysis)
  TOMOGRAPHER_ENABLED_IF(UseBinningAnalysis)
  CDataBase(const ValueCalculator & valcalc_, HistogramParams histogram_params_, int binning_num_levels_,
	    MHRWParamsType p, RngSeedType base_seed = 0)
    : Base(std::move(p), base_seed), valcalc(valcalc_), histogram_params(histogram_params_),
      binningNumLevels(binning_num_levels_)
  {
  }

  //! The value calculator instance
  const ValueCalculator valcalc;
  //! The parameters of the histogram that we are collecting
  const HistogramParams histogram_params;
  //! The number of levels in the binning analysis (only if we are using a binning analysis)
  const Tools::StoreIfEnabled<int, UseBinningAnalysis> binningNumLevels;


  /** \brief Create the stats collector (without binning analysis)
   *
   * This method is provided so that user-provided \ref pageInterfaceMHRandomWalkTaskCData
   * "CData random walk description classes" can easily create the necessary value
   * histogram stats collector for collecting a histogram of values during a \ref
   * MHRandomWalkTask.  This function should typically be called from within the user's \a
   * setupRandomWalkAndRun() function.
   */
  template<typename LoggerType, TOMOGRAPHER_ENABLED_IF_TMPL(!UseBinningAnalysis)>
  inline ValueHistogramMHRWStatsCollector<ValueCalculator,LoggerType,HistogramType>
  createValueStatsCollector(LoggerType & logger) const
  {
    return ValueHistogramMHRWStatsCollector<ValueCalculator,LoggerType,HistogramType>(
	histogram_params,
	valcalc,
	logger
	);
  }

  /** \brief Create the stats collector (with binning analysis)
   *
   * This method is provided so that user-provided \ref pageInterfaceMHRandomWalkTaskCData
   * "CData random walk description classes" can easily create the necessary value
   * histogram stats collector for collecting a histogram of values during a \ref
   * MHRandomWalkTask.  This function should typically be called from within the user's \a
   * setupRandomWalkAndRun() function.
   */
  template<typename LoggerType, TOMOGRAPHER_ENABLED_IF_TMPL(UseBinningAnalysis)>
  inline ValueHistogramWithBinningMHRWStatsCollector<
    typename tomo_internal::valuehist_types<CDataBase, true>::BinningMHRWStatsCollectorParams,
    LoggerType>
  createValueStatsCollector(LoggerType & logger) const
  {
    typedef typename tomo_internal::valuehist_types<CDataBase, true>::BinningMHRWStatsCollectorParams
      BinningMHRWStatsCollectorParams;

    return ValueHistogramWithBinningMHRWStatsCollector<BinningMHRWStatsCollectorParams,LoggerType>(
	histogram_params,
	valcalc,
        binningNumLevels.value,
	logger
	);
  }


  typedef typename tomo_internal::valuehist_types<CDataBase, UseBinningAnalysis>::AggregatedHistogramType
    AggregatedHistogramType;


  /** \brief Convenience function for aggregating histograms resulting from value-histogram tasks
   *
   * The \a TaskResultType is expected to be a subclass of \ref MHRWStatsResultsBaseSimple.
   * This will automatically be the case if you're using our \ref CDataBase type, and if
   * you have defined your custom \a MHRWStatsResults type (see \ref
   * pageInterfaceMHRandomWalkTaskCData) using \a CDataBase::MHRWStatsResultsBaseType as base
   * class.
   */
  template<typename TaskResultType>
  AggregatedHistogramType aggregateResultHistograms(const std::vector<TaskResultType*> & task_result_list)
  {
    return AggregatedHistogramType::aggregate(
        histogram_params,
        task_result_list,
        [](const TaskResultType * task_result)
        -> const typename AggregatedHistogramType::HistogramType &
        {
          // .histogram is already the "scaled histogram".  This works both
          // for the "simple" version as well as for the
          // "binning-analysis-error-bar" version.
          return task_result->stats_results.histogram;
        });
  }


};
// define static members:
template<typename ValueCalculator_, bool UseBinningAnalysis_,
         typename MHWalkerParams_, typename RngSeedType_,
         typename IterCountIntType_, typename CountRealType_,
         typename HistCountIntType_>
constexpr bool CDataBase<ValueCalculator_,UseBinningAnalysis_,MHWalkerParams_,RngSeedType_,
                         IterCountIntType_,CountRealType_,HistCountIntType_>::UseBinningAnalysis;






// ---------------------------------------------------------












namespace tomo_internal {

template<typename StatsResultsType, typename = void>
struct maybe_show_error_summary_helper {
  static inline void print(std::ostream & , const StatsResultsType & ) { }
};
template<typename StatsResultsType>
struct maybe_show_error_summary_helper<
  StatsResultsType,
  typename std::enable_if<
    !std::is_same<
      decltype(((StatsResultsType*)NULL)->errorBarConvergenceSummary()),
      void
    >::value
  >::type
  >
{
  static inline void print(std::ostream & stream, const StatsResultsType & stats_results)
  {
    stream << "    error bars: " << stats_results.errorBarConvergenceSummary() << "\n";
  }
};
template<typename StatsResultsType>
inline void maybe_show_error_summary(std::ostream & stream, const StatsResultsType & stats_results)
{
  maybe_show_error_summary_helper<StatsResultsType>::print(stream, stats_results) ;
}


template<typename TaskResultType>
inline void print_hist_short_bar_summary(std::ostream & stream, int dig_w, std::size_t j,
                                         const TaskResultType * task_result, int columns)
{
  const auto acceptance_ratio = task_result->acceptance_ratio;
  histogramShortBarWithInfo(stream,
                            streamstr("#" << std::setw(dig_w) << j << ": "),
                            task_result->stats_results.histogram,
                            Tomographer::Tools::fmts(" [accept ratio = %.2f]", acceptance_ratio),
                            false, columns);
  if (acceptance_ratio > MHRWAcceptanceRatioRecommendedMax ||
      acceptance_ratio < MHRWAcceptanceRatioRecommendedMin) {
    stream << "    *** Accept ratio out of recommended bounds ["
           << MHRWAcceptanceRatioRecommendedMin << ", " << MHRWAcceptanceRatioRecommendedMax
           << "] ! Adapt step size ***\n";
  }
  maybe_show_error_summary(stream, task_result->stats_results);
}

} // namespace tomo_internal







/** \brief Produce a final, human-readable report of the whole procedure
 *
 * The report is written in the C++ stream \a str.  You should provide the shared
 * constant data structure \a cdata used for the random walk, so that the random walk
 * parameters can be displayed.
 *
 * You may specify the maximum width of your terminal in \a max_width, in which case we
 * try very hard not make lines longer than that, and to fill all available horizontal
 * space.
 *
 * If \a print_histogram is \c true, then the histogram is also printed in a human
 * readable form, using \ref HistogramWithErrorBars::prettyPrint().
 */
template<typename CDataBaseType, typename TaskResultType, typename AggregatedHistogramType>
inline void printFinalReport(std::ostream & stream, const CDataBaseType & cdata,
                             const std::vector<TaskResultType*> & task_results,
                             const AggregatedHistogramType & aggregated_histogram,
                             int max_width = 0, bool print_histogram = true)
{
  Tools::ConsoleFormatterHelper h(max_width); // possibly detect terminal width etc.

  // produce report on runs
  stream << "\n"
         << h.centerLine("Final Report of Runs")
         << h.hrule()
    ;
  cdata.printBasicCDataMHRWInfo(stream);

  int dig_w = (int)std::ceil(std::log10((double)task_results.size()));
  for (std::size_t j = 0; j < task_results.size(); ++j) {
    tomo_internal::print_hist_short_bar_summary(stream, dig_w, j, task_results[j], (int)h.columns());
  }
  stream << h.hrule()
         << "\n";

  if (print_histogram) {
    // and the final histogram
    stream << h.centerLine("Final Histogram")
           << h.hrule();
    histogramPrettyPrint(stream, aggregated_histogram.final_histogram, (int)h.columns());
    stream << h.hrule()
           << "\n";
  }
}










} // namespace ValueHistogramTools
} // namespace MHRWTasks
} // namespace Tomographer


#endif
