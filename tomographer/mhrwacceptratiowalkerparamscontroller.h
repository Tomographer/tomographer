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

#ifndef _TOMOGRAPHER_MHRWACCEPTRATIOWALKERPARAMSCONTROLLER_H
#define _TOMOGRAPHER_MHRWACCEPTRATIOWALKERPARAMSCONTROLLER_H

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



/** \file mhrwacceptratiowalkerparamscontroller.h
 * \brief Tools for automatically and dynamically adjusting the step size of the random walk
 *
 * See \ref Tomographer::MHRWAcceptRatioWalkerParamsController
 */


namespace Tomographer {


/** \brief Default parameters for MHRWAcceptRatioWalkerParamsController
 */
namespace MHRWAcceptRatioWalkerParamsControllerDefaults {

static constexpr double AcceptableAcceptanceRatioMin = 
  MHRWAcceptanceRatioRecommendedMin;
static constexpr double AcceptableAcceptanceRatioMax =
  MHRWAcceptanceRatioRecommendedMax;
static constexpr double DesiredAcceptanceRatioMin = 
  (0.9  * AcceptableAcceptanceRatioMin + 0.1  * AcceptableAcceptanceRatioMax);
static constexpr double DesiredAcceptanceRatioMax =
  (0.35 * AcceptableAcceptanceRatioMin + 0.65 * AcceptableAcceptanceRatioMax);
static constexpr double EnsureNThermFixedParamsFraction = 0.5;

} // MHRWAcceptRatioWalkerParamsControllerDefaults



/** \brief Basic functionality for a \ref pageInterfaceMHRWController to adjust \a
 *         MHWalkerParams based on keeping the acceptance ratio within a required range
 *
 * The \a MHRWAcceptanceRatioBasedParamsAdjusterType must be a \ref
 * pageInterfaceMHRWAcceptanceRatioBasedParamsAdjuster compliant type.
 *
 * See \ref MHRWStepSizeController for an example.
 */
template<typename MHRWAcceptanceRatioBasedParamsAdjusterType_,
         typename MHRWMovingAverageAcceptanceRatioStatsCollectorType_,
         typename BaseLoggerType_ = Logger::VacuumLogger,
         typename IterCountIntType_ = int>
class TOMOGRAPHER_EXPORT MHRWAcceptRatioWalkerParamsController
{
public:
  enum { AdjustmentStrategy = MHRWControllerAdjustEveryIterationWhileThermalizing };
  
  typedef MHRWAcceptanceRatioBasedParamsAdjusterType_ MHRWAcceptanceRatioBasedParamsAdjusterType;
  typedef MHRWMovingAverageAcceptanceRatioStatsCollectorType_
    MHRWMovingAverageAcceptanceRatioStatsCollectorType;
  typedef BaseLoggerType_ BaseLoggerType;
  typedef IterCountIntType_ IterCountIntType;

private:
  const MHRWMovingAverageAcceptanceRatioStatsCollectorType & accept_ratio_stats_collector;
  
  MHRWAcceptanceRatioBasedParamsAdjusterType & params_adjuster;

  const double desired_accept_ratio_min;
  const double desired_accept_ratio_max;
  const double acceptable_accept_ratio_min;
  const double acceptable_accept_ratio_max;
  
  IterCountIntType orig_n_therm;
  IterCountIntType last_corrected_unacceptable_iter_k;
  
  /** \brief Ensure that at least this fraction of the original \a n_therm sweeps are
   *         performed at constant (converged) parameters before completing the
   *         thermalization runs
   */
  const double ensure_n_therm_fixed_params_fraction;
  
  Logger::LocalLogger<BaseLoggerType> llogger;

public:
  MHRWAcceptRatioWalkerParamsController(
    const MHRWMovingAverageAcceptanceRatioStatsCollectorType & accept_ratio_stats_collector_,
    BaseLoggerType & baselogger_,
    MHRWAcceptanceRatioBasedParamsAdjusterType & params_adjuster_,
    double desired_accept_ratio_min_ =
      MHRWAcceptRatioWalkerParamsControllerDefaults::DesiredAcceptanceRatioMin,
    double desired_accept_ratio_max_ =
      MHRWAcceptRatioWalkerParamsControllerDefaults::DesiredAcceptanceRatioMax,
    double acceptable_accept_ratio_min_ =
      MHRWAcceptRatioWalkerParamsControllerDefaults::AcceptableAcceptanceRatioMin,
    double acceptable_accept_ratio_max_ =
      MHRWAcceptRatioWalkerParamsControllerDefaults::AcceptableAcceptanceRatioMax,
    double ensure_n_therm_fixed_params_fraction_ = 
      MHRWAcceptRatioWalkerParamsControllerDefaults::EnsureNThermFixedParamsFraction
    )
  : accept_ratio_stats_collector(accept_ratio_stats_collector_),
    params_adjuster(params_adjuster_),
    desired_accept_ratio_min(desired_accept_ratio_min_),
    desired_accept_ratio_max(desired_accept_ratio_max_),
    acceptable_accept_ratio_min(acceptable_accept_ratio_min_),
    acceptable_accept_ratio_max(acceptable_accept_ratio_max_),
    orig_n_therm(0),
    last_corrected_unacceptable_iter_k(0),
    ensure_n_therm_fixed_params_fraction(ensure_n_therm_fixed_params_fraction_),
    llogger("Tomographer::MHRWAcceptRatioWalkerParamsController", baselogger_)
  {
  }
  

  inline double desiredAcceptRatioMin() const { return desired_accept_ratio_min; }
  inline double desiredAcceptRatioMax() const { return desired_accept_ratio_max; }
  inline double acceptableAcceptRatioMin() const { return acceptable_accept_ratio_min; }
  inline double acceptableAcceptRatioMax() const { return acceptable_accept_ratio_max; }

  inline double ensureNThermFixedParamsFraction() const { return ensure_n_therm_fixed_params_fraction; }

  inline IterCountIntType originalNTherm() const { return orig_n_therm; }

  
  template<typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType>
  inline void init(MHRWParamsType & params, const MHWalker & mhwalker, const MHRandomWalkType & mhrw)
  {
    auto logger = llogger.subLogger(TOMO_ORIGIN) ;

    orig_n_therm = params.n_therm;

    params_adjuster.initParams(params, mhwalker, mhrw) ;
  }

  template<bool IsThermalizing, bool IsAfterSample,
           typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType,
           TOMOGRAPHER_ENABLED_IF_TMPL(IsThermalizing)> // Only while thermalizing
  inline void adjustParams(MHRWParamsType & params, const MHWalker & mhwalker,
                           IterCountIntType iter_k, const MHRandomWalkType & mhrw)
  {
    auto logger = llogger.subLogger(TOMO_ORIGIN) ;

    logger.longdebug([&](std::ostream & stream) {
        stream << "cur params = " << params << " and accept_ratio = "
               << accept_ratio_stats_collector.movingAverageAcceptanceRatio();
      });

    // only adjust every moving-avg-accept-ratio-buffer-size
    if ( ! accept_ratio_stats_collector.hasMovingAverageAcceptanceRatio() ||
         (iter_k % (IterCountIntType)accept_ratio_stats_collector.bufferSize()) != 0 ) {
      return;
    }

    logger.longdebug([&](std::ostream & stream) {
        stream << "will consider correction. iter_k = " << iter_k
               << ", last_corrected_unacceptable_iter_k = " << last_corrected_unacceptable_iter_k;
      });

    const double accept_ratio = accept_ratio_stats_collector.movingAverageAcceptanceRatio();

    if (!std::isfinite(accept_ratio) ||   // no statistics gathered yet -- just continue at this pace
        (accept_ratio >= desired_accept_ratio_min && // or accept_ratio already in the desired interval
         accept_ratio <= desired_accept_ratio_max)) {
      return;
    }

    logger.longdebug([&](std::ostream & stream) {
        stream << "will adjust.";
      });


    if (accept_ratio < acceptable_accept_ratio_min || accept_ratio > acceptable_accept_ratio_max) {
      last_corrected_unacceptable_iter_k = iter_k;
    }

    params_adjuster.adjustParamsForAcceptRatio( params, accept_ratio, *this, mhwalker, iter_k, mhrw ) ;
  }

  template<typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType>
  bool allowDoneThermalization(const MHRWParamsType & params, const MHWalker & /*mhwalker*/,
                               IterCountIntType iter_k, const MHRandomWalkType & /*mhrw*/)
  {
    auto logger = llogger.subLogger(TOMO_ORIGIN);

    const double accept_ratio = accept_ratio_stats_collector.movingAverageAcceptanceRatio();
    logger.longdebug([&](std::ostream & stream) {
        stream << "iter_k=" << iter_k << ", accept_ratio=" << accept_ratio;
      });
    if (!std::isfinite(accept_ratio) ||
        accept_ratio < desired_accept_ratio_min || accept_ratio > desired_accept_ratio_max) {
      logger.longdebug("not allowing, based on accept_ratio=%.4g", accept_ratio); 
      return false;
    }

    if ((iter_k - last_corrected_unacceptable_iter_k)
        < params.n_sweep*(ensure_n_therm_fixed_params_fraction*orig_n_therm)) {
      logger.longdebug([&](std::ostream & stream) {
          stream << "not allowing, based on iter_k=" << iter_k
                 << " & last_corrected_unacceptable_iter_k=" << last_corrected_unacceptable_iter_k; 
        }) ;
      return false; // not passed enough thermalizing iterations since last correction
    }

    logger.longdebug("all fine, can return"); 
    return true;
  }

  template<typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType>
  bool allowDoneRuns(const MHRWParamsType & /*params*/, const MHWalker & /*mhwalker*/,
                     IterCountIntType /*iter_k*/, const MHRandomWalkType & /*mhrw*/)
  {
    return true; // do whatever you like
  }

  template<typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType>
  inline void thermalizingDone(MHRWParamsType & /*params*/, const MHWalker & /*mhwalker*/,
                               const MHRandomWalkType & /*mhrw*/) const
  {
  }

  template<typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType>
  inline void done(MHRWParamsType & /*params*/, const MHWalker & /*mhwalker*/,
                   const MHRandomWalkType & /*mhrw*/) const
  {
  }
};





} // namespace Tomographer



#endif
