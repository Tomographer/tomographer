
#include "common.h"

#include <exception>
#include <stdexcept>

#include "pyhistogram.h"

#include <omp.h>


#include <tomographer2/tools/loggers.h>
#include <tomographer2/densedm/dmtypes.h>
#include <tomographer2/densedm/indepmeasllh.h>
#include <tomographer2/densedm/tspacellhwalker.h>
#include <tomographer2/densedm/tspacefigofmerit.h>
#include <tomographer2/mhrw.h>
#include <tomographer2/mhrwtasks.h>
#include <tomographer2/mhrw_valuehist_tasks.h>
#include <tomographer2/multiprocomp.h>
#include <tomographer2/valuecalculator.h>
#include <tomographer2/tools/signal_status_report.h>
#include <tomographer2/mathtools/pos_semidef_util.h>











namespace Py {

typedef Tomographer::MHRWParams<CountIntType, RealType> MHRWParams;




struct FullStatusReport {
  FullStatusReport() : num_completed(-1), num_total_runs(-1), workers_running(), workers_reports() { }
  int num_completed;
  int num_total_runs;
  boost::python::list workers_running; // std::vector<bool>
  boost::python::list workers_reports; // std::vector<TaskStatusReportType>
};

}




//
// Data types for our quantum objects.  For the sake of the example, we just leave the
// size to be dynamic, that is, fixed at run time and not at compile time.
//
typedef Tomographer::DenseDM::DMTypes<Eigen::Dynamic, RealType> DMTypes;


//
// The class which will store our tomography data. Just define this as "DenseLLH" as a
// shorthand.
//
typedef Tomographer::DenseDM::IndepMeasLLH<DMTypes> DenseLLH;


//
// The type of value calculator we would like to use.  Here, we settle for the expectation
// value of an observable, as we are interested in the square fidelity to the pure Bell
// Phi+ state (=expectation value of the observable |Phi+><Phi+|).
//
typedef Tomographer::MultiplexorValueCalculator<
  RealType,
  Tomographer::DenseDM::TSpace::FidelityToRefCalculator<DMTypes, RealType>,
  Tomographer::DenseDM::TSpace::PurifDistToRefCalculator<DMTypes, RealType>,
  Tomographer::DenseDM::TSpace::TrDistToRefCalculator<DMTypes, RealType>,
  Tomographer::DenseDM::TSpace::ObservableValueCalculator<DMTypes>
  > ValueCalculator;






//
// We need to define a class which adds the capacity of creating the "master" random walk
// object to the engine in Tomographer::MHRWTasks::ValueHistogramTasks, which take care of
// running the random walks etc. as needed.
//
struct OurCData : public Tomographer::MHRWTasks::ValueHistogramTasks::CDataBase<
  ValueCalculator, // our value calculator
  true // use binning analysis
  >
{
  OurCData(const DenseLLH & llh_, // data from the the tomography experiment
	   ValueCalculator valcalc, // the figure-of-merit calculator
	   HistogramParams hist_params, // histogram parameters
	   int binning_num_levels, // number of binning levels in the binning analysis
	   Py::MHRWParams mhrw_params, // parameters of the random walk
	   std::size_t base_seed) // a random seed to initialize the random number generator
    : CDataBase<ValueCalculator,true>(valcalc, hist_params, binning_num_levels, mhrw_params, base_seed),
      llh(llh_)
  {
  }

  const DenseLLH llh;

  //
  // This function is called automatically by the task manager/dispatcher.  It should
  // return a LLHMHWalker object which controls the random walk.
  //
  template<typename Rng, typename LoggerType>
  inline Tomographer::DenseDM::TSpace::LLHMHWalker<DenseLLH,Rng,LoggerType>
  createMHWalker(Rng & rng, LoggerType & logger) const
  {
    return Tomographer::DenseDM::TSpace::LLHMHWalker<DenseLLH,Rng,LoggerType>(
	llh.dmt.initMatrixType(),
	llh,
	rng,
	logger
	);
  }

};




class TomorunInvalidInputError : public std::exception
{
  std::string _msg;
public:
  TomorunInvalidInputError(std::string msg) : _msg(std::move(msg)) { }
  virtual ~TomorunInvalidInputError() { }

  const char * what() const throw() { return _msg.c_str(); }
};






boost::python::object py_tomorun(
    int dim,
    const Eigen::MatrixXd& Exn,
    const boost::python::list& Emn,
    const Eigen::VectorXi& Nm,
    std::string fig_of_merit,
    const Eigen::MatrixXcd& ref_state,
    const Eigen::MatrixXcd& observable,
    const Py::UniformBinsHistogramParams& hist_params,
    const Py::MHRWParams& mhrw_params,
    int binning_num_levels,
    int num_repeats,
    boost::python::object progress_fn,
    int progress_interval_ms
    )
{
  Tomographer::Logger::LocalLogger<TPyLoggerType> logger(TOMO_ORIGIN, tpy_logger);

  logger.debug("py_tomorun()");

  typedef DMTypes::MatrixType MatrixType;

  DMTypes dmt(dim);

  // prepare llh object
  DenseLLH llh(dmt);

  if (Exn.rows()) { // use Exn
    if (boost::python::len(Emn)) { // error: both Exn & Emn specified
      throw TomorunInvalidInputError("You can't specify both Exn and Emn arguments");
    }
    // use Exn
    if (Exn.rows() != Nm.rows()) {
      throw TomorunInvalidInputError("Mismatch in number of measurements: Exn.rows()="
                                     + std::to_string(Exn.rows()) + " but Nm.rows()=" + std::to_string(Nm.rows()));
    }
    for (std::size_t k = 0; k < (std::size_t)Nm.rows(); ++k) {
      llh.addMeasEffect(Exn.row(k).transpose(), Nm(k), true);
    }
  } else if (boost::python::len(Emn)) {
    // use Emn
    if (boost::python::len(Emn) != Nm.rows()) {
      throw TomorunInvalidInputError("Mismatch in number of measurements: len(Emn)="
                                     + std::to_string(boost::python::len(Emn)) +
                                     " but Nm.rows()=" + std::to_string(Nm.rows()));
    }
    for (std::size_t k = 0; k < (std::size_t)Nm.rows(); ++k) {
      MatrixType POVMeffect = boost::python::extract<MatrixType>(Emn[k]);
      llh.addMeasEffect(POVMeffect, Nm(k), true);
    }
  } else {
    // no measurements specified
    throw TomorunInvalidInputError("No measurements specified. Please specify either the `Exn' or the `Emn' argument");
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

  if (fig_of_merit == "fidelity" || fig_of_merit == "trace-dist" || fig_of_merit == "purif-dist") {

    Eigen::SelfAdjointEigenSolver<MatrixType> eig(ref_state);

    typedef typename Eigen::SelfAdjointEigenSolver<MatrixType>::RealVectorType RealVectorType;
    
    MatrixType U = eig.eigenvectors();
    RealVectorType d = eig.eigenvalues();
  
    Tomographer::MathTools::forcePosVecKeepSum<RealVectorType>(d, Eigen::NumTraits<RealType>::dummy_precision());
  
    // TODO: ensure that something was given

    rho_ref = U * d.asDiagonal() * U.adjoint();
    T_ref = U * d.cwiseSqrt().asDiagonal() * U.adjoint();

  } else if (fig_of_merit == "obs-value") {

    // TODO: ensure that something was given
    A = observable;
    
  } else {
    throw TomorunInvalidInputError("Invalid figure of merit: `"+fig_of_merit+"'");
  }

  ValueCalculator valcalc(
      // index of the valuecalculator to actually use:
      (fig_of_merit == "fidelity" ? 0 :
       (fig_of_merit == "purif-dist" ? 1 :
        (fig_of_merit == "tr-dist" ? 2 :
         (fig_of_merit == "obs-value" ? 3 :
          throw TomorunInvalidInputError(streamstr("Invalid valtype: " << fig_of_merit))
             )))),
        // the valuecalculator instances which are available:
        Tomographer::DenseDM::TSpace::FidelityToRefCalculator<DMTypes, RealType>(T_ref),
        Tomographer::DenseDM::TSpace::PurifDistToRefCalculator<DMTypes, RealType>(T_ref),
        Tomographer::DenseDM::TSpace::TrDistToRefCalculator<DMTypes, RealType>(rho_ref),
        Tomographer::DenseDM::TSpace::ObservableValueCalculator<DMTypes>(dmt, A)
        );


  // prepare the random walk tasks

  typedef Tomographer::MHRWTasks::MHRandomWalkTask<OurCData, std::mt19937>  OurMHRandomWalkTask;

  typedef typename OurCData::template ResultsCollectorType<TPyLoggerType>::Type OurResultsCollector;

  // seed for random number generator
  auto base_seed = std::chrono::system_clock::now().time_since_epoch().count();

  if (binning_num_levels <= 0) {
    // choose automatically. Make sure that the last level has ~128 samples to calculate std deviation.
    binning_num_levels = (int)(std::floor(std::log(mhrw_params.n_run/128) / std::log(2)) + 1e-3) ;
  }

  OurCData taskcdat(llh, valcalc, hist_params, binning_num_levels, mhrw_params, base_seed);

  OurResultsCollector results(logger.parentLogger());

  auto tasks = Tomographer::MultiProc::OMP::makeTaskDispatcher<OurMHRandomWalkTask>(
      &taskcdat, // constant data
      &results, // results collector
      logger.parentLogger(), // the main logger object
      num_repeats, // num_runs
      1 // n_chunk
      );

  std::chrono::steady_clock::time_point time_start;

  // TODO: add some form of progress feedback, e.g. call to a custom Python function.
  tasks.setStatusReportHandler(
      [&](const Tomographer::MultiProc::FullStatusReport<OurMHRandomWalkTask::StatusReportType> & report) {
        // check to see if we got any KeyboardInterrupt
        // PyErr_CheckSignals() returns -1 if an exception was raised
        if (PyErr_CheckSignals() == -1) {
          tasks.requestInterrupt();
          return;
        }
        // call the python progress callback:
        if (!progress_fn.is_none()) {
          try {
            //            fprintf(stderr, "PREPARING REPORT\n");
            Py::FullStatusReport r;
            r.num_completed = report.num_completed;
            r.num_total_runs = report.num_total_runs;
            //            fprintf(stderr, "PREPARING REPORT - B\n");
            for (std::size_t k = 0; k < report.workers_reports.size(); ++k) {
              //              fprintf(stderr, "PREPARING REPORT - worker\n");
              // worker running? add this value
              r.workers_running.append(report.workers_running[k]);
              // and prepare the report object
              const auto& wrp = report.workers_reports[k];
              boost::python::dict d;
              // generic status report fields
              d["worker_id"] = k;
              d["fraction_done"] = wrp.fraction_done;
              d["msg"] = wrp.msg;
              // fields specific to MHRWValueHistogramTasks
              d["kstep"] = wrp.kstep;
              d["mhrw_params"] = wrp.mhrw_params;
              d["acceptance_ratio"] = wrp.acceptance_ratio;
              d["n_total_iters"] = wrp.n_total_iters;
              //              fprintf(stderr, "PREPARING REPORT - worker dict ready\n");
              // add this report
              r.workers_reports.append(d);
              //              fprintf(stderr, "PREPARING REPORT - worker dict added\n");
            }
            // call python callback
            //            fprintf(stderr, "PROGRESS CALLBACK ...\n");
            progress_fn(boost::python::object(r));
            //            fprintf(stderr, "PROGRESS CALLBACK DONE\n");
          } catch (boost::python::error_already_set & ) {
            tasks.requestInterrupt();
            return;
          }
        }
        // borrowed from tomographer2/tools/signal_status_handler.h: --->  DEBUG::
        /*
        std::string elapsed = Tomographer::Tools::fmtDuration(std::chrono::steady_clock::now() - time_start);
        fprintf(stderr,
                "\n"
                "=========================== Intermediate Progress Report ============================\n"
                "  Total Completed Runs: %d/%d: %5.2f%%\n"
                "  %s total elapsed\n",
                report.num_completed, report.num_total_runs,
                (double)report.num_completed/report.num_total_runs*100.0,
                elapsed.c_str());
        if (report.workers_running.size() == 1) {
          if (report.workers_running[0]) {
            fprintf(stderr, "--> %s\n", report.workers_reports[0].msg.c_str());
          }
        } else if (report.workers_running.size() > 1) {
          fprintf(stderr,
                  "Current Run(s) information (workers working/spawned %d/%d):\n",
                  (int)std::count(report.workers_running.begin(), report.workers_running.end(), true),
                  (int)report.workers_running.size()
              );
          for (unsigned int k = 0; k < report.workers_running.size(); ++k) {
            std::string msg = report.workers_running[k] ? report.workers_reports[k].msg : std::string("<idle>");
            fprintf(stderr, "=== #%2u: %s\n", k, msg.c_str());
          }
        } else {
          // no info. (workers_running.size() == 0)
        }
        fprintf(stderr,
                "=====================================================================================\n\n");
        */
        // <----
      });
  tasks.requestPeriodicStatusReport(progress_interval_ms);

  // and run our tomo process

  time_start = std::chrono::steady_clock::now();

  try {
    tasks.run();
  } catch (const Tomographer::MultiProc::TasksInterruptedException & e) {
    if (PyErr_Occurred() != NULL) {
      // tell boost.python that the exception is already set
      throw boost::python::error_already_set();
    }
    // no Python exception set?? -- set a RuntimeError via Boost
    throw e;
  } catch (std::exception & e) {
    //fprintf(stderr, "EXCEPTION: %s\n", e.what());
    // another exception
    if (PyErr_Occurred() != NULL) { // an inner boost::python::error_already_set() was caught & rethrown by MultiProc::OMP
      throw boost::python::error_already_set();
    }
    throw e; // error via boost::python
  }

  auto time_end = std::chrono::steady_clock::now();

  logger.debug("Random walks done.");

  // delta-time, in seconds and fraction of seconds
  std::string elapsed_s = Tomographer::Tools::fmtDuration(time_end - time_start);

  boost::python::dict res;

  res["final_histogram"] = boost::python::object(results.finalHistogram());
  res["simple_final_histogram"] = boost::python::object(results.simpleFinalHistogram());

  std::string final_report;
  { std::ostringstream ss;
    results.printFinalReport(ss, taskcdat);
    final_report = ss.str();
    res["final_report"] = final_report;
  }

  logger.debug([&](std::ostream & stream) {
      stream << final_report << "\n";
      stream << "Computation time: " <<  elapsed_s << "\n";
    });

  return res;
}







void py_tomo_tomorun()
{
  auto logger = Tomographer::Logger::makeLocalLogger(TOMO_ORIGIN, tpy_logger);

  logger.debug("py_tomo_tomorun() ...");

  logger.debug("MHRWParams ...");
  { typedef Py::MHRWParams Kl;
    boost::python::class_<Py::MHRWParams>("MHRWParams")
      .def(boost::python::init<>())
      .def(boost::python::init<RealType,CountIntType,CountIntType,CountIntType>())
      .add_property("step_size", +[](const Kl & p) { return p.step_size; },
                    +[](Kl & p, RealType step_size) { p.step_size = step_size; })
      .add_property("n_sweep", +[](const Kl & p) { return p.n_sweep; },
                    +[](Kl & p, CountIntType n_sweep) { p.n_sweep = n_sweep; })
      .add_property("n_therm", +[](const Kl & p) { return p.n_therm; },
                    +[](Kl & p, CountIntType n_therm) { p.n_therm = n_therm; })
      .add_property("n_run", +[](const Kl & p) { return p.n_run; },
                    +[](Kl & p, CountIntType n_run) { p.n_run = n_run; })
      ;
  }

  logger.debug("multiproc module ... ");

  boost::python::object multiprocsubmod(boost::python::borrowed(PyImport_AddModule("tomographer.multiproc")));
  boost::python::scope().attr("multiproc") = multiprocsubmod;
  {
    // now inside submodule
    boost::python::scope multiprocmodule(multiprocsubmod);

    logger.debug("multiproc.FullStatusReport ...");

    { typedef Py::FullStatusReport Kl;
      boost::python::class_<Py::FullStatusReport>("FullStatusReport")
        .add_property("num_completed", +[](const Kl& r) { return r.num_completed; })
        .add_property("num_total_runs", +[](const Kl& r) { return r.num_total_runs; })
        .add_property("workers_running", +[](const Kl& r) { return r.workers_running; })
        .add_property("workers_reports", +[](const Kl& r) { return r.workers_reports; })
        ;
    }
  }

  logger.debug("tomorun module ...");
  
  boost::python::object tomorunsubmod(boost::python::borrowed(PyImport_AddModule("tomographer.tomorun")));
  boost::python::scope().attr("tomorun") = tomorunsubmod;
  {
    // now inside submodule
    boost::python::scope tomorunmodule(tomorunsubmod);

    logger.debug("tomorun.tomorun() ...");

    // the main run call:
    boost::python::def(
        "tomorun", // function name
        &py_tomorun, // fn pointer
        (boost::python::arg("dim"),
         boost::python::arg("Exn") = Eigen::MatrixXd(),
         boost::python::arg("Emn") = boost::python::list(),
         boost::python::arg("Nm") = Eigen::VectorXi(),
         boost::python::arg("fig_of_merit") = std::string("obs-value"),
         boost::python::arg("ref_state") = Eigen::MatrixXcd(),
         boost::python::arg("observable") = Eigen::MatrixXcd(),
         boost::python::arg("hist_params") = Py::UniformBinsHistogramParams(),
         boost::python::arg("mhrw_params") = Py::MHRWParams(),
         boost::python::arg("binning_num_levels") = -1,
         boost::python::arg("num_repeats") = omp_get_num_procs(),
         boost::python::arg("progress_fn") = boost::python::object(),
         boost::python::arg("progress_interval_ms") = (int)500),
        "Docstring here"
        );

    logger.debug("tomorun.TomorunInvalidInputError ...");

    boost::python::register_exception_translator<TomorunInvalidInputError>(
        +[](const TomorunInvalidInputError & exc) {
          PyErr_SetString(PyExc_RuntimeError, exc.what());
        });
  }

  logger.debug("py_tomo_tomorun() complete.");
}
