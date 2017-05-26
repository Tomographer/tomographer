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


#include "tomographerpy/common.h"

#include <exception>
#include <stdexcept>
#include <limits>

#include <tomographer/tools/loggers.h>
#include <tomographer/densedm/dmtypes.h>
#include <tomographer/densedm/indepmeasllh.h>
#include <tomographer/densedm/tspacellhwalker.h>
#include <tomographer/densedm/tspacefigofmerit.h>
#include <tomographer/mhrw.h>
#include <tomographer/mhrwstepsizecontroller.h>
#include <tomographer/mhrwvalueerrorbinsconvergedcontroller.h>
#include <tomographer/mhrwtasks.h>
#include <tomographer/mhrw_valuehist_tools.h>
#include <tomographer/multiprocthreads.h>
#include <tomographer/valuecalculator.h>
//#include <tomographer/tools/signal_status_report.h>
#include <tomographer/mathtools/pos_semidef_util.h>


#include "tomographerpy/pyhistogram.h"
#include "tomographerpy/pymultiproc.h"
#include "tomographerpy/pymhrw.h"
#include "tomographerpy/pymhrwtasks.h"
#include "tomographerpy/exc.h"
#include "tomographerpy/pygil.h"

#include "common_p.h"



using namespace pybind11::literals;




// define an exception class for invalid inputs
TOMOGRAPHER_DEFINE_MSG_EXCEPTION_BASE(TomorunInvalidInputError, "Invalid Input: ", tpy::TomographerCxxError) ;



namespace tpy {

class CallableValueCalculator
{
public:
  typedef tpy::RealType ValueType;
  
  CallableValueCalculator(py::object fn_)
    : fn(fn_)
  {
  }

  tpy::RealType getValue(const Eigen::Ref<const tpy::CplxMatrixType> & T) const
  {
    py::gil_scoped_acquire gil_acquire;
    return fn(py::cast(T)).cast<tpy::RealType>();
  }


private:
  py::object fn;
};


typedef Tomographer::DenseDM::DMTypes<Eigen::Dynamic, tpy::RealType> DMTypes;

typedef Tomographer::MHRWParams<Tomographer::MHWalkerParamsStepSize<tpy::RealType>, tpy::CountIntType>
  CxxMHRWParamsType;



#define SWITCH_WHICH_LLHWALKER( Code )                                  \
  switch (which) {                                                      \
  case Full:                                                            \
    { auto llhwalker = llhwalker_full; Code ; }                         \
    break;                                                              \
  case Light:                                                           \
    { auto llhwalker = llhwalker_light; Code ; }                        \
    break;                                                              \
  default:                                                              \
    throw std::runtime_error("Invalid 'which': " + std::to_string((int)which)); \
  }


enum LLH_MHWalker_Which {
  LLH_MHWalker_Full = 0,
  LLH_MHWalker_Light
};

template<typename DenseLLHType, typename RngType, typename LoggerType>
class LLH_MHWalker
{
public:
  typedef DMTypes::MatrixType PointType;

  typedef Tomographer::MHWalkerParamsStepSize<tpy::RealType> WalkerParams;

  typedef typename DenseLLHType::LLHValueType FnValueType;

  static constexpr int UseFnSyntaxType = Tomographer::MHUseFnLogValue;

  enum { Full = LLH_MHWalker_Full, Light = LLH_MHWalker_Light } ;

  TOMO_STATIC_ASSERT_EXPR(
      (int)Tomographer::DenseDM::TSpace::LLHMHWalker<DenseLLHType,RngType,LoggerType>::UseFnSyntaxType
      == (int)Tomographer::MHUseFnLogValue
      ) ;
  TOMO_STATIC_ASSERT_EXPR(
      (int)Tomographer::DenseDM::TSpace::LLHMHWalkerLight<DenseLLHType,RngType,LoggerType>::UseFnSyntaxType
      == (int)Tomographer::MHUseFnLogValue ) ;

  LLH_MHWalker(LLH_MHWalker_Which which_, const DenseLLHType & llh_, RngType & rng_, LoggerType & baselogger)
    : which(which_),
      llh(llh_),
      rng(rng_),
      llogger("tpy::LLH_MHWalker", baselogger)
  {
    switch (which) {
    case Full:
      llhwalker_light = NULL;
      llhwalker_full = new Tomographer::DenseDM::TSpace::LLHMHWalker<DenseLLHType,RngType,LoggerType>(
          llh.dmt.initMatrixType(),
          llh,
          rng,
          llogger.parentLogger()
          ) ;
      break;
    case Light:
      llhwalker_full = NULL;
      llhwalker_light = new Tomographer::DenseDM::TSpace::LLHMHWalkerLight<DenseLLHType,RngType,LoggerType>(
          llh.dmt.initMatrixType(),
          llh,
          rng,
          llogger.parentLogger()
          ) ;
      break;
    default:
      throw std::runtime_error("Invalid 'which': " + std::to_string((int)which));
    }
  }

  ~LLH_MHWalker()
  {
    if (llhwalker_light != NULL) {
      delete llhwalker_light;
    }
    if (llhwalker_full != NULL) {
      delete llhwalker_full;
    }
  }

  inline PointType startPoint() const {
    SWITCH_WHICH_LLHWALKER( return llhwalker->startPoint(); ) ;
  }
  inline void init() {
    SWITCH_WHICH_LLHWALKER( llhwalker->init(); ) ;
  }
  inline void thermalizingDone() {
    SWITCH_WHICH_LLHWALKER( llhwalker->thermalizingDone(); ) ;
  }
  inline void done() {
    SWITCH_WHICH_LLHWALKER( llhwalker->done(); ) ;
  }

  inline PointType jumpFn(const PointType & curpt, WalkerParams walker_params) {
    SWITCH_WHICH_LLHWALKER( return llhwalker->jumpFn(curpt, std::move(walker_params)) ; ) ;
  }

  inline FnValueType fnLogVal(const PointType & curpt) const {
    SWITCH_WHICH_LLHWALKER( return llhwalker->fnLogVal(curpt) ; ) ;
  }

private:
  const LLH_MHWalker_Which which;
  const DenseLLHType & llh;
  RngType & rng;
  Tomographer::Logger::LocalLogger<LoggerType> llogger;

  Tomographer::DenseDM::TSpace::LLHMHWalker<DenseLLHType,RngType,LoggerType> * llhwalker_full{NULL};
  Tomographer::DenseDM::TSpace::LLHMHWalkerLight<DenseLLHType,RngType,LoggerType> * llhwalker_light{NULL};

};

#undef SWITCH_WHICH_LLHWALKER

} // namespace tpy



typedef Tomographer::DenseDM::IndepMeasLLH<tpy::DMTypes> DenseLLH;

typedef std::mt19937 RngType;


typedef Tomographer::MultiplexorValueCalculator<
  tpy::RealType,
  Tomographer::DenseDM::TSpace::FidelityToRefCalculator<tpy::DMTypes, tpy::RealType>,
  Tomographer::DenseDM::TSpace::PurifDistToRefCalculator<tpy::DMTypes, tpy::RealType>,
  Tomographer::DenseDM::TSpace::TrDistToRefCalculator<tpy::DMTypes, tpy::RealType>,
  Tomographer::DenseDM::TSpace::ObservableValueCalculator<tpy::DMTypes>,
  tpy::CallableValueCalculator
  > ValueCalculator;




//
// We need to define a class which adds the capacity of creating the "master" random walk
// object to the engine in Tomographer::MHRWTasks::ValueHistogramTools, which take care of
// running the random walks etc. as needed.
//
struct OurCData : public Tomographer::MHRWTasks::ValueHistogramTools::CDataBase<
  ValueCalculator, // our value calculator
  true, // use binning analysis
  Tomographer::MHWalkerParamsStepSize<tpy::RealType>, // MHWalkerParams
  RngType::result_type, // RngSeedType
  tpy::CountIntType, // IterCountIntType
  tpy::RealType, // CountRealType
  tpy::CountIntType // HistCountIntType
  >
{
public:

  OurCData(const DenseLLH & llh_, // data from the the tomography experiment
	   ValueCalculator valcalc, // the figure-of-merit calculator
	   HistogramParams hist_params, // histogram parameters
	   int binning_num_levels, // number of binning levels in the binning analysis
	   tpy::MHRWParams mhrw_params, // parameters of the random walk
	   RngType::result_type base_seed, // a random seed to initialize the random number generator
           std::string jumps_method_, // "light" or "full"
           py::dict ctrl_step_size_params_, // parameters for step size controller
           py::dict ctrl_converged_params_ // parameters for value bins converged controller
      )
    : CDataBase<ValueCalculator,true>(
        valcalc, hist_params, binning_num_levels,
        tpy::CxxMHRWParamsType(
            tpy::pyMHWalkerParamsFromPyObj<Tomographer::MHWalkerParamsStepSize<tpy::RealType>>(mhrw_params.mhwalker_params),
            mhrw_params.n_sweep, mhrw_params.n_therm, mhrw_params.n_run),
        base_seed),
      llh(llh_),
      jumps_method(jumps_method_),
      ctrl_step_size_params(ctrl_step_size_params_),
      ctrl_converged_params(ctrl_converged_params_)
  {
  }

  const DenseLLH llh;

  const std::string jumps_method;
  const py::dict ctrl_step_size_params;
  const py::dict ctrl_converged_params;


  // the value result is always the first of a tuple
  struct MHRWStatsResultsType : public MHRWStatsResultsBaseType
  {
    template<typename... Types>
    MHRWStatsResultsType(std::tuple<ValueStatsCollectorResultType, Types...> && r)
      : MHRWStatsResultsBaseType(std::move(std::get<0>(r)))
    { }
  };


  //
  // This function is called automatically by the task manager/dispatcher.  It should
  // return a LLHMHWalker object which controls the random walk.
  //
  template<typename Rng, typename LoggerType, typename RunFn>
  inline void
  setupRandomWalkAndRun(Rng & rng, LoggerType & baselogger, RunFn run) const
  {
    auto logger = Tomographer::Logger::makeLocalLogger("pytomorun.cxx:OurCData::setupRandomWalkAndRun()",
                                                       baselogger) ;

    //
    // NOTE: This function can be called from multiple threads simultaneously as
    // the GIL (Global Interpreter Lock) is currently not held.  Don't call any
    // Python-related function without acquiring the Python GIL, see examples
    // below.  Also, Don't write to global variables.
    //
    // However, do not call logger methods while holding the GIL, as the logger
    // will itself try to acquire the GIL again causing a deadlock.
    //
    // A handy macro is TPY_EXPR_WITH_GIL( expression ), which evaluates the
    // given expression while holding the GIL and returns the result,
    // immediately re-releasing the GIL. Just surround any Python-related calls
    // with that macro.
    //

    tpy::LLH_MHWalker<DenseLLH,Rng,LoggerType> mhwalker(
        ( jumps_method == "light" ? tpy::LLH_MHWalker_Light
          : (jumps_method == "full" ? tpy::LLH_MHWalker_Full
             : throw TomorunInvalidInputError("Invalid jumps method: '" + jumps_method + "'"))) ,
	llh,
	rng,
	baselogger
	);

    logger.debug("Created MHWalker.") ;

    auto value_stats = createValueStatsCollector(baselogger);

    logger.debug("Created value stats collector.") ;

    int mvavg_numsamples = 2048;
    double ar_params[4] = {0};
    double ensure_n_therm_fixed_params_fraction = 0;
    {
      py::gil_scoped_acquire gil; // acquire GIL for these calls

      // the stats collector is there whether we enable the controller or not
      mvavg_numsamples = ctrl_step_size_params.attr("get")("num_samples", 2048).cast<int>();
    
      if (ctrl_step_size_params.attr("get")("enabled", true).cast<bool>()) {
        ar_params[0] = ctrl_step_size_params.attr("get")(
            "desired_accept_ratio_min",
            Tomographer::MHRWAcceptRatioWalkerParamsControllerDefaults::DesiredAcceptanceRatioMin
            ).cast<double>();
        ar_params[1] = ctrl_step_size_params.attr("get")(
            "desired_accept_ratio_max",
            Tomographer::MHRWAcceptRatioWalkerParamsControllerDefaults::DesiredAcceptanceRatioMax
            ).cast<double>();
        ar_params[2] = ctrl_step_size_params.attr("get")(
            "acceptable_accept_ratio_min",
            Tomographer::MHRWAcceptRatioWalkerParamsControllerDefaults::AcceptableAcceptanceRatioMin
            ).cast<double>();
        ar_params[3] = ctrl_step_size_params.attr("get")(
            "acceptable_accept_ratio_max",
            Tomographer::MHRWAcceptRatioWalkerParamsControllerDefaults::AcceptableAcceptanceRatioMax
            ).cast<double>();
        ensure_n_therm_fixed_params_fraction = ctrl_step_size_params.attr("get")(
            "ensure_n_therm_fixed_params_fraction",
            Tomographer::MHRWAcceptRatioWalkerParamsControllerDefaults::EnsureNThermFixedParamsFraction
            ).cast<double>();
      } else {
        ar_params[0] = 0;
        ar_params[1] = 1;
        ar_params[2] = 0;
        ar_params[3] = 1;
        ensure_n_therm_fixed_params_fraction = 0;
      }
    }

    Tomographer::MHRWMovingAverageAcceptanceRatioStatsCollector<>
      movavg_accept_stats(mvavg_numsamples);

    auto ctrl_step = 
      Tomographer::mkMHRWStepSizeController<MHRWParamsType>(
          movavg_accept_stats,
          baselogger,
          ar_params[0],
          ar_params[1],
          ar_params[2],
          ar_params[3],
          ensure_n_therm_fixed_params_fraction
          );

    logger.debug("Created auto step size controller.") ;

    int check_frequency_sweeps = 0;
    Eigen::Index max_allowed[3] = {0};
    {
      py::gil_scoped_acquire gilacq;
      if (ctrl_converged_params.attr("get")("enabled", true).cast<bool>()) {
        check_frequency_sweeps =
          ctrl_converged_params.attr("get")("check_frequency_sweeps", 1024).cast<int>();
        max_allowed[0] =
          ctrl_converged_params.attr("get")("max_allowed_unknown", 2).cast<Eigen::Index>();
        max_allowed[1] =
          ctrl_converged_params.attr("get")("max_allowed_unknown_notisolated", 0).cast<Eigen::Index>();
        max_allowed[2] =
          ctrl_converged_params.attr("get")("max_allowed_not_converged", 0).cast<Eigen::Index>();
      } else {
        check_frequency_sweeps = 0;
        max_allowed[0] = std::numeric_limits<Eigen::Index>::max();
        max_allowed[1] = std::numeric_limits<Eigen::Index>::max();
        max_allowed[2] = std::numeric_limits<Eigen::Index>::max();
      }
    }

    // value error bins convergence controller
    auto ctrl_convergence = 
      Tomographer::mkMHRWValueErrorBinsConvergedController(
          value_stats, baselogger,
          check_frequency_sweeps,
          max_allowed[0], max_allowed[1], max_allowed[2]
          );

    logger.debug("Created bins convergence controller.") ;

    // combined to a:
    auto ctrl_combined =
      Tomographer::mkMHRWMultipleControllers(ctrl_step, ctrl_convergence);

    auto stats = mkMultipleMHRWStatsCollectors(value_stats, movavg_accept_stats);

    logger.debug("random walk set up, ready to go") ;

    run(mhwalker, stats, ctrl_combined);
  }

};







py::object py_tomorun(
    int dim,
    const Eigen::MatrixXd& Exn,
    const py::list& Emn,
    const Eigen::VectorXi& Nm,
    py::object fig_of_merit,
    const Eigen::MatrixXcd& ref_state,
    const Eigen::MatrixXcd& observable,
    const tpy::HistogramParams& hist_params,
    const tpy::MHRWParams& mhrw_params,
    int binning_num_levels,
    int num_repeats,
    py::object progress_fn,
    int progress_interval_ms,
    std::string jumps_method,
    py::dict ctrl_step_size_params,
    py::dict ctrl_converged_params
    )
{
  Tomographer::Logger::LocalLogger<tpy::PyLogger> logger(TOMO_ORIGIN, *tpy::logger);

  logger.debug("py_tomorun()");

  typedef tpy::DMTypes::MatrixType MatrixType;

  tpy::DMTypes dmt(dim);

  // prepare llh object
  DenseLLH llh(dmt);

  if (Exn.rows()) { // use Exn
    if (py::len(Emn)) { // error: both Exn & Emn specified
      throw TomorunInvalidInputError("You can't specify both Exn and Emn arguments");
    }
    // use Exn
    if (Exn.rows() != Nm.rows()) {
      throw TomorunInvalidInputError("Mismatch in number of measurements: Exn.rows()="
                                     + std::to_string(Exn.rows()) + " but Nm.rows()="
                                     + std::to_string(Nm.rows()));
    }
    for (Eigen::Index k = 0; k < Nm.rows(); ++k) {
      llh.addMeasEffect(Exn.row(k).transpose(), Nm(k), true);
    }
  } else if (py::len(Emn)) {
    // use Emn
    if (py::len(Emn) != (std::size_t)Nm.rows()) {
      throw TomorunInvalidInputError("Mismatch in number of measurements: len(Emn)="
                                     + std::to_string(py::len(Emn)) +
                                     " but Nm.rows()=" + std::to_string(Nm.rows()));
    }
    for (Eigen::Index k = 0; k < Nm.rows(); ++k) {
      MatrixType POVMeffect = Emn[(std::size_t)k].cast<MatrixType>();
      llh.addMeasEffect(POVMeffect, Nm(k), true);
    }
  } else {
    // no measurements specified
    throw TomorunInvalidInputError("No measurements specified. Please specify either the `Exn' "
                                   "or the `Emn' argument");
  }

  logger.debug([&](std::ostream & ss) {
      ss << "\n\nExn: size="<<llh.Exn().size()<<"\n"
	 << llh.Exn() << "\n";
      ss << "\n\nNx: size="<<llh.Nx().size()<<"\n"
	 << llh.Nx() << "\n";
    });


  // prepare figure of merit

  MatrixType T_ref(dmt.initMatrixType());
  MatrixType rho_ref(dmt.initMatrixType());
  MatrixType A(dmt.initMatrixType());

  bool fig_of_merit_callable = py::hasattr(fig_of_merit, "__call__");
  std::string fig_of_merit_s;
  if (fig_of_merit_callable) {
    fig_of_merit_s = "<custom>";
  } else {
    fig_of_merit_s = fig_of_merit.cast<std::string>();
  }

  if (fig_of_merit_s == "fidelity" || fig_of_merit_s == "tr-dist" || fig_of_merit_s == "purif-dist") {

    if (ref_state.rows() != dmt.dim() || ref_state.cols() != dmt.dim()) {
      throw TomorunInvalidInputError(streamstr("Expected " << dmt.dim() << " x " << dmt.dim() << " complex matrix as "
                                               "'ref_state' argument for fig_of_merit='"<<fig_of_merit_s<<"'")) ;
    }

    Eigen::SelfAdjointEigenSolver<MatrixType> eig(ref_state);

    typedef typename Eigen::SelfAdjointEigenSolver<MatrixType>::RealVectorType RealVectorType;
    
    MatrixType U = eig.eigenvectors();
    RealVectorType d = eig.eigenvalues();
  
    Tomographer::MathTools::forcePosVecKeepSum<RealVectorType>(
        d,
        Eigen::NumTraits<tpy::RealType>::dummy_precision()
        );
  
    // TODO: ensure that something was given

    rho_ref = U * d.asDiagonal() * U.adjoint();
    T_ref = U * d.cwiseSqrt().asDiagonal() * U.adjoint();

  } else if (fig_of_merit_s == "obs-value") {

    if (observable.rows() != dmt.dim() || observable.cols() != dmt.dim()) {
      throw TomorunInvalidInputError(streamstr("Expected " << dmt.dim() << " x " << dmt.dim() << " complex matrix as "
                                               "'observable' argument for fig_of_merit='obs-value'")) ;
    }

    A = observable;
    
  } else if (fig_of_merit_callable) {

    // custom callable

  } else {
    throw TomorunInvalidInputError(std::string("Invalid figure of merit: ") +
                                   py::repr(fig_of_merit).cast<std::string>());
  }

  ValueCalculator valcalc(
      // index of the valuecalculator to actually use:
      (fig_of_merit_s == "fidelity" ? 0 :
       (fig_of_merit_s == "purif-dist" ? 1 :
        (fig_of_merit_s == "tr-dist" ? 2 :
         (fig_of_merit_s == "obs-value" ? 3 :
          (fig_of_merit_callable ? 4 :
           throw TomorunInvalidInputError(std::string("Invalid valtype: ")
                                          + py::repr(fig_of_merit).cast<std::string>())
              ))))),
        // the valuecalculator instances which are available:
      [&]() { return new Tomographer::DenseDM::TSpace::FidelityToRefCalculator<tpy::DMTypes, tpy::RealType>(T_ref); },
      [&]() { return new Tomographer::DenseDM::TSpace::PurifDistToRefCalculator<tpy::DMTypes, tpy::RealType>(T_ref); },
      [&]() { return new Tomographer::DenseDM::TSpace::TrDistToRefCalculator<tpy::DMTypes, tpy::RealType>(rho_ref); },
      [&]() { return new Tomographer::DenseDM::TSpace::ObservableValueCalculator<tpy::DMTypes>(dmt, A); },
      [&]() { return new tpy::CallableValueCalculator(fig_of_merit); }
        );

  logger.debug([&](std::ostream & stream) {
      stream << "Value calculator set up with fig_of_merit=" << py::repr(fig_of_merit).cast<std::string>();
    });


  // prepare the random walk tasks

  typedef Tomographer::MHRWTasks::MHRandomWalkTask<OurCData, RngType>  OurMHRandomWalkTask;

  // seed for random number generator
  RngType::result_type base_seed =
    (RngType::result_type)std::chrono::system_clock::now().time_since_epoch().count();


  // number of renormalization levels in the binning analysis
  const int recommended_num_samples_last_level = 128;
  if (binning_num_levels <= 0) {
    // choose automatically. Make sure that the last level has ~128 samples to calculate std deviation.
    binning_num_levels = (int)(std::floor(std::log(mhrw_params.n_run/recommended_num_samples_last_level)
                                          / std::log(2)) + 1e-3) ;
    if (binning_num_levels < 1) {
      binning_num_levels = 1;
    }
  }
  if (binning_num_levels < 4) {
    logger.warning([&](std::ostream & stream) {
        stream << "You are using few binning levels = " << binning_num_levels
               << " (perhaps because n_run is low), and the resulting error bars "
               << "might not be reliable." ;
      });
  }
  const tpy::CountIntType binning_last_level_num_samples =
    (tpy::CountIntType)std::ldexp((double)mhrw_params.n_run, - binning_num_levels);
  logger.debug([&](std::ostream & stream) {
      stream << "Binning analysis: " << binning_num_levels << " levels, with "
             << binning_last_level_num_samples << " samples at last level";
    });
  // warn if number of samples @ last level is below recommended value
  if (binning_last_level_num_samples < recommended_num_samples_last_level) {
    logger.warning([&](std::ostream & stream) {
        stream << "The number of samples (" << binning_last_level_num_samples
               << ") at the last binning level is below the recommended value ("
               << recommended_num_samples_last_level << ").  Consider increasing n_run "
               << "or decreasing binning_num_levels.";
      });
  }

  OurCData taskcdat(llh, valcalc, hist_params, binning_num_levels, mhrw_params,
                    base_seed, jumps_method, ctrl_step_size_params, ctrl_converged_params);

  typedef std::chrono::steady_clock StdClockType;
  StdClockType::time_point time_start;

  logger.debug([&](std::ostream & stream) {
      stream << "about to create the task dispatcher.  this pid = " << getpid() << "; this thread id = "
             << std::this_thread::get_id();
    }) ;

  tpy::GilProtectedPyLogger logger_with_gil(logger.parentLogger(), false);

  Tomographer::MultiProc::CxxThreads::TaskDispatcher<OurMHRandomWalkTask,OurCData,tpy::GilProtectedPyLogger>
    tasks(
        &taskcdat, // constant data
        logger_with_gil, // the main logger object -- automatically acquires the GIL for emitting messages
        num_repeats // num_runs
        );

  tpy::setTasksStatusReportPyCallback(tasks, progress_fn, progress_interval_ms, true /* GIL */);

  {
    logger_with_gil.requireGilAcquisition(true);
    py::gil_scoped_release gil_release;

    // and run our tomo process

    time_start = StdClockType::now();

    try {
      tasks.run();
    } catch (Tomographer::MultiProc::TasksInterruptedException & e) {
      // Tasks interrupted
      logger.debug("Tasks interrupted.");
      // acquire GIL for PyErr_Occurred()
      py::gil_scoped_acquire gil_acquire;
      if (PyErr_Occurred() != NULL) {
        // tell pybind11 that the exception is already set
        throw py::error_already_set();
      }
      // no Python exception set?? -- set a RuntimeError via pybind11
      throw;
    } catch (std::exception & e) {
      // another exception
      logger.debug("Inner exception: %s", e.what());
      // acquire GIL for PyErr_Occurred()
      py::gil_scoped_acquire gil_acquire;
      if (PyErr_Occurred() != NULL) {
        // an inner py::error_already_set() was caught & rethrown by MultiProc::CxxThreads
        throw py::error_already_set();
      }
      throw; // error via pybind11
    }

  } // gil released scope
  logger_with_gil.requireGilAcquisition(false);

  auto time_end = StdClockType::now();

  logger.debug("Random walks done.");

  // delta-time, in seconds and fraction of seconds
  std::string elapsed_s = Tomographer::Tools::fmtDuration(time_end - time_start);


  // individual results from each task
  const auto & task_results = tasks.collectedTaskResults();

  // ... aggregated into a full averaged histogram
  auto aggregated_histogram = taskcdat.aggregateResultHistograms(task_results) ;


  py::dict res;

  res["final_histogram"] = tpy::HistogramWithErrorBars(aggregated_histogram.final_histogram);
  res["simple_final_histogram"] = tpy::HistogramWithErrorBars(aggregated_histogram.simple_final_histogram);
  res["elapsed_seconds"] = 1.0e-6 * std::chrono::duration_cast<std::chrono::microseconds>(
      time_end - time_start
      ).count();

  py::list runs_results;
  for (std::size_t k = 0; k < task_results.size(); ++k) {
    const auto & run_result = *task_results[k];
    runs_results.append(
        tpy::MHRandomWalkTaskResult(
            py::cast(tpy::ValueHistogramWithBinningMHRWStatsCollectorResult(run_result.stats_results)),
            tpy::MHRWParams(py::dict("step_size"_a=run_result.mhrw_params.mhwalker_params.step_size),
                            run_result.mhrw_params.n_sweep,
                            run_result.mhrw_params.n_therm,
                            run_result.mhrw_params.n_run),
            run_result.acceptance_ratio
            )
        );
  }
  res["runs_results"] = runs_results;

  // full final report
  std::string final_report;
  { std::ostringstream ss;
    Tomographer::MHRWTasks::ValueHistogramTools::printFinalReport(
        ss, // where to output
        taskcdat, // the cdata
        task_results, // the results
        aggregated_histogram // aggregated
        );
    final_report = ss.str();
    res["final_report"] = final_report;
  }

  // final report of runs only
  { std::ostringstream ss;
    Tomographer::MHRWTasks::ValueHistogramTools::printFinalReport(
        ss, // where to output
        taskcdat, // the cdata
        task_results, // the results
        aggregated_histogram, // aggregated
        0, // width -- use default
        false // don't print the histogram
        );
    res["final_report_runs"] = ss.str();
  }

  logger.debug([&](std::ostream & stream) {
      stream << final_report << "\n";
      stream << "Computation time: " <<  elapsed_s << "\n";
    });

  return res;
}



PyObject * pyTomorunInvalidInputError = NULL;


void py_tomo_tomorun(py::module rootmodule)
{
  auto logger = Tomographer::Logger::makeLocalLogger(TOMO_ORIGIN, *tpy::logger);

  logger.debug("py_tomo_tomorun() ...");

  logger.debug("tomorun module ...");
  
  py::module tomorunmodule = rootmodule.def_submodule(
      "tomorun",
      ( "Perform a random in the full state space of a quantum system according to "
        "our practical, reliable procedure, and collect a histogram of a specific "
        "figure of merit." )
      ) ;

  logger.debug("tomorun.tomorun() ...");

  // the main run call:
  tomorunmodule.def(
      "tomorun", // function name
      &py_tomorun, // fn pointer
      "dim"_a,
      "Exn"_a = Eigen::MatrixXd(),
      "Emn"_a = py::list(),
      "Nm"_a = Eigen::VectorXi(),
      "fig_of_merit"_a = "obs-value"_s,
      "ref_state"_a = Eigen::MatrixXcd(),
      "observable"_a = Eigen::MatrixXcd(),
      "hist_params"_a = tpy::HistogramParams(),
      "mhrw_params"_a = tpy::MHRWParams(),
      "binning_num_levels"_a = -1,
      "num_repeats"_a = std::thread::hardware_concurrency(),
      "progress_fn"_a = py::none(),
      "progress_interval_ms"_a = (int)500,
      "jumps_method"_a = py::str("full"),
      "ctrl_step_size_params"_a = py::dict(),
      "ctrl_converged_params"_a = py::dict(),
      // doc
      ( "tomorun(dim, ...)\n"
        "\n"
        "\n"
        "Produce a histogram of a figure of merit during a random walk in quantum state\n"
        "space according to the distribution :math:`\\mu_{B^n}(\\cdot)` defined in Ref. [1]. The\n"
        "likelihood function is specified with independent POVM effects (see below)."
        "\n"
        "This python function provides comparable functionality to the `tomorun` executable program, and\n"
        "allows for a better seamless interoperability with `NumPy`---all data matrices here are specified\n"
        "as `NumPy` arrays.\n"
        "\n"
        ":param dim: The dimension of the quantum system\n\n"
        ":param Exn: The observed POVM effects, specified as a matrix in which each row is the\n"
        "            X-parameterization of a POVM effect. You may want to specify `Emn` instead,\n"
        "            which may be simpler.\n\n"
        ":param Emn: The observed POVM effects, specified as a list of :math:`\\textit{dim}\\times\\textit{dim}`\n"
        "            matrices.\n\n"
        ":param Nm:  the list of observed frequency counts for each POVM effect in `Emn` or `Exn`.\n\n"
        ":param fig_of_merit:  The choice of the figure of merit to study.  This is either a Python string or a\n"
        "            Python callable.  If it is a string, it must be one of 'obs-value',\n"
        "            'fidelity', 'tr-dist' or 'purif-dist' (see below for more info).  If it is a callable, it\n"
        "            should accept a single argument, the T-parameterization of the density matrix, and should\n"
        "            calculate and return the figure of merit.  The T-parameterization is a matrix :math:`T`\n"
        "            that :math:`\\rho=TT^\\dagger`.\n\n"
        ":param ref_state:  For figures of merit which compare to a reference state ('fidelity', 'tr-dist',\n"
        "            and 'purif-dist'), this is the reference state to calculate the figure of merit with,\n"
        "            specified as a density matrix.\n\n"
        ":param observable:  For the 'obs-value' figure of merit, specify the observable here as a matrix.\n\n"
        ":param hist_params:  The requested range of values to look at when collecting a histogram of the\n"
        "            figure of mert.  This should be a :py:class:`tomographer.HistogramParams`\n"
        "            instance.\n\n"
        ":param mhrw_params:  The parameters of the random walk, including the step size, the sweep size,\n"
        "            the number of thermalization sweeps, and the number of live sweeps.  Specify a\n"
        "            :py:class:`tomographer.MHRWParams` instance here.\n\n"
        ":param binning_num_levels:  The number of levels in the binning analysis [2]. One should make sure\n"
        "            that there are enough bins at the last level to estimate the standard\n"
        "            deviation. This is done automatically by default (or if you specify the value `-1`),\n"
        "            so in normal circumstances you won't have to change the default value.\n\n"
        ":param num_repeats:  The number of independent random walks to run in parallel.\n\n"
        ":param progress_fn:  A python callback function to monitor progress.  The function should accept\n"
        "            a single argument of type :py:class:`tomographer.multiproc.FullStatusReport`.  Check\n"
        "            out :py:class:`tomographer.jpyutil.RandWalkProgressBar` if you are using a\n"
        "            Jupyter notebook.  See below for more information on status progress reporting.\n\n"
        ":param progress_interval_ms: The approximate time interval in milliseconds between two progress\n"
        "            reports.\n\n"
        ":param jumps_method: Method to use for the jumps in the random walk.  This may be either \"full\"\n"
        "            or \"light\".  The \"full\" method is the one described in the paper, with a jump\n"
        "            corresponding to moving the purified bipartite state vector uniformly on the hypersphere.\n"
        "            The \"light\" method is an optimized version, where only an \"elementary rotation\"---a\n"
        "            simple qubit rotation---is applied onto two randomly chosen computational basis elements\n"
        "            on the purified bipartite state vector.  In the end the random walk explores the same space\n"
        "            with the same distribution.  The \"light\" can go much faster especially for large\n"
        "            dimensions, but may be slower to converge.\n\n"
        ":param ctrl_step_size_params: A python dict with parameters to set up the controller which dynamically adjusts\n"
        "            the step size of the random walk during the thermalization runs. The possible keys\n"
        "            are:\n\n"
        "              - 'enabled' (set to 'True' or 'False'): Whether to enable the controller or not. If\n"
        "                disabled, the step size is not automatically adjusted.\n\n"
        "              - 'desired_accept_ratio_min', 'desired_accept_ratio_max': The range in which we would\n"
        "                like to keep the acceptance ratio, by adapting the step size.\n\n"
        "              - 'acceptable_accept_ratio_min', 'acceptable_accept_ratio_max': The range of values which\n"
        "                the acceptance ratio is not to exceed.\n\n"
        "              - 'ensure_n_therm_fixed_params_fraction': Whenever the step size is adjusted, the controller\n"
        "                guarantees that at least this fraction of the given number `n_therm` of thermalization\n"
        "                sweeps is carried out before finishing the thermalization phase.\n\n"
        "            See also :tomocxx:`this doc of the corresponding\n"
        "            C++ controller class <class_tomographer_1_1_m_h_r_w_step_size_controller.html>`.\n\n"
        ":param ctrl_converged_params: A dictionary to set up the controller which dynamically keeps\n"
        "            the random walk running while the error bars from binning haven't converged as\n"
        "            required. The possible keys are:\n\n"
        "              - 'enabled' (set to 'True' or 'False'): Whether to enable the controller or not. If\n"
        "                disabled, the error bars of the histogram bins will not be checked for convergence before\n"
        "                terminating the random walk.\n\n"
        "              - 'max_allowed_unknown', 'max_allowed_unknown_notisolated', 'max_allowed_not_converged':\n"
        "                The maximum allowed number of bins for which the error bars via binning analysis have \n"
        "                the respective convergence status.  Only after all these requirements are met will the\n"
        "                random walk be allowed to finish.\n\n"
        "              - 'check_frequency_sweeps': How often to check for the convergence\n"
        "                of the binning analysis error bars (in number of sweeps).\n\n"
        "            See also :tomocxx:`this doc of the corresponding\n"
        "            C++ controller class <class_tomographer_1_1_m_h_r_w_value_error_bins_converged_controller.html>`.\n"
        "\n"
        "\n"
        ".. rubric:: Figures of merit"
        "\n"
        "\n"
        "The value of the `fig_of_merit` argument may be a Python string, in which case it should be one of\n"
        "the following:\n"
        "\n"
        "  - \"obs-value\": the expectation value of an observable. You should specify the argument\n"
        "    `observable` as a 2-D `NumPy` array specifying the observable you are interested in.\n"
        "\n"
        "  - \"tr-dist\": the trace distance to a reference state. You should specify the argument\n"
        "    `ref_state` as a 2-D `NumPy` array specifying the density matrix of the state which should\n"
        "    serve as reference state.\n"
        "\n"
        "  - \"fidelity\": the (root) fidelity to a reference state [3]. You should specify the argument\n"
        "    `ref_state` as a 2-D `NumPy` array specifying the density matrix of the state which should\n"
        "    serve as reference state. (For the squared fidelity to a pure reference state, see note below.)\n"
        "\n"
        "  - \"purif-dist\": the purified distance to a reference state [5]. You should specify the argument\n"
        "    `ref_state` as a 2-D `NumPy` array specifying the density matrix of the state which should\n"
        "    serve as reference state.\n"
        "\n"
        ".. note:: For the squared fidelity to a pure state (usually preferred in experimental papers),\n"
        "          you should use \"obs-value\" with the observable being the density matrix of the\n"
        "          reference state [4].\n"
        "\n"
        "The value of the `fig_of_merit` argument may also be a Python callable which directly calculates the\n"
        "figure of merit.  It should accept a single argument, the T-parameterization of the density matrix\n"
        "given as a `NumPy` array (defined such that :math:`\\rho=TT^\\dagger`), and should return the value\n"
        "of the figure of merit.  For example, to calculate the purity of the state\n"
        ":math:`\\operatorname{tr}(\\rho^2)`::\n"
        "\n"
        "        import numpy as np\n"
        "        import numpy.linalg as npl\n"
        "        ...\n"
        "        r = tomographer.tomorun.tomorun(...,\n"
        "                                        fig_of_merit=lambda T: npl.norm(np.dot(T,T.T.conj())),\n"
        "                                        ...)\n"
        "\n"
        "\n"
        ".. rubric:: Return value\n"
        "\n"
        "The `tomorun()` function returns a Python dictionary with the following keys and values set:\n"
        "\n"
        "  - ``final_histogram``: a :py:class:`~tomographer.HistogramWithErrorBars` instance with the final "
        "histogram data.  The histogram has the parameters specified in the `hist_params` argument. "
        "The histogram is NOT normalized to a probabilty density; you should call its "
        " :py:meth:`~tomographer.HistogramWithErrorBars.normalized()` method if you need a "
        " normalized histogram.\n\n"
        "  - ``simple_final_histogram``: a :py:class:`~tomographer.HistogramWithErrorBars` obtained "
        "from averaging the raw histograms from each task run, ignoring their error bars from"
        " the binning analysis.  Under normal circumstances there is no "
        "reason you should ignore the binning analysis, so normally you should not be using this "
        "member.  This member is only useful if you want to test the error bars from the binning analysis "
        "against \"naive\" error bars\n\n"
        "  - ``elapsed_seconds``: the total time elapsed while running the random walks, in seconds.\n\n"
        "  - ``final_report_runs``: a human-readable summary report of each task run.  Allows the user to "
        "visually check that all error bars have converged in the binning analysis, and to get an approximate "
        "visual representation of what the histogram looks like for each run.\n\n"
        "  - ``final_report``: a human-readable summary of the whole procedure. This includes the final "
        "report of all the runs contained in ``final_report_runs``, as well as a visual representation of the "
        "final averaged histogram.\n\n"
        "  - ``runs_results``: a list of all the raw results provided by each task run.  Each item of the "
        "list is an instance of :py:class:`tomographer.mhrwtasks.MHRandomWalkTaskResult`, with its `stats_results`"
        " member being a instance of :py:class:`tomographer.ValueHistogramWithBinningMHRWStatsCollectorResult`.\n\n"
        "\n\n"
        ".. rubric:: Status reporting"
        "\n\n"
        "You may receive periodic status reports via a custom Python callback, so that you can stay informed "
        "of the overall progress.  The callback specified to `progress_fn` will be called approximately "
        "every `progress_interval_ms` milliseconds with information on the overall progress given as a "
        ":py:class:`tomographer.multiproc.FullStatusReport` object.  The individual workers provide the following "
        "additional information, formatted within the `data` dictionary attribute of each "
        ":py:class:`~tomographer.multiproc.WorkerStatusReport` object:\n\n"
        "  - ``data['mhrw_params']`` -- a :py:class:`~tomographer.MHRWParams` instance with the current "
        "parameters of the random walk\n\n"
        "  - ``data['acceptance_ratio']`` -- the current acceptance ratio of the Metropolis-Hastings random walk, "
        "as a real value between zero and one. You should try to keep this value around ~0.25.  The acceptance ratio "
        "is not available during the thermalizing runs.\n\n"
        "  - ``data['kstep']`` -- the current iteration step number (an iteration corresponds to creating a jump "
        "proposal, and to jump with a certain probability)\n\n"
        "  - ``data['n_total_iters']`` -- the total number of iterations this random walk is going to complete. "
        "This is equal to ``n_sweep*(n_therm + n_run)``."
        "\n\n"
        ".. rubric:: Footnotes and references"
        "\n\n"
        "| [1] Christandl and Renner, Phys. Rev. Lett. 12:120403 (2012), arXiv:1108.5329\n"
        "| [2] Ambegaokar and Troyer, Am. J. Phys., 78(2):150 (2010), arXiv:0906.0943\n"
        "| [3] The root fidelity is defined as "
        ":math:`F(\\rho,\\sigma)=\\left\\Vert\\rho^{1/2}\\sigma^{1/2}\\right\\Vert_1`, "
        "as in Nielsen and Chuang, \"Quantum Computation and Quantum Information\".\n"
        "| [4] Indeed, for pure :math:`\\rho_\\mathrm{ref}`, "
        ":math:`F^2(\\rho,\\rho_\\mathrm{ref}) = \\mathrm{tr}(\\rho\\rho_\\mathrm{ref})`.\n"
        "| [5] The purified distance, also called \"infidelity\" in the literature, is "
        "defined as :math:`P(\\rho,\\sigma) = \\sqrt{1 - F^2(\\rho,\\sigma)}`.\n"
        "\n\n"
          )
      );

  logger.debug("tomorun.TomorunInvalidInputError ...");

  tpy::registerExceptionWithDocstring<TomorunInvalidInputError>(
      tomorunmodule,
      "TomorunInvalidInputError",
      tpy::TomographerCxxErrorObj.ptr(),
      // docstring:
      "Exception which gets raised if invalid input is supplied to the "
      ":py:func:`tomographer.tomorun.tomorun()` function.");

  logger.debug("py_tomo_tomorun() complete.");
}
