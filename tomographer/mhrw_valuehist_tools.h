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
 */


namespace Tomographer {
namespace MHRWTasks {
namespace ValueHistogramTools {



namespace tomo_internal {
template<typename CDataBaseType, bool UseBinningAnalysis>
struct histogram_types {// version WITHOUT binning analysis:
  typedef Histogram<typename CDataBaseType::ValueCalculator::ValueType,
                               typename CDataBaseType::HistCountIntType> HistogramType;
  /// we know that ValueHistogramMHRWStatsCollector<ValueCalculator,...,HistogramType>::ResultType is HistogramType
  typedef HistogramType ValueStatsCollectorResultType;
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
  typedef typename BinningMHRWStatsCollectorParams::Result ValueStatsCollectorResultType;
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
 * We provide some useful typedefs, as well as the \ref createStatsCollector() required by
 * the \ref pageInterfaceMHRandomWalkTaskCData.
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
 * \tparam MHWalkerParams the real type to use for representing a step size in the random walk.
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
         typename MHWalkerParams_ = MHWalkerParamsStepSize<double>, typename IterCountIntType_ = int,
	 typename CountRealType_ = double, typename HistCountIntType_ = IterCountIntType_>
TOMOGRAPHER_EXPORT struct CDataBase
  : public MHRWTasks::CDataBase<MHWalkerParams_, IterCountIntType_>,
    public virtual Tools::NeedOwnOperatorNew<ValueCalculator_>::ProviderType
{
  //! The MHRWTasks::CDataBase base class
  typedef MHRWTasks::CDataBase<MHWalkerParams_, IterCountIntType_> Base;

  //! The integer type which serves to count the number of iterations (see \ref MHRWParams)
  typedef typename Base::IterCountIntType IterCountIntType;
  //! The real type which serves to describe the step size of the random walk (typically \c double)
  typedef typename Base::MHWalkerParams MHWalkerParams;

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
  typedef typename tomo_internal::histogram_types<CDataBase,UseBinningAnalysis>::ValueStatsCollectorResultType
    ValueStatsCollectorResultType;

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
  typedef typename tomo_internal::histogram_types<CDataBase,UseBinningAnalysis>::HistogramType HistogramType;

  /** \brief The appropriate parameters type for the histogram reported by the task
   */
  typedef typename tomo_internal::histogram_types<CDataBase,UseBinningAnalysis>::HistogramParams HistogramParams;

  //! Type for the parameters of the random walk.
  typedef MHRWParams<MHWalkerParams, IterCountIntType> MHRWParamsType;
 

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
   * This method is provided in compliance with the \ref
   * pageInterfaceMHRandomWalkTaskCData for \ref MHRandomWalkTask.
   */
  template<typename LoggerType, TOMOGRAPHER_ENABLED_IF_TMPL(UseBinningAnalysis)>
  inline ValueHistogramWithBinningMHRWStatsCollector<
    typename tomo_internal::histogram_types<CDataBase, true>::BinningMHRWStatsCollectorParams,
    LoggerType>
  createValueStatsCollector(LoggerType & logger) const
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

};
// define static members:
template<typename ValueCalculator_, bool UseBinningAnalysis_,
         typename MHWalkerParams_, typename IterCountIntType_,
	 typename CountRealType_, typename HistCountIntType_>
constexpr bool CDataBase<ValueCalculator_,UseBinningAnalysis_,MHWalkerParams_,IterCountIntType_,CountRealType_,HistCountIntType_>::UseBinningAnalysis;







} // namespace ValueHistogramTools
} // namespace MHRWTasks
} // namespace Tomographer


#endif
