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

#include <tomographer2/tools/loggers.h>
#include <tomographer2/mhrwtasks.h>

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


// forward declaration of the CDataBase type
template<typename ValueCalculator_, bool UseBinningAnalysis_ = true,
	 typename CountIntType_ = int, typename StepRealType_ = double,
	 typename CountRealType_ = double>
struct CDataBase;
// forward declarations of the ResultsCollectors
template<typename CDataBaseType, typename LoggerType>
struct ResultsCollectorSimple;
template<typename CDataBaseType, typename LoggerType>
struct ResultsCollectorWithBinningAnalysis;


namespace tomo_internal {
template<typename CDataBaseType, bool UseBinningAnalysis>
struct histogram_types {// version WITHOUT binning analysis:
  typedef UniformBinsHistogram<typename CDataBaseType::ValueCalculator::ValueType> HistogramType;
  /// we know that ValueHistogramMHRWStatsCollector<ValueCalculator,...,HistogramType>::Result is HistogramType
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
}


template<typename ValueCalculator_, bool UseBinningAnalysis_ = true,
	 typename CountIntType_ = int, typename StepRealType_ = double,
	 typename CountRealType_ = double>
struct CDataBase : public MHRWTasks::CDataBase<CountIntType_, StepRealType_>
{
  typedef MHRWTasks::CDataBase<CountIntType_, StepRealType_> Base; // base class

  using typename Base::CountIntType;
  using typename Base::StepRealType;

  typedef ValueCalculator_ ValueCalculator;
  typedef CountRealType_ CountRealType;

  static constexpr bool UseBinningAnalysis = UseBinningAnalysis_;

  typedef typename tomo_internal::histogram_types<CDataBase,UseBinningAnalysis>::MHRWStatsCollectorResultType
    MHRWStatsCollectorResultType;
  typedef typename tomo_internal::histogram_types<CDataBase,UseBinningAnalysis>::HistogramType HistogramType;
  typedef typename tomo_internal::histogram_types<CDataBase,UseBinningAnalysis>::HistogramParams HistogramParams;


  TOMOGRAPHER_ENABLED_IF(!UseBinningAnalysis)
  CDataBase(const ValueCalculator & valcalc_, HistogramParams histogram_params_)
    : valcalc(valcalc_), _histogram_params(histogram_params_)
  {
  }

  TOMOGRAPHER_ENABLED_IF(UseBinningAnalysis)
  CDataBase(const ValueCalculator & valcalc_, HistogramParams histogram_params_, int binning_num_levels_)
    : valcalc(valcalc_), _histogram_params(histogram_params_),
      binning_num_levels(binning_num_levels_)
  {
  }

  const ValueCalculator valcalc;
  const HistogramParams histogram_params;
  const Tools::store_if_enabled<int, UseBinningAnalysis> binning_num_levels;




  template<typename LoggerType, TOMOGRAPHER_ENABLED_IF_TMPL(!UseBinningAnalysis)>
  inline ValueHistogramMHRWStatsCollector<ValueCalculator,LoggerType,HistogramType>
  createStatsCollector(LoggerType & logger) const
  {
    return Tomographer::ValueHistogramMHRWStatsCollector<ValueCalculator,LoggerType,HistogramType>(
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

    return Tomographer::ValueHistogramWithBinningMHRWStatsCollector<BinningMHRWStatsCollectorParams,LoggerType>(
	histogram_params,
	valcalc,
        binning_num_levels,
	logger
	);
  }

  // corresponding results collector : case WITHOUT binning analysis
  template<typename LoggerType, typename CountRealType,
	   TOMOGRAPHER_ENABLED_IF_TMPL(!UseBinningAnalysis)>
  struct ResultsCollectorType {
    typedef ResultsCollectorSimple<CDataBase<ValueCalculator,UseBinningAnalysis,CountIntType,StepRealType,CountRealType>,
				   CountRealType> type;
  };
  // corresponding results collector : case WITH binning analysis
  template<typename LoggerType, typename CountRealType,
	   TOMOGRAPHER_ENABLED_IF_TMPL(!UseBinningAnalysis)>
  struct ResultsCollectorType {
    typedef ResultsCollectorWithBinningAnalysis<CDataBase<ValueCalculator,UseBinningAnalysis,CountIntType,StepRealType,CountRealType>,
						CountRealType> type;
  };
};
// define static members:
template<typename ValueCalculator_, bool UseBinningAnalysis_,
	 typename CountIntType_, typename StepRealType_,
	 typename CountRealType_>
constexpr bool CDataBase<ValueCalculator_,UseBinningAnalysis_,CountIntType_,StepRealType_,CountRealType_>::UseBinningAnalysis;




//
// RUN TASK INFO
//

struct RunTaskInfo
{
  RunTaskInfo()
    : n_run(0), n_therm(0), n_sweep(0),
      step_size(std::numeric_limits<double>::quiet_NaN()),
      acceptance_ratio(std::numeric_limits<double>::quiet_NaN())
  {
  }

  template<typename TaskResultType>
  RunTaskInfo(TaskResultType && t)
    : n_run(t.n_run),
      n_therm(t.n_therm),
      n_sweep(t.n_sweep),
      step_size(t.step_size),
      acceptance_ratio(t.acceptance_ratio)
  {
  }

  std::size_t n_run;
  std::size_t n_therm;
  std::size_t n_sweep;
  double step_size;
  double acceptance_ratio;
};


/** \brief Results collector, if no binning analysis is being used.
 *
 * You can directly get the right type by querying the type \a
 * "CDataBase::ResultsCollectorType<..>::type".
 */
template<typename CDataBaseType_, typename LoggerType_>
struct ResultsCollectorSimple
{
  typedef CDataBaseType_ CDataBaseType;
  typedef typename CDataBaseType::ValueCalculator ValueCalculator;
  typedef typename CDataBaseType::CountRealType CountRealType;
  typedef LoggerType_ LoggerType;
  
  typedef typename CDataBaseType::HistogramType HistogramType;
  typedef typename CDataBaseType::HistogramParams HistogramParams;
  typedef UniformBinsHistogram<typename HistogramType::Scalar, CountRealType> NormalizedHistogramType;
  typedef Tomographer::AveragedHistogram<NormalizedHistogramType, CountRealType> FinalHistogramType;
  
  ResultsCollectorSimple(LoggerType & logger_)
    : _finalized(false), finalhistogram(HistogramParams()),
      _collected_runtaskinfos(), _collected_histograms(),
      _llogger("MHRWTasks::ValueHistogramTasks::ResultsCollectorSimple", logger_)
  {
  }

  inline bool isFinalized() const { return _finalized; }
  
  inline const FinalHistogramType & finalHistogram() const {
    assert(isFinalized() && "You may only call finalHistogram() after the runs have been finalized.");
    return _finalhistogram;
  }
    
  inline const std::vector<RunTaskInfo> & collectedRunTaskInfos() const {
    assert(isFinalized() && "You may only call collectedRunTaskInfos() after the runs have been finalized.");
    return _collected_runtaskinfos;
  }

  inline const std::vector<NormalizedHistogramType> & collectedHistograms() const {
    assert(isFinalized() && "You may only call collectedHistograms() after the runs have been finalized.");
    return _collected_histograms;
  }


  inline void print_histogram_csv(std::ostream & stream)
  {
    stream << "Value\tCounts\tError\n"
	   << std::scientific << std::setprecision(10);
    for (int kk = 0; kk < finalhistogram.bins.size(); ++kk) {
      stream << (double)finalhistogram.params.bin_lower_value(kk) << "\t"
	     << (double)finalhistogram.bins(kk) << "\t"
	     << (double)finalhistogram.delta(kk) << "\n";
    }
  }

private:
  bool _finalized;
  FinalHistogramType finalhistogram;

  std::vector<RunTaskInfo> _collected_runtaskinfos;
  std::vector<NormalizedHistogramType> _collected_histograms;

  Logger::LocalLogger<LoggerType>  _llogger;
    
public:

  // these functions will be called by the task manager/dispatcher
   
  template<typename Cnt, typename CData>
  inline void init(Cnt num_total_runs, Cnt /*n_chunk*/, const CData * pcdata)
  {
    assert(!isFinalized() && "init() called after results have been finalized!");

    _collected_histograms.resize(num_total_runs);
    _collected_runtaskinfos.reserve(num_total_runs);
    _finalhistogram.reset(pcdata->histogram_params);
  }
  template<typename Cnt, typename TaskResultType, typename CData>
  inline void collect_result(Cnt task_no, const TaskResultType& taskresult, const CData * /*pcdata*/)
  {
    assert(!isFinalized() && "collect_result() called after results have been finalized!");

    auto logger = _llogger.sublogger(TOMO_ORIGIN);
    logger.debug([&](std::ostream & str) {
	str << "Got task result. Histogram is:\n" << taskresult.stats_collector_result.pretty_print();
      });

    NormalizedHistogramType thishistogram = taskresult.stats_collector_result;
    typename NormalizedHistogramType::CountType normalization =
      thishistogram.bins.sum() + thishistogram.off_chart;
    thishistogram.bins /= normalization;
    thishistogram.off_chart /= normalization;

    _collected_histograms[task_no] = thishistogram;
    _collected_runtaskinfos[task_no] = RunTaskInfo(taskresult);
    _finalhistogram.add_histogram(thishistogram);
  }
  template<typename Cnt, typename CData>
  inline void runs_finished(Cnt, const CData *)
  {
    assert(!isFinalized() && "runs_finished() called after results have been finalized!");

    _finalized = true;
    _finalhistogram.finalize();
  }

}; // struct ResultsCollectorSimple





/** \brief Results collector, if no binning analysis is being used.
 *
 * You can directly get the right type by querying the type \a
 * "CDataBase::ResultsCollectorType<..>::type".
 */
template<typename CDataBaseType_, typename LoggerType_>
struct ResultsCollectorWithBinning
{
  typedef CDataBaseType_ CDataBaseType;
  typedef typename CDataBaseType::ValueCalculator ValueCalculator;
  typedef typename CDataBaseType::CountRealType_ CountRealType;
  typedef LoggerType_ LoggerType;
  
  typedef typename tomo_internal::histogram_types<CDataBaseType_,true>::BinningMHRWStatsCollectorParams
    BinningMHRWStatsCollectorParams;

  typedef typename CDataBaseType::HistogramType HistogramType;
  typedef typename CDataBaseType::HistogramParams HistogramParams;

  //! The final histogram, properly averaged
  typedef Tomographer::AveragedHistogram<HistogramType, CountRealType> FinalHistogramType;

  /** \brief The "simple" histogram, as if without binning analysis.
   *
   * Note we need a `double` counting type, because the histograms we'll be recording
   * are normalized.
   */
  typedef UniformBinsHistogram<typename HistogramType::Scalar, CountRealType> NormalizedSimpleHistogramType;
  typedef Tomographer::AveragedHistogram<NormalizedSimpleHistogramType, double> SimpleFinalHistogramType;

  ResultsCollectorWithBinning(LoggerType & logger_)
    : _finalized(false), _finalhistogram(), _simplefinalhistogram(),
      _collected_runtaskinfos(), _collected_histograms(),
      _llogger("MHRWTasks::ValueHistogramTasks::ResultsCollectorWithBinning", logger_)
  {
  }

  inline bool isFinalized() const { return _finalized; }
  
  inline const FinalHistogramType & finalHistogram() const {
    assert(isFinalized() && "You may only call finalHistogram() after the runs have been finalized.");
    return _finalhistogram;
  }
    
  inline const SimpleFinalHistogramType & simpleFinalHistogram() const {
    assert(isFinalized() && "You may only call simpleFinalHistogram() after the runs have been finalized.");
    return _simplefinalhistogram;
  }
    
  inline const std::vector<RunTaskInfo> & collectedRunTaskInfos() const {
    assert(isFinalized() && "You may only call collectedRunTaskInfos() after the runs have been finalized.");
    return _collected_runtaskinfos;
  }

  inline const std::vector<NormalizedHistogramType> & collectedHistograms() const {
    assert(isFinalized() && "You may only call collectedHistograms() after the runs have been finalized.");
    return _collected_histograms;
  }


  inline void print_histogram_csv(std::ostream & stream)
  {
    stream << "Value\tCounts\tError\tSimpleError\n"
           << std::scientific << std::setprecision(10);
    for (int kk = 0; kk < finalhistogram.bins.size(); ++kk) {
      stream << (double)finalhistogram.params.bin_lower_value(kk) << "\t"
             << (double)finalhistogram.bins(kk) << "\t"
             << (double)finalhistogram.delta(kk) << "\t"
             << (double)simplefinalhistogram.delta(kk) << "\n";
    }
  }

private:
  bool _finalized;
  FinalHistogramType _finalhistogram;
  SimpleFinalHistogramType _simplefinalhistogram;

  std::vector<RunTaskInfo> _collected_runtaskinfos;
  std::vector<NormalizedHistogramType> _collected_histograms;

  Logger::LocalLogger<LoggerType>  _llogger;
    
public:
    
  template<typename Cnt, typename CData>
  inline void init(Cnt num_total_runs, Cnt /*n_chunk*/, const CData * pcdata)
  {
    assert(!isFinalized() && "init() called after results have been finalized!");

    _collected_histograms.resize(num_total_runs);
    _collected_runtaskinfos.resize(num_total_runs);
    _finalhistogram.reset(pcdata->histogram_params);
    _simplefinalhistogram.reset(pcdata->histogram_params);
  }

  template<typename Cnt, typename TaskResultType, typename CData>
  inline void collect_result(Cnt task_no, TaskResultType && taskresult, const CData *)
  {
    assert(!isFinalized() && "collect_result() called after results have been finalized!");

    auto logger = _llogger.sublogger(TOMO_ORIGIN);
    
    _collected_runtaskinfos[task_no] = RunTaskInfo(taskresult);
    auto stats_coll_result = taskresult.stats_collector_result;
    _collected_histograms[task_no] = stats_coll_result;
    
    logger.debug([&](std::ostream & str) {
	str << "(). Got task result. Histogram (w/ error bars from binning analysis):\n"
	    << stats_coll_result.hist.pretty_print();
      });
    
    if ((stats_coll_result.converged_status !=
	 Eigen::ArrayXi::Constant(stats_coll_result.hist.num_bins(), BinningAnalysisParamsType::CONVERGED)).any()) {
      logger.debug([&,this](std::ostream & str) {
	  str << "Error bars have not converged! The error bars at different binning levels are:\n"
	      << stats_coll_result.error_levels << "\n"
	      << "\t-> convergence analysis: \n";
	  for (std::size_t k = 0; k < stats_coll_result.hist.num_bins(); ++k) {
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
    
    // because stats_coll_result is a histogram WITH error bars, add_histogram will do the
    // right thing and take them into account.
    finalhistogram.add_histogram(stats_coll_result.hist);
    
    logger.debug("added histogram.");
    
    // this one is declared for histograms WITHOUT error bars (SimpleHistogramType is a
    // UniformBinsHistogram), so it will just ignore the error bars.
    logger.debug([&](std::ostream & str) {
	str << "Simple histogram is:\n";
	histogram_pretty_print<SimpleHistogramType>(str, stats_coll_result.hist);
      });
    simplefinalhistogram.add_histogram(stats_coll_result.hist);
    logger.debug("done.");
  }
  template<typename Cnt, typename CData>
  inline void runs_finished(Cnt, const CData *)
  {
    assert(!isFinalized() && "runs_finished() called after results have been finalized!");

    _finalized = true;
    _finalhistogram.finalize();
    _simplefinalhistogram.finalize();
  }

};







} // namespace ValueHistogramTasks
} // namespace MHRWTasks
} // namespace Tomographer


#endif
