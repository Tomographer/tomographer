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

#ifndef _TOMOGRAPHER_MHRWSTEPSIZECONTROLLER_H
#define _TOMOGRAPHER_MHRWSTEPSIZECONTROLLER_H

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


/** \file mhrwstepsizecontroller.h
 * \brief Tools for automatically and dynamically adjusting the step size of the random walk
 *
 * See \ref Tomographer::MHRWStepSizeController
 */


namespace Tomographer {




/** \brief Default parameters for MHRWStepSizeController
 */
namespace MHRWStepSizeControllerDefaults {

static constexpr double AcceptableAcceptanceRatioMin = 
  MHRWAcceptanceRatioRecommendedMin;
static constexpr double AcceptableAcceptanceRatioMax =
  MHRWAcceptanceRatioRecommendedMax;
static constexpr double DesiredAcceptanceRatioMin = 
  (0.9  * AcceptableAcceptanceRatioMin + 0.1  * AcceptableAcceptanceRatioMax);
static constexpr double DesiredAcceptanceRatioMax =
  (0.35 * AcceptableAcceptanceRatioMin + 0.65 * AcceptableAcceptanceRatioMax);
static constexpr double EnsureNThermFixedParamsFraction = 0.5;
} // MHRWStepSizeControllerDefaults





/** \brief A \ref pageInterfaceMHRWController dynamically adjusting the step size to keep
 *         a good acceptance ratio
 *
 */
template<typename MHRWMovingAverageAcceptanceRatioStatsCollectorType_,
         typename BaseLoggerType_ = Logger::VacuumLogger,
         typename StepRealType_ = double,
         typename IterCountIntType_ = int>
class TOMOGRAPHER_EXPORT MHRWStepSizeController
{
public:
  enum { AdjustmentStrategy = MHRWControllerAdjustEveryIterationWhileThermalizing };
  
  typedef MHRWMovingAverageAcceptanceRatioStatsCollectorType_
    MHRWMovingAverageAcceptanceRatioStatsCollectorType;
  typedef BaseLoggerType_ BaseLoggerType;
  typedef StepRealType_ StepRealType;
  typedef IterCountIntType_ IterCountIntType;

private:
  const MHRWMovingAverageAcceptanceRatioStatsCollectorType & accept_ratio_stats_collector;
  
  const double desired_accept_ratio_min;
  const double desired_accept_ratio_max;
  const double acceptable_accept_ratio_min;
  const double acceptable_accept_ratio_max;
  
  IterCountIntType last_corrected_unacceptable_iter_k;
  StepRealType last_set_step_size;
  IterCountIntType orig_n_therm;
  StepRealType orig_step_times_sweep;
  
  /** \brief Ensure that at least this fraction of the original \a n_therm sweeps are
   *         performed at constant (converged) parameters before completing the
   *         thermalization runs
   */
  const double ensure_n_therm_fixed_params_fraction;
  
  Logger::LocalLogger<BaseLoggerType> llogger;

public:
  MHRWStepSizeController(
    const MHRWMovingAverageAcceptanceRatioStatsCollectorType & accept_ratio_stats_collector_,
    BaseLoggerType & baselogger_,
    double desired_accept_ratio_min_ =
      MHRWStepSizeControllerDefaults::DesiredAcceptanceRatioMin,
    double desired_accept_ratio_max_ =
      MHRWStepSizeControllerDefaults::DesiredAcceptanceRatioMax,
    double acceptable_accept_ratio_min_ =
      MHRWStepSizeControllerDefaults::AcceptableAcceptanceRatioMin,
    double acceptable_accept_ratio_max_ =
      MHRWStepSizeControllerDefaults::AcceptableAcceptanceRatioMax,
    double ensure_n_therm_fixed_params_fraction_ = 
      MHRWStepSizeControllerDefaults::EnsureNThermFixedParamsFraction
    )
  : accept_ratio_stats_collector(accept_ratio_stats_collector_),
    desired_accept_ratio_min(desired_accept_ratio_min_),
    desired_accept_ratio_max(desired_accept_ratio_max_),
    acceptable_accept_ratio_min(acceptable_accept_ratio_min_),
    acceptable_accept_ratio_max(acceptable_accept_ratio_max_),
    last_corrected_unacceptable_iter_k(0),
    last_set_step_size(std::numeric_limits<StepRealType>::quiet_NaN()),
    orig_n_therm(0),
    orig_step_times_sweep(0),
    ensure_n_therm_fixed_params_fraction(ensure_n_therm_fixed_params_fraction_),
    llogger("Tomographer::MHRWStepSizeController", baselogger_)
  {
  }
      
  
  template<typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType>
  inline void init(MHRWParamsType & params, const MHWalker & /*mhwalker*/, const MHRandomWalkType & /*mhrw*/)
  {
    auto logger = llogger.subLogger(TOMO_ORIGIN) ;

    orig_n_therm = params.n_therm;
    if (std::isfinite(params.mhwalker_params.step_size) && 
        params.mhwalker_params.step_size > 0) {
      // valid step size
      orig_step_times_sweep = params.n_sweep * params.mhwalker_params.step_size;
    } else {
      // invalid step size
      const StepRealType default_start_step_size = 0.01;
      logger.debug([&](std::ostream & stream) {
          stream << "Invalid step_size = " << params.mhwalker_params.step_size
                 << ", set default of = " << default_start_step_size;
        });
      params.mhwalker_params.step_size = default_start_step_size;
      params.n_sweep = (typename MHRWParamsType::CountIntType)(StepRealType(1)/default_start_step_size) + 1;
      orig_step_times_sweep = 1;
    }
  }

  template<bool IsThermalizing, bool IsAfterSample,
           typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType,
           TOMOGRAPHER_ENABLED_IF_TMPL(IsThermalizing)> // Only while thermalizing
  inline void adjustParams(MHRWParamsType & params, const MHWalker & /*mhwalker*/,
                           IterCountIntType iter_k, const MHRandomWalkType & /*mhrw*/)
  {
    auto logger = llogger.subLogger(TOMO_ORIGIN) ;

    logger.longdebug([&](std::ostream & stream) {
        stream << "cur params = " << params << " and accept_ratio = "
               << accept_ratio_stats_collector.movingAverageAcceptanceRatio();
      });

    // // only adjust every max(sweep,moving-avg-accept-ratio-buffer-size)
    // if ( ! accept_ratio_stats_collector.hasMovingAverageAcceptanceRatio() ||
    //      (iter_k % std::max((IterCountIntType)params.n_sweep,
    //                         (IterCountIntType)accept_ratio_stats_collector.bufferSize())) != 0 ) {
    //   return;
    // }
    // only adjust every moving-avg-accept-ratio-buffer-size
    if ( ! accept_ratio_stats_collector.hasMovingAverageAcceptanceRatio() ||
         (iter_k % (IterCountIntType)accept_ratio_stats_collector.bufferSize()) != 0 ) {
      return;
    }

    logger.longdebug([&](std::ostream & stream) {
        stream << "will consider correction.  " <<
          ", last_corrected_unacceptable_iter_k = " << last_corrected_unacceptable_iter_k;
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

    // new step size -- guess a slight increase or decrease depending on too high or too low

    StepRealType new_step_size = params.mhwalker_params.step_size;

    // accept ratio too high -- increase step size
    if (accept_ratio >= 2*desired_accept_ratio_max) {
      new_step_size *= 1.5;
    } else if (accept_ratio >= 1.3*desired_accept_ratio_max) {
      new_step_size *= 1.2;
    } else if (accept_ratio >= desired_accept_ratio_max) {
      new_step_size *= 1.05;
    } else if (accept_ratio <= 0.5*desired_accept_ratio_min) {
      new_step_size *= 0.5;
    } else if (accept_ratio <= 0.75*desired_accept_ratio_min) {
      new_step_size *= 0.8;
    } else {// if (accept_ratio <= desired_accept_ratio_min
      new_step_size *= 0.95;
    }

    logger.longdebug([&](std::ostream & stream) {
        stream << "blind guess corrected step_size to " << new_step_size;
      });

    _adjust_step_size(iter_k, params, new_step_size);

  }

  template<typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType>
  bool allowDoneThermalization(const MHRWParamsType & params, const MHWalker & /*mhwalker*/,
                               IterCountIntType iter_k, const MHRandomWalkType & /*mhrw*/)
  {
    auto logger = llogger.subLogger(TOMO_ORIGIN);

    const double accept_ratio = accept_ratio_stats_collector.movingAverageAcceptanceRatio();
    logger.longdebug("iter_k=%lu, accept_ratio=%f", (unsigned long)iter_k, accept_ratio); 
    if (!std::isfinite(accept_ratio) ||
        accept_ratio < desired_accept_ratio_min || accept_ratio > desired_accept_ratio_max) {
      logger.longdebug("not allowing, based on accept_ratio=%f", accept_ratio); 
      return false;
    }

    if ((iter_k - last_corrected_unacceptable_iter_k)
        < params.n_sweep*(ensure_n_therm_fixed_params_fraction*orig_n_therm)) {
      logger.longdebug("not allowing, based on iter_k=%lu & last_corrected_unacceptable_iter_k=%lu",
                       (unsigned long)iter_k, (unsigned long)last_corrected_unacceptable_iter_k); 
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

private:

  template<typename MHRWParamsType>
  void _adjust_step_size(IterCountIntType iter_k, MHRWParamsType & params, StepRealType new_step_size)
  {
    auto logger = llogger.subLogger(TOMO_ORIGIN);

    // only allow the new step size to be within a certain range of the previous one
    const auto cur_step_size = params.mhwalker_params.step_size;
    if (new_step_size < 0.7*cur_step_size) { new_step_size = 0.7*cur_step_size; }
    if (new_step_size > 1.5*cur_step_size) { new_step_size = 1.5*cur_step_size; }

    params.mhwalker_params.step_size = new_step_size;

    // store last set step size
    last_set_step_size = new_step_size;

    // adapt sweep size
    params.n_sweep = (typename MHRWParamsType::CountIntType)(orig_step_times_sweep / new_step_size + 1);

    // update n_therm to make sure we have enough thermalization sweeps
    _ensure_enough_thermalization_sweeps(iter_k, params);

    logger.longdebug([&](std::ostream & stream) {
        stream << "New params = " << params;
      });
  }

  template<typename MHRWParamsType>
  void _ensure_enough_thermalization_sweeps(IterCountIntType iter_k, MHRWParamsType & params)
  {
    auto logger = llogger.subLogger(TOMO_ORIGIN);

    const auto n_therm_min = (iter_k/params.n_sweep) + 1 + (ensure_n_therm_fixed_params_fraction * orig_n_therm);
    if (params.n_therm < (typename MHRWParamsType::CountIntType)n_therm_min) {
      logger.longdebug([&](std::ostream & stream) {
          stream << "There aren't enough thermalization sweeps. I'm setting n_therm = " << n_therm_min;
        });
      params.n_therm = (typename MHRWParamsType::CountIntType)n_therm_min;
    }
  }


public:

  inline StepRealType getLastSetStepSize() const { return last_set_step_size; }

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


template<typename MHRWParamsType,
         typename MHRWMovingAverageAcceptanceRatioStatsCollectorType_,
         typename BaseLoggerType_
         >
inline 
MHRWStepSizeController<MHRWMovingAverageAcceptanceRatioStatsCollectorType_,
                       BaseLoggerType_,
                       typename MHRWParamsType::MHWalkerParams::StepRealType,
                       typename MHRWParamsType::CountIntType>
mkMHRWStepSizeController(
    const MHRWMovingAverageAcceptanceRatioStatsCollectorType_ & accept_ratio_stats_collector_,
    BaseLoggerType_ & baselogger_,
    double desired_accept_ratio_min_ =
      MHRWStepSizeControllerDefaults::DesiredAcceptanceRatioMin,
    double desired_accept_ratio_max_ =
      MHRWStepSizeControllerDefaults::DesiredAcceptanceRatioMax,
    double acceptable_accept_ratio_min_ =
      MHRWStepSizeControllerDefaults::AcceptableAcceptanceRatioMin,
    double acceptable_accept_ratio_max_ =
      MHRWStepSizeControllerDefaults::AcceptableAcceptanceRatioMax,
    double ensure_n_therm_fixed_params_fraction_ = 
      MHRWStepSizeControllerDefaults::EnsureNThermFixedParamsFraction
    )
{
  return MHRWStepSizeController<MHRWMovingAverageAcceptanceRatioStatsCollectorType_,
                                BaseLoggerType_,
                                typename MHRWParamsType::MHWalkerParams::StepRealType,
                                typename MHRWParamsType::CountIntType>(
                                    accept_ratio_stats_collector_,
                                    baselogger_,
                                    desired_accept_ratio_min_,
                                    desired_accept_ratio_max_,
                                    acceptable_accept_ratio_min_,
                                    acceptable_accept_ratio_max_,
                                    ensure_n_therm_fixed_params_fraction_
                                    );
}






namespace Tools {

template<typename MHRWMovingAverageAcceptanceRatioStatsCollectorType,
         typename BaseLoggerType,
         typename StepRealType,
         typename IterCountIntType>
struct TOMOGRAPHER_EXPORT
StatusProvider<MHRWStepSizeController<MHRWMovingAverageAcceptanceRatioStatsCollectorType,
                                      BaseLoggerType, StepRealType, IterCountIntType> >
{
  typedef MHRWStepSizeController<MHRWMovingAverageAcceptanceRatioStatsCollectorType,
                                 BaseLoggerType, StepRealType, IterCountIntType> StatusableObject;

  static constexpr bool CanProvideStatusLine = true;

  static inline std::string getStatusLine(const StatusableObject * obj) {
    double last_step = (double)obj->getLastSetStepSize();
    if (std::isfinite(last_step)) {
      return Tomographer::Tools::fmts("step size = %.3g", last_step);
    } else {
      return std::string();
    }
  }
};

}



} // namespace Tomographer



#endif
