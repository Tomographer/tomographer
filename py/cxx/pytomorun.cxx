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

#include "py_operators_p.h"

#include <limits.h> // CHAR_BIT
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
  typedef tpy::RealScalar ValueType;
  
  CallableValueCalculator(py::object fn_)
    : fn(fn_)
  {
  }

  tpy::RealScalar getValue(const Eigen::Ref<const tpy::CplxMatrixType> & T) const
  {
    py::gil_scoped_acquire gil_acquire;
    return fn(py::cast(T)).cast<tpy::RealScalar>();
  }


private:
  py::object fn;
};


typedef Tomographer::DenseDM::DMTypes<Eigen::Dynamic, tpy::RealScalar> DMTypes;

typedef Tomographer::MHRWParams<Tomographer::MHWalkerParamsStepSize<tpy::RealScalar>,
                                tpy::IterCountIntType>
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

  typedef Tomographer::MHWalkerParamsStepSize<tpy::RealScalar> WalkerParams;

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

  LLH_MHWalker(LLH_MHWalker_Which which_, const DenseLLHType & llh_,
               RngType & rng_, LoggerType & baselogger)
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
  tpy::RealScalar,
  Tomographer::DenseDM::TSpace::FidelityToRefCalculator<tpy::DMTypes, tpy::RealScalar>,
  Tomographer::DenseDM::TSpace::PurifDistToRefCalculator<tpy::DMTypes, tpy::RealScalar>,
  Tomographer::DenseDM::TSpace::TrDistToRefCalculator<tpy::DMTypes, tpy::RealScalar>,
  Tomographer::DenseDM::TSpace::ObservableValueCalculator<tpy::DMTypes>,
  tpy::CallableValueCalculator
  > ValueCalculator;


typedef Tomographer::MHRWTasks::ValueHistogramTools::CDataBase<
  ValueCalculator, // our value calculator
  true, // use binning analysis
  Tomographer::MHWalkerParamsStepSize<tpy::RealScalar>, // MHWalkerParams
  RngType::result_type, // RngSeedType
  tpy::IterCountIntType, // IterCountIntType
  tpy::CountRealType, // CountRealType
  tpy::HistCountIntType // HistCountIntType
  >
  CDataBaseType;


//
// We need to define a class which adds the capacity of creating the "master" random walk
// object to the engine in Tomographer::MHRWTasks::ValueHistogramTools, which take care of
// running the random walks etc. as needed.
//
struct OurCData : public CDataBaseType
{
public:

  OurCData(const DenseLLH & llh_, // data from the the tomography experiment
	   ValueCalculator valcalc, // the figure-of-merit calculator
	   HistogramParams hist_params, // histogram parameters
	   int binning_num_levels, // number of binning levels in the binning analysis
	   tpy::MHRWParams mhrw_params, // parameters of the random walk
	   std::vector<RngType::result_type> task_seeds, // the random seeds to initialize the
                                                         // random number generators for each task
           tpy::LLH_MHWalker_Which jumps_method_which_, // enum value (LLH_MHWalker_Which)
           py::dict ctrl_step_size_params_, // parameters for step size controller
           py::dict ctrl_converged_params_ // parameters for value bins converged controller
      )
    : CDataBaseType(
        valcalc, hist_params, binning_num_levels,
        tpy::CxxMHRWParamsType(
            tpy::pyMHWalkerParamsFromPyObj<Tomographer::MHWalkerParamsStepSize<tpy::RealScalar> >(
                mhrw_params.mhwalker_params),
            mhrw_params.n_sweep, mhrw_params.n_therm, mhrw_params.n_run),
        task_seeds),
      llh(llh_),
      jumps_method_which(jumps_method_which_),
      ctrl_step_size_params(ctrl_step_size_params_),
      ctrl_converged_params(ctrl_converged_params_)
  {
  }

  const DenseLLH llh;

  const tpy::LLH_MHWalker_Which jumps_method_which;
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
        jumps_method_which,
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

    Tomographer::MHRWMovingAverageAcceptanceRatioStatsCollector<tpy::IterCountIntType>
      movavg_accept_stats(mvavg_numsamples);

    auto ctrl_step = 
      Tomographer::mkMHRWStepSizeController<tpy::CxxMHRWParamsType>(
          movavg_accept_stats,
          baselogger,
          ar_params[0],
          ar_params[1],
          ar_params[2],
          ar_params[3],
          ensure_n_therm_fixed_params_fraction
          );

    logger.debug("Created auto step size controller.") ;

    tpy::IterCountIntType check_frequency_sweeps = 0;
    Eigen::Index max_allowed[3] = {0};
    double max_add_run_iters = 1.5;
    {
      py::gil_scoped_acquire gilacq;
      if (ctrl_converged_params.attr("get")("enabled", true).cast<bool>()) {
        check_frequency_sweeps =
          ctrl_converged_params.attr("get")("check_frequency_sweeps", 1024).cast<tpy::IterCountIntType>();
        max_allowed[0] =
          ctrl_converged_params.attr("get")("max_allowed_unknown",
                                            1+2*histogram_params.num_bins/100).template cast<Eigen::Index>();
        max_allowed[1] =
          ctrl_converged_params.attr("get")("max_allowed_unknown_notisolated",
                                            1+histogram_params.num_bins/100).template cast<Eigen::Index>();
        max_allowed[2] =
          ctrl_converged_params.attr("get")("max_allowed_not_converged",
                                            1+histogram_params.num_bins/200).template cast<Eigen::Index>();
        max_add_run_iters =
          ctrl_converged_params.attr("get")("max_add_run_iters", 1.5).cast<double>();
      } else {
        check_frequency_sweeps = 0;
        max_allowed[0] = std::numeric_limits<Eigen::Index>::max();
        max_allowed[1] = std::numeric_limits<Eigen::Index>::max();
        max_allowed[2] = std::numeric_limits<Eigen::Index>::max();
        max_add_run_iters = -1.0;
      }
    }

    // value error bins convergence controller
    auto ctrl_convergence = 
      Tomographer::mkMHRWValueErrorBinsConvergedController<tpy::IterCountIntType>(
          value_stats, baselogger,
          check_frequency_sweeps,
          max_allowed[0], max_allowed[1], max_allowed[2],
          max_add_run_iters
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



template<typename T, typename LocalLoggerType>
inline T dict_pop_type_or_warning(py::dict dict, std::string key, LocalLoggerType & logger,
                                  std::string warningmsg, const T & default_val)
{
  if (!dict.contains(py::cast(key))) {
    logger.warning(warningmsg);
    return default_val;
  }
  return dict.attr("pop")(py::cast(std::move(key))).cast<T>();
}
template<typename T, typename LocalLoggerType>
inline T dict_pop_type_or_warning(py::dict dict, std::string key, LocalLoggerType & logger,
                                  std::string warningmsg) // uses default-constructed T() as default
{
  return dict_pop_type_or_warning(std::move(dict), std::move(key), logger, std::move(warningmsg), T()) ;
}


#ifndef PYTOMORUN_RANDOM_DEVICE
#define PYTOMORUN_RANDOM_DEVICE "/dev/random"
#endif

template<typename LocalLoggerType>
std::vector<RngType::result_type> get_random_device_seeds(std::size_t num_seeds,
                                                          LocalLoggerType & logger)
{
  logger.info([&](std::ostream & stream) {
      stream << "Collecting " << num_seeds << " random seed" << (num_seeds!=1 ? "s" : "")
             << " from random device ...";
    });
  std::random_device devrandom(PYTOMORUN_RANDOM_DEVICE);
  std::uniform_int_distribution<RngType::result_type> dseed(
      std::numeric_limits<RngType::result_type>::min(),
      std::numeric_limits<RngType::result_type>::max()
      ); // full range of result_type

  std::vector<RngType::result_type> seeds((std::size_t)num_seeds);
  for (std::size_t k = 0; k < num_seeds; ++k) {
    RngType::result_type seed = dseed(devrandom);
    logger.debug([&](std::ostream & stream) { stream << "Seed for task #" << k << " = " << seed; });
    seeds[(std::size_t)k] = seed;
  }

  logger.info("... seed(s) collected.");
  return seeds;
}


py::object py_tomorun(
    const int dim,
    py::kwargs kwargs
    )
{
  Tomographer::Logger::LocalLogger<tpy::PyLogger> logger(TOMO_ORIGIN, *tpy::logger);

  logger.debug("py_tomorun()");

  //
  // first, read out the POVM effects & frequencies from kwargs
  //

  typedef tpy::DMTypes::MatrixType MatrixType;
  //typedef Eigen::Matrix<tpy::DMTypes::ComplexScalar,Eigen::Dynamic,Eigen::Dynamic> DynCMatType;
  typedef Eigen::Matrix<tpy::DMTypes::RealScalar,Eigen::Dynamic,Eigen::Dynamic> DynRMatType;
  typedef Eigen::Matrix<DenseLLH::IntFreqType,Eigen::Dynamic,1> NmType;

  tpy::DMTypes dmt(dim);

  // prepare llh object
  DenseLLH llh(dmt);

  if (!kwargs.contains("Emn"_s) && !kwargs.contains("Exn"_s)) {
    throw TomorunInvalidInputError("No measurements specified. Please specify either the `Emn' "
                                   "or the `Exn' argument");
  }
  if (!kwargs.contains("Nm"_s)) {
    throw TomorunInvalidInputError("No measurement outcome counts specified. "
                                   "Please specify the `Nm' argument.");
  }

  NmType Nm = kwargs.attr("pop")("Nm"_s).cast<NmType>();

  if (kwargs.contains("Exn"_s)) {
    // use Exn
    const DynRMatType Exn = kwargs.attr("pop")("Exn"_s).cast<DynRMatType>();
    if (kwargs.contains("Emn"_s)) { // error: both Exn & Emn specified
      throw TomorunInvalidInputError("You can't specify both Exn and Emn arguments");
    }
    if (Exn.cols() != dmt.dim2()) {
      throw TomorunInvalidInputError("Exn argument is expected to have exactly dim^2 = "
                                     + std::to_string(dmt.dim2()) + " columns");
    }
    if (Exn.rows() != Nm.rows()) {
      throw TomorunInvalidInputError("Mismatch in number of measurements: Exn.rows()="
                                     + std::to_string(Exn.rows()) + " but Nm.rows()="
                                     + std::to_string(Nm.rows()));
    }
    for (Eigen::Index k = 0; k < Nm.rows(); ++k) {
      llh.addMeasEffect(Exn.row(k).transpose(), Nm(k), true);
    }
  } else {
    // use Emn
    tomographer_assert(kwargs.contains("Emn"_s)) ; // we did this check already before

    py::object Emn = kwargs.attr("pop")("Emn"_s);
    
    const std::size_t len_Emn = py::len(Emn);
    if (len_Emn != (std::size_t)Nm.rows()) {
      throw TomorunInvalidInputError("Mismatch in number of measurements: len(Emn)="
                                     + std::to_string(len_Emn) +
                                     " but Nm.rows()=" + std::to_string(Nm.rows()));
    }
    for (Eigen::Index k = 0; k < Nm.rows(); ++k) {
      MatrixType POVMeffect = Emn[py::cast(k)].cast<MatrixType>();
      llh.addMeasEffect(POVMeffect, Nm(k), true);
    }
  }

  logger.debug([&](std::ostream & ss) {
      ss << "\n\nllh.Exn: size="<<llh.Exn().size()<<"\n"
	 << llh.Exn() << "\n";
      ss << "\n\nllh.Nx: size="<<llh.Nx().size()<<"\n"
	 << llh.Nx() << "\n";
    });


  // prepare figure of merit

  MatrixType T_ref(dmt.initMatrixType());
  MatrixType rho_ref(dmt.initMatrixType());
  MatrixType A(dmt.initMatrixType());

  py::object fig_of_merit = py::none();
  if (kwargs.contains("fig_of_merit")) {
    fig_of_merit = kwargs.attr("pop")("fig_of_merit"_s);
  } else {
    logger.warning("The `fig_of_merit' argument wasn't specified. I'm assuming `fig_of_merit=\"obs-value\"', "
                   "and will expect an `observable=' argument.");
    fig_of_merit = "obs-value"_s;
  }

  bool fig_of_merit_callable = py::hasattr(fig_of_merit, "__call__");
  std::string fig_of_merit_s;
  if (fig_of_merit_callable) {
    fig_of_merit_s = "<custom>";
  } else {
    fig_of_merit_s = fig_of_merit.cast<std::string>();
  }

  if (fig_of_merit_s == "fidelity" || fig_of_merit_s == "tr-dist" || fig_of_merit_s == "purif-dist") {

    if (!kwargs.contains("ref_state"_s)) {
      throw TomorunInvalidInputError("Expected `ref_state=' argument for figure of merit '"+fig_of_merit_s+"'");
    }
    MatrixType ref_state = kwargs.attr("pop")("ref_state"_s).cast<MatrixType>();
    
    // allow the user to also specify observable=, but warn that the argument will be ignored
    if (kwargs.contains("observable"_s)) {
      kwargs.attr("pop")("observable"_s);
      logger.warning("Ignoring additional argument `observable=' which is not used for "
                     "figure of merit '"+fig_of_merit_s+"'");
    }

    if (ref_state.rows() != dmt.dim() || ref_state.cols() != dmt.dim()) {
      throw TomorunInvalidInputError(streamstr("Expected " << dmt.dim() << " x " << dmt.dim() << " complex matrix as "
                                               "`ref_state=' argument for fig_of_merit='"<<fig_of_merit_s<<"'")) ;
    }

    Eigen::SelfAdjointEigenSolver<MatrixType> eig(ref_state);

    typedef typename Eigen::SelfAdjointEigenSolver<MatrixType>::RealVectorType RealVectorType;
    
    MatrixType U = eig.eigenvectors();
    RealVectorType d = eig.eigenvalues();
  
    Tomographer::MathTools::forcePosVecKeepSum<RealVectorType>(
        d,
        Eigen::NumTraits<tpy::RealScalar>::dummy_precision()
        );

    rho_ref = U * d.asDiagonal() * U.adjoint();
    T_ref = U * d.cwiseSqrt().asDiagonal() * U.adjoint();

  } else if (fig_of_merit_s == "obs-value") {

    if (!kwargs.contains("observable"_s)) {
      throw TomorunInvalidInputError("Expected `observable=' argument for figure of merit '"+fig_of_merit_s+"'");
    }
    // allow the user to also specify ref_state=, but warn that the argument will be ignored
    if (kwargs.contains("ref_state"_s)) {
      kwargs.attr("pop")("ref_state"_s);
      logger.warning("Ignoring additional argument `ref_state=' which is not used for "
                     "figure of merit '"+fig_of_merit_s+"'");
    }

    MatrixType observable = kwargs.attr("pop")("observable"_s).cast<MatrixType>();
    
    if (observable.rows() != dmt.dim() || observable.cols() != dmt.dim()) {
      throw TomorunInvalidInputError(streamstr("Expected " << dmt.dim() << " x " << dmt.dim() << " complex matrix as "
                                               "`observable=' argument for fig_of_merit='obs-value'")) ;
    }

    A = observable;
    
  } else if (fig_of_merit_callable) {

    // ok, custom callable
    logger.debug("Using custom callable as figure of merit.");

    // allow the user to also specify observable= and/or ref_state=, but warn that those
    // arguments will be ignored
    if (kwargs.contains("ref_state"_s)) {
      kwargs.attr("pop")("ref_state"_s);
      logger.warning("Ignoring additional argument `ref_state=' which is not used for "
                     "a custom callable figure of merit");
    }
    if (kwargs.contains("observable"_s)) {
      kwargs.attr("pop")("observable"_s);
      logger.warning("Ignoring additional argument `observable=' which is not used for "
                     "a custom callable figure of merit");
    }

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
      [&]() { return new Tomographer::DenseDM::TSpace::FidelityToRefCalculator<tpy::DMTypes, tpy::RealScalar>(T_ref); },
      [&]() { return new Tomographer::DenseDM::TSpace::PurifDistToRefCalculator<tpy::DMTypes, tpy::RealScalar>(T_ref); },
      [&]() { return new Tomographer::DenseDM::TSpace::TrDistToRefCalculator<tpy::DMTypes, tpy::RealScalar>(rho_ref); },
      [&]() { return new Tomographer::DenseDM::TSpace::ObservableValueCalculator<tpy::DMTypes>(dmt, A); },
      [&]() { return new tpy::CallableValueCalculator(fig_of_merit); }
        );

  logger.debug([&](std::ostream & stream) {
      stream << "Value calculator set up with fig_of_merit=" << py::repr(fig_of_merit).cast<std::string>();
    });

  //
  // Get the params for the histogram and the mhrw
  //
  const tpy::HistogramParams hist_params =
    dict_pop_type_or_warning<tpy::HistogramParams>(
        kwargs, "hist_params", logger,
        "`hist_params=' argument not specified, using possibly meaningless default params.");

  if (!kwargs.contains("mhrw_params"_s)) {
    throw TomorunInvalidInputError("Please specify the parameters of the random walk using the "
                                   "`mrhw_params=' argument.");
  }

  const tpy::MHRWParams mhrw_params =
    kwargs.attr("pop")("mhrw_params"_s).cast<tpy::MHRWParams>();
  
  // some validity checks
  if (hist_params.num_bins < 1) {
    throw TomorunInvalidInputError("Invalid hist_params: must have num_bins >= 1") ;
  }
  if (mhrw_params.n_sweep < 1) {
    throw TomorunInvalidInputError("Invalid mhrw_params: must have n_sweep >= 1") ;
  }
  if (mhrw_params.n_therm < 0) {
    throw TomorunInvalidInputError("Invalid mhrw_params: must have n_therm >= 0") ;
  }
  if (mhrw_params.n_run < 0) {
    throw TomorunInvalidInputError("Invalid mhrw_params: must have n_run >= 0") ;
  }

  // get number of repetitions of the random walk task -- defaults to number of available cores
  const tpy::TaskCountIntType num_repeats =
    kwargs.attr("pop")("num_repeats"_s, std::thread::hardware_concurrency()).cast<tpy::TaskCountIntType>();

  if (num_repeats < 1) {
    throw TomorunInvalidInputError("num_repeats must be >= 1") ;
  }

  // prepare the random walk tasks

  typedef Tomographer::MHRWTasks::MHRandomWalkTask<OurCData, RngType>  OurMHRandomWalkTask;

  // seed for random number generator
  py::object rng_base_seed = py::none();
  if (kwargs.contains("rng_base_seed"_s)) {
    rng_base_seed = kwargs.attr("pop")("rng_base_seed"_s);
  }
  // parse rng seed argument
  RngType::result_type base_seed = 0;
  bool rng_seed_use_random_device_all = false;
  if (rng_base_seed.is_none()) {
    // argument not given, or given but passed `None'
    base_seed = (RngType::result_type) std::chrono::system_clock::now().time_since_epoch().count();
  } else if (py::isinstance<py::str>(rng_base_seed)) {
    // a string, expected 'random-device' or 'random-device-all'
    const std::string base_seed_arg = rng_base_seed.cast<std::string>();
    if (base_seed_arg == "random-device") {
      base_seed = get_random_device_seeds(1, logger)[0];
    } else if (base_seed_arg == "random-device-all") {
      rng_seed_use_random_device_all = true;
    } else {
      throw TomorunInvalidInputError(streamstr("Invalid argument for rng_base_seed = '" << base_seed_arg << "'"));
    }
  } else {
    // go through py::int_ first, in case the given integer is too large to represent on
    // RngType::result_type
    py::int_ base_seed_int = rng_base_seed.cast<py::int_>();

    if (pyop(base_seed_int) > py::int_(std::numeric_limits<RngType::result_type>::max())) {
      // in case the specified integer is too large, keep only the N lower
      // weight bits -- Do this from the python side with arbitrary precision,
      // to avoid headaches with overflow in C/C++
      logger.warning([&](std::ostream & stream) {
          stream << "Too large integer given as `rng_base_seed' -- only the "
                 << sizeof(RngType::result_type)*CHAR_BIT
                 << " lower-weight bits will be used (value is platform-dependent).";
        }) ;
      pyop mask = ( pyop(py::int_(1)) << py::int_(sizeof(RngType::result_type)*CHAR_BIT) ) - py::int_(1);
      base_seed_int = (pyop(base_seed_int) & mask).object();
    }

    // finally, get the seed as a RngType::result_type
    base_seed = base_seed_int.cast<RngType::result_type>();
  }

  // build list of seeds for the tasks.
  std::vector<RngType::result_type> task_seeds((std::size_t)num_repeats, 0);
  if ( ! rng_seed_use_random_device_all ) {
    // output the base rng seed
    logger.longdebug([&](std::ostream & stream) {
        stream << "Base RNG seed = " << base_seed << "\n";
      }) ;
    for (std::size_t k = 0; k < (std::size_t)num_repeats; ++k) {
      task_seeds[k] = base_seed + (RngType::result_type)k;
    }
  } else {
    // read all seeds from random device
    task_seeds = get_random_device_seeds((std::size_t)num_repeats, logger);
  }


  // number of renormalization levels in the binning analysis
  // Defaults to -1 --> autodetect
  int binning_num_levels = kwargs.attr("pop")("binning_num_levels"_s, -1).cast<int>();
  
  const tpy::IterCountIntType recommended_num_samples_last_level = 128;
  binning_num_levels = Tomographer::sanitizeBinningLevels(binning_num_levels, mhrw_params.n_run,
                                                          recommended_num_samples_last_level,
                                                          logger) ;

  // make sure n_run is a multiple of the number of samples needed to flush the
  // whole binning analysis
  const auto binning_samples_size = 1 << binning_num_levels;
  if ( (mhrw_params.n_run % binning_samples_size) != 0) {
    logger.warning([&](std::ostream & stream) {
        stream << "The number of run sweeps (="<<mhrw_params.n_run<<") is not a multiple of the "
               << "binning analysis sample size (="<<binning_samples_size<<"), this could lead to samples "
               << "being ignored by the error analysis (you should avoid this)! Please strongly consider "
               << "adjusting `mhrw_params.n_run' or `binning_num_levels'.";
      }) ;
  }

  const std::string jumps_method = kwargs.attr("pop")("jumps_method"_s, "full"_s).cast<std::string>();
  tpy::LLH_MHWalker_Which jumps_method_which = tpy::LLH_MHWalker_Full;

  if (jumps_method == "light") {
    jumps_method_which = tpy::LLH_MHWalker_Light;
  } else if (jumps_method == "full") {
    jumps_method_which = tpy::LLH_MHWalker_Full;
  } else {
    throw TomorunInvalidInputError("Invalid jumps method: '" + jumps_method + "'");
  }

  // controller parameters -- default to empty dictionaries
  py::dict ctrl_step_size_params =
    kwargs.attr("pop")("ctrl_step_size_params"_s, py::dict()).cast<py::dict>() ;
  py::dict ctrl_converged_params =
    kwargs.attr("pop")("ctrl_converged_params"_s, py::dict()).cast<py::dict>() ;

  py::object progress_fn = kwargs.attr("pop")("progress_fn"_s, py::none());
  int progress_interval_ms = kwargs.attr("pop")("progress_interval_ms"_s, 500).cast<int>();

  //
  // At this point, we should have consumed all our arguments. Anything left in `kwargs'
  // is an error.
  //
  if (py::len(kwargs)) {
    throw TomorunInvalidInputError("Unknown extra arguments given: " +
                                   (", "_s.attr("join")(kwargs.attr("keys")())).cast<std::string>()) ;
  }

  //
  // Prepare task dispatcher, do some GIL management, and run the tasks.
  //

  OurCData taskcdat(llh, valcalc, hist_params, binning_num_levels, mhrw_params,
                    task_seeds, jumps_method_which, ctrl_step_size_params, ctrl_converged_params);

  logger.debug([&](std::ostream & stream) {
      stream << "about to create the task dispatcher.  this pid = " << getpid() << "; this thread id = "
             << std::this_thread::get_id();
    }) ;

  tpy::GilProtectedPyLogger logger_with_gil(logger.parentLogger(), false);

  Tomographer::MultiProc::CxxThreads::TaskDispatcher<OurMHRandomWalkTask,OurCData,tpy::GilProtectedPyLogger,
                                                     tpy::TaskCountIntType>
    tasks(
        &taskcdat, // constant data
        logger_with_gil, // the main logger object -- automatically acquires the GIL for emitting messages
        num_repeats // num_runs
        );

  tpy::setTasksStatusReportPyCallback(tasks, progress_fn, progress_interval_ms, true /* GIL */);

  typedef std::chrono::steady_clock StdClockType;
  StdClockType::time_point time_start;

  {
    logger_with_gil.requireGilAcquisition(true);
    py::gil_scoped_release gil_release;

    // and run our tomo process

    time_start = StdClockType::now();

    try {
      tasks.run();
    } catch (tpy::PyFetchedException & pyerr) {

      // acquire GIL for PyErr_Restore()
      py::gil_scoped_acquire gil_acquire;

      pyerr.restorePyException();
      throw py::error_already_set();
      
    } catch (Tomographer::MultiProc::TasksInterruptedException & e) {

      // acquire GIL for PyErr_Occurred()
      py::gil_scoped_acquire gil_acquire;

      // Tasks interrupted
      logger.debug("Tasks interrupted."); // needs GIL, which we have acquired

      if (PyErr_Occurred() != NULL) {
        // tell pybind11 that the exception is already set
        throw py::error_already_set();
      }
      // no Python exception set?? -- set a RuntimeError via pybind11
      throw;

    } catch (std::exception & e) {

      // acquire GIL for PyErr_Occurred()
      py::gil_scoped_acquire gil_acquire;

      // another exception
      logger.debug("Inner exception: %s", e.what()); // needs GIL, which we have acquired

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
        "                random walk be allowed to finish (or until 'max_add_run_iters' faction of run sweeps is\n"
        "                exceeded).  Default: `max_allowed_unknown = 1 + 2% of num_bins`,\n"
        "                `max_allowed_unknown_notisolated = 1 + 1% of bins` and\n"
        "                `max_allowed_not_converged = 1 + .5% of bins`.\n\n"
        "              - 'check_frequency_sweeps': How often to check for the convergence\n"
        "                of the binning analysis error bars (in number of sweeps).\n\n"
        "              - 'max_add_run_iters': End the random walk after a certain amount runs\n"
        "                regardless of bins error bars convergence status. Specify the amount as\n"
        "                a fraction of the set number of run sweeps, e.g. a value of 1.5 prolongs\n"
        "                the random walk by at most 50% of the run sweeps. Set to a negative value\n"
        "                to run as long as necessary to make error bars converge as requested.\n\n"
        "            See also :tomocxx:`this doc of the corresponding C++ controller\n"
        "            class <class_tomographer_1_1_m_h_r_w_value_error_bins_converged_controller.html>`.\n\n"
        ":param rng_base_seed: The base seed to use for the random number generator.  If `None` (the default), a\n"
        "            base seed is generated based on the current time.  Each different run of the random walk\n"
        "            is seeded with incremental seeds starting with the base seed (e.g., if the base seed is \n"
        "            910533, the different tasks are given the seeds 910533, 910534, 910535, ...).  Alternatively,\n"
        "            you may specify `rng_base_seed='random-device'` to read a single base seed from a system\n"
        "            random device (e.g. ``/dev/random``), or `rng_base_seed='random-device-all'` to read a list of\n"
        "            seeds from the random device to use for each individual random walk, instead of using\n"
        "            incremental seeds.\n"
        "            \n"
        "            .. versionadded:: 5.3\n"
        "               Added the `rng_base_seed` argument\n"
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
