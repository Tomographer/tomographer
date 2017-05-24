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

#ifndef TOMORUN_DISPATCH
#define TOMORUN_DISPATCH

#include <random>

#include <tomographer/tools/cxxutil.h>
#include <tomographer/tools/loggers.h>
#include <tomographer/tools/ezmatio.h>
#include <tomographer/tools/signal_status_report.h>
#include <tomographer/tools/eigenutil.h>
#include <tomographer/densedm/dmtypes.h>
#include <tomographer/densedm/param_herm_x.h>
#include <tomographer/densedm/indepmeasllh.h>
#include <tomographer/densedm/tspacefigofmerit.h>
#include <tomographer/mhrw.h>
#include <tomographer/mhrwtasks.h>
#include <tomographer/mhrw_valuehist_tools.h>
#include <tomographer/densedm/tspacellhwalker.h>
#include <tomographer/mathtools/pos_semidef_util.h>





// -----------------------------------------------------------------------------


//typedef std::mt19937 BaseRngType;


template<typename BaseRngType_>
struct DeviceSeededRng : public BaseRngType_
{
  typedef BaseRngType_ BaseRngType;
  using typename BaseRngType::result_type;
  
  DeviceSeededRng(result_type k) : BaseRngType(_get_device_seed(k)) { }

  static inline result_type _get_device_seed(result_type k)
  {
#if TOMORUN_USE_DEVICE_SEED != 0
    (void)k; // unused
    
    result_type finalseed;
    // no two threads simultaneously executing the same code here please
    TOMORUN_THREAD_CRITICAL_SECTION {
      std::random_device devrandom( TOMORUN_RANDOM_DEVICE );
      std::uniform_int_distribution<result_type> dseed(
          std::numeric_limits<result_type>::min(),
          std::numeric_limits<result_type>::max()
          ); // full range of result_type
      
      finalseed =  dseed(devrandom);
      // DEBUG: fprintf(stderr, "Rng: device seed=%lu\n", (unsigned long) finalseed) ;
    } // return out of critical region for OpenMP:
    return finalseed;
#else
    return k;
#endif
  }
};

typedef DeviceSeededRng<std::mt19937>  TomorunRng;





template<typename DenseLLH_, typename CDataBaseType_,
         bool ControlStepSize, bool ControlValueErrorBins, bool UseTSpaceLLHWalkerLight>
struct TomorunCData : public CDataBaseType_
{
  typedef CDataBaseType_ Base; // base class

  static constexpr bool BinningAnalysisEnabled = Base::UseBinningAnalysis;

  typedef DenseLLH_ DenseLLH;

  using typename Base::ValueCalculator;
  using typename Base::MHRWParamsType;
  using typename Base::ValueStatsCollectorResultType;
  using typename Base::MHRWStatsResultsBaseType;

  typedef TomorunRng RngType;

  // the value result is always the first of a tuple
  struct MHRWStatsResultsType : public MHRWStatsResultsBaseType
  {
    template<typename... Types>
    MHRWStatsResultsType(std::tuple<ValueStatsCollectorResultType, Types...> && r)
      : MHRWStatsResultsBaseType(std::move(std::get<0>(r)))
    { }
  };

 
  TOMOGRAPHER_ENABLED_IF(!BinningAnalysisEnabled)
  TomorunCData(const DenseLLH & llh_, ValueCalculator valcalc,
               const ProgOptions * opt, RngType::result_type base_seed)
    : Base(valcalc,
	   typename Base::HistogramParams(opt->val_min, opt->val_max, opt->val_nbins),
	   typename Base::MHRWParamsType(opt->step_size, opt->Nsweep, opt->Ntherm, opt->Nrun),
	   base_seed),
      llh(llh_),
      ctrl_moving_avg_samples(opt->control_step_size_moving_avg_samples),
      ctrl_max_allowed_unknown(opt->control_binning_converged_max_unknown),
      ctrl_max_allowed_unknown_notisolated(opt->control_binning_converged_max_unknown_notisolated),
      ctrl_max_allowed_not_converged(opt->control_binning_converged_max_not_converged)
  {
  }

  TOMOGRAPHER_ENABLED_IF(BinningAnalysisEnabled)
  TomorunCData(const DenseLLH & llh_, ValueCalculator valcalc,
               const ProgOptions * opt, RngType::result_type base_seed)
    : Base(valcalc,
	   typename Base::HistogramParams(opt->val_min, opt->val_max, opt->val_nbins),
	   opt->binning_analysis_num_levels,
	   typename Base::MHRWParamsType(opt->step_size, opt->Nsweep, opt->Ntherm, opt->Nrun),
	   base_seed),
      llh(llh_),
      ctrl_moving_avg_samples(opt->control_step_size_moving_avg_samples),
      ctrl_max_allowed_unknown(opt->control_binning_converged_max_unknown),
      ctrl_max_allowed_unknown_notisolated(opt->control_binning_converged_max_unknown_notisolated),
      ctrl_max_allowed_not_converged(opt->control_binning_converged_max_not_converged)
  {
  }

  const DenseLLH llh;

  const TomorunInt ctrl_moving_avg_samples;
  const Eigen::Index ctrl_max_allowed_unknown;
  const Eigen::Index ctrl_max_allowed_unknown_notisolated;
  const Eigen::Index ctrl_max_allowed_not_converged;


  template<typename RngType, typename LoggerType,
           TOMOGRAPHER_ENABLED_IF_TMPL(!UseTSpaceLLHWalkerLight)>
  inline Tomographer::DenseDM::TSpace::LLHMHWalker<DenseLLH,RngType,LoggerType>
  createLLHWalker(RngType & rng, LoggerType & logger) const
  {
    return { llh.dmt.initMatrixType(), llh, rng, logger };
  }

  template<typename RngType, typename LoggerType,
           TOMOGRAPHER_ENABLED_IF_TMPL(UseTSpaceLLHWalkerLight)>
  inline Tomographer::DenseDM::TSpace::LLHMHWalkerLight<DenseLLH,RngType,LoggerType>
  createLLHWalker(RngType & rng, LoggerType & logger) const
  {
    return { llh.dmt.initMatrixType(), llh, rng, logger };
  }

  template<typename RngType, typename LoggerType, typename RunFn,
           TOMOGRAPHER_ENABLED_IF_TMPL(!ControlStepSize && !ControlValueErrorBins)>
  inline void setupRandomWalkAndRun(RngType & rng, LoggerType & logger, RunFn run) const
  {
    auto llhwalker = createLLHWalker(rng, logger);

    auto value_stats = Base::createValueStatsCollector(logger);
    auto stats = Tomographer::mkMultipleMHRWStatsCollectors(value_stats);

    run(llhwalker, stats);
  }

  template<typename Rng, typename LoggerType, typename RunFn,
           TOMOGRAPHER_ENABLED_IF_TMPL(ControlStepSize && !ControlValueErrorBins)>
  inline void setupRandomWalkAndRun(Rng & rng, LoggerType & logger, RunFn run) const
  {
    auto llhwalker = createLLHWalker(rng, logger);

    auto value_stats = Base::createValueStatsCollector(logger);

    // step size controller, with its buddy moving-average-accept-ratio stats collector
    Tomographer::MHRWMovingAverageAcceptanceRatioStatsCollector<TomorunInt>
      movavg_accept_stats(ctrl_moving_avg_samples);
    auto ctrl_step = 
      Tomographer::mkMHRWStepSizeController<MHRWParamsType>(movavg_accept_stats, logger);

    auto stats = Tomographer::mkMultipleMHRWStatsCollectors(value_stats, movavg_accept_stats);

    run(llhwalker, stats, ctrl_step);
  }

  template<typename Rng, typename LoggerType, typename RunFn,
           TOMOGRAPHER_ENABLED_IF_TMPL(!ControlStepSize && ControlValueErrorBins)>
  inline void setupRandomWalkAndRun(Rng & rng, LoggerType & logger, RunFn run) const
  {
    auto llhwalker = createLLHWalker(rng, logger);

    auto value_stats = Base::createValueStatsCollector(logger);

    // value error bins convergence controller
    auto ctrl_convergence = 
      Tomographer::mkMHRWValueErrorBinsConvergedController<TomorunInt>(
          value_stats, logger, 1024,
          ctrl_max_allowed_unknown,
          ctrl_max_allowed_unknown_notisolated,
          ctrl_max_allowed_not_converged
          );

    auto stats = Tomographer::mkMultipleMHRWStatsCollectors(value_stats);

    run(llhwalker, stats, ctrl_convergence);
  }

  template<typename Rng, typename LoggerType, typename RunFn,
           TOMOGRAPHER_ENABLED_IF_TMPL(ControlStepSize && ControlValueErrorBins)>
  inline void setupRandomWalkAndRun(Rng & rng, LoggerType & logger, RunFn run) const
  {
    auto llhwalker = createLLHWalker(rng, logger);

    auto value_stats = Base::createValueStatsCollector(logger);

    //
    // both controllers:
    //
    // step size controller, with its buddy moving-average-accept-ratio stats collector
    Tomographer::MHRWMovingAverageAcceptanceRatioStatsCollector<TomorunInt>
      movavg_accept_stats(ctrl_moving_avg_samples);
    auto ctrl_step = 
      Tomographer::mkMHRWStepSizeController<MHRWParamsType>(movavg_accept_stats, logger);
    // value error bins convergence controller
    auto ctrl_convergence = 
      Tomographer::mkMHRWValueErrorBinsConvergedController<TomorunInt>(
          value_stats, logger, 1024,
          ctrl_max_allowed_unknown,
          ctrl_max_allowed_unknown_notisolated,
          ctrl_max_allowed_not_converged
          );
    // combined to a:
    auto ctrl_combined =
      Tomographer::mkMHRWMultipleControllers(ctrl_step, ctrl_convergence);

    auto stats = Tomographer::mkMultipleMHRWStatsCollectors(value_stats, movavg_accept_stats);

    run(llhwalker, stats, ctrl_combined);
  }

};





//
// Here goes the actual program. Now the program options have been translated to template
// parameters appropriately.
//
template<bool UseBinningAnalysisErrorBars,
         bool ControlStepSize, bool ControlValueErrorBins, bool UseLLHWalkerLight,
         typename DenseLLH, typename ValueCalculator, typename LoggerType>
inline void tomorun(const DenseLLH & llh, const ProgOptions * opt,
		    ValueCalculator valcalc, LoggerType & baselogger)
{
  Tomographer::Logger::LocalLogger<LoggerType> logger(TOMO_ORIGIN, baselogger);

  //
  // create the Task Dispatcher and run.
  //

  typedef TomorunCData<
    DenseLLH,
    Tomographer::MHRWTasks::ValueHistogramTools::CDataBase<
      ValueCalculator,
      UseBinningAnalysisErrorBars,
      Tomographer::MHWalkerParamsStepSize<TomorunReal>,
      TomorunRng::result_type, // RngSeedType
      TomorunInt, // IterCountIntType
      TomorunReal, // CountRealType
      TomorunInt // HistCountIntType
      >,
    ControlStepSize,
    ControlValueErrorBins,
    UseLLHWalkerLight
    > OurCData;

  typedef Tomographer::MHRWTasks::MHRandomWalkTask<OurCData, typename OurCData::RngType>  OurMHRandomWalkTask;

  // seed for random number generator, if no random device is available
  typename OurCData::RngType::result_type default_base_seed =
    (typename OurCData::RngType::result_type)std::chrono::system_clock::now().time_since_epoch().count();

  OurCData taskcdat(llh, valcalc, opt, default_base_seed);

  TomorunMultiProcTaskDispatcher<OurMHRandomWalkTask, OurCData, LoggerType> tasks(
      &taskcdat, // constant data
      logger.parentLogger(), // the main logger object
      (int)opt->Nrepeats // num_runs
      );

  // set up signal handling
  auto srep = Tomographer::Tools::makeSigHandlerTaskDispatcherStatusReporter(&tasks, logger.parentLogger());
  Tomographer::Tools::installSignalHandler(SIGINT, &srep);

  if (opt->periodic_status_report_ms > 0) {
    tasks.requestPeriodicStatusReport(opt->periodic_status_report_ms);
  }

  // and run our tomo process

  auto time_start = TimerClock::now();

  tasks.run();

  auto time_end = TimerClock::now();

  logger.debug("Random walks done.");

  // delta-time, in seconds and fraction of seconds
  std::string elapsed_s = Tomographer::Tools::fmtDuration(time_end - time_start);

  // individual results from each task
  const auto & task_results = tasks.collectedTaskResults();

  // ... aggregated into a full averaged histogram
  auto aggregated_histogram = taskcdat.aggregateResultHistograms(task_results) ;

  logger.info([&](std::ostream & stream) {
      Tomographer::MHRWTasks::ValueHistogramTools::printFinalReport(
          stream, // where to output
          taskcdat, // the cdata
          task_results, // the results
          aggregated_histogram // aggregated
          );
    });

  // save the histogram to a CSV file if the user required it
  if (opt->write_histogram.size()) {
    std::string csvfname = opt->write_histogram + "-histogram.csv";
    std::ofstream outf;
    outf.open(csvfname);
    aggregated_histogram.printHistogramCsv(outf);
    logger.info([&](std::ostream & str) { str << "Wrote histogram to CSV file " << csvfname << "."; });
  }

  logger.info([&](std::ostream & stream) {
      stream << "Computation time: " << elapsed_s << "\n\n";
    });
}








//
// And here is the dispatcher. It will call the correct variant of tomorun() (with the
// correct template parameters), depending on what we were asked to do.
//
//
template<int FixedDim, int FixedMaxDim, int FixedMaxPOVMEffects,
         bool UseBinningAnalysisErrorBars,
         bool ControlStepSize, bool ControlValueErrorBins, bool UseLLHWalkerLight,
         typename LoggerType>
inline void tomorun_dispatch(const Eigen::Index dim, ProgOptions * opt, Tomographer::MAT::File * matf,
                             LoggerType & baselogger)
{
  Tomographer::Logger::LocalLogger<LoggerType> logger(TOMO_ORIGIN, baselogger);

  logger.debug("preparing to dispatch. FixedDim=%d, FixedMaxDim=%d, FixedMaxPOVMEffects=%d",
               FixedDim, FixedMaxDim, FixedMaxPOVMEffects);

  //
  // Typedefs for tomography data types
  //

  typedef Tomographer::DenseDM::DMTypes<FixedDim, TomorunReal, FixedMaxDim> DMTypes;
  typedef Tomographer::DenseDM::IndepMeasLLH<DMTypes, TomorunReal, TomorunInt, FixedMaxPOVMEffects, true>
    OurDenseLLH;

  //
  // Read data from file
  //

  DMTypes dmt(dim);
  OurDenseLLH llh(dmt);

  typename Tomographer::Tools::EigenStdVector<typename DMTypes::MatrixType>::type Emn;
  Emn = Tomographer::MAT::value<decltype(Emn)>(matf->var("Emn"));
  Eigen::Matrix<TomorunInt,Eigen::Dynamic,1> Nm;
  Nm = Tomographer::MAT::value<Eigen::Matrix<TomorunInt,Eigen::Dynamic,1> >(matf->var("Nm"));
  ensure_valid_input((Eigen::Index)Emn.size() == Nm.size(),
		     "number of POVM effects in `Emn' doesn't match length of `Nm'");
  if (Emn.size() > 0) {
    ensure_valid_input(Emn[0].cols() == dim && Emn[0].rows() == dim,
		       streamstr("POVM effects don't have dimension " << dim << " x " << dim));
  }

  for (std::size_t k = 0; k < Emn.size(); ++k) {
    llh.addMeasEffect(Emn[k], Nm((Eigen::Index)k), TOMORUN_DO_SLOW_POVM_CONSISTENCY_CHECKS);
  }

  logger.debug([&](std::ostream & ss) {
      ss << "\n\nExn: size="<<llh.Exn().size()<<"\n"
	 << llh.Exn() << "\n";
      ss << "\n\nNx: size="<<llh.Nx().size()<<"\n"
	 << llh.Nx() << "\n";
    });

  llh.setNMeasAmplifyFactor(opt->NMeasAmplifyFactor);

  //
  // Data has now been successfully read. Now, dispatch to the correct template function
  // for futher processing.
  //

  logger.debug("Creating the MultiplexorValueCalculator.");

  auto multiplexor_value_calculator =
    makeTomorunMultiplexorValueCalculatorType<DMTypes>(opt->valtype.valtype, dmt, opt->valtype.ref_obj_name, matf);

  tomorun<UseBinningAnalysisErrorBars, ControlStepSize, ControlValueErrorBins, UseLLHWalkerLight>(
      llh,
      opt,
      multiplexor_value_calculator,
      logger.parentLogger());
        
}



#define DISPATCH_STATIC_BOOL(VALUE_, Token_, ...)    \
  do { if ((VALUE_)) {                               \
      static constexpr bool Token_ = true;           \
      { __VA_ARGS__ }                                \
    } else {                                         \
      static constexpr bool Token_ = false;          \
      { __VA_ARGS__ }                                \
    } } while (0)
    


template<int FixedDim, int FixedMaxDim, int FixedMaxPOVMEffects, typename LoggerType>
inline void tomorun_dispatch_st(const int dim, ProgOptions * opt, Tomographer::MAT::File * matf,
                                LoggerType & logger)
{
  DISPATCH_STATIC_BOOL(
      opt->light_jumps, UseLLHWalkerLight,
      DISPATCH_STATIC_BOOL(
          opt->control_step_size, ControlStepSize,
          {
            if (opt->binning_analysis_error_bars) {
              static constexpr bool UseBinningAnalysisErrorBars = true;
              DISPATCH_STATIC_BOOL(
                  opt->control_binning_converged, ControlValueErrorBins,
                  {
                    tomorun_dispatch<FixedDim, FixedMaxDim, FixedMaxPOVMEffects,
                                     UseBinningAnalysisErrorBars, ControlStepSize, ControlValueErrorBins,
                                     UseLLHWalkerLight, 
                                     LoggerType>(dim, opt, matf, logger);
                  }
                  ) ;
            } else {
              static constexpr bool UseBinningAnalysisErrorBars = false;
              // no binning analysis, we cannot control binning converged
              tomorun_dispatch<FixedDim, FixedMaxDim, FixedMaxPOVMEffects,
                               UseBinningAnalysisErrorBars, ControlStepSize, false, UseLLHWalkerLight,
                               LoggerType>(dim, opt, matf, logger);
            }
          }
          ) ;
      ) ;
}







#endif
