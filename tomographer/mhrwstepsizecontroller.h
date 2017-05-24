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
#include <tomographer/mhrwacceptratiowalkerparamscontroller.h>


/** \file mhrwstepsizecontroller.h
 * \brief Tools for automatically and dynamically adjusting the step size of the random walk
 *
 * See \ref Tomographer::MHRWStepSizeController
 */


namespace Tomographer {



/** \brief A \ref pageInterfaceMHRWController dynamically adjusting the step size to keep
 *         a good acceptance ratio
 *
 * This controller is based on \ref Tomographer::MHRWAcceptRatioWalkerParamsController. In
 * case you're wondering, the \a MHRWAcceptanceRatioBasedParamsAdjusterType we're using is
 * this class itself.  This class conforms both to \ref pageInterfaceMHRWController and
 * \ref pageInterfaceMHRWAcceptanceRatioBasedParamsAdjuster.
 */
template<typename MHRWMovingAverageAcceptanceRatioStatsCollectorType_,
         typename BaseLoggerType_ = Logger::VacuumLogger,
         typename StepRealType_ = double,
         typename IterCountIntType_ = int>
class TOMOGRAPHER_EXPORT MHRWStepSizeController
  : public MHRWAcceptRatioWalkerParamsController<
  // we will be our own MHRWAcceptanceRatioBasedParamsAdjusterType
  MHRWStepSizeController<MHRWMovingAverageAcceptanceRatioStatsCollectorType_,
                         BaseLoggerType_, StepRealType_, IterCountIntType_>,
  // other params
  MHRWMovingAverageAcceptanceRatioStatsCollectorType_,
  BaseLoggerType_,
  IterCountIntType_
  >
{
public:
  typedef  MHRWAcceptRatioWalkerParamsController<
    // we will be our own MHRWAcceptanceRatioBasedParamsAdjusterType
    MHRWStepSizeController<MHRWMovingAverageAcceptanceRatioStatsCollectorType_,
                           BaseLoggerType_, StepRealType_, IterCountIntType_>,
    // other params
    MHRWMovingAverageAcceptanceRatioStatsCollectorType_,
    BaseLoggerType_,
    IterCountIntType_
    > Base;
  
  using Base::AdjustmentStrategy;

  typedef MHRWMovingAverageAcceptanceRatioStatsCollectorType_
    MHRWMovingAverageAcceptanceRatioStatsCollectorType;
  typedef BaseLoggerType_ BaseLoggerType;
  typedef StepRealType_ StepRealType;
  typedef IterCountIntType_ IterCountIntType;

private:

  StepRealType last_set_step_size;

  StepRealType orig_step_times_sweep;
  
  Logger::LocalLogger<BaseLoggerType> llogger;

public:
  MHRWStepSizeController(
    const MHRWMovingAverageAcceptanceRatioStatsCollectorType & accept_ratio_stats_collector_,
    BaseLoggerType & baselogger_,
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
  : Base(accept_ratio_stats_collector_,
         baselogger_,
         *this,
         desired_accept_ratio_min_,
         desired_accept_ratio_max_,
         acceptable_accept_ratio_min_,
         acceptable_accept_ratio_max_,
         ensure_n_therm_fixed_params_fraction_),
    last_set_step_size(std::numeric_limits<StepRealType>::quiet_NaN()),
    orig_step_times_sweep(0),
    llogger("Tomographer::MHRWStepSizeController", baselogger_)
  {
  }
      

  // callbacks for MHRWAcceptanceRatioBasedParamsAdjusterType:
  
  template<typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType>
  inline void initParams(MHRWParamsType & params, const MHWalker & , const MHRandomWalkType & )
  {
    auto logger = llogger.subLogger(TOMO_ORIGIN) ;

    if (std::isfinite(params.mhwalker_params.step_size) && 
        params.mhwalker_params.step_size > 0) {
      // valid step size
      orig_step_times_sweep = params.n_sweep * params.mhwalker_params.step_size;
    } else {
      // invalid step size
      const StepRealType default_start_step_size = StepRealType(0.01);
      logger.debug([&](std::ostream & stream) {
          stream << "Invalid step_size = " << params.mhwalker_params.step_size
                 << ", set default of = " << default_start_step_size;
        });
      params.mhwalker_params.step_size = default_start_step_size;
      params.n_sweep = (typename MHRWParamsType::CountIntType)(StepRealType(1)/default_start_step_size) + 1;
      orig_step_times_sweep = 1;
    }
  }

  template<typename MHRWParamsType, typename MeType, typename MHWalker, typename MHRandomWalkType>
  inline void adjustParamsForAcceptRatio(MHRWParamsType & params, double accept_ratio, const MeType & /* self */,
                                         const MHWalker & /*mhwalker*/, IterCountIntType iter_k,
                                         const MHRandomWalkType & /*mhrw*/)
  {
    auto logger = llogger.subLogger(TOMO_ORIGIN) ;

    // new step size -- guess a slight increase or decrease depending on too high or too low

    StepRealType new_step_size = params.mhwalker_params.step_size;

    const auto desired_accept_ratio_min = Base::desiredAcceptRatioMin();
    const auto desired_accept_ratio_max = Base::desiredAcceptRatioMax();
    //const auto acceptable_accept_ratio_min = Base::acceptableAcceptRatioMin();
    //const auto acceptable_accept_ratio_max = Base::acceptableAcceptRatioMax();

    // accept ratio too high -- increase step size
    if (accept_ratio >= 2*desired_accept_ratio_max) {
      new_step_size *= StepRealType(1.5);
    } else if (accept_ratio >= StepRealType(1.3)*desired_accept_ratio_max) {
      new_step_size *= StepRealType(1.2);
    } else if (accept_ratio >= desired_accept_ratio_max) {
      new_step_size *= StepRealType(1.05);
    } else if (accept_ratio <= StepRealType(0.5)*desired_accept_ratio_min) {
      new_step_size *= StepRealType(0.5);
    } else if (accept_ratio <= StepRealType(0.75)*desired_accept_ratio_min) {
      new_step_size *= StepRealType(0.8);
    } else {// if (accept_ratio <= desired_accept_ratio_min
      new_step_size *= StepRealType(0.95);
    }

    logger.longdebug([&](std::ostream & stream) {
        stream << "Corrected step_size to " << new_step_size;
      });

    // only allow the new step size to be within a certain range of the previous one
    const auto cur_step_size = params.mhwalker_params.step_size;
    if (new_step_size < StepRealType(0.7)*cur_step_size) {
      new_step_size = StepRealType(0.7)*cur_step_size;
    }
    if (new_step_size > StepRealType(1.5)*cur_step_size) {
      new_step_size = StepRealType(1.5)*cur_step_size;
    }

    params.mhwalker_params.step_size = new_step_size;

    // store last set step size
    last_set_step_size = new_step_size;

    // adapt sweep size
    params.n_sweep = (typename MHRWParamsType::CountIntType)(orig_step_times_sweep / new_step_size + 1);

    // ensure there are enough n_therm sweeps left
    const typename MHRWParamsType::CountIntType n_therm_min =
      (typename MHRWParamsType::CountIntType)(
          (iter_k/params.n_sweep) + 1 + (Base::ensureNThermFixedParamsFraction() * Base::originalNTherm())
          );
    if (params.n_therm < n_therm_min) {
      logger.longdebug([&](std::ostream & stream) {
          stream << "There aren't enough thermalization sweeps. I'm setting n_therm = " << n_therm_min;
        });
      params.n_therm = n_therm_min;
    }

    logger.longdebug([&](std::ostream & stream) {
        stream << "New params = " << params;
      });
  }

  inline StepRealType getLastSetStepSize() const { return last_set_step_size; }

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
