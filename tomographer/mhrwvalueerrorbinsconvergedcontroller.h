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

#ifndef _TOMOGRAPHER_MHRWVALUEERRORBINSCONVERGEDCONTROLLER_H
#define _TOMOGRAPHER_MHRWVALUEERRORBINSCONVERGEDCONTROLLER_H

#include <cstddef>
#include <cmath>
#include <cstdlib>

#include <algorithm> // std::max
#include <limits>
#include <random>
#include <iomanip>

#include <tomographer/tools/loggers.h>
#include <tomographer/tools/fmt.h>
#include <tomographer/tools/cxxutil.h>
#include <tomographer/tools/needownoperatornew.h>
#include <tomographer/mhrw.h>
#include <tomographer/mhrwstatscollectors.h>


/** \file mhrwvalueerrorbinsconvergedcontroller.h
 *
 * \brief Tools for automatically stopping the random walk when enough samples have been
 *        taken to make all binning analysis error bars converge for each histogram bin
 *
 * See \ref Tomographer::MHRWValueErrorBinsConvergedController.
 */


namespace Tomographer {


/** \brief A \ref pageInterfaceMHRWController which stops the random walk after enough
 *         samples have been taken to make all bins of a histogram have converged error
 *         bars
 *
 * \todo ...... DOC ..............
 */
template<typename ValueHistogramWithBinningMHRWStatsCollectorType_,
         typename IterCountIntType_,
         typename BaseLoggerType_>
class TOMOGRAPHER_EXPORT MHRWValueErrorBinsConvergedController
{
public:
  // we never have to adjust the params, we just forbid from stopping too early in the
  // allowDoneRuns() callback
  enum { AdjustmentStrategy = MHRWControllerDoNotAdjust };
  
  typedef ValueHistogramWithBinningMHRWStatsCollectorType_
    ValueHistogramWithBinningMHRWStatsCollectorType;

  typedef IterCountIntType_ IterCountIntType;

  typedef BaseLoggerType_ BaseLoggerType;

private:
  const ValueHistogramWithBinningMHRWStatsCollectorType & value_stats_collector;
  
  const IterCountIntType check_frequency_sweeps;

  IterCountIntType last_forbidden_iter_number;

  const Eigen::Index max_allowed_unknown;
  const Eigen::Index max_allowed_unknown_notisolated;
  const Eigen::Index max_allowed_not_converged;

  const double max_add_run_iters;

  Logger::LocalLogger<BaseLoggerType> llogger;

public:
  /** \brief Constructor
   *
   * Note: You may disable the controller entirely by setting \a check_frequency_sweeps=0.
   *
   */
  MHRWValueErrorBinsConvergedController(
      const ValueHistogramWithBinningMHRWStatsCollectorType & value_stats_collector_,
      BaseLoggerType & baselogger_,
      IterCountIntType check_frequency_sweeps_ = 1024,
      Eigen::Index max_allowed_unknown_ = 0,
      Eigen::Index max_allowed_unknown_notisolated_ = 0,
      Eigen::Index max_allowed_not_converged_ = 0,
      double max_add_run_iters_ = 1.5
      )
    : value_stats_collector(value_stats_collector_),
      check_frequency_sweeps( maybeadjust_check_freq_seeps(check_frequency_sweeps_,
                                                           value_stats_collector,
                                                           baselogger_) ),
      last_forbidden_iter_number(0),
      max_allowed_unknown(max_allowed_unknown_),
      max_allowed_unknown_notisolated(max_allowed_unknown_notisolated_),
      max_allowed_not_converged(max_allowed_not_converged_),
      max_add_run_iters(max_add_run_iters_),
      llogger("Tomographer::MHRWValueErrorBinsConvergedAdjuster", baselogger_)
  {
    const auto binning_samples_size = value_stats_collector.getBinningAnalysis().effectiveSampleSize();
    if ((check_frequency_sweeps % binning_samples_size) != 0) {
      llogger.warning([&](std::ostream & stream) {
          stream << "check_frequency_sweeps (="<<check_frequency_sweeps_<<") is not a multiple of the "
                 << "binning analysis sample size (="<<binning_samples_size<<"), this could lead to samples "
                 << "being ignored by the error analysis (avoid this)!";
        }) ;
    }
  }
  
  template<typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType>
  inline void init(MHRWParamsType & /*params*/, const MHWalker & /*mhwalker*/,
                   const MHRandomWalkType & /*mhrw*/) const
  {
  }

  template<typename MHRWParamsType, typename MHWalker, typename CountIntType, typename MHRandomWalkType>
  bool allowDoneThermalization(const MHRWParamsType & /*params*/, const MHWalker & /*mhwalker*/,
                               CountIntType /*iter_k*/, const MHRandomWalkType & /*mhrw*/) const
  {
    return true;
  }

  template<typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType>
  bool allowDoneRuns(const MHRWParamsType & params, const MHWalker & /*mhwalker*/,
                     IterCountIntType iter_k, const MHRandomWalkType & /*mhrw*/)
  {

    if (check_frequency_sweeps == 0) {
      // controller is manually disabled by setting check_frequency_sweeps=0
      return true;
    }

    auto logger = llogger.subLogger(TOMO_ORIGIN);

    if (last_forbidden_iter_number > 0 &&
        (iter_k-last_forbidden_iter_number) < params.n_sweep*check_frequency_sweeps) {
      // not enough new samples since last time we rejected to finish the random walk
      return false;
    }

    if (iter_k % (params.n_sweep*check_frequency_sweeps) != 0) {
      // Make sure we only interrupt on an exact multiple of
      // check_frequency_sweeps.  This is needed because we want to make sure
      // the binning analysis has processed exactly all the samples
      return false;
    }

    logger.longdebug([&](std::ostream & stream) {
        stream << "params=" << params << ", iter_k=" << iter_k
               << ", max_add_run_iters=" << max_add_run_iters;
      });

    // if we have exceeded the maximum number of run iterations, emit a warning
    // and stop (max_add_run_iters < 0 disables this feature)
    if (max_add_run_iters > 0 && iter_k > max_add_run_iters * params.n_run * params.n_sweep) {
      logger.warning([&](std::ostream & stream) {
          stream << "Ending random walk after reaching maximum sweep number "
                 << iter_k/params.n_sweep <<" ("
                 << 100.0*iter_k/(params.n_sweep*params.n_run) << "% of set run length)";
        }) ;
      return true;
    }

    // re-check if the error bars have converged

    // do a convergence analysis of the error bars
    const auto& binning_analysis = value_stats_collector.getBinningAnalysis();
    const auto binmeans = value_stats_collector.binMeans();

    const auto error_levels = binning_analysis.calcErrorLevels(binmeans);
    const auto conv_status = binning_analysis.determineErrorConvergence(error_levels);

    const auto conv_summary = BinningErrorBarConvergenceSummary::fromConvergedStatus(conv_status);

    logger.longdebug([&](std::ostream & stream) { stream << "Convergence summary = " << conv_summary; }) ;

    if (conv_summary.n_not_converged > max_allowed_not_converged ||
        conv_summary.n_unknown > max_allowed_unknown ||
        (conv_summary.n_unknown-conv_summary.n_unknown_isolated) > max_allowed_unknown_notisolated) {
      // too many unconverged error bars, continue running
      last_forbidden_iter_number = iter_k;
      return false;
    }

    // all ok
    return true;
  }

  template<typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType>
  inline void thermalizingDone(const MHRWParamsType & /*params*/, const MHWalker & /*mhwalker*/,
                               const MHRandomWalkType & /*mhrw*/) const
  {
  }

  template<typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType>
  inline void done(MHRWParamsType & /*params*/, const MHWalker & /*mhwalker*/,
                   const MHRandomWalkType & /*mhrw*/) const
  {
  }

private:
  // ensure that check_frequency_sweeps is a multiple of the binning analysis sample size
  inline static IterCountIntType maybeadjust_check_freq_seeps(
      IterCountIntType check_frequency_sweeps_,
      const ValueHistogramWithBinningMHRWStatsCollectorType & valstats,
      BaseLoggerType & logger)
  {
    if (check_frequency_sweeps_ == 0) {
      return 0; // all ok
    }
    IterCountIntType binning_samples_size = (IterCountIntType)valstats.getBinningAnalysis().effectiveSampleSize();
    if ((check_frequency_sweeps_ % binning_samples_size) == 0) {
      // all ok
      return check_frequency_sweeps_;
    }
    // ensure that `check_frequency_sweeps_' is a multiple of `binning_samples_size'
    IterCountIntType corrected = ( check_frequency_sweeps_ / binning_samples_size + 1) * binning_samples_size;
    logger.debug("Tomographer::MHRWValueErrorBinsConvergedController", [&](std::ostream & stream) {
        stream << "check_frequency_sweeps (="<<check_frequency_sweeps_<<") is not a multiple of the "
          "binning analysis sample size (="<<binning_samples_size<<"), correcting to " << corrected;
          });
    return corrected;
  }
};


/** \brief Convenience function to create a MHRWValueErrorBinsConvergedController (using
 *         template argument deduction)
 *
 * \since Added in %Tomographer 5.0
 */
template<typename IterCountIntType_ = int,
         // these types are deduced from the args anyway:
         typename ValueHistogramWithBinningMHRWStatsCollectorType_ = void,
         typename BaseLoggerType_ = void>
inline MHRWValueErrorBinsConvergedController<ValueHistogramWithBinningMHRWStatsCollectorType_,
                                             IterCountIntType_,
                                             BaseLoggerType_>
mkMHRWValueErrorBinsConvergedController(
    const ValueHistogramWithBinningMHRWStatsCollectorType_ & value_stats_collector_,
    BaseLoggerType_ & baselogger_,
    IterCountIntType_ check_frequency_sweeps_ = 1024,
    Eigen::Index max_allowed_unknown_ = 0,
    Eigen::Index max_allowed_unknown_notisolated_ = 0,
    Eigen::Index max_allowed_not_converged_ = 0,
    double max_add_run_iters = 1.5
    )
{
  return MHRWValueErrorBinsConvergedController<ValueHistogramWithBinningMHRWStatsCollectorType_,
                                               IterCountIntType_,
                                               BaseLoggerType_>(
                                                   value_stats_collector_,
                                                   baselogger_,
                                                   check_frequency_sweeps_,
                                                   max_allowed_unknown_,
                                                   max_allowed_unknown_notisolated_,
                                                   max_allowed_not_converged_,
                                                   max_add_run_iters
                                                   ) ;
}




} // namespace Tomographer



#endif
