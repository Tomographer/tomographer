
// This should be first included header, it includes pybind11:: as py::
#include <tomographerpy/common.h>

#include <tomographer/tools/loggers.h>
#include <tomographer/densedm/dmtypes.h>
#include <tomographer/densedm/tspacellhwalker.h>
#include <tomographer/densedm/tspacefigofmerit.h>
#include <tomographer/mhrw.h>
#include <tomographer/mhrwstepsizecontroller.h>
#include <tomographer/mhrwvalueerrorbinsconvergedcontroller.h>
#include <tomographer/mhrwtasks.h>
#include <tomographer/mhrw_valuehist_tools.h>
#include <tomographer/multiprocthreads.h>
#include <tomographer/valuecalculator.h>

#include <tomographerpy/pylogger.h>
#include <tomographerpy/pygil.h>
#include <tomographerpy/pyhistogram.h>
#include <tomographerpy/pymultiproc.h>
#include <tomographerpy/pymhrw.h>
#include <tomographerpy/pymhrwtasks.h>
#include <tomographerpy/pydensedm.h>
#include <tomographerpy/exc.h>

// Use "xyz"_a for specifying Python keyword arguments and "xyz"_s for creating
// Python strings
using namespace pybind11::literals;

//
// Our global module-level logger
//
static tpy::PyLogger * pylogger = nullptr;

//
// C++/Python Exception for errors which occur in this module
//
TOMOGRAPHER_DEFINE_MSG_EXCEPTION(MyCustomModuleError, "Error in my_custom_module: ") ;


//
// Write our C++ code as in "test/minimal_tomorun.cxx",
// "test/minimal_tomorun_controlled.cxx" and/or
// "test/minimal_single_random_walk.cxx"
//

typedef tpy::DMTypes::MatrixType MatrixType;

typedef Tomographer::DenseDM::IndepMeasLLH<tpy::DMTypes> DenseLLH;

typedef std::mt19937 RngType;

typedef Tomographer::DenseDM::TSpace::ObservableValueCalculator<tpy::DMTypes>
  ValueCalculator;

// tpy::MHRWParamsType and Python's `MHRWType` have an abstract py::object as
// parameter for the MHWalker (so it can be used for any type of random walk),
// but the MHWalker we use here needs the specific type
// MHWalkerParamsStepSize<RealType>: it needs this MHRWParams type:
typedef Tomographer::MHRWParams<Tomographer::MHWalkerParamsStepSize<tpy::RealType>,
                                tpy::CountIntType>  CxxMHRWParamsType;

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
  //
  // Construct the CData object. Note that here, the GIL is still held so we
  // don't need to worry about acquiring it.
  //
  OurCData(const DenseLLH & llh_, // data from the the tomography experiment
	   ValueCalculator valcalc, // the figure-of-merit calculator
	   tpy::HistogramParams hist_params, // histogram parameters
           tpy::MHRWParams mhrw_params, // parameters of the random walk
           py::dict p // various optional further user-given parameters
      )
    : CDataBase<ValueCalculator,true>(
        // parameters for Tomographer::MHRWTasks::ValueHistogramTools::CDataBase
        // cosntructor:
        valcalc,
        hist_params,
        p.attr("get")("binning_num_levels", 7).cast<int>(),
        CxxMHRWParamsType(
            tpy::pyMHWalkerParamsFromPyObj<Tomographer::MHWalkerParamsStepSize<tpy::RealType> >(
                mhrw_params.mhwalker_params
                ),
            mhrw_params.n_sweep,
            mhrw_params.n_therm,
            mhrw_params.n_run ),
        // base_seed: just give current time as seed for RNG
        (RngType::result_type)std::chrono::system_clock::now()
          .time_since_epoch().count()
        ),
      llh(llh_),
      params(p)
  {
  }

  const DenseLLH llh;

  // optional parameters for us given by the user
  const py::dict params;

  // See the MHRandomWalkTaskCData type interface: this class is the "stats
  // results" of the random walk task.  (MHRWStatsResultsBaseType from
  // ValueHistogramTools is a convenience class which contains the result
  // histogram type of the underlying value-histogram-stats-collector.)
  struct MHRWStatsResultsType : public MHRWStatsResultsBaseType
  {
    // the value result is always the first of the stats collector tuple, store
    // this result
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
    auto logger = Tomographer::Logger::makeLocalLogger(TOMO_ORIGIN, baselogger) ;

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
    // immediately re-releasing the GIL. So a simple methodology is to simply
    // surround any Python-related calls with that macro.
    //

    // the MHWalker type
    Tomographer::DenseDM::TSpace::LLHMHWalker<DenseLLH,Rng,LoggerType> mhwalker(
        llh.dmt.initMatrixType(),
	llh,
	rng,
	baselogger
	);

    logger.debug("Created MHWalker.") ;

    auto value_stats = createValueStatsCollector(baselogger);

    logger.debug("Created value stats collector.") ;


    Tomographer::MHRWMovingAverageAcceptanceRatioStatsCollector<>
      movavg_accept_stats(
          TPY_EXPR_WITH_GIL( params.attr("get")("num_samples", 2048).cast<int>() )
          );

    auto ctrl_step = 
      Tomographer::mkMHRWStepSizeController<MHRWParamsType>(
          movavg_accept_stats,
          baselogger
          // we could specify here the desired & acceptable ranges for
          // acceptance ratio ... just leave defaults for this example
          );

    logger.debug("Created auto step size controller.") ;

    // value error bins convergence controller
    auto ctrl_convergence = 
      Tomographer::mkMHRWValueErrorBinsConvergedController(
          value_stats,
          baselogger,
          1024, // check_frequency_sweeps -- reasonable default
          TPY_EXPR_WITH_GIL( params.attr("get")("max_allowed_unknown", 2).cast<Eigen::Index>() ),
          TPY_EXPR_WITH_GIL( params.attr("get")("max_allowed_unknown_notisolated", 0).cast<Eigen::Index>() ),
          TPY_EXPR_WITH_GIL( params.attr("get")("max_allowed_not_converged", 0).cast<Eigen::Index>() )
          );

    logger.debug("Created bins convergence controller.") ;

    // combine controllers:
    auto ctrl_combined =
      Tomographer::mkMHRWMultipleControllers(ctrl_step, ctrl_convergence);

    // combine stats collectors:
    auto stats = mkMultipleMHRWStatsCollectors(value_stats, movavg_accept_stats);

    logger.debug("random walk set up, ready to go") ;

    run(mhwalker, stats, ctrl_combined);
  }

};




//
// HERE IS THE MAIN C++ FUNCTION DOING THE WORK
//


py::object run_function(
    int dim,
    const py::list& Emn,
    const Eigen::VectorXi& Nm,
    const tpy::HistogramParams& hist_params,
    const tpy::MHRWParams& mhrw_params,
    int num_repeats,
    py::kwargs params
    )
{
  auto logger = Tomographer::Logger::makeLocalLogger(TOMO_ORIGIN, *pylogger);

  logger.debug("run_function()");

  tpy::DMTypes dmt(dim);
  DenseLLH llh(dmt);

  if (py::len(Emn) != (std::size_t)Nm.rows()) {
    throw MyCustomModuleError(streamstr("Mismatch in number of measurements: len(Emn)=" << py::len(Emn)
                                        << " but Nm.rows()=" << Nm.rows()));
  }
  for (std::size_t k = 0; k < (std::size_t)Nm.rows(); ++k) {
    MatrixType POVMeffect = Emn[k].cast<MatrixType>();
    llh.addMeasEffect(POVMeffect, Nm(k), true);
  }

  logger.debug([&](std::ostream & ss) {
      ss << "\n\nExn: size="<<llh.Exn().size()<<"\n"
	 << llh.Exn() << "\n";
      ss << "\n\nNx: size="<<llh.Nx().size()<<"\n"
	 << llh.Nx() << "\n";
    });

  // prepare the ValueCalculator -- Expectation value of |0><0|
  MatrixType proj0 = dmt.initMatrixType();
  proj0(0,0) = 1;

  ValueCalculator valcalc(dmt, proj0);

  // prepare the random walk tasks
  typedef Tomographer::MHRWTasks::MHRandomWalkTask<OurCData, RngType>  OurMHRandomWalkTask;

  OurCData taskcdat(llh, valcalc, hist_params, mhrw_params, params);

  logger.debug("Created task cdata.") ;

  tpy::GilProtectedPyLogger gil_protected_logger(logger.parentLogger());
  // Logger shouldn't acquire the GIL for the first messages emitted by tasks
  // constructor (e.g.), before we release the GIL later.
  gil_protected_logger.requireGilAcquisition(false);

  Tomographer::MultiProc::CxxThreads::TaskDispatcher<OurMHRandomWalkTask,
                                                     OurCData,
                                                     tpy::GilProtectedPyLogger>
    tasks(
      &taskcdat, // constant data
      gil_protected_logger, // the main logger object
      num_repeats // num_runs
      );

  logger.debug("Created task dispatcher.") ;

  //
  // Set up progress reporting. Even if we didn't want progress reporting, this
  // here is needed (we can just pass py::none() as callback) in order to
  // intercept Ctrl+C and signals and react accordingly (interrupt tasks).
  //
  setTasksStatusReportPyCallback(tasks,
                                 params.attr("get")("progress_fn", py::none()),
                                 params.attr("get")("progress_interval_ms", 500).cast<int>(),
                                 true /* needs GIL acquisition */);

  // Now, do the work

  logger.debug("About to release the GIL and start working.") ;

  auto time_start = std::chrono::steady_clock::now();

  {
    // release the GIL at this point, before we create separate threads
    gil_protected_logger.requireGilAcquisition(true);
    py::gil_scoped_release gil_released;

    try {

      // create threads, dispatch tasks, run them, and wait for them to finish.
      tasks.run();

    } catch (std::exception & e) {
      fprintf(stderr, "Caught some C++ exception.\n");
      py::gil_scoped_acquire gilacq; // need GIL acquisition for PyErr_Occurred()
      if (PyErr_Occurred() != NULL) {
        // an inner py::error_already_set() was caught & rethrown by MultiProc::CxxThread as an std::exception
        throw py::error_already_set();
      }
      throw; // error via pybind11
    }
  }
  // now GIL is acquired again
  gil_protected_logger.requireGilAcquisition(false);

  auto time_end = std::chrono::steady_clock::now();

  logger.debug("Random walks done.");

  // delta-time, in seconds and fraction of seconds
  std::string elapsed_s = Tomographer::Tools::fmtDuration(time_end - time_start);

  py::dict res;

  // individual results from each task
  const auto & task_results = tasks.collectedTaskResults();
  // ... aggregated into a full averaged histogram
  auto aggregated_histogram = taskcdat.aggregateResultHistograms(task_results) ;

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
  { std::ostringstream ss;
    Tomographer::MHRWTasks::ValueHistogramTools::printFinalReport(
        ss, // where to output
        taskcdat, // the cdata
        task_results, // the results
        aggregated_histogram // aggregated
        );
    res["final_report"] = ss.str();
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
      stream << res["final_report"].cast<std::string>() << "\n";
      stream << "Computation time: " <<  elapsed_s << "\n";
    });

  return res;
}



//
// HERE IS THE INTERFACE CODE EXPOSING THE PYTHON MODULE
//


PYBIND11_PLUGIN(my_custom_module)
{
  py::module m("my_custom_module",
               "Module description goes here");

  tpy::import_tomographer();

  // create our logger, ownership will be taken over by Python/PyBind11
  pylogger = new tpy::PyLogger;
  pylogger->initPythonLogger("my_custom_module"); // logger name for `logging` module
  m.attr("cxxlogger") = pylogger; // ownership is transferred here

  auto logger = Tomographer::Logger::makeLocalLogger(TOMO_ORIGIN, *pylogger);

  logger.debug("my_custom_module() initializing ...");

  py::register_exception<MyCustomModuleError>(m, "MyCustomModuleError");
  logger.debug("registered MyCustomModuleError");

  // add a function call that executes our random walk:
  m.def(
      "run", // function name
      &run_function, // C++ function pointer
      // argument keyword names, with default values
      py::arg("dim"),
      py::arg("Emn") = py::list(),
      py::arg("Nm") = Eigen::VectorXi(),
      py::arg("hist_params") =  tpy::HistogramParams(),
      py::arg("mhrw_params") = tpy::MHRWParams(),
      py::arg("num_repeats") = 8,
      // also accepts **kwargs, see signature of run_function() above.
      // Docstring:
      "Docstring here"
      );


  // return the constructed Python module
  return m.ptr();
}
