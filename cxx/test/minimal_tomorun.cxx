/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
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

#include <tomographer2/tools/loggers.h>
#include <tomographer2/densedm/dmtypes.h>
#include <tomographer2/densedm/indepmeasllh.h>
#include <tomographer2/densedm/tspacellhwalker.h>
#include <tomographer2/densedm/tspacefigofmerit.h>
#include <tomographer2/mhrw.h>
#include <tomographer2/mhrwtasks.h>
#include <tomographer2/mhrw_valuehist_tasks.h>
#include <tomographer2/multiprocomp.h>
#include <tomographer2/tools/signal_status_report.h>



//
// Data types for our quantum objects.  For the sake of the example, we just leave the
// size to be dynamic, that is, fixed at run time and not at compile time.
//
typedef Tomographer::DenseDM::DMTypes<Eigen::Dynamic, double> DMTypes;


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
typedef Tomographer::DenseDM::TSpace::ObservableValueCalculator<DMTypes> ValueCalculator;

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
	   MHRWParamsType mhrw_params, // parameters of the random walk
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




//
// The root logger which takes care of handling the log messages.  Here, we log to the
// standard output (recall that stdout is seen as a "file").
//
// The level of the logger can be changed to one of Tomographer::Logger::LONGDEBUG,
// Tomographer::Logger::DEBUG, Tomographer::Logger::INFO, Tomographer::Logger::WARNING or
// Tomographer::Logger::ERROR.
//
typedef Tomographer::Logger::FileLogger BaseLoggerType;
BaseLoggerType rootlogger(stdout, Tomographer::Logger::DEBUG);


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
  // Data is ready, prepare & launch the random walks.  Use OpenMP parallelization.
  //

  typedef Tomographer::MHRWTasks::MHRandomWalkTask<OurCData, std::mt19937>  OurMHRandomWalkTask;

  typedef typename OurCData::ResultsCollectorType<BaseLoggerType>::Type OurResultsCollector;

  // parameters of the random walk
  const OurCData::MHRWParamsType mhrw_params(
      0.04, // step size (choose such that acceptance ratio ~ 0.25)
      25, // sweep size (should be chosen such that  sweep_size*step_size ~ 1)
      500, // # of thermalization sweeps
      32768 // # of live sweeps in which samples are collected
      );

  // seed for random number generator -- just use the current time
  auto base_seed = std::chrono::system_clock::now().time_since_epoch().count();

  // number of levels for the binning analysis
  const int binning_num_levels = 8;

  // instantiate the class which stores the shared data.
  OurCData taskcdat(llh, valcalc, hist_params, binning_num_levels, mhrw_params, base_seed);

  // the object which will collect & store the results at the end of the day
  OurResultsCollector results(logger.parentLogger());

  // repeat the whole random walk this number of times.  These random walks will run in
  // parallel depending on the number of CPUs available.
  const int num_repeats = 4;

  // create the task manager/dispatcher, using the OpenMP implementation
  auto tasks = Tomographer::MultiProc::OMP::makeTaskDispatcher<OurMHRandomWalkTask>(
      &taskcdat, // constant data
      &results, // results collector
      logger.parentLogger(), // the main logger object
      num_repeats, // num_runs
      1 // n_chunk
      );

  // set up signal handling -- really easy we we'll do this.  Hit CTRL+C to get an instant
  // status report.
  auto srep = Tomographer::Tools::makeSigHandlerTaskDispatcherStatusReporter(&tasks, logger.parentLogger());
  Tomographer::Tools::installSignalHandler(SIGINT, &srep);

  //
  // Finally, run our tomo process
  //

  logger.debug("all set, ready to go");

  auto time_start = std::chrono::system_clock::now();

  srep.time_start = time_start;

  tasks.run(); // GO!

  auto time_end = std::chrono::system_clock::now();

  logger.debug("Random walks done.");

  // delta-time, formatted in hours, minutes, seconds and fraction of seconds
  std::string elapsed_s = Tomographer::Tools::fmtDuration(time_end - time_start);

  //
  // The 'results' object contains all the interesting results.  It is a
  // ResultsCollectorWithBinningAnalysis object located in the namespace
  // Tomographer::MHRWTasks::ValueHistogramTasks (check out the API documentation).
  //
  // The most interesting methods are probably finalHistogram() and
  // collectedRunTaskResults().
  //
  auto histogram = results.finalHistogram();

  logger.info([&](std::ostream & stream) {
      stream << "FINAL HISTOGRAM OF THE FIGURE OF MERIT:\n"
	     << histogram.prettyPrint()
	     << "\n\n";
      // We should report if the error bars from the binning anlaysis have
      // converged nicely. This is a bit quick-and-dirty, see the
      // "tomorun_dispatch.h" source for a slightly more sophisticated way
      int num_converged = 0, num_unknown = 0, num_notconverged = 0;
      double accept_ratio = 0.0;
      for (std::size_t j = 0; j < results.numTasks(); ++j) {
        auto collectedresult = results.collectedRunTaskResult(j);
        num_converged += collectedresult->stats_collector_result.converged_status
          .cwiseEqual(OurResultsCollector::BinningAnalysisParamsType::CONVERGED).count();
        num_unknown += collectedresult->stats_collector_result.converged_status
          .cwiseEqual(OurResultsCollector::BinningAnalysisParamsType::UNKNOWN_CONVERGENCE).count();
        num_notconverged += collectedresult->stats_collector_result.converged_status
          .cwiseEqual(OurResultsCollector::BinningAnalysisParamsType::NOT_CONVERGED).count();
        accept_ratio += collectedresult->acceptance_ratio;
      }
      stream << "Average Acceptance Ratio: "
             << std::setprecision(2) << accept_ratio / results.numTasks() << "\n\n";
      stream << "Error bars from binning analysis: "
             << num_converged << " converged, "
             << num_unknown << " unknown, "
             << num_notconverged << " not converged / "
             << num_converged+num_unknown+num_notconverged << " total\n\n";
      stream << "Computation time: " << elapsed_s
	     << "\n\n";
    });


  // success.
  return 0;
}
