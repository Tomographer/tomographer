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
// The root logger which takes care of handling the log messages.  Here, we log to the
// standard output (recall that stdout is seen as a "file").
//
// The level of the logger can be changed to one of Tomographer::Logger::LONGDEBUG,
// Tomographer::Logger::DEBUG, Tomographer::Logger::INFO, Tomographer::Logger::WARNING or
// Tomographer::Logger::ERROR.
//
typedef Tomographer::Logger::FileLogger BaseLoggerType;
BaseLoggerType rootlogger(stdout, Tomographer::Logger::DEBUG); //LONGDEBUG);


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

  typedef Tomographer::UniformBinsHistogramWithErrorBars<double> HistogramType;

  // parameters of the histogram of the figure of merit: cover the range [0.75, 1.0] by
  // dividing it into 50 bins
  typedef HistogramType::Params HistogramParams;
  const HistogramParams hist_params(0.75, 1.0, 50);


  // parameters of the random walk
  typedef Tomographer::MHRWParams<Tomographer::MHWalkerParamsStepSize<>,int> MHRWParamsType;
  const MHRWParamsType mhrw_params(
      1e-8,
      1e8,
      1024, // # of thermalization sweeps
      32768 // # of live sweeps in which samples are collected
      );

  // seed for random number generator -- just use the current time
  typedef std::mt19937 Rng;
  Rng::result_type base_seed = (Rng::result_type)std::chrono::system_clock::now().time_since_epoch().count();
  Rng rng(base_seed);

  typedef BaseLoggerType LoggerType;

  typedef Tomographer::DenseDM::TSpace::LLHMHWalker<DenseLLH,Rng,LoggerType> MHWalkerType;
  MHWalkerType mhwalker(
      llh.dmt.initMatrixType(),
      llh,
      rng,
      logger.parentLogger()
      );

  // 
  // Create the stats collectors
  //
  const int binning_num_levels = 8;

  typedef Tomographer::ValueHistogramWithBinningMHRWStatsCollectorParams<ValueCalculator>
    BinningMHRWStatsCollectorParams;

  typedef Tomographer::ValueHistogramWithBinningMHRWStatsCollector<BinningMHRWStatsCollectorParams,LoggerType>
    HistogramStatsCollector;

  HistogramStatsCollector histstatscollector(
	hist_params,
	valcalc,
        binning_num_levels,
	logger.parentLogger()
	);

  Tomographer::MHRWMovingAverageAcceptanceRatioStatsCollector<>
    avgacceptstatscollector;


  typedef Tomographer::PeriodicStatusReportMHRWStatsCollector<MHRWParamsType> OurStatusReportCheck;
  OurStatusReportCheck statreportcheck(
      // frequency interval -- every this many milliseconds
      std::chrono::milliseconds(100),
      // send-status-function
      [](Tomographer::MHRWStatusReport<MHRWParamsType> report) {
        // simply print out the report
        std::cerr << report.msg << "\n";
      }
      );

  typedef 
    Tomographer::MultipleMHRWStatsCollectors<HistogramStatsCollector,
                                             Tomographer::MHRWMovingAverageAcceptanceRatioStatsCollector<>,
                                             OurStatusReportCheck>
    StatsCollectors;

  StatsCollectors full_stats_coll(histstatscollector, avgacceptstatscollector, statreportcheck);

  typedef Tomographer::MHRWStepSizeController<Tomographer::MHRWMovingAverageAcceptanceRatioStatsCollector<>,
                                              LoggerType>
    MHRWStepSizeControllerType;
  typedef Tomographer::MHRWValueErrorBinsConvergedController<HistogramStatsCollector,int,LoggerType>
    MHRWBinsConvergedControllerType;

  MHRWStepSizeControllerType mhrw_controller_step(avgacceptstatscollector, logger.parentLogger());
  MHRWBinsConvergedControllerType mhrw_controller_conv(histstatscollector, logger.parentLogger());

  typedef Tomographer::MHRWMultipleControllers<MHRWStepSizeControllerType,MHRWBinsConvergedControllerType>
    MHRWControllerType;

  MHRWControllerType mhrw_controller(mhrw_controller_step, mhrw_controller_conv);

  typedef Tomographer::MHRandomWalk<Rng,MHWalkerType,StatsCollectors,MHRWControllerType,LoggerType,int>
    MHRandomWalkType;

  MHRandomWalkType rwalk(
      // MH random walk parameters
      mhrw_params,
      // the MHWalker
      mhwalker,
      // our stats collectors
      full_stats_coll,
      // dynamic controller
      mhrw_controller,
      // a random number generator
      rng,
      // and a logger
      logger.parentLogger()
      );


  logger.debug("all set, ready to go");

  auto time_start = std::chrono::system_clock::now();

  rwalk.run();

  auto time_end = std::chrono::system_clock::now();

  logger.debug("Random walk done.");

  // delta-time, formatted in hours, minutes, seconds and fraction of seconds
  std::string elapsed_s = Tomographer::Tools::fmtDuration(time_end - time_start);

  // the result collected directly, fresh from our stats collector
  auto result = histstatscollector.stealResult();
  const auto & histogram = result.histogram;

  // histogram has type Tomographer::AveragedHistogram, you can use it like any other
  // histogram object with error bars (see "Histogram Type Interface").  E.g.:
  logger.info([&](std::ostream & stream) {
      stream << "Histogram has " << histogram.numBins() << " bins, range is ["
             << histogram.params.min << ".." << histogram.params.max  << "]\n\n";
      stream << histogram.prettyPrint();
      stream << "\n";
      stream << "Error bars: " << result.errorBarConvergenceSummary() << "\n";
    });

  // success.
  return 0;
}
