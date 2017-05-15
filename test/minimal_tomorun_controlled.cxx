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

#include <iostream>

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



//
// Data types for our quantum objects.  For the sake of the example, we just
// leave the size to be dynamic, that is, fixed at run time and not at compile
// time.
//
typedef Tomographer::DenseDM::DMTypes<Eigen::Dynamic, double> DMTypes;


//
// The class which will store our tomography data. Just define this as
// "DenseLLH" as a shorthand.
//
typedef Tomographer::DenseDM::IndepMeasLLH<DMTypes> DenseLLH;


//
// The type of value calculator we would like to use.  Here, we settle for the
// expectation value of an observable, as we are interested in the square
// fidelity to the pure Bell Phi+ state (=expectation value of the observable
// |Phi+><Phi+|).
//
typedef Tomographer::DenseDM::TSpace::ObservableValueCalculator<DMTypes>
  ValueCalculator;

//
// We need to define a class which adds the capacity of creating the "master"
// random walk object to the engine in
// Tomographer::MHRWTasks::ValueHistogramTasks, which take care of running the
// random walks etc. as needed.
//
struct OurCData : public Tomographer::MHRWTasks::ValueHistogramTools::CDataBase<
  ValueCalculator, // our value calculator
  true // use binning analysis
  >
{
  OurCData(const DenseLLH & llh_, // data from the the tomography experiment
	   ValueCalculator valcalc, // the figure-of-merit calculator
	   HistogramParams hist_params, // histogram parameters
	   int binning_num_levels, // number of binning levels in the binning analysis
	   MHRWParamsType mhrw_params, // parameters of the random walk
	   int base_seed) // a random seed to initialize the random number generator
    : CDataBase<ValueCalculator,true>(valcalc, hist_params, binning_num_levels,
                                        mhrw_params, base_seed),
      llh(llh_)
  {
  }

  const DenseLLH llh;

  // The result of a task run -- pass on to the default StatsResults type
  // provided by ValueHistogramTools.  Because we have several stats collectors
  // set, we need to pick out the result of our
  // "value-histogram-stats-collector", which is the first one in the
  // multiple-stats-collector object which we have create in
  // setupRandomWalkAndRun().  We thus pick out the first result in the tuple of
  // all stats collectors results, i.e. the one at index 0 (using std::get).
  struct MHRWStatsResultsType : public MHRWStatsResultsBaseType
  {
    template<typename... Types>
    MHRWStatsResultsType(std::tuple<ValueStatsCollectorResultType, Types...> && r)
      : MHRWStatsResultsBaseType(std::move(std::get<0>(r)))
    {
    }
  };

  //
  // This function is called automatically by the task manager/dispatcher via
  // MHRWTasks.  It should set up the random walk as required (at a minimum,
  // create a MHWalker instance and pass on the default value stats collector
  // from Tomographer::MHRWTasks::ValueHistogramTools), and run it.
  //
  // We should not forget to call run(), to actually run the random walk!
  //
  // Here, our example is slightly more involved as in "minimal_tomorun". In
  // addition, we'll include more stats collectors and set up some random walk
  // controllers.
  //
  template<typename Rng, typename LoggerType, typename ExecFn>
  inline void setupRandomWalkAndRun(Rng & rng, LoggerType & logger, ExecFn run) const
  {
    auto val_stats_collector = createValueStatsCollector(logger);
    Tomographer::MHRWMovingAverageAcceptanceRatioStatsCollector<> movavg_accept_stats;
    auto stats_collectors =
      Tomographer::mkMultipleMHRWStatsCollectors(val_stats_collector, movavg_accept_stats);

    auto therm_step_controller =
      Tomographer::mkMHRWStepSizeController<MHRWParamsType>(movavg_accept_stats, logger);
    auto numsamples_controller =
      Tomographer::mkMHRWValueErrorBinsConvergedController(val_stats_collector, logger);

    auto controllers = Tomographer::mkMHRWMultipleControllers(therm_step_controller, numsamples_controller);

    Tomographer::DenseDM::TSpace::LLHMHWalker<DenseLLH,Rng,LoggerType> mhwalker(
            llh.dmt.initMatrixType(),
            llh,
            rng,
            logger
            );

    run(mhwalker, stats_collectors, controllers);
  };

};


//
// The root logger which takes care of handling the log messages.  Here, we log to the
// standard output (recall that stdout/stderr are seen as a "file").
//
// The level of the logger can be changed to one of Tomographer::Logger::LONGDEBUG,
// Tomographer::Logger::DEBUG, Tomographer::Logger::INFO, Tomographer::Logger::WARNING or
// Tomographer::Logger::ERROR.
//
typedef Tomographer::Logger::FileLogger BaseLoggerType;
BaseLoggerType rootlogger(stderr, Tomographer::Logger::DEBUG);


int main()
{
  //
  // Prepare the logger in which we can log debug/info/error messages.
  //

  auto logger = Tomographer::Logger::makeLocalLogger("main()", rootlogger);

  logger.debug("starting up");

  //
  // Specify the dimension of the quantum tomography setting.
  //

  const int dim = 4; // two qubits
  DMTypes dmt(dim);

  //
  // Prepare data from the tomography experiment.
  //
  // In this hypothetical experiment, we assumed that the observables
  // \sigma_x\otimes\sigma_x, \sigma_y\otimes\sigma_y and \sigma_z\otimes\sigma_z are each
  // measured 100 times. Each measurement setting has two possible outcomes, +1 or -1, and
  // hence there are in total 6 POVM effects.
  //

  DenseLLH llh(dmt);
  
  // POVM effects for  \sigma_x \otimes \sigma_x
  
  DMTypes::MatrixType Exxplus(dmt.initMatrixType());
  Exxplus <<
    0.5,    0,    0,  0.5,
    0,    0.5,  0.5,    0,
    0,    0.5,  0.5,    0,
    0.5,    0,    0,  0.5;

  llh.addMeasEffect(Exxplus, 95);  // 95 counts of "+1" out of 100 for \sigma_x\otimes\sigma_x

  DMTypes::MatrixType Exxminus(dmt.initMatrixType());
  Exxminus <<
    0.5,    0,    0, -0.5,
    0,    0.5, -0.5,    0,
    0,   -0.5,  0.5,    0,
    -0.5,   0,    0,  0.5;

  llh.addMeasEffect(Exxminus, 5);  // 95 counts of "-1" out of 100 for \sigma_x\otimes\sigma_x

  // POVM effects for  \sigma_y \otimes \sigma_y
  
  DMTypes::MatrixType Eyyplus(dmt.initMatrixType());
  Eyyplus <<
    0.5,    0,    0, -0.5,
    0,    0.5,  0.5,    0,
    0,    0.5,  0.5,    0,
    -0.5,   0,    0,  0.5;

  llh.addMeasEffect(Eyyplus, 8);  // 8 counts of "+1" out of 100 for \sigma_y\otimes\sigma_y

  DMTypes::MatrixType Eyyminus(dmt.initMatrixType());
  Eyyminus <<
    0.5,    0,    0,  0.5,
    0,    0.5, -0.5,    0,
    0,   -0.5,  0.5,    0,
    0.5,    0,    0,  0.5;

  llh.addMeasEffect(Eyyminus, 92);  // 92 counts of "-1" out of 100 for \sigma_y\otimes\sigma_y

  // POVM effects for  \sigma_z \otimes \sigma_z
  
  DMTypes::MatrixType Ezzplus(dmt.initMatrixType());
  Ezzplus <<
    1,   0,   0,   0,
    0,   0,   0,   0,
    0,   0,   0,   0,
    0,   0,   0,   1;

  llh.addMeasEffect(Ezzplus, 98);  // 98 counts of "+1" out of 100 for \sigma_z\otimes\sigma_z

  DMTypes::MatrixType Ezzminus(dmt.initMatrixType());
  Ezzminus <<
    0,   0,   0,   0,
    0,   1,   0,   0,
    0,   0,   1,   0,
    0,   0,   0,   0;

  llh.addMeasEffect(Ezzminus, 2);  // 2 counts of "-1" out of 100 for \sigma_z\otimes\sigma_z

  logger.debug("data entered OK");

  //
  // Prepare the figure of merit calculator: Squared fidelity to the pure entangled Bell
  // state |\Phi^+>
  //

  DMTypes::MatrixType phiplus(dmt.initMatrixType());
  phiplus <<
    0.5,    0,    0,   0.5,
    0,      0,    0,     0,
    0,      0,    0,     0,
    0.5,    0,    0,   0.5;

  // our main ValueCalculator instance, which is in fact an alias for the
  // ObservableValueCalculator class. [If we wanted to choose which figure of merit to
  // compute at run-time, then we should use a MultiplexorValueCalculator.]
  ValueCalculator valcalc(dmt, phiplus);

  // parameters of the histogram of the figure of merit: cover the range [0.75, 1.0] by
  // dividing it into 50 bins
  const OurCData::HistogramParams hist_params(0.75, 1.0, 50);


  //
  // Data is ready, prepare & launch the random walks.  Use C++ Threads
  // parallelization.
  //


  // The task type, for the MultiProc interface
  typedef Tomographer::MHRWTasks::MHRandomWalkTask<OurCData, std::mt19937>
    OurMHRandomWalkTask;

  // parameters of the random walk
  const OurCData::MHRWParamsType mhrw_params(
      // step size -- this will be automatically adjusted during
      // thermalization sweeps. Here we only need to provide a rough guess
      // as a starting point:
      0.1,
      // sweep size -- will be automatically adjusted during thermalization
      // sweeps.  The only thing that matters here is that we choose it
      // such that sweep_size*step_size ~ 1.  As the controller modifies
      // the step size, it will adapt the sweep size to keep the product
      // sweep_size*step_size constant.
      10,
      // Number of thermalization sweeps -- might take longer if the step size
      // needs to be adapted a lot before finding the right acceptance ratio
      500,
      // Number of live sweeps in which samples are collected -- might take
      // longer if the controller sees that the error bars haven't converged yet
      // and lets the random walk run for longer (so you shouldn't get surprised
      // if the random walk goes past 100% done)
      32768
      );

  // seed for random number generator -- just use the current time
  std::mt19937::result_type base_seed =
    std::chrono::system_clock::now().time_since_epoch().count();

  // number of levels for the binning analysis
  const int binning_num_levels = 8;

  // instantiate the class which stores the shared data.
  OurCData taskcdat(llh, valcalc, hist_params, binning_num_levels,
                    mhrw_params, base_seed);

  // repeat the whole random walk this number of times.  These random walks will
  // run in parallel depending on the number of CPUs available.
  const int num_repeats = 4;

  // create the task manager/dispatcher, using the CxxThreads implementation
  auto tasks =
    Tomographer::MultiProc::CxxThreads::mkTaskDispatcher<OurMHRandomWalkTask>(
        &taskcdat, // constant data
        logger.parentLogger(), // the main logger object
        num_repeats // num_runs
        );

  // get status reports every 500 milliseconds printed out on std::cout
  tasks.setStatusReportHandler([&](decltype(tasks)::FullStatusReportType report) {
      std::cout << "--- intermediate status report ---\n"
                << report.getHumanReport() << "\n" ;
    });
  tasks.requestPeriodicStatusReport(500) ;

  //
  // Finally, run our tomo process
  //

  logger.debug("all set, ready to go");

  auto time_start = std::chrono::system_clock::now();

  tasks.run(); // GO!

  auto time_end = std::chrono::system_clock::now();

  logger.debug("Random walks done.");

  // delta-time, formatted in hours, minutes, seconds and fraction of seconds
  std::string elapsed_s = Tomographer::Tools::fmtDuration(time_end - time_start);


  const auto & task_results = tasks.collectedTaskResults();

  auto aggregated_histogram = taskcdat.aggregateResultHistograms(task_results) ;

  const auto & histogram = aggregated_histogram.final_histogram;

  // histogram has type Tomographer::AveragedHistogram, you can use it like any other
  // Histogram or HistogramWithErrorBars.  You can pretty-print it with:
  logger.info([&](std::ostream & stream) {
      stream << "Nice little histogram after all that work: \n"
             << histogram.prettyPrint()
             << "\n";
      });

  logger.info([&](std::ostream & stream) {
      // Tomographer::MHRWTasks::ValueHistogramTools::printFinalReport() will generate a
      // default tomorun-like report with the parameters of the random walk, an overview
      // of each histogram of each task repeat, short info on the convergence of the
      // binning error bars, and the final histogram itself along with error bars
      Tomographer::MHRWTasks::ValueHistogramTools::printFinalReport(
          stream, // where to output
          taskcdat, // the cdata
          task_results, // the results
          aggregated_histogram // aggregated
          );
    });


  // success.
  return 0;
}
