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
  typedef UniformBinsHistogram<typename CDataBaseType::ValueCalculator::ValueType,
                               typename CDataBaseType::HistCountIntType> HistogramType;
  /// we know that ValueHistogramMHRWStatsCollector<ValueCalculator,...,HistogramType>::ResultType is HistogramType
  typedef HistogramType MHRWStatsCollectorResultType;
  typedef typename HistogramType::Params HistogramParams;
};
template<typename CDataBaseType>
struct histogram_types<CDataBaseType, true> {// version WITH binning analysis:
  /// the \ref ValueHistogramWithBinningMHRWStatsCollectorParams...
  typedef ValueHistogramWithBinningMHRWStatsCollectorParams<
    typename CDataBaseType::ValueCalculator, typename CDataBaseType::HistCountIntType,
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
TOMOGRAPHER_EXPORT
void print_hist_short_bar_with_accept_info(std::ostream & str, int dig_w, std::size_t j, const HistogramType & hist,
                                           double acceptance_ratio, int columns)
{
  histogramShortBarWithInfo(str,
                            streamstr("#" << std::setw(dig_w) << j << ": "),
                            hist,
                            Tomographer::Tools::fmts(" [accept ratio = %.2f]", acceptance_ratio),
                            false, columns);
  if (acceptance_ratio > MHRWAcceptanceRatioRecommendedMax ||
      acceptance_ratio < MHRWAcceptanceRatioRecommendedMin) {
    str << "    *** Accept ratio out of recommended bounds ["<<MHRWAcceptanceRatioRecommendedMin
        <<", "<<MHRWAcceptanceRatioRecommendedMax<<"] ! Adapt step size ***\n";
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
 * This class is to be used in conjunction with Tomographer::MHRWTasks::MHRandomWalkTask
 * and a const-shared-data structure inheriting from
 * Tomographer::MHRWTasks::ValueHistogramTasks::CData and complying with the \ref
 * pageInterfaceMHRandomWalkTaskCData.
 *
 * This results collector should only be used if each task provides a "raw"
 * histogram, ie. without any error bars.  If you have tasks which already
 * provide some error bars from a binning analysis, then you should be using the
 * class \ref ResultsCollectorWithBinningAnalysis.
 *
 * Note that you can directly get the right ResultsCollector type by querying
 * the type \a "CDataBase::ResultsCollectorType<..>::Type".
 */
template<typename CDataBaseType_, typename LoggerType_>
TOMOGRAPHER_EXPORT struct ResultsCollectorSimple
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
  //! The integer type which serves to count the number of iterations (see \ref MHRWParams)
  typedef typename CDataBaseType::IterCountIntType IterCountIntType;
  //! The real type which serves to describe the step size of the random walk (typically \c double)
  typedef typename CDataBaseType::StepRealType StepRealType;
  //! The logger type we can send messages to
  typedef LoggerType_ LoggerType;
  
  /** \brief The histogram type corresponding to the result of a task
   *
   * \warning This histogram type must not provide error bars.
   */
  typedef typename CDataBaseType::HistogramType HistogramType;
  //! The parameters type used to describe our histogram range and number of bins
  typedef typename CDataBaseType::HistogramParams HistogramParams;
  /** \brief The type of the histogram resulting from a single task, but scaled so that
   *         each bin value corresponds to the fraction of data points in bin
   */
  typedef UniformBinsHistogram<typename HistogramType::Scalar, CountRealType> ScaledHistogramType;
  /** \berief The type of the final resulting, averaged histogram
   *
   * The scale of the histogram is chosen such that each bin value corresponds to the
   * fraction of data points observed in this bin.  To normalize the histogram to a unit
   * probability density, use \ref UniformBinsHistogram::normalized().  This scaling is
   * the same as that used by the histogram produced using a binning analysis, see \ref
   * ValueHistogramWithBinningMHRWStatsCollectorResult.
   */
  typedef AveragedHistogram<ScaledHistogramType, CountRealType> FinalHistogramType;
  
  /** \brief The result type of our ValueHistogramMHRWStatsCollector
   */
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
    : public MHRandomWalkTaskResult<MHRWStatsCollectorResultType,IterCountIntType,StepRealType>,
      public virtual Tools::NeedOwnOperatorNew<ScaledHistogramType>::ProviderType
  {
    typedef MHRandomWalkTaskResult<MHRWStatsCollectorResultType,IterCountIntType,StepRealType> Base;

    RunTaskResult()
      : Base(), histogram()
    {
    }

    template<typename BaseType, typename ScaledHistogramTypeRef>
    RunTaskResult(BaseType&& b, ScaledHistogramTypeRef&& histogram_)
      : Base(std::forward<BaseType>(b)), histogram(histogram_)
    {
    }

    /** \brief The resulting histogram.
     *
     * The scale of the histogram is chosen such that each bin value corresponds to the
     * fraction of data points observed in this bin.  To normalize the histogram to a unit
     * probability density, use \ref UniformBinsHistogram::normalized().  This scaling is
     * the same as that used by the histogram produced using a binning analysis, see \ref
     * ValueHistogramWithBinningMHRWStatsCollectorResult.
     */
    const ScaledHistogramType histogram;
  };
  
  typedef std::vector<RunTaskResult*> RunTaskResultList;
  
  
  /** \brief Constructor.
   *
   * Messages will be logged to the given logger.
   */
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
  
  /** \brief Returns \c TRUE after all runs have finished and results processed.
   *
   * Note that it is the task of the multiprocessing task dispatcher (e.g. \ref
   * MultiProc::OMP::TaskDispatcher::run) to finalize the results, as required by the \ref
   * pageTaskManagerDispatcher API).
   */
  inline bool isFinalized() const { return _finalized; }
  
  /** \brief The final histogram, with error bars
   *
   * The error bars are calculated from the standard devitation of the histogram values
   * reported by the different tasks.  Make sure enough tasks have been run in order for
   * the error bars to be meaningful.
   *
   * The scale of the histogram is chosen such that each bin value corresponds to the
   * fraction of data points observed in this bin.  To normalize the histogram to a unit
   * probability density, use \ref UniformBinsHistogram::normalized().
   */
  inline FinalHistogramType finalHistogram() const {
    tomographer_assert(isFinalized() && "You may only call finalHistogram() after the runs have been finalized.");
    return _finalhistogram;
  }
  
  /** \brief Return the number of tasks that were run
   *
   * You may only call this function after the results have been finalized (see \ref
   * isFinalized()), i.e. after the task dispatcher has finished executing all tasks.
   */
  inline std::size_t numTasks() const {
    tomographer_assert(isFinalized() && "You may only call numTasks() after the runs have been finalized.");
    return _collected_runtaskresults.size();
  }
  
  /** \brief Return a list of the resulting report of each run task
   *
   * The result (i.e. the individual task's final report) is stored in a \ref
   * RunTaskResult structure.
   *
   * You may only call this function after the results have been finalized (see \ref
   * isFinalized()), i.e. after the task dispatcher has finished executing all tasks.
   */
  inline const RunTaskResultList & collectedRunTaskResults() const {
    tomographer_assert(isFinalized() && "You may only call collectedRunTaskResults() after the runs have been finalized.");
    return _collected_runtaskresults;
  }

  /** \brief Return a list of the resulting report of one specific run task
   *
   * The result (i.e. the individual task's final report) is stored in a \ref
   * RunTaskResult structure.
   *
   * \param task_no must be an index such that <code>task_no >= 0 && task_no <
   *        numTasks()</code>.
   *
   * You may only call this function after the results have been finalized (see \ref
   * isFinalized()), i.e. after the task dispatcher has finished executing all tasks.
   */
  inline const RunTaskResult * collectedRunTaskResult(std::size_t task_no) const {
    tomographer_assert(isFinalized() && "You may only call collectedRunTaskResult(std::size_t) after the runs have been finalized.");
    tomographer_assert(task_no < _collected_runtaskresults.size());
    return _collected_runtaskresults[task_no];
  }

  /** \brief Produce a comma-separated-value (CSV) representation of the final histogram
   *
   * The histogram data is written in CSV format on the C++ output stream \a stream.  You
   * may specify the cell separator \a sep (by default a TAB char), the line separator (by
   * default a simple newline), and the precision used when exporting the values.  Numbers
   * are written in scientific format (e.g. <code>1.205115485e-01</code>).
   *
   * Three columns are outputted.  Titles are outputted on the first line.  The first
   * column holds the values, i.e., the x-axis of the histogram; the second column holds
   * the counts (normalized to the number of samples); the third column the error bar on
   * the counts.
   */
  template<typename RealType = double>
  inline void printHistogramCsv(std::ostream & stream, std::string sep = "\t", std::string linesep = "\n", int precision = 10)
  {
    // ### Q: why the RealType template parameter??
    stream << "Value" << sep << "Counts" << sep << "Error" << linesep
	   << std::scientific << std::setprecision(precision);
    for (int kk = 0; kk < _finalhistogram.bins.size(); ++kk) {
      stream << RealType(_finalhistogram.params.binLowerValue(kk)) << sep
	     << RealType(_finalhistogram.bins(kk)) << sep
	     << RealType(_finalhistogram.delta(kk)) << linesep;
    }
  }

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
   * readable form, using \ref UniformBinsHistogramWithErrorBars::prettyPrint().
   *
   */
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
   
  //! In compliance with \ref pageInterfaceResultsCollector
  template<typename Cnt, typename CData>
  inline void init(Cnt num_total_runs, Cnt /*n_chunk*/, const CData * pcdata)
  {
    tomographer_assert(!isFinalized() && "init() called after results have been finalized!");

    _collected_runtaskresults.resize(num_total_runs, NULL);
    _finalhistogram.reset(pcdata->histogram_params);
  }
  //! In compliance with \ref pageInterfaceResultsCollector
  template<typename Cnt, typename TaskResultType, typename CData>
  inline void collectResult(Cnt task_no, TaskResultType&& taskresult, const CData * /*pcdata*/)
  {
    tomographer_assert(!isFinalized() && "collectResult() called after results have been finalized!");

    auto logger = _llogger.subLogger(TOMO_ORIGIN);
    logger.debug([&](std::ostream & str) {
	str << "Got task result. Histogram is:\n" << taskresult.stats_collector_result.prettyPrint();
      });

    // Scale this histogram using the number of samples, so that the scaling of the
    // histogram coincides to that used by the binning analysis.  This so we can compare
    // those histograms obtained by the two procedures.
    //
    // NOTE: This does not normalize the histogram to unit area.  Use \ref
    // UniformBinsHistogram::normalized() for that.
    //
    ScaledHistogramType thishistogram = taskresult.stats_collector_result;
    typename ScaledHistogramType::CountType numsamples =
      thishistogram.bins.sum() + thishistogram.off_chart;
    thishistogram.bins /= numsamples;
    thishistogram.off_chart /= numsamples;

    _finalhistogram.addHistogram(thishistogram);
    _collected_runtaskresults[task_no]
      = new RunTaskResult(std::forward<TaskResultType>(taskresult), std::move(thishistogram));
  }
  /** \brief In compliance with \ref pageInterfaceResultsCollector &mdash; finalizes the results
   *
   * After this function is called (which the task dispatcher does automatically), the
   * results are finalized (isFinalized() now returns \a true) and the functions
   * finalHistogram(), collectedRunTaskResults(), etc. may be called.
   */
  template<typename Cnt, typename CData>
  inline void runsFinished(Cnt, const CData *)
  {
    tomographer_assert(!isFinalized() && "runsFinished() called after results have been finalized!");

    _finalized = true;
    _finalhistogram.finalize();
  }

}; // struct ResultsCollectorSimple





/** \brief Results collector, if binning analysis is being used.
 *
 * This class takes care of collecting the results of the executed random walk
 * tasks, and merges them together to provide a final, averaged histogram with
 * error bars.
 *
 * This class complies with the \ref pageInterfaceResultsCollector.
 *
 * This class is to be used in conjunction with Tomographer::MHRWTasks::MHRandomWalkTask
 * and a const-shared-data structure inheriting from
 * Tomographer::MHRWTasks::ValueHistogramTasks::CData and complying with the \ref
 * pageInterfaceMHRandomWalkTaskCData.
 *
 * This results collector should only be used if each task provides a histogram with error
 * bars which come from a binning analysis (with \ref
 * ValueHistogramWithBinningMHRWStatsCollector).  If you have tasks which give you "raw"
 * histogram without any error bars, you should use the class \ref
 * ResultsCollectorSimple.
 *
 * In fact, we provide two final histograms. One has the error bars from the binning
 * analyses combined together from ech task, providing reliable error bars if the error
 * analysis converged in each task.  For the other histogram, we simply average the "raw"
 * histograms from each task, ignoring the binning analysis completely, and determining
 * error bars as the standard deviation from the different runs, as for the simple case
 * with \ref ResultsCollectorSimple.  This allows to compare the error bars from the
 * binning analysis to the naive statistical error bars resulting from many trials.  You
 * do need enough tasks, though, to have a reliable "naive" estimate of the error bars.
 *
 * Note that you can directly get the right ResultsCollector type by querying
 * the type \a "CDataBase::ResultsCollectorType<..>::Type".
 */
template<typename CDataBaseType_, typename LoggerType_>
TOMOGRAPHER_EXPORT struct ResultsCollectorWithBinningAnalysis
  : public virtual Tools::NeedOwnOperatorNew<
      UniformBinsHistogram<typename CDataBaseType_::HistogramType::Scalar,
                           typename CDataBaseType_::CountRealType>
    >::ProviderType
{
  //! The structure which hold the constant shared data describing what we have to do
  typedef CDataBaseType_ CDataBaseType;
  //! The class which calculates the value we are collecting a histogram of (\ref pageInterfaceValueCalculator compliant)
  typedef typename CDataBaseType::ValueCalculator ValueCalculator;
  //! The real type which serves to average histogram counts (typically \c double)
  typedef typename CDataBaseType::CountRealType CountRealType;
  //! The integer type which serves to count the number of iterations (see \ref MHRWParams)
  typedef typename CDataBaseType::IterCountIntType IterCountIntType;
  //! The real type which serves to describe the step size of the random walk (typically \c double)
  typedef typename CDataBaseType::StepRealType StepRealType;
  //! The logger type we can send messages to
  typedef LoggerType_ LoggerType;
  
  /** \brief The parameters for the relevant \ref
   *         ValueHistogramWithBinningMHRWStatsCollector
   *
   * This is a \ref ValueHistogramWithBinningMHRWStatsCollectorParams type.
   */
  typedef typename tomo_internal::histogram_types<CDataBaseType_,true>::BinningMHRWStatsCollectorParams
    BinningMHRWStatsCollectorParams;

  /** \brief The parameters for the corresponding \ref BinningAnalysis type.
   *
   * This is a \ref BinningAnalysisParams type.
   */
  typedef typename BinningMHRWStatsCollectorParams::BinningAnalysisParamsType BinningAnalysisParamsType;

  /** \brief The result type of our stats collector
   *
   * This is a structure containing a histogram with error bars, along with a convergence
   * analysis of the errors.  It is a \ref
   * ValueHistogramWithBinningMHRWStatsCollectorResult type.
   */
  typedef typename BinningMHRWStatsCollectorParams::Result MHRWStatsCollectorResultType;

  /** \brief The histogram type reported by each task
   *
   * This is \ref ValueHistogramWithBinningMHRWStatsCollectorParams::HistogramType.
   */
  typedef typename CDataBaseType::HistogramType HistogramType;
  /** \brief The histogram parameters type reported by each task
   *
   * This is \ref ValueHistogramWithBinningMHRWStatsCollectorParams::HistogramParams.
   */
  typedef typename CDataBaseType::HistogramParams HistogramParams;

  //! The final histogram, properly averaged
  typedef AveragedHistogram<HistogramType, CountRealType> FinalHistogramType;

  /** \brief The "simple" histogram, as if without binning analysis
   *
   * Note we need a real (e.g. \a double) counting type, because the histograms we'll be
   * recording are scaled.
   */
  typedef UniformBinsHistogram<typename HistogramType::Scalar, CountRealType> SimpleScaledHistogramType;
  /** \brief Properly averaged "simple" histogram, with naive statistical standard
   *         deviation error bars from the several task runs
   */
  typedef AveragedHistogram<SimpleScaledHistogramType, double> SimpleFinalHistogramType;

  /** \brief Stores information about the result of a task run.
   *
   * This structure is precisely the type that the task itself returns; see \ref
   * MHRandomWalkTaskResult.
   *
   */
  typedef MHRandomWalkTaskResult<MHRWStatsCollectorResultType,IterCountIntType,StepRealType> RunTaskResult;
  //! A list which will store the result of each task
  typedef std::vector<RunTaskResult*> RunTaskResultList;


  TOMO_STATIC_ASSERT_EXPR( CDataBaseType::UseBinningAnalysis ) ;

  /** \brief Constructor
   *
   * Messages will be logged to the given logger.
   */
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

  /** \brief Returns \c TRUE after all runs have finished and results processed.
   *
   * Note that it is the task of the multiprocessing task dispatcher (e.g. \ref
   * MultiProc::OMP::TaskDispatcher::run) to finalize the results, as required by the \ref
   * pageTaskManagerDispatcher API).
   */
  inline bool isFinalized() const { return _finalized; }
  
  /** \brief The final histogram, with all the error bars combined
   *
   * The error bars are calculated by combining the error bars obtained via a binning
   * analysis for each task.  See the page \ref pageTheoryAveragedHistogram for how this
   * is calculated.
   *
   * The scale of the histogram is chosen such that each bin value corresponds to the
   * fraction of data points observed in this bin.  To normalize the histogram to a unit
   * probability density, use \ref UniformBinsHistogram::normalized().
   */
  inline FinalHistogramType finalHistogram() const {
    tomographer_assert(isFinalized() && "You may only call finalHistogram() after the runs have been finalized.");
    return _finalhistogram;
  }
    
  /** \brief The final histogram, with naive error bars ignoring the binning analysis
   *
   * The error bars are calculated from the standard devitation of the histogram values
   * reported by the different tasks, exactly as for \ref ResultsCollectorSimple.  Make
   * sure enough tasks have been run in order for the error bars to be meaningful.
   *
   * The scale of the histogram is chosen such that each bin value corresponds to the
   * fraction of data points observed in this bin.  To normalize the histogram to a unit
   * probability density, use \ref UniformBinsHistogram::normalized().  This scale is
   * chosen because it coincides with that used by the histograms reported by the binning
   * analysis.
   */
  inline SimpleFinalHistogramType simpleFinalHistogram() const {
    tomographer_assert(isFinalized() && "You may only call simpleFinalHistogram() after the runs have been finalized.");
    return _simplefinalhistogram;
  }
  
  /** \brief Return the number of tasks that were run
   *
   * You may only call this function after the results have been finalized (see \ref
   * isFinalized()), i.e. after the task dispatcher has finished executing all tasks.
   */
  inline std::size_t numTasks() const {
    tomographer_assert(isFinalized() && "You may only call numTasks() after the runs have been finalized.");
    return _collected_runtaskresults.size();
  }

  /** \brief Return a list of the resulting report of each run task
   *
   * The result (i.e. the individual task's final report) is stored in a \ref
   * RunTaskResult structure, which is simply a \ref MHRandomWalkTaskResult type.
   *
   * You may only call this function after the results have been finalized (see \ref
   * isFinalized()), i.e. after the task dispatcher has finished executing all tasks.
   */
  inline const RunTaskResultList & collectedRunTaskResults() const {
    tomographer_assert(isFinalized() && "You may only call collectedRunTaskResults() after the runs have been finalized.");
    return _collected_runtaskresults;
  }

  /** \brief Return a list of the resulting report of one specific run task
   *
   * The result (i.e. the individual task's final report) is stored in a \ref
   * RunTaskResult structure, which is simply a \ref MHRandomWalkTaskResult type.
   *
   * \param task_no must be an index such that <code>task_no >= 0 && task_no <
   *        numTasks()</code>.
   *
   * You may only call this function after the results have been finalized (see \ref
   * isFinalized()), i.e. after the task dispatcher has finished executing all tasks.
   */
  inline const RunTaskResult * collectedRunTaskResult(std::size_t task_no) const {
    tomographer_assert(isFinalized() && "You may only call collectedRunTaskResult(std::size_t) after the runs have been finalized.");
    tomographer_assert(task_no < _collected_runtaskresults.size());
    return _collected_runtaskresults[task_no];
  }


  /** \brief Produce a comma-separated-value (CSV) representation of the final histogram
   *
   * The histogram data is written in CSV format on the C++ output stream \a stream.  You
   * may specify the cell separator \a sep (by default a TAB char), the line separator (by
   * default a simple newline), and the precision used when exporting the values.  Numbers
   * are written in scientific format (e.g. <code>1.205115485e-01</code>).
   *
   * The output consists of four columns. Titles are printed on the first line.  The first
   * column holds the values, i.e., the x-axis of the histogram; the second column holds
   * the counts (normalized to the number of samples); the third column holds the error
   * bar on the counts (reliable error bar from binning analysis), and the fourth column
   * holds the naive error bar obtained when we ignore the binning analysis.
   */
  template<typename RealType = double>
  inline void printHistogramCsv(std::ostream & stream, std::string sep = "\t", std::string linesep = "\n",
                                int precision = 10)
  {
    // ### Q: why the RealType template parameter??
    stream << "Value" << sep << "Counts" << sep << "Error" << sep << "SimpleError" << linesep
           << std::scientific << std::setprecision(precision);
    for (int kk = 0; kk < _finalhistogram.bins.size(); ++kk) {
      stream << RealType(_finalhistogram.params.binLowerValue(kk)) << sep
             << RealType(_finalhistogram.bins(kk)) << sep
	     << RealType(_finalhistogram.delta(kk)) << sep
	     << RealType(_simplefinalhistogram.delta(kk)) << linesep;
    }
  }

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
   * readable form, using \ref UniformBinsHistogramWithErrorBars::prettyPrint().
   */
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
      const auto nbins = stats_coll_result.converged_status.size();
      const auto n_conv = stats_coll_result.converged_status
        .cwiseEqual(BinningAnalysisParamsType::CONVERGED).count();
      Eigen::ArrayXi unkn_arr = (stats_coll_result.converged_status
                                 .cwiseEqual(BinningAnalysisParamsType::UNKNOWN_CONVERGENCE))
        .template cast<int>();
      // little heuristic to see whether the "unknown" converged error bars are isolated or not
      const auto n_unknown = unkn_arr.count();
      const auto n_unknown_followingotherunknown
        = unkn_arr.segment(0,nbins-1).cwiseProduct(unkn_arr.segment(1,nbins-1)).count();
      const auto n_unknown_isolated = n_unknown - n_unknown_followingotherunknown;
      const auto n_notconv = stats_coll_result.converged_status
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
    
  //! In compliance with \ref pageInterfaceResultsCollector
  template<typename Cnt, typename CData>
  inline void init(Cnt num_total_runs, Cnt /*n_chunk*/, const CData * pcdata)
  {
    tomographer_assert(!isFinalized() && "init() called after results have been finalized!");

    _collected_runtaskresults.resize(num_total_runs, NULL);
    _finalhistogram.reset(pcdata->histogram_params);
    _simplefinalhistogram.reset(pcdata->histogram_params);
  }

  //! In compliance with \ref pageInterfaceResultsCollector
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
	histogramPrettyPrint<SimpleScaledHistogramType>(str, stats_coll_result.hist);
      });
    _simplefinalhistogram.addHistogram(stats_coll_result.hist);

    _collected_runtaskresults[task_no] = new RunTaskResult(std::move(taskresult));
    
    logger.debug("done.");
  }

  //! In compliance with \ref pageInterfaceResultsCollector
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
 * This class should be used in conjunction with a \ref ResultsCollectorSimple or \ref
 * ResultsCollectorWithBinningAnalysis.  For convenience, you can query the member \ref
 * ResultsCollectorType to get the correct results collector type according to whether we
 * are using a binning analysis and with the correct parameters type.
 *
 * We provide some useful typedefs, as well as the \ref createStatsCollector() required by
 * the \ref pageInterfaceMHRandomWalkTaskCData.
 *
 * \tparam ValueCalculator the value calculator type you wish to use; defining which value
 *         you are interested in collecting a histogram of during the random walk;
 *
 * \tparam UseBinningAnalysis whether or not to use a binning analysis to obtain reliable
 *         error bars during the random walk.
 *
 * \tparam IterCountIntType the integer type to use for counting iterations.
 *
 * \tparam StepRealType the real type to use for representing a step size in the random walk.
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
 *   : public Tomographer::MHRWTasks::ValueHistogramTasks::CDataBase< ... >
 * {
 *   typedef Tomographer::MHRWTasks::ValueHistogramTasks::CDataBase< ... > Base;
 *
 *   TOMOGRAPHER_ENABLED_IF(!BinningAnalysisEnabled)
 *   MyCData(ValueCalculator valcalc, int base_seed, ...)
 *     : Base(valcalc,
 *            typename Base::HistogramParams(...),
 *            typename Base::MHRWParamsType(...),
 *            base_seed)
 *   { ... }
 *
 *   TOMOGRAPHER_ENABLED_IF(BinningAnalysisEnabled)
 *   MyCData(ValueCalculator valcalc, int base_seed, int binning_analysis_num_levels, ...)
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
template<typename ValueCalculator_, bool UseBinningAnalysis_ = true,
	 typename IterCountIntType_ = int, typename StepRealType_ = double,
	 typename CountRealType_ = double, typename HistCountIntType_ = IterCountIntType_>
TOMOGRAPHER_EXPORT struct CDataBase
  : public MHRWTasks::CDataBase<IterCountIntType_, StepRealType_>,
    public virtual Tools::NeedOwnOperatorNew<ValueCalculator_>::ProviderType
{
  //! The MHRWTasks::CDataBase base class
  typedef MHRWTasks::CDataBase<IterCountIntType_, StepRealType_> Base;

  //! The integer type which serves to count the number of iterations (see \ref MHRWParams)
  typedef typename Base::IterCountIntType IterCountIntType;
  //! The real type which serves to describe the step size of the random walk (typically \c double)
  typedef typename Base::StepRealType StepRealType;

  //! The integer counting type in our underlying raw histogram type
  typedef HistCountIntType_ HistCountIntType;

  //! The class which calculates the value we are collecting a histogram of (\ref pageInterfaceValueCalculator compliant)
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
  typedef typename tomo_internal::histogram_types<CDataBase,UseBinningAnalysis>::MHRWStatsCollectorResultType
    MHRWStatsCollectorResultType;

  /** \brief The histogram type reported by the task
   *
   * Depending on whether we are using a binning analysis, this histogram type will have
   * error bars.
   *
   * It is either:
   *
   *   - (no binning analysis): a UniformBinsHistogram of appropriate parameters type
   *
   *   - (with binning analysis): a UniformBinsHistogramWithErrorBars of appropriate
   *     parameters types.  This is in fact \ref
   *     ValueHistogramWithBinningMHRWStatsCollectorParams::HistogramType.
   */
  typedef typename tomo_internal::histogram_types<CDataBase,UseBinningAnalysis>::HistogramType HistogramType;

  /** \brief The appropriate parameters type for the histogram reported by the task
   */
  typedef typename tomo_internal::histogram_types<CDataBase,UseBinningAnalysis>::HistogramParams HistogramParams;

  //! Type for the parameters of the random walk.
  typedef MHRWParams<IterCountIntType, StepRealType> MHRWParamsType;
 

  //! Constructor (only for without binning analysis)
  TOMOGRAPHER_ENABLED_IF(!UseBinningAnalysis)
  CDataBase(const ValueCalculator & valcalc_, HistogramParams histogram_params_,
	    MHRWParamsType p, int base_seed = 0)
    : Base(std::move(p), base_seed), valcalc(valcalc_), histogram_params(histogram_params_),
      binningNumLevels()
  {
  }

  //! Constructor (only for with binning analysis)
  TOMOGRAPHER_ENABLED_IF(UseBinningAnalysis)
  CDataBase(const ValueCalculator & valcalc_, HistogramParams histogram_params_, int binning_num_levels_,
	    MHRWParamsType p, int base_seed = 0)
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
   * This method is provided in compliance with the \ref
   * pageInterfaceMHRandomWalkTaskCData for \ref MHRandomWalkTask.
   */
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

  /** \brief Create the stats collector (with binning analysis)
   *
   * This method is provided in compliance with the \ref
   * pageInterfaceMHRandomWalkTaskCData for \ref MHRandomWalkTask.
   */
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

  /** \brief Helper to get the results collector type
   *
   * You may use the following syntax to get the correct results collector type to use
   * with this cdata (with or without binning analysis and correct parameter types):
   * \code
   *   typedef MyCData::ResultsCollectorType<LoggerType>::Type  ResultsCollectorType;
   * \endcode
   * where \c MyCData is your CData class.
   *
   * This class also provides the task result type for convenience:
   * \code
   *   typedef MyCData::ResultsCollectorType<LoggerType>::RunTaskResultType  RunTaskResultType;
   * \endcode
   *
   */
  template<typename LoggerType>
  struct ResultsCollectorType {
    typedef
#ifndef TOMOGRAPHER_PARSED_BY_DOXYGEN
    typename
    tomo_internal::ResultsCollectorTypeHelper<CDataBase<ValueCalculator,UseBinningAnalysis,IterCountIntType,StepRealType,CountRealType,HistCountIntType>,
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
	 typename IterCountIntType_, typename StepRealType_,
	 typename CountRealType_, typename HistCountIntType_>
constexpr bool CDataBase<ValueCalculator_,UseBinningAnalysis_,IterCountIntType_,StepRealType_,CountRealType_,HistCountIntType_>::UseBinningAnalysis;









} // namespace ValueHistogramTasks
} // namespace MHRWTasks
} // namespace Tomographer


#endif
