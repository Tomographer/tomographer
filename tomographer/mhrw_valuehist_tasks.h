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

#ifndef TOMOGRAPHER_MHRW_VALUEHIST_TASKS_H
#define TOMOGRAPHER_MHRW_VALUEHIST_TASKS_H

#include <iostream>
#include <iomanip>

#include <tomographer/tools/loggers.h>
#include <tomographer/tools/cxxutil.h> // tomographer_assert()
#include <tomographer/tools/needownoperatornew.h>
#include <tomographer/mhrwstatscollectors.h>
#include <tomographer/mhrwtasks.h>

#include <boost/math/constants/constants.hpp>


/** \file mhrw_valuehist_tasks.h
 *
 * \brief Tasks for collecting a histogram of values during a Metropolis-Hastings random walk
 *
 * See \ref Tomographer::MHRWTasks::ValueHistogramTasks .
 */


namespace Tomographer {
namespace MHRWTasks {
namespace ValueHistogramTasks {


namespace tomo_internal {
template<typename CDataBaseType, bool UseBinningAnalysis>
struct histogram_types {// version WITHOUT binning analysis:
  typedef UniformBinsHistogram<typename CDataBaseType::ValueCalculator::ValueType> HistogramType;
  /// we know that ValueHistogramMHRWStatsCollector<ValueCalculator,...,HistogramType>::ResultType is HistogramType
  typedef HistogramType MHRWStatsCollectorResultType;
  typedef typename HistogramType::Params HistogramParams;
};
template<typename CDataBaseType>
struct histogram_types<CDataBaseType, true> {// version WITH binning analysis:
  /// the \ref ValueHistogramWithBinningMHRWStatsCollectorParams...
  typedef ValueHistogramWithBinningMHRWStatsCollectorParams<
    typename CDataBaseType::ValueCalculator, typename CDataBaseType::CountIntType,
    typename CDataBaseType::CountRealType, Eigen::Dynamic, Eigen::Dynamic
    >  BinningMHRWStatsCollectorParams;
  //
  typedef typename BinningMHRWStatsCollectorParams::Result MHRWStatsCollectorResultType;
  typedef typename BinningMHRWStatsCollectorParams::HistogramType HistogramType;
  typedef typename BinningMHRWStatsCollectorParams::HistogramParams HistogramParams;
};
} // namespace tomo_internal




// ------------------------------------------------




namespace tomo_internal {
template<typename HistogramType>
void print_hist_short_bar_with_accept_info(std::ostream & str, int dig_w, int j, const HistogramType & hist,
                                           double acceptance_ratio, int columns)
{
  histogramShortBarWithInfo(str,
                            streamstr("#" << std::setw(dig_w) << j << ": "),
                            hist,
                            Tomographer::Tools::fmts(" [accept ratio = %.2f]", acceptance_ratio),
                            false, columns);
  if (acceptance_ratio > 0.35 || acceptance_ratio < 0.2) {
    str << "    *** Accept ratio out of recommended bounds [0.20, 0.35] ! Adapt step size ***\n";
  }
}
} // namespace tomo_internal








/** \brief Results collector, if no binning analysis is being used.
 *
 * This class takes care of collecting the results of the executed random walk
 * tasks, and merges them together to provide a final, averaged histogram with
 * error bars.
 *
 * This class complies with the \ref pageInterfaceResultsCollector.
 *
 * This results collector should only be used if each task provides a "raw"
 * histogram, ie. without any error bars.  If you have tasks which already
 * provide some error bars from a binning analysis, then you should be using the
 * class \ref ResultsCollectorWithBinningAnalysis.
 *
 * Note that you can directly get the right ResultsCollector type by querying
 * the type \a "CDataBase::ResultsCollectorType<..>::Type".
 *
 * \todo DOC !!!!
 */
template<typename CDataBaseType_, typename LoggerType_>
struct ResultsCollectorSimple
  : public virtual Tools::NeedOwnOperatorNew<
      AveragedHistogram<UniformBinsHistogram<typename CDataBaseType_::HistogramType::Scalar,
                                             typename CDataBaseType_::CountRealType>,
                        typename CDataBaseType_::CountRealType>
    >::ProviderType
{
  //! The structure which hold the constant shared data describing what we have to do
  typedef CDataBaseType_ CDataBaseType;
  //! The class which calculates the value we are collecting a histogram of (\ref pageInterfaceValueCalculator compliant)
  typedef typename CDataBaseType::ValueCalculator ValueCalculator;
  //! The real type which serves to average histogram counts (typically \c double)
  typedef typename CDataBaseType::CountRealType CountRealType;
  //! The integer type which serves as histogram counts (typically \c int)
  typedef typename CDataBaseType::CountIntType CountIntType;
  //! The real type which serves to describe the step size of the random walk (typically \c double)
  typedef typename CDataBaseType::StepRealType StepRealType;
  //! The logger type we can send messages to
  typedef LoggerType_ LoggerType;
  
  /** \brief The histogram type corresponding to the result of a task
   *
   * NOTE: if this histogram actually has error bars, they will be silently ignored.
   */
  typedef typename CDataBaseType::HistogramType HistogramType;
  //! The parameters type used to describe our histogram range and number of bins
  typedef typename CDataBaseType::HistogramParams HistogramParams;
  //! The type of the histogram resulting from a single task, but normalized to unit area under the curve
  typedef UniformBinsHistogram<typename HistogramType::Scalar, CountRealType> NormalizedHistogramType;
  //! The type of the final resulting, averaged and normalized histogram
  typedef AveragedHistogram<NormalizedHistogramType, CountRealType> FinalHistogramType;
  
  typedef HistogramType MHRWStatsCollectorResultType;
  
  TOMO_STATIC_ASSERT_EXPR( CDataBaseType::UseBinningAnalysis == false ) ;
  
  /** \brief Stores information about the result of a task run.
   *
   * This structure is not the type that the task itself returns; rather, it is a way for
   * us to store all the relevant information together in the result collector object (see
   * \ref ResultsCollectorSimple::RunTaskResultList and \ref
   * ResultsCollectorSimple::collectedRunTaskResults())
   *
   */
  struct RunTaskResult
    : public MHRandomWalkTaskResult<MHRWStatsCollectorResultType,CountIntType,StepRealType>,
      public virtual Tools::NeedOwnOperatorNew<NormalizedHistogramType>::ProviderType
  {
    typedef MHRandomWalkTaskResult<MHRWStatsCollectorResultType,CountIntType,StepRealType> Base;

    RunTaskResult()
      : Base(), histogram()
    {
    }

    template<typename BaseType, typename NormalizedHistogramTypeRef>
    RunTaskResult(BaseType&& b, NormalizedHistogramTypeRef&& histogram_)
      : Base(std::forward<BaseType>(b)), histogram(histogram_)
    {
    }

    const NormalizedHistogramType histogram;
  };
  
  typedef std::vector<RunTaskResult*> RunTaskResultList;
  

  ResultsCollectorSimple(LoggerType & logger_)
    : _finalized(false), _finalhistogram(HistogramParams()),
      _collected_runtaskresults(),
      _llogger("MHRWTasks::ValueHistogramTasks::ResultsCollectorSimple", logger_)
  {
  }

  ~ResultsCollectorSimple()
  {
    for (std::size_t j = 0; j < _collected_runtaskresults.size(); ++j) {
      if (_collected_runtaskresults[j] != NULL) {
        delete _collected_runtaskresults[j];
      }
    }
  }
  
  inline bool isFinalized() const { return _finalized; }
  
  inline FinalHistogramType finalHistogram() const {
    tomographer_assert(isFinalized() && "You may only call finalHistogram() after the runs have been finalized.");
    return _finalhistogram;
  }
  
  inline std::size_t numTasks() const {
    tomographer_assert(isFinalized() && "You may only call numTasks() after the runs have been finalized.");
    return _collected_runtaskresults.size();
  }
  
  inline const RunTaskResultList & collectedRunTaskResults() const {
    tomographer_assert(isFinalized() && "You may only call collectedRunTaskResults() after the runs have been finalized.");
    return _collected_runtaskresults;
  }

  inline const RunTaskResult * collectedRunTaskResult(std::size_t task_no) const {
    tomographer_assert(isFinalized() && "You may only call collectedRunTaskResult(std::size_t) after the runs have been finalized.");
    tomographer_assert(task_no < _collected_runtaskresults.size());
    return _collected_runtaskresults[task_no];
  }

  template<typename RealType = double>
  inline void printHistogramCsv(std::ostream & stream, std::string sep = "\t", std::string linesep = "\n", int precision = 10)
  {
    stream << "Value" << sep << "Counts" << sep << "Error" << linesep
	   << std::scientific << std::setprecision(precision);
    for (int kk = 0; kk < _finalhistogram.bins.size(); ++kk) {
      stream << RealType(_finalhistogram.params.binLowerValue(kk)) << sep
	     << RealType(_finalhistogram.bins(kk)) << sep
	     << RealType(_finalhistogram.delta(kk)) << linesep;
    }
  }


  inline void printFinalReport(std::ostream & str, const CDataBaseType & cdata,
                               int max_width = 0, bool print_histogram = true)
  {
    Tools::ConsoleFormatterHelper h(max_width); // possibly detect terminal width etc.

    const auto& res = *this;

    const RunTaskResultList & collresults = res.collectedRunTaskResults();
    const FinalHistogramType finalhistogram = res.finalHistogram();
    str << "\n"
        << h.centerLine("Final Report of Runs")
        << h.hrule()
      ;
    cdata.printBasicCDataMHRWInfo(str);
    int dig_w = (int)std::ceil(std::log10(res.numTasks()));
    for (std::size_t j = 0; j < res.numTasks(); ++j) {
      tomo_internal::print_hist_short_bar_with_accept_info(str, dig_w, j, collresults[j]->histogram,
                                                           collresults[j]->acceptance_ratio, h.columns());
    }
    str << h.hrule()
        << "\n";

    if (print_histogram) {
      // and the final histogram
      str << h.centerLine("Final Histogram")
          << h.hrule();
      histogramPrettyPrint(str, finalhistogram, h.columns());
      str << h.hrule()
          << "\n";
    }
  }
    

  
private:
  bool _finalized;
  FinalHistogramType _finalhistogram;

  RunTaskResultList _collected_runtaskresults;

  Logger::LocalLogger<LoggerType>  _llogger;

public:

  // these functions will be called by the task manager/dispatcher
   
  template<typename Cnt, typename CData>
  inline void init(Cnt num_total_runs, Cnt /*n_chunk*/, const CData * pcdata)
  {
    tomographer_assert(!isFinalized() && "init() called after results have been finalized!");

    _collected_runtaskresults.resize(num_total_runs, NULL);
    _finalhistogram.reset(pcdata->histogram_params);
  }
  template<typename Cnt, typename TaskResultType, typename CData>
  inline void collectResult(Cnt task_no, TaskResultType&& taskresult, const CData * /*pcdata*/)
  {
    tomographer_assert(!isFinalized() && "collectResult() called after results have been finalized!");

    auto logger = _llogger.subLogger(TOMO_ORIGIN);
    logger.debug([&](std::ostream & str) {
	str << "Got task result. Histogram is:\n" << taskresult.stats_collector_result.prettyPrint();
      });

    NormalizedHistogramType thishistogram = taskresult.stats_collector_result;
    typename NormalizedHistogramType::CountType normalization =
      thishistogram.bins.sum() + thishistogram.off_chart;
    thishistogram.bins /= normalization;
    thishistogram.off_chart /= normalization;

    _finalhistogram.addHistogram(thishistogram);
    _collected_runtaskresults[task_no]
      = new RunTaskResult(std::forward<TaskResultType>(taskresult), std::move(thishistogram));
  }
  template<typename Cnt, typename CData>
  inline void runsFinished(Cnt, const CData *)
  {
    tomographer_assert(!isFinalized() && "runsFinished() called after results have been finalized!");

    _finalized = true;
    _finalhistogram.finalize();
  }

}; // struct ResultsCollectorSimple





/** \brief Results collector, if no binning analysis is being used.
 *
 * You can directly get the right type by querying the type \a
 * "CDataBase::ResultsCollectorType<..>::type".
 *
 * \todo DOC !!!!
 */
template<typename CDataBaseType_, typename LoggerType_>
struct ResultsCollectorWithBinningAnalysis
  : public virtual Tools::NeedOwnOperatorNew<
      UniformBinsHistogram<typename CDataBaseType_::HistogramType::Scalar,
                           typename CDataBaseType_::CountRealType>
    >::ProviderType
{
  typedef CDataBaseType_ CDataBaseType;
  typedef typename CDataBaseType::ValueCalculator ValueCalculator;
  typedef typename CDataBaseType::CountRealType CountRealType;
  typedef typename CDataBaseType::CountIntType CountIntType;
  typedef typename CDataBaseType::StepRealType StepRealType;
  typedef LoggerType_ LoggerType;
  
  typedef typename tomo_internal::histogram_types<CDataBaseType_,true>::BinningMHRWStatsCollectorParams
    BinningMHRWStatsCollectorParams;

  typedef typename BinningMHRWStatsCollectorParams::BinningAnalysisParamsType BinningAnalysisParamsType;

  typedef typename BinningMHRWStatsCollectorParams::Result MHRWStatsCollectorResultType;

  typedef typename CDataBaseType::HistogramType HistogramType;
  typedef typename CDataBaseType::HistogramParams HistogramParams;

  //! The final histogram, properly averaged
  typedef AveragedHistogram<HistogramType, CountRealType> FinalHistogramType;

  /** \brief The "simple" histogram, as if without binning analysis.
   *
   * Note we need a `double` counting type, because the histograms we'll be recording
   * are normalized.
   */
  typedef UniformBinsHistogram<typename HistogramType::Scalar, CountRealType> SimpleNormalizedHistogramType;
  typedef AveragedHistogram<SimpleNormalizedHistogramType, double> SimpleFinalHistogramType;


  typedef MHRandomWalkTaskResult<MHRWStatsCollectorResultType,CountIntType,StepRealType> RunTaskResult;
  typedef std::vector<RunTaskResult*> RunTaskResultList;

  TOMO_STATIC_ASSERT_EXPR( CDataBaseType::UseBinningAnalysis ) ;


  ResultsCollectorWithBinningAnalysis(LoggerType & logger_)
    : _finalized(false), _finalhistogram(), _simplefinalhistogram(),
      _collected_runtaskresults(),
      _llogger("MHRWTasks::ValueHistogramTasks::ResultsCollectorWithBinningAnalysis", logger_)
  {
  }

  ~ResultsCollectorWithBinningAnalysis()
  {
    for (std::size_t j = 0; j < _collected_runtaskresults.size(); ++j) {
      if (_collected_runtaskresults[j] != NULL) {
        delete _collected_runtaskresults[j];
      }
    }
  }

  inline bool isFinalized() const { return _finalized; }
  
  inline FinalHistogramType finalHistogram() const {
    tomographer_assert(isFinalized() && "You may only call finalHistogram() after the runs have been finalized.");
    return _finalhistogram;
  }
    
  inline SimpleFinalHistogramType simpleFinalHistogram() const {
    tomographer_assert(isFinalized() && "You may only call simpleFinalHistogram() after the runs have been finalized.");
    return _simplefinalhistogram;
  }
  
  inline std::size_t numTasks() const {
    tomographer_assert(isFinalized() && "You may only call numTasks() after the runs have been finalized.");
    return _collected_runtaskresults.size();
  }

  inline const RunTaskResultList & collectedRunTaskResults() const {
    tomographer_assert(isFinalized() && "You may only call collectedRunTaskResults() after the runs have been finalized.");
    return _collected_runtaskresults;
  }

  inline const RunTaskResult * collectedRunTaskResult(std::size_t task_no) const {
    tomographer_assert(isFinalized() && "You may only call collectedRunTaskResult(std::size_t) after the runs have been finalized.");
    tomographer_assert(task_no < _collected_runtaskresults.size());
    return _collected_runtaskresults[task_no];
  }


  template<typename RealType = double>
  inline void printHistogramCsv(std::ostream & stream, std::string sep = "\t", std::string linesep = "\n",
                                int precision = 10)
  {
    stream << "Value" << sep << "Counts" << sep << "Error" << sep << "SimpleError" << linesep
           << std::scientific << std::setprecision(precision);
    for (int kk = 0; kk < _finalhistogram.bins.size(); ++kk) {
      stream << RealType(_finalhistogram.params.binLowerValue(kk)) << sep
             << RealType(_finalhistogram.bins(kk)) << sep
	     << RealType(_finalhistogram.delta(kk)) << sep
	     << RealType(_simplefinalhistogram.delta(kk)) << linesep;
    }
  }


  inline void printFinalReport(std::ostream & str, const CDataBaseType & cdata,
                               int max_width = 0, bool print_histogram = true)
  {
    Tools::ConsoleFormatterHelper h(max_width); // possibly detect terminal width etc.

    const auto& res = *this;
  
    // produce report on runs
    const RunTaskResultList & collresults = res.collectedRunTaskResults();
    const FinalHistogramType finalhistogram = res.finalHistogram();
    str << "\n"
        << h.centerLine("Final Report of Runs")
        << h.hrule()
      ;
    cdata.printBasicCDataMHRWInfo(str);
    int dig_w = (int)std::ceil(std::log10(res.numTasks()));
    for (std::size_t j = 0; j < res.numTasks(); ++j) {
      const auto& stats_coll_result = collresults[j]->stats_collector_result;

      tomo_internal::print_hist_short_bar_with_accept_info(str, dig_w, j, stats_coll_result.hist,
                                                           collresults[j]->acceptance_ratio, h.columns());

      // error bars stats:
      const int nbins = stats_coll_result.converged_status.size();
      const int n_conv = stats_coll_result.converged_status
        .cwiseEqual(BinningAnalysisParamsType::CONVERGED).count();
      Eigen::ArrayXi unkn_arr = (stats_coll_result.converged_status
                                 .cwiseEqual(BinningAnalysisParamsType::UNKNOWN_CONVERGENCE))
        .template cast<int>();
      // little heuristic to see whether the "unknown" converged error bars are isolated or not
      const int n_unknown = unkn_arr.count();
      const int n_unknown_followingotherunknown
        = unkn_arr.segment(0,nbins-1).cwiseProduct(unkn_arr.segment(1,nbins-1)).count();
      const int n_unknown_isolated = n_unknown - n_unknown_followingotherunknown;
      const int n_notconv = stats_coll_result.converged_status
        .cwiseEqual(BinningAnalysisParamsType::NOT_CONVERGED).count();
      str << "    error bars: " << n_conv << " converged / "
          << n_unknown << " maybe (" << n_unknown_isolated << " isolated) / "
          << n_notconv << " not converged\n";
    }
    str << h.hrule()
        << "\n";

    if (print_histogram) {
      // and the final histogram
      str << h.centerLine("Final Histogram")
          << h.hrule();
      histogramPrettyPrint(str, finalhistogram, h.columns());
      str << h.hrule()
          << "\n";
    }
  }

  
private:
  bool _finalized;
  FinalHistogramType _finalhistogram;
  SimpleFinalHistogramType _simplefinalhistogram;

  RunTaskResultList _collected_runtaskresults;

  Logger::LocalLogger<LoggerType>  _llogger;

  
public:
    
  template<typename Cnt, typename CData>
  inline void init(Cnt num_total_runs, Cnt /*n_chunk*/, const CData * pcdata)
  {
    tomographer_assert(!isFinalized() && "init() called after results have been finalized!");

    _collected_runtaskresults.resize(num_total_runs, NULL);
    _finalhistogram.reset(pcdata->histogram_params);
    _simplefinalhistogram.reset(pcdata->histogram_params);
  }

  template<typename Cnt, typename TaskResultType, typename CData>
  inline void collectResult(Cnt task_no, TaskResultType && taskresult, const CData *)
  {
    tomographer_assert(!isFinalized() && "collectResult() called after results have been finalized!");

    auto logger = _llogger.subLogger(TOMO_ORIGIN);
    
    auto stats_coll_result = taskresult.stats_collector_result;

    logger.debug([&](std::ostream & str) {
	str << "(). Got task result. Histogram (w/ error bars from binning analysis):\n"
	    << stats_coll_result.hist.prettyPrint();
      });
    
    if ((stats_coll_result.converged_status !=
	 Eigen::ArrayXi::Constant(stats_coll_result.hist.numBins(), BinningAnalysisParamsType::CONVERGED)).any()) {
      logger.debug([&,this](std::ostream & str) {
	  str << "Error bars have not converged! The error bars at different binning levels are:\n"
	      << stats_coll_result.error_levels << "\n"
	      << "\t-> convergence analysis: \n";
	  for (std::size_t k = 0; k < stats_coll_result.hist.numBins(); ++k) {
	    str << "\t    val[" << std::setw(3) << k << "] = "
		<< std::setw(12) << stats_coll_result.hist.bins(k)
		<< " +- " << std::setw(12) << stats_coll_result.hist.delta(k);
	    if (stats_coll_result.converged_status(k) == BinningAnalysisParamsType::CONVERGED) {
	      str << "  [CONVERGED]";
	    } else if (stats_coll_result.converged_status(k) == BinningAnalysisParamsType::NOT_CONVERGED) {
	      str << "  [NOT CONVERGED]";
	    } else if (stats_coll_result.converged_status(k) == BinningAnalysisParamsType::UNKNOWN_CONVERGENCE) {
	      str << "  [UNKNOWN]";
	    } else {
	      str << "  [UNKNOWN CONVERGENCE STATUS: " << stats_coll_result.converged_status(k) << "]";
	    }
	    str << "\n";
	  }
	});
    }
    
    // because stats_coll_result is a histogram WITH error bars, addHistogram
    // will do the right thing and take them into account.
    _finalhistogram.addHistogram(stats_coll_result.hist);
    
    logger.debug("added histogram.");
    
    // this one is declared for histograms WITHOUT error bars (SimpleHistogramType is a
    // UniformBinsHistogram), so it will just ignore the error bars.
    logger.debug([&](std::ostream & str) {
	str << "Simple histogram is:\n";
	histogramPrettyPrint<SimpleNormalizedHistogramType>(str, stats_coll_result.hist);
      });
    _simplefinalhistogram.addHistogram(stats_coll_result.hist);

    _collected_runtaskresults[task_no] = new RunTaskResult(std::move(taskresult));
    
    logger.debug("done.");
  }

  template<typename Cnt, typename CData>
  inline void runsFinished(Cnt, const CData *)
  {
    tomographer_assert(!isFinalized() && "runs_finished() called after results have been finalized!");

    _finalized = true;
    _finalhistogram.finalize();
    _simplefinalhistogram.finalize();
  }

};







// ------------------------------------------------


namespace tomo_internal {
template<typename CDataBaseType, typename LoggerType, bool UseBinningAnalysis>
struct ResultsCollectorTypeHelper {
  typedef ResultsCollectorSimple<CDataBaseType, LoggerType> type;
};
template<typename CDataBaseType, typename LoggerType>
struct ResultsCollectorTypeHelper<CDataBaseType, LoggerType, true> {
  typedef ResultsCollectorWithBinningAnalysis<CDataBaseType, LoggerType> type;
};
} // namespace tomo_internal




/** \brief constant data for our MH random walk tasks with value histogram stats collector
 *
 *
 * \todo DOC !!!!
 */
template<typename ValueCalculator_, bool UseBinningAnalysis_ = true,
	 typename CountIntType_ = int, typename StepRealType_ = double,
	 typename CountRealType_ = double>
struct CDataBase
  : public MHRWTasks::CDataBase<CountIntType_, StepRealType_>,
    public virtual Tools::NeedOwnOperatorNew<ValueCalculator_>::ProviderType
{
  typedef MHRWTasks::CDataBase<CountIntType_, StepRealType_> Base; // base class

  typedef typename Base::CountIntType CountIntType;
  typedef typename Base::StepRealType StepRealType;

  typedef ValueCalculator_ ValueCalculator;
  typedef CountRealType_ CountRealType;

  static constexpr bool UseBinningAnalysis = UseBinningAnalysis_;

  typedef typename tomo_internal::histogram_types<CDataBase,UseBinningAnalysis>::MHRWStatsCollectorResultType
    MHRWStatsCollectorResultType;
  typedef typename tomo_internal::histogram_types<CDataBase,UseBinningAnalysis>::HistogramType HistogramType;
  typedef typename tomo_internal::histogram_types<CDataBase,UseBinningAnalysis>::HistogramParams HistogramParams;

  typedef MHRWParams<CountIntType, StepRealType> MHRWParamsType;
 

  TOMOGRAPHER_ENABLED_IF(!UseBinningAnalysis)
  CDataBase(const ValueCalculator & valcalc_, HistogramParams histogram_params_,
	    MHRWParamsType p, int base_seed = 0)
    : Base(std::move(p), base_seed), valcalc(valcalc_), histogram_params(histogram_params_),
      binningNumLevels()
  {
  }

  TOMOGRAPHER_ENABLED_IF(UseBinningAnalysis)
  CDataBase(const ValueCalculator & valcalc_, HistogramParams histogram_params_, int binning_num_levels_,
	    MHRWParamsType p, int base_seed = 0)
    : Base(std::move(p), base_seed), valcalc(valcalc_), histogram_params(histogram_params_),
      binningNumLevels(binning_num_levels_)
  {
  }

  const ValueCalculator valcalc;
  const HistogramParams histogram_params;
  const Tools::StoreIfEnabled<int, UseBinningAnalysis> binningNumLevels;


  template<typename LoggerType, TOMOGRAPHER_ENABLED_IF_TMPL(!UseBinningAnalysis)>
  inline ValueHistogramMHRWStatsCollector<ValueCalculator,LoggerType,HistogramType>
  createStatsCollector(LoggerType & logger) const
  {
    return ValueHistogramMHRWStatsCollector<ValueCalculator,LoggerType,HistogramType>(
	histogram_params,
	valcalc,
	logger
	);
  }

  template<typename LoggerType, TOMOGRAPHER_ENABLED_IF_TMPL(UseBinningAnalysis)>
  inline ValueHistogramWithBinningMHRWStatsCollector<
    typename tomo_internal::histogram_types<CDataBase, true>::BinningMHRWStatsCollectorParams,
    LoggerType>
  createStatsCollector(LoggerType & logger) const
  {
    typedef typename tomo_internal::histogram_types<CDataBase, true>::BinningMHRWStatsCollectorParams
      BinningMHRWStatsCollectorParams;

    return ValueHistogramWithBinningMHRWStatsCollector<BinningMHRWStatsCollectorParams,LoggerType>(
	histogram_params,
	valcalc,
        binningNumLevels.value,
	logger
	);
  }

  //! Helper to get the results collector type
  template<typename LoggerType>
  struct ResultsCollectorType {
    typedef
#ifndef TOMOGRAPHER_PARSED_BY_DOXYGEN
    typename
    tomo_internal::ResultsCollectorTypeHelper<CDataBase<ValueCalculator,UseBinningAnalysis,CountIntType,StepRealType,CountRealType>,
					      LoggerType, UseBinningAnalysis>::type
#else
    [...] // parsed by doxygen -- make this more readable
#endif
    Type;

    typedef typename Type::RunTaskResult RunTaskResultType;
  };
};
// define static members:
template<typename ValueCalculator_, bool UseBinningAnalysis_,
	 typename CountIntType_, typename StepRealType_,
	 typename CountRealType_>
constexpr bool CDataBase<ValueCalculator_,UseBinningAnalysis_,CountIntType_,StepRealType_,CountRealType_>::UseBinningAnalysis;









} // namespace ValueHistogramTasks
} // namespace MHRWTasks
} // namespace Tomographer


#endif
