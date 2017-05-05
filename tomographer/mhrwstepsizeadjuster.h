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

#ifndef _TOMOGRAPHER_MHRWSTEPSIZEADJUSTER_H
#define _TOMOGRAPHER_MHRWSTEPSIZEADJUSTER_H

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


/** \file mhrwstepsizeadjuster.h
 * \brief Routines for performing a Metropolis-Hastings random walk.
 *
 */


namespace Tomographer {


/** \brief A \ref pageInterfaceMHWalkerParamsAdjuster dynamically adjusting the step size to keep a good acceptance ratio
 *
 */
template<typename MHRWMovingAverageAcceptanceRatioStatsCollectorType_,
         int EmpiricalDataBufferSize = 4,
         typename BaseLoggerType = Logger::VacuumLogger,
         typename StepRealType = double,
         typename CountIntType = int>
TOMOGRAPHER_EXPORT class MHRWStepSizeAdjuster
  : public Tools::EigenAlignedOperatorNewProvider
{
public:
  enum { AdjustmentStrategy = MHWalkerParamsAdjustEveryIterationWhileThermalizing };
  
  typedef MHRWMovingAverageAcceptanceRatioStatsCollectorType_
    MHRWMovingAverageAcceptanceRatioStatsCollectorType;

private:
  const MHRWMovingAverageAcceptanceRatioStatsCollectorType & accept_ratio_stats_collector;
  
  typedef Eigen::Array<double,EmpiricalDataBufferSize,2> EmpiricalDataArrayType;
  EmpiricalDataArrayType stepsizes_acceptratios_empirical_data;
  int n_empirical_data;
  
  double desired_accept_ratio_min;
  double desired_accept_ratio_max;
  
  CountIntType last_corrected_iter_k;
  StepRealType last_set_step_size;
  CountIntType orig_n_therm;
  StepRealType orig_step_times_sweep;
  
  /** \brief Ensure that at least this fraction of the original \a n_therm sweeps are
   *         performed at constant (converged) parameters before completing the
   *         thermalization runs
   */
  double ensure_n_therm_fixed_params_fraction;
  
  Logger::LocalLogger<BaseLoggerType> llogger;

public:
  MHRWStepSizeAdjuster(const MHRWMovingAverageAcceptanceRatioStatsCollectorType & accept_ratio_stats_collector_,
                       BaseLoggerType & baselogger_,
                       double desired_accept_ratio_min_ = (0.75*MHRWAcceptanceRatioRecommendedMin +
                                                           0.25*MHRWAcceptanceRatioRecommendedMax),
                       double desired_accept_ratio_max_ = (0.5*MHRWAcceptanceRatioRecommendedMin +
                                                           0.5*MHRWAcceptanceRatioRecommendedMax),
                       double ensure_n_therm_fixed_params_fraction_ = 0.5)
  : accept_ratio_stats_collector(accept_ratio_stats_collector_),
    stepsizes_acceptratios_empirical_data(Eigen::Array<double,EmpiricalDataBufferSize,2>::Zero()),
    n_empirical_data(0),
    desired_accept_ratio_min(desired_accept_ratio_min_),
    desired_accept_ratio_max(desired_accept_ratio_max_),
    last_corrected_iter_k(0),
    last_set_step_size(0),
    orig_n_therm(0),
    orig_step_times_sweep(0),
    ensure_n_therm_fixed_params_fraction(ensure_n_therm_fixed_params_fraction_),
    llogger("Tomographer::MHRWStepSizeAdjuster", baselogger_)
  {
  }
      
  
  template<typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType>
  inline void initParams(MHRWParamsType & params, const MHWalker & /*mhwalker*/, const MHRandomWalkType & /*mhrw*/)
  {
    orig_n_therm = params.n_therm;
    orig_step_times_sweep = params.n_sweep * params.mhwalker_params.step_size;

    // ensure enough thermalization steps that we'll at least have a change to adjust the parameters once
    auto min_n_therm = 2 * std::max((CountIntType)params.n_sweep,
                                    (CountIntType)accept_ratio_stats_collector.bufferSize());
    if (params.n_therm < min_n_therm) {
      params.n_therm = min_n_therm;
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

    // only adjust every max(sweep,moving-avg-accept-ratio-buffer-size)
    if ( ! accept_ratio_stats_collector.hasMovingAverageAcceptanceRatio() ||
         (iter_k % std::max((CountIntType)params.n_sweep,
                            (CountIntType)accept_ratio_stats_collector.bufferSize())) != 0 ) {
      return;
    }

    logger.longdebug([&](std::ostream & stream) {
        stream << "will consider correction.  n_empirical_data = " << n_empirical_data <<
          ", last_corrected_iter_k = " << last_corrected_iter_k;
      });

    const double accept_ratio = accept_ratio_stats_collector.movingAverageAcceptanceRatio();

    if (accept_ratio >= desired_accept_ratio_min && accept_ratio <= desired_accept_ratio_max) {
      return;
    }

    logger.longdebug([&](std::ostream & stream) {
        stream << "will adjust.";
      });

    last_corrected_iter_k = iter_k;

    const auto cur_step_size = params.mhwalker_params.step_size;

    // analyze acceptance ratio stats and correct step size
    Eigen::Index ind;
    // // stored value which is closest to the current point
    // auto delta = (stepsizes_acceptratios_empirical_data.col(0)
    //               - Eigen::Array<StepRealType,EmpiricalDataBufferSize,1>::Constant(cur_step_size)).abs().minCoeff(&ind);
    // if (delta < 0.05*cur_step_size) {
    //   // we already have a data point for a step_size which is very close to this one, so
    //   // don't store another point; rather, update this one.
    //   //
    //   // ind is already set.
    //   logger.longdebug([&](std::ostream & stream) {
    //       stream << "found existing point which is close to the current one, delta = " << delta
    //              << ". Storing at index " << ind;
    //     });
    // } else {
      // add a new point, overwriting the last data point taken
      ind = n_empirical_data % stepsizes_acceptratios_empirical_data.rows();
      ++n_empirical_data;
//    }
    // store the point    
    stepsizes_acceptratios_empirical_data(ind, 0) = cur_step_size;
    stepsizes_acceptratios_empirical_data(ind, 1) = accept_ratio;

    logger.longdebug([&](std::ostream & stream) {
        stream << "stored current empirical data point; ind = "
               << ind
               //<< ", delta = " << delta
               << ", cur data = \n"
               << stepsizes_acceptratios_empirical_data;
      });


//    if (n_empirical_data < stepsizes_acceptratios_empirical_data.rows()) {

      // first gather data by moving using guesses
      
      // new step size -- guess a slight increase or decrease depending on too high or too low

      // accept ratio too high -- increase step size
      StepRealType new_step_size =  params.mhwalker_params.step_size;
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
    params.n_sweep = orig_step_times_sweep / new_step_size;

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

  template<bool IsThermalizing, bool IsAfterSample,
           typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType,
           TOMOGRAPHER_ENABLED_IF_TMPL(!IsThermalizing)> // After thermalizing, keep parameters fixed
  inline void adjustParams(MHRWParamsType & /*params*/, const MHWalker & /*mhwalker*/,
                           int /*iter_k*/, const MHRandomWalkType & /*mhrw*/) const
  {
  }

  template<typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType>
  inline void thermalizingDone(MHRWParamsType & /*params*/, const MHWalker & /*mhwalker*/, const MHRandomWalkType & /*mhrw*/) const
  {
  }
};



namespace Tools {

template<typename MHRWMovingAverageAcceptanceRatioStatsCollectorType,
         int EmpiricalDataBufferSize, typename CountIntType>
struct StatusProvider<MHRWStepSizeAdjuster<MHRWMovingAverageAcceptanceRatioStatsCollectorType,
                                           EmpiricalDataBufferSize, CountIntType> >
{
  typedef MHRWStepSizeAdjuster<MHRWMovingAverageAcceptanceRatioStatsCollectorType,
                               EmpiricalDataBufferSize, CountIntType> StatusableObject;

  static constexpr bool CanProvideStatusLine = true;
  static inline std::string getStatusLine(const StatusableObject * obj) {
    return Tomographer::Tools::fmts("set step size = %.3g", (double)obj->getLastSetStepSize());
  }
};

}



} // namespace Tomographer



#endif
