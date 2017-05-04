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

#include <limits>
#include <random>
#include <iomanip>

#include <tomographer/tools/loggers.h>
#include <tomographer/tools/fmt.h>
#include <tomographer/tools/cxxutil.h>
#include <tomographer/tools/needownoperatornew.h>
#include <tomographer/mhrw.h>
#include <tomographer/mhrwstatscollector.h>


/** \file mhrwstepsizeadjuster.h
 * \brief Routines for performing a Metropolis-Hastings random walk.
 *
 */


namespace Tomographer {



template<int EmpiricalDataBufferSize = 4, typedef StepRealType = double, typedef CountIntType = int>
class MHRWStepSizeAdjuster
{
private:

  const MHRWMovingAverageAcceptanceRatioStatsCollector & accept_ratio_stats_collector;

  Eigen::Array<double,EmpiricalDataBufferSize,2> stepsizes_acceptratios_empirical_data;
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

public:
  enum { AdjustmentStrategy = MHWalkerParamsAdjustEveryIterationWhileThermalizing };

  MHRWStepSizeAdjuster(
      const MHRWMovingAverageAcceptanceRatioStatsCollector & accept_ratio_stats_collector_,
      double desired_accept_ratio_min_ = (0.75*MHRWAcceptanceRatioRecommendedMin + 0.25*MHRWAcceptanceRatioRecommendedMax),
      double desired_accept_ratio_max_ = (0.5*MHRWAcceptanceRatioRecommendedMin + 0.5*MHRWAcceptanceRatioRecommendedMax),
      double ensure_n_therm_fixed_params_fraction_ = 0.3
      )
    : accept_ratio_stats_collector(accept_ratio_stats_collector_),
      stepsizes_acceptratios_empirical_data(Eigen::Array<double,EmpiricalDataBufferSize,2>::Zeros()),
      n_empirical_data(0),
      desired_accept_ratio_min(desired_accept_ratio_min_),
      desired_accept_ratio_max(desired_accept_ratio_max_),
      last_corrected_iter_k(0),
      last_set_step_size(0),
      orig_n_therm(0),
      orig_step_times_sweep(0),
  { }


  template<typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType>
  inline void initParams(MHRWParamsType & params, const MHWalker & /*mhwalker*/, const MHRandomWalkType & /*mhrw*/)
  {
    orig_n_therm = params.n_therm;
    orig_step_times_sweep = params.n_sweep * params.mhwalker_params.step_size;
  }

  template<bool IsThermalizing, bool IsAfterSample,
           typename MHRWParamsType, typename MHWalker, typename CountIntType, typename MHRandomWalkType,
           TOMOGRAPHER_ENALBED_IF_TMPL(IsThermalizing)> // Only while thermalizing
  inline void adjustParams(MHRWParamsType & params, const MHWalker & /*mhwalker*/,
                           CountIntType iter_k, const MHRandomWalkType & /*mhrw*/)
  {
    // only adjust every max(sweep,moving-avg-accept-ratio-buffer-size)
    if ( ! accept_ratio_stats_collector.hasMovingAverageAcceptanceRatio() ||
         (iter_k % std::max(params.n_sweep, accept_ratio_stats_collector.bufferSize())) != 0 ) {
      return;
    }

    const double accept_ratio = accept_ratio_stats_collector.movingAverageAcceptanceRatio();
      
    if (accept_ratio >= desired_accept_ratio_min && accept_ratio <= desired_accept_ratio_max) {
      return;
    }

    last_corrected_iter_k = iter_k;

    // analyze acceptance ratio stats and correct step size
    stepsizes_acceptratios_empirical_data[n_empirical_data % stepsizes_acceptratios_empirical_data.size()] = accept_ratio;
    ++n_empirical_data;

    if (n_empirical_data < stepsizes_acceptratios_empirical_data.size()) {

      // first gather data by moving using guesses
      
      // new step size -- guess a slight increase or decrease depending on too high or too low

      // accept ratio too high -- increase step size
      if (accept_ratio >= 2*desired_accept_ratio_max) {
        params.mhwalker_params.step_size *= 1.5;
      } else if (accept_ratio >= desired_accept_ratio_max) {
        params.mhwalker_params.step_size *= 1.2;
      } else if (accept_ratio <= 0.5*desired_accept_ratio_min) {
        params.mhwalker_params.step_size *= 0.5;
      } else {// if (accept_ratio <= desired_accept_ratio_min
        params.mhwalker_params.step_size *= 0.8;
      }

      last_set_step_size = params.mhwalker_params.step_size;

    } else {

      // rough linear fit through our data points.
      // See http://mathworld.wolfram.com/LeastSquaresFitting.html

      const auto & D = stepsizes_acceptratios_empirical_data.template cast<StepRealType>();

      StepRealType sx = D.col(0).sum();
      StepRealType sy = D.col(1).sum();
      StepRealType sxy = D.col(0).transpose()*D.col(1);
      StepRealType sx2 = D.col(0).norm();

      // fit is y = a + b*x, with y = acceptance ratio and x = step size

      StepRealType denom = (n*sx2 - sx*sx);
      StepRealType a = (sy*sx2 - sx*sxy) / denom;
      StepRealType b = (n*sxy - sx*sy) / denom;

      // find x0 such that y0==target-accept-ratio
      StepRealType target_accept_ratio = (desired_accept_ratio_max+desired_accept_ratio_min)/2;
      StepRealType new_step_size = (target_accept_ratio - a) / b;

      // set this as new step size
      params.mhwalker_params.step_size = new_step_size;
      // and adapt sweep size
      params.n_sweep = orig_step_times_sweep / new_step_size;

      last_set_step_size = params.mhwalker_params.step_size;
    }

  }

  inline StepRealType getLastSetStepSize() const { return last_set_step_size; }
  inline double getLastAcceptRatio() const
  {
    return stepsizes_acceptratios_empirical_data[ (n_empirical_data-1) % stepsizes_acceptratios_empirical_data.size()];
  }

  template<bool IsThermalizing, bool IsAfterSample,
           typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType,
           TOMOGRAPHER_ENALBED_IF_TMPL(!IsThermalizing)> // After thermalizing, keep parameters fixed
  inline void adjustParams(MHRWParamsType & params, const MHWalker & /*mhwalker*/,
                           int /*iter_k*/, const MHRandomWalkType & /*mhrw*/) const
  {
  }
};



namespace Tools {

template<int EmpiricalDataBufferSize, typedef CountIntType>
struct StatusProvider<MHRWStepSizeAdjuster<EmpiricalDataBufferSize, CountIntType> >
{
  typedef MHRWStepSizeAdjuster<EmpiricalDataBufferSize, CountIntType> StatusableObject;

  static constexpr bool CanProvideStatusLine = true;
  static inline std::string getStatusLine(const StatusableObject * obj) {
    return Tomographer::Tools::fmts("accept ratio = %.3g  ->  step size = %.3g",
                                    obj->getLastAcceptRatio(), (double)obj->getLastSetStepSize());
  }
};

}



} // namespace Tomographer



#endif
