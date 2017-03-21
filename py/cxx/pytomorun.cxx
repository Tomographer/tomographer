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
#include <tomographer/mhrwtasks.h>
#include <tomographer/mhrw_valuehist_tasks.h>
#include <tomographer/multiprocthreads.h>
#include <tomographer/valuecalculator.h>
#include <tomographer/tools/signal_status_report.h>
#include <tomographer/mathtools/pos_semidef_util.h>


#include "tomographerpy/pyhistogram.h"
#include "tomographerpy/pymultiproc.h"
#include "tomographerpy/pymhrw.h"
#include "tomographerpy/pymhrwtasks.h"
#include "tomographerpy/exc.h"

#include "common_p.h"



namespace tpy {

class CallableValueCalculator
{
public:
  typedef RealType ValueType;
  
  CallableValueCalculator(py::object fn_)
    : fn(fn_)
  {
  }

  RealType getValue(const Eigen::Ref<const tpy::CplxMatrixType> & T) const
  {
    py::gil_scoped_acquire gil_acquire;
    return fn(py::cast(T)).cast<RealType>();
  }


private:
  py::object fn;
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
  Tomographer::DenseDM::TSpace::ObservableValueCalculator<DMTypes>,
  tpy::CallableValueCalculator
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
	   tpy::MHRWParams mhrw_params, // parameters of the random walk
	   int base_seed) // a random seed to initialize the random number generator
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




// define an exception class for invalid inputs
TOMOGRAPHER_DEFINE_MSG_EXCEPTION_BASE(TomorunInvalidInputError, "Invalid Input: ", TomographerCxxError) ;






py::object py_tomorun(
    int dim,
    const Eigen::MatrixXd& Exn,
    const py::list& Emn,
    const Eigen::VectorXi& Nm,
    py::object fig_of_merit,
    const Eigen::MatrixXcd& ref_state,
    const Eigen::MatrixXcd& observable,
    const tpy::UniformBinsHistogramParams& hist_params,
    const tpy::MHRWParams& mhrw_params,
    int binning_num_levels,
    int num_repeats,
    py::object progress_fn,
    int progress_interval_ms
    )
{
  Tomographer::Logger::LocalLogger<PyLogger> logger(TOMO_ORIGIN, *tpy::logger);

  logger.debug("py_tomorun()");

  typedef DMTypes::MatrixType MatrixType;

  DMTypes dmt(dim);

  // prepare llh object
  DenseLLH llh(dmt);

  if (Exn.rows()) { // use Exn
    if (py::len(Emn)) { // error: both Exn & Emn specified
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
  } else if (py::len(Emn)) {
    // use Emn
    if (py::len(Emn) != (std::size_t)Nm.rows()) {
      throw TomorunInvalidInputError("Mismatch in number of measurements: len(Emn)="
                                     + std::to_string(py::len(Emn)) +
                                     " but Nm.rows()=" + std::to_string(Nm.rows()));
    }
    for (std::size_t k = 0; k < (std::size_t)Nm.rows(); ++k) {
      MatrixType POVMeffect = Emn[k].cast<MatrixType>();
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

  bool fig_of_merit_callable = py::hasattr(fig_of_merit, "__call__");
  std::string fig_of_merit_s;
  if (fig_of_merit_callable) {
    fig_of_merit_s = "<custom>";
  } else {
    fig_of_merit_s = fig_of_merit.cast<std::string>();
  }

  if (fig_of_merit_s == "fidelity" || fig_of_merit_s == "tr-dist" || fig_of_merit_s == "purif-dist") {

    Eigen::SelfAdjointEigenSolver<MatrixType> eig(ref_state);

    typedef typename Eigen::SelfAdjointEigenSolver<MatrixType>::RealVectorType RealVectorType;
    
    MatrixType U = eig.eigenvectors();
    RealVectorType d = eig.eigenvalues();
  
    Tomographer::MathTools::forcePosVecKeepSum<RealVectorType>(d, Eigen::NumTraits<RealType>::dummy_precision());
  
    // TODO: ensure that something was given

    rho_ref = U * d.asDiagonal() * U.adjoint();
    T_ref = U * d.cwiseSqrt().asDiagonal() * U.adjoint();

  } else if (fig_of_merit_s == "obs-value") {

    // TODO: ensure that something was given
    A = observable;
    
  } else if (fig_of_merit_callable) {

    // custom callable

  } else {
    throw TomorunInvalidInputError(std::string("Invalid figure of merit: ")+py::repr(fig_of_merit).cast<std::string>());
  }

  ValueCalculator valcalc(
      // index of the valuecalculator to actually use:
      (fig_of_merit_s == "fidelity" ? 0 :
       (fig_of_merit_s == "purif-dist" ? 1 :
        (fig_of_merit_s == "tr-dist" ? 2 :
         (fig_of_merit_s == "obs-value" ? 3 :
          (fig_of_merit_callable ? 4 :
           throw TomorunInvalidInputError(std::string("Invalid valtype: ") + py::repr(fig_of_merit).cast<std::string>())
              ))))),
        // the valuecalculator instances which are available:
        Tomographer::DenseDM::TSpace::FidelityToRefCalculator<DMTypes, RealType>(T_ref),
        Tomographer::DenseDM::TSpace::PurifDistToRefCalculator<DMTypes, RealType>(T_ref),
        Tomographer::DenseDM::TSpace::TrDistToRefCalculator<DMTypes, RealType>(rho_ref),
        Tomographer::DenseDM::TSpace::ObservableValueCalculator<DMTypes>(dmt, A),
        tpy::CallableValueCalculator(fig_of_merit)
        );


  // prepare the random walk tasks

  typedef Tomographer::MHRWTasks::MHRandomWalkTask<OurCData, std::mt19937>  OurMHRandomWalkTask;

  typedef typename OurCData::ResultsCollectorType<PyLogger>::Type OurResultsCollector;

  // seed for random number generator
  auto base_seed = std::chrono::system_clock::now().time_since_epoch().count();

  if (binning_num_levels <= 0) {
    // choose automatically. Make sure that the last level has ~128 samples to calculate std deviation.
    binning_num_levels = (int)(std::floor(std::log(mhrw_params.n_run/128) / std::log(2)) + 1e-3) ;
  }

  OurCData taskcdat(llh, valcalc, hist_params, binning_num_levels, mhrw_params,
                    (int)(base_seed % std::numeric_limits<int>::max()));

  OurResultsCollector results(logger.parentLogger());

  typedef
#if defined(__GNUC__) && __GNUC__ == 4 && __GNUC_MINOR__ <= 6 && !defined(__clang__)
    std::chrono::monotonic_clock // for GCC/G++ 4.6
#else
    std::chrono::steady_clock
#endif
    StdClockType;
  StdClockType::time_point time_start;

  logger.debug([&](std::ostream & stream) {
      stream << "about to create the task dispatcher.  this pid = " << getpid() << "; this thread id = "
             << std::this_thread::get_id();
    }) ;

  {
    py::gil_scoped_release gil_release;
    auto tmp_pylogger_gil_ = tpy::logger->pushRequireGilAcquisition();

    Tomographer::MultiProc::CxxThreads::TaskDispatcher<OurMHRandomWalkTask,OurCData,OurResultsCollector,PyLogger>
      tasks(
          &taskcdat, // constant data
          &results, // results collector
          logger.parentLogger(), // the main logger object
          num_repeats // num_runs
          );

    setTasksStatusReportPyCallback(tasks, progress_fn, progress_interval_ms, true /* GIL */);

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

  } // gil release scope

  auto time_end = StdClockType::now();

  logger.debug("Random walks done.");

  // delta-time, in seconds and fraction of seconds
  std::string elapsed_s = Tomographer::Tools::fmtDuration(time_end - time_start);

  py::dict res;

  res["final_histogram"] = results.finalHistogram();
  res["simple_final_histogram"] = results.simpleFinalHistogram();
  res["elapsed_seconds"] = 1.0e-6 * std::chrono::duration_cast<std::chrono::microseconds>(
      time_end - time_start
      ).count();

  py::list runs_results;
  for (std::size_t k = 0; k < results.numTasks(); ++k) {
    const auto & run_result = *results.collectedRunTaskResult(k);
    runs_results.append(run_result);
  }
  res["runs_results"] = runs_results;

  // full final report
  std::string final_report;
  { std::ostringstream ss;
    results.printFinalReport(ss, taskcdat);
    final_report = ss.str();
    res["final_report"] = final_report;
  }

  // final report of runs only
  { std::ostringstream ss;
    results.printFinalReport(ss, taskcdat, 0, false);
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
      "hist_params"_a = tpy::UniformBinsHistogramParams(),
      "mhrw_params"_a = tpy::MHRWParams(),
      "binning_num_levels"_a = -1,
      "num_repeats"_a = std::thread::hardware_concurrency(),
      "progress_fn"_a = py::none(),
      "progress_interval_ms"_a = (int)500,
      // doc
      ( "tomorun(dim, ...)\n\n"
        "\n\n"
        "Produce a histogram of a figure of merit during a random walk in quantum state "
        "space according to the distribution :math:`\\mu_{B^n}(\\cdot)` defined in Ref. [1]. The "
        "likelihood function is specified with independent POVM effects (see below)."
        "\n\n"
        "This python function provides comparable functionality to the `tomorun` executable program, and "
        "allows for a better seamless interoperability with `NumPy`---all data matrices here are specified "
        "as `NumPy` arrays."
        "\n\n"
        ":param dim: The dimension of the quantum system\n"
        ":param Exn: The observed POVM effects, specified as a matrix in which each row is the\n"
        "            X-parameterization of a POVM effect. You may want to specify `Emn` instead,\n"
        "            which may be simpler.\n"
        ":param Emn: The observed POVM effects, specified as a list of :math:`\\textit{dim}\\times\\textit{dim}`\n"
        "            matrices.\n"
        ":param Nm:  the list of observed frequency counts for each POVM effect in `Emn` or `Exn`.\n"
        ":param fig_of_merit:  The choice of the figure of merit to study.  This is either a Python string or a\n"
        "            Python callable.  If it is a string, it must be one of 'obs-value',\n"
        "            'fidelity', 'tr-dist' or 'purif-dist' (see below for more info).  If it is a callable, it\n"
        "            should accept a single argument, the T-parameterization of the density matrix, and should\n"
        "            calculate and return the figure of merit.  The T-parameterization is a matrix :math:`T`\n"
        "            that :math:`\\rho=TT^\\dagger`.\n"
        ":param ref_state:  For figures of merit which compare to a reference state ('fidelity', 'tr-dist',\n"
        "            and 'purif-dist'), this is the reference state to calculate the figure of merit with,\n"
        "            specified as a density matrix.\n"
        ":param observable:  For the 'obs-value' figure of merit, specify the observable here as a matrix.\n"
        ":param hist_params:  The requested range of values to look at when collecting a histogram of the\n"
        "            figure of mert.  This should be a :py:class:`tomographer.UniformBinsHistogramParams`\n"
        "            instance.\n"
        ":param mhrw_params:  The parameters of the random walk, including the step size, the sweep size,\n"
        "            the number of thermalization sweeps, and the number of live sweeps.  Specify a\n"
        "            :py:class:`tomographer.MHRWParams` instance here."
        ":param binning_num_levels:  The number of levels in the binning analysis [2]. One should make sure\n"
        "            that there are enough bins at the last level to estimate the standard\n"
        "            deviation. This is done automatically by default (or if you specify the value `-1`),\n"
        "            so in normal circumstances you won't have to change the default value.\n"
        ":param num_repeats:  The number of independent random walks to run in parallel.  (The instances\n"
        "            will run serially if `tomographer` was compiled without OpenMP.)\n"
        ":param progress_fn:  A python callback function to monitor progress.  The function should accept\n"
        "            a single argument of type :py:class:`tomographer.multiproc.FullStatusReport`.  Check\n"
        "            out :py:class:`tomographer.jpyutil.RandWalkProgressBar` if you are using a\n"
        "            Jupyter notebook.  See below for more information on status progress reporting.\n"
        ":param progress_interval_ms: The approximate time interval in milliseconds between two progress reports.\n"
        "\n\n"
        ".. rubric:: Figures of merit"
        "\n\n"
        "The value of the `fig_of_merit` argument may be a Python string, in which case it should be one of "
        "the following:\n\n"
        "  - \"obs-value\": the expectation value of an observable. You should specify the argument "
        "`observable` as a 2-D `NumPy` array specifying the observable you are interested in. "
        "\n\n"
        "  - \"tr-dist\": the trace distance to a reference state. You should specify the argument "
        "`ref_state` as a 2-D `NumPy` array specifying the density matrix of the state which should serve "
        "as reference state."
        "\n\n"
        "  - \"fidelity\": the (root) fidelity to a reference state [3]. You should specify the argument "
        "`ref_state` as a 2-D `NumPy` array specifying the density matrix of the state which should serve "
        "as reference state."
        "\n\n"
        "    .. note:: For the squared fidelity to a pure state (usually preferred in "
        "experimental papers), you should use \"obs-value\" with the observable "
        "being the density matrix of the reference state [4]."
        "\n\n"
        "  - \"purif-dist\": the purified distance to a reference state [5]. You should specify the argument "
        "`ref_state` as a 2-D `NumPy` array specifying the density matrix of the state which should serve "
        "as reference state.\n\n"
        "The value of the `fig_of_merit` argument may also be a Python callable which directly calculates the "
        "figure of merit.  It should accept a single argument, the T-parameterization of the density matrix given "
        "as a `NumPy` array (defined such that :math:`\\rho=TT^\\dagger`), and should return the value of the figure "
        "of merit.  For example, to calculate the purity of the state :math:`\\operatorname{tr}(\\rho^2)`::\n\n"
        "        import numpy as np\n"
        "        import numpy.linalg as npl\n"
        "        ...\n"
        "        r = tomographer.tomorun.tomorun(...,\n"
        "                                        fig_of_merit=lambda T: npl.norm(np.dot(T,T.T.conj())),\n"
        "                                        ...)\n"
        "\n\n"
        ".. rubric:: Return value"
        "\n\n"
        "The `tomorun()` function returns a Python dictionary with the following keys and values set:\n\n"
        "  - ``final_histogram``: a :py:class:`~tomographer.AveragedErrorBarHistogram` instance with the final "
        "histogram data.  The histogram has the parameters specified in the `hist_params` argument. "
        "The histogram is NOT normalized to a probabilty density; you should call "
        " :py:meth:`~tomographer.UniformBinsHistogramWithErrorBars.normalized()` if you need a "
        " normalized histogram.\n\n"
        "  - ``simple_final_histogram``: a :py:class:`~tomographer.AveragedSimpleRealHistogram` obtained "
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
        "list is an instance of :py:class:`tomographer.mhrwtasks.MHRandomWalkValueHistogramTaskResult`.\n\n"
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

  register_exception_with_docstring<TomorunInvalidInputError>(
      tomorunmodule,
      "TomorunInvalidInputError",
      tpy::TomographerCxxErrorObj.ptr(),
      // docstring:
      "Exception which gets raised if invalid input is supplied to the "
      ":py:func:`tomographer.tomorun.tomorun()` function.");

  logger.debug("py_tomo_tomorun() complete.");
}
