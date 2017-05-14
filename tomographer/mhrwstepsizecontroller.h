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


/** \brief A \ref pageInterfaceMHRWController dynamically adjusting the step size to keep
 *         a good acceptance ratio
 *
 */
template<typename MHRWMovingAverageAcceptanceRatioStatsCollectorType_,
//         int EmpiricalDataBufferSize = 4,
         typename BaseLoggerType = Logger::VacuumLogger,
         typename StepRealType = double,
         typename CountIntType = int>
TOMOGRAPHER_EXPORT class MHRWStepSizeController
//  : public Tools::EigenAlignedOperatorNewProvider // not needed without empirical data buffer
{
public:
  enum { AdjustmentStrategy = MHRWControllerAdjustEveryIterationWhileThermalizing };
  
  typedef MHRWMovingAverageAcceptanceRatioStatsCollectorType_
    MHRWMovingAverageAcceptanceRatioStatsCollectorType;

private:
  const MHRWMovingAverageAcceptanceRatioStatsCollectorType & accept_ratio_stats_collector;
  
//  typedef Eigen::Array<double,EmpiricalDataBufferSize,2> EmpiricalDataArrayType;
//  EmpiricalDataArrayType stepsizes_acceptratios_empirical_data;
//  int n_empirical_data;
  
  const double desired_accept_ratio_min;
  const double desired_accept_ratio_max;
  const double acceptable_accept_ratio_min;
  const double acceptable_accept_ratio_max;
  
  CountIntType last_corrected_unacceptable_iter_k;
  StepRealType last_set_step_size;
  CountIntType orig_n_therm;
  StepRealType orig_step_times_sweep;
  
  /** \brief Ensure that at least this fraction of the original \a n_therm sweeps are
   *         performed at constant (converged) parameters before completing the
   *         thermalization runs
   */
  const double ensure_n_therm_fixed_params_fraction;
  
  Logger::LocalLogger<BaseLoggerType> llogger;

public:
  MHRWStepSizeController(const MHRWMovingAverageAcceptanceRatioStatsCollectorType & accept_ratio_stats_collector_,
                       BaseLoggerType & baselogger_,
                       double desired_accept_ratio_min_ = (0.75*MHRWAcceptanceRatioRecommendedMin +
                                                           0.25*MHRWAcceptanceRatioRecommendedMax),
                       double desired_accept_ratio_max_ = (0.5*MHRWAcceptanceRatioRecommendedMin +
                                                           0.5*MHRWAcceptanceRatioRecommendedMax),
                       double acceptable_accept_ratio_min_ = MHRWAcceptanceRatioRecommendedMin,
                       double acceptable_accept_ratio_max_ = MHRWAcceptanceRatioRecommendedMax,
                       double ensure_n_therm_fixed_params_fraction_ = 0.5)
  : accept_ratio_stats_collector(accept_ratio_stats_collector_),
//    stepsizes_acceptratios_empirical_data(Eigen::Array<double,EmpiricalDataBufferSize,2>::Zero()),
//    n_empirical_data(0),
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
      params.n_sweep = (StepRealType(1)/default_start_step_size) + 1;
      orig_step_times_sweep = 1;
    }
  }

  template<bool IsThermalizing, bool IsAfterSample,
           typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType,
           TOMOGRAPHER_ENABLED_IF_TMPL(IsThermalizing)> // Only while thermalizing
  inline void adjustParams(MHRWParamsType & params, const MHWalker & /*mhwalker*/,
                           CountIntType iter_k, const MHRandomWalkType & /*mhrw*/)
  {
    auto logger = llogger.subLogger(TOMO_ORIGIN) ;

    logger.longdebug([&](std::ostream & stream) {
        stream << "cur params = " << params << " and accept_ratio = "
               << accept_ratio_stats_collector.movingAverageAcceptanceRatio();
      });

    // // only adjust every max(sweep,moving-avg-accept-ratio-buffer-size)
    // if ( ! accept_ratio_stats_collector.hasMovingAverageAcceptanceRatio() ||
    //      (iter_k % std::max((CountIntType)params.n_sweep,
    //                         (CountIntType)accept_ratio_stats_collector.bufferSize())) != 0 ) {
    //   return;
    // }
    // only adjust every moving-avg-accept-ratio-buffer-size
    if ( ! accept_ratio_stats_collector.hasMovingAverageAcceptanceRatio() ||
         (iter_k % (CountIntType)accept_ratio_stats_collector.bufferSize()) != 0 ) {
      return;
    }

    logger.longdebug([&](std::ostream & stream) {
        stream << "will consider correction.  " <<
          //          "n_empirical_data = " << n_empirical_data <<
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

//    const auto cur_step_size = params.mhwalker_params.step_size;

//     // analyze acceptance ratio stats and correct step size
//     Eigen::Index ind;
//     // // stored value which is closest to the current point
//     // auto delta = (stepsizes_acceptratios_empirical_data.col(0)
//     //               - Eigen::Array<StepRealType,EmpiricalDataBufferSize,1>::Constant(cur_step_size)).abs().minCoeff(&ind);
//     // if (delta < 0.05*cur_step_size) {
//     //   // we already have a data point for a step_size which is very close to this one, so
//     //   // don't store another point; rather, update this one.
//     //   //
//     //   // ind is already set.
//     //   logger.longdebug([&](std::ostream & stream) {
//     //       stream << "found existing point which is close to the current one, delta = " << delta
//     //              << ". Storing at index " << ind;
//     //     });
//     // } else {
//       // add a new point, overwriting the last data point taken
//       ind = n_empirical_data % stepsizes_acceptratios_empirical_data.rows();
//       ++n_empirical_data;
// //    }
//     // store the point    
//     stepsizes_acceptratios_empirical_data(ind, 0) = cur_step_size;
//     stepsizes_acceptratios_empirical_data(ind, 1) = accept_ratio;

//     logger.longdebug([&](std::ostream & stream) {
//         stream << "stored current empirical data point; ind = "
//                << ind
//                //<< ", delta = " << delta
//                << ", cur data = \n"
//                << stepsizes_acceptratios_empirical_data;
//       });


//    if (n_empirical_data < stepsizes_acceptratios_empirical_data.rows()) {


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

    // } else {

    //   // rough linear fit through our data points.
    //   // See http://mathworld.wolfram.com/LeastSquaresFitting.html

    //   const auto D = stepsizes_acceptratios_empirical_data.template cast<StepRealType>();

    //   logger.longdebug([&](std::ostream & stream) {
    //       stream << "about to perform fit, D = \n" << D;
    //     });

    //   StepRealType sx = D.col(0).sum();
    //   StepRealType sy = D.col(1).sum();
    //   StepRealType sxy = (D.col(0) * D.col(1)).sum(); // "*" does element-wise product on arrays
    //   StepRealType sx2 = D.col(0).matrix().squaredNorm();

    //   // fit is y = a + b*x, with y = acceptance ratio and x = step size

    //   constexpr int n = EmpiricalDataBufferSize;
    //   StepRealType denom = (n*sx2 - sx*sx);
    //   StepRealType b = (n*sxy - sx*sy) / denom;
    //   StepRealType a = (sy*sx2 - sx*sxy) / denom;

    //   if (std::abs(b)*0.1*cur_step_size < 0.01) {
    //     // b is too small, finding the x-intercept will be unreliable.  Instead, resort to
    //     // "dumb" guess
    //   }


    //   // find x0 such that y0==target-accept-ratio
    //   StepRealType target_accept_ratio = (desired_accept_ratio_max+desired_accept_ratio_min)/2;
    //   StepRealType new_step_size = (target_accept_ratio - a) / b;

    //   // set this as new step size
    //   _adjust_step_size(iter_k, params, new_step_size);

    //   logger.longdebug([&](std::ostream & stream) {
    //       stream << "after fit, corrected step_size to " << params.mhwalker_params.step_size;
    //     });
    // }

  }

  template<typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType>
  bool allowDoneThermalization(const MHRWParamsType & params, const MHWalker & /*mhwalker*/,
                               CountIntType iter_k, const MHRandomWalkType & /*mhrw*/)
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
                     CountIntType /*iter_k*/, const MHRandomWalkType & /*mhrw*/)
  {
    return true; // do whatever you like
  }

private:

  template<typename MHRWParamsType>
  void _adjust_step_size(CountIntType iter_k, MHRWParamsType & params, StepRealType new_step_size)
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
    params.n_sweep = orig_step_times_sweep / new_step_size + 1;

    // update n_therm to make sure we have enough thermalization sweeps
    _ensure_enough_thermalization_sweeps(iter_k, params);

    logger.longdebug([&](std::ostream & stream) {
        stream << "New params = " << params;
      });
  }

  template<typename MHRWParamsType>
  void _ensure_enough_thermalization_sweeps(CountIntType iter_k, MHRWParamsType & params)
  {
    auto logger = llogger.subLogger(TOMO_ORIGIN);

    const auto n_therm_min = (iter_k/params.n_sweep) + 1 + (ensure_n_therm_fixed_params_fraction * orig_n_therm);
    if (params.n_therm < n_therm_min) {
      logger.longdebug([&](std::ostream & stream) {
          stream << "There aren't enough thermalization sweeps. I'm setting n_therm = " << n_therm_min;
        });
      params.n_therm = n_therm_min;
    }
  }


public:

  inline StepRealType getLastSetStepSize() const { return last_set_step_size; }
  // inline double getLastAcceptRatio() const
  // {
  //   // if (n_empirical_data < 1) {
  //   //   return std::numeric_limits<double>::quiet_NaN();
  //   // }
  //   // return stepsizes_acceptratios_empirical_data((n_empirical_data-1) % stepsizes_acceptratios_empirical_data.rows(), 1);
  // }

  // ###: Let's not even declare this, because it should never be invoked according to our
  // ###  AdjustmentStrategy:
  // 
  // template<bool IsThermalizing, bool IsAfterSample,
  //          typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType,
  //          TOMOGRAPHER_ENABLED_IF_TMPL(!IsThermalizing)> // After thermalizing, keep parameters fixed
  // inline void adjustParams(MHRWParamsType & /*params*/, const MHWalker & /*mhwalker*/,
  //                          int /*iter_k*/, const MHRandomWalkType & /*mhrw*/) const
  // {
  // }

  template<typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType>
  inline void thermalizingDone(MHRWParamsType & /*params*/, const MHWalker & /*mhwalker*/, const MHRandomWalkType & /*mhrw*/) const
  {
  }
};



namespace Tools {

template<typename MHRWMovingAverageAcceptanceRatioStatsCollectorType,
//         int EmpiricalDataBufferSize,
         typename BaseLoggerType,
         typename StepRealType,
         typename CountIntType>
struct StatusProvider<MHRWStepSizeController<MHRWMovingAverageAcceptanceRatioStatsCollectorType,
//                                           EmpiricalDataBufferSize,
                                           BaseLoggerType, StepRealType, CountIntType> >
{
  typedef MHRWStepSizeController<MHRWMovingAverageAcceptanceRatioStatsCollectorType,
//                               EmpiricalDataBufferSize,
                               BaseLoggerType, StepRealType, CountIntType> StatusableObject;

  static constexpr bool CanProvideStatusLine = true;
  static inline std::string getStatusLine(const StatusableObject * obj) {
    double last_step = (double)obj->getLastSetStepSize();
    if (std::isfinite(last_step)) {
      return Tomographer::Tools::fmts("set step size = %.3g", last_step);
    } else {
      return std::string();
    }
  }
};

}



} // namespace Tomographer



#endif
