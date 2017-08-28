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
#include <tomographer/tools/eigenutil.h>
#include <tomographer/densedm/dmtypes.h>
#include <tomographer/densedm/indepmeasllh.h>
#include <tomographer/densedm/tspacellhwalker.h>
#include <tomographer/densedm/tspacefigofmerit.h>
#include <tomographer/mhrw.h>
#include <tomographer/mhrwtasks.h>
#include <tomographer/mhrw_valuehist_tools.h>

#include <tomographer/mpi/multiprocmpi.h>

#include <boost/serialization/base_object.hpp>


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
// The base CData object we use.  See Tomographer:::MHRWTasks::ValueHistogramTools::CDataBase.
//
typedef Tomographer::MHRWTasks::ValueHistogramTools::CDataBase<
  ValueCalculator, // our value calculator
  true, // use binning analysis
  Tomographer::MHWalkerParamsStepSize<double>, // MHWalkerParams
  std::mt19937::result_type, // RngSeedType
  long, // IterCountIntType
  double, // CountRealType
  int // HistCountIntType
  >
  BaseCData;

//
// We need to define a class which adds the capacity of creating the "master"
// random walk object to the engine in
// Tomographer::MHRWTasks::ValueHistogramTools, which take care of running the
// random walks etc. as needed.
//
struct OurCData : public BaseCData
{
  OurCData(DenseLLH * llh_, // data from the the tomography experiment
	   ValueCalculator valcalc, // the figure-of-merit calculator
	   HistogramParams hist_params, // histogram parameters
	   int binning_num_levels, // number of binning levels in the binning analysis
	   MHRWParamsType mhrw_params, // parameters of the random walk
	   RngSeedType base_seed) // a random seed to initialize the random number generator
    : BaseCData(valcalc, hist_params, binning_num_levels,
                mhrw_params, base_seed),
      llh(llh_)
  {
  }

  DenseLLH * llh;

  // The result of a task run -- just use the default StatsResults type provided
  // by ValueHistogramTools.  If we had several stats collectors set, we would
  // need to pick out the result corresponding to the
  // value-histogram-stats-collector (see "minimal_tomorun_controlled.cxx" for
  // an example).
  typedef MHRWStatsResultsBaseType MHRWStatsResultsType;

  //
  // This function is called automatically by the task manager/dispatcher via
  // MHRWTasks.  It should set up the random walk as required (at a minimum,
  // create a MHWalker instance and pass on the default value stats collector
  // from Tomographer::MHRWTasks::ValueHistogramTools), and run it.
  //
  // We should not forget to call run(), to actually run the random walk!
  //
  template<typename Rng, typename LoggerType, typename ExecFn>
  inline void setupRandomWalkAndRun(Rng & rng, LoggerType & logger, ExecFn run) const
  {
    auto val_stats_collector = createValueStatsCollector(logger);

    Tomographer::DenseDM::TSpace::LLHMHWalker<DenseLLH,Rng,LoggerType> mhwalker(
            llh->dmt.initMatrixType(),
            *llh,
            rng,
            logger
            );

    run(mhwalker, val_stats_collector);
  };

private:
  OurCData() : BaseCData(), llh(NULL) { } // for serialization

  friend boost::serialization::access;
  template<typename Archive>
  void serialize(Archive & a, unsigned int /* version */)
  {
    a & boost::serialization::base_object<BaseCData>(*this);
    a & llh;
  }
};


//
// The root logger which takes care of handling the log messages.  Here, we log to the
// standard output (recall that stdout/stderr are seen as a "file").
//
// The level of the logger can be set to one of Tomographer::Logger::LONGDEBUG,
// Tomographer::Logger::DEBUG, Tomographer::Logger::INFO, Tomographer::Logger::WARNING or
// Tomographer::Logger::ERROR.
//
typedef Tomographer::Logger::FileLogger BaseLoggerType;
BaseLoggerType rootlogger(stderr, Tomographer::Logger::DEBUG);


// The task type, for the MultiProc interface
typedef Tomographer::MHRWTasks::MHRandomWalkTask<OurCData, std::mt19937>
  OurMHRandomWalkTask;


int main()
{
  //
  // MPI initializations
  //
  mpi::environment mpi_env;
  mpi::communicator mpi_world;

  const bool is_master = (mpi_world.rank() == 0);


  //
  // Prepare the logger in which we can log debug/info/error messages.
  //
  typedef Tomographer::Logger::OriginPrefixedLogger<decltype(rootlogger)> PrefixLoggerType;
  PrefixLoggerType baselogger(rootlogger, streamstr(mpi_world.rank() << "/" << mpi_world.size()<<"|"));

  auto logger = Tomographer::Logger::makeLocalLogger("main()", baselogger);


  logger.debug("starting up");


  DenseLLH * llh = NULL;
  OurCData * taskcdat = NULL;

  if (is_master) {
    //
    // Master gets to prepare all the data.
    //
    
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

    llh = new DenseLLH(dmt);
  
    // POVM effects for  \sigma_x \otimes \sigma_x
  
    DMTypes::MatrixType Exxplus(dmt.initMatrixType());
    Exxplus <<
      0.5,    0,    0,  0.5,
      0,    0.5,  0.5,    0,
      0,    0.5,  0.5,    0,
      0.5,    0,    0,  0.5;

    llh->addMeasEffect(Exxplus, 95);  // 95 counts of "+1" out of 100 for \sigma_x\otimes\sigma_x

    DMTypes::MatrixType Exxminus(dmt.initMatrixType());
    Exxminus <<
      0.5,    0,    0, -0.5,
      0,    0.5, -0.5,    0,
      0,   -0.5,  0.5,    0,
      -0.5,   0,    0,  0.5;

    llh->addMeasEffect(Exxminus, 5);  // 95 counts of "-1" out of 100 for \sigma_x\otimes\sigma_x

    // POVM effects for  \sigma_y \otimes \sigma_y
  
    DMTypes::MatrixType Eyyplus(dmt.initMatrixType());
    Eyyplus <<
      0.5,    0,    0, -0.5,
      0,    0.5,  0.5,    0,
      0,    0.5,  0.5,    0,
      -0.5,   0,    0,  0.5;

    llh->addMeasEffect(Eyyplus, 8);  // 8 counts of "+1" out of 100 for \sigma_y\otimes\sigma_y

    DMTypes::MatrixType Eyyminus(dmt.initMatrixType());
    Eyyminus <<
      0.5,    0,    0,  0.5,
      0,    0.5, -0.5,    0,
      0,   -0.5,  0.5,    0,
      0.5,    0,    0,  0.5;

    llh->addMeasEffect(Eyyminus, 92);  // 92 counts of "-1" out of 100 for \sigma_y\otimes\sigma_y

    // POVM effects for  \sigma_z \otimes \sigma_z
  
    DMTypes::MatrixType Ezzplus(dmt.initMatrixType());
    Ezzplus <<
      1,   0,   0,   0,
      0,   0,   0,   0,
      0,   0,   0,   0,
      0,   0,   0,   1;

    llh->addMeasEffect(Ezzplus, 98);  // 98 counts of "+1" out of 100 for \sigma_z\otimes\sigma_z

    DMTypes::MatrixType Ezzminus(dmt.initMatrixType());
    Ezzminus <<
      0,   0,   0,   0,
      0,   1,   0,   0,
      0,   0,   1,   0,
      0,   0,   0,   0;

    llh->addMeasEffect(Ezzminus, 2);  // 2 counts of "-1" out of 100 for \sigma_z\otimes\sigma_z

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
    const OurCData::HistogramParams hist_params(0.7, 1.0, 50);

    // parameters of the random walk
    const OurCData::MHRWParamsType mhrw_params(
        0.04, // step size (choose such that acceptance ratio ~ 0.25)
        50, // sweep size (should be chosen such that  sweep_size*step_size >~ 1)
        1024, // # of thermalization sweeps
        32768 // # of live sweeps in which samples are collected
        );

    // seed for random number generator -- just use the current time
    std::mt19937::result_type base_seed =
      (std::mt19937::result_type)std::chrono::system_clock::now().time_since_epoch().count();

    // number of levels for the binning analysis
    const int binning_num_levels = 8;

    // instantiate the class which stores the shared data.
    taskcdat = new OurCData(llh, valcalc, hist_params, binning_num_levels,
                            mhrw_params, base_seed);

    logger.debug("Master here, data ready") ;
  } else {
    taskcdat = NULL;
    logger.debug("Not master, skipping through all the init process") ;
  }

  //
  // Data is ready, prepare & launch the random walks.  Use MPI.
  //


  // repeat the whole random walk this number of times.  These random walks will
  // run in parallel depending on the number of CPUs available.
  const int num_repeats = 20;

  // create the task manager/dispatcher, using the MPI implementation !!
  auto tasks =
    Tomographer::MultiProc::MPI::mkTaskDispatcher<OurMHRandomWalkTask>(
        taskcdat, // constant data
        mpi_world,
        logger.parentLogger(), // the main logger object
        num_repeats // num_runs
        );

  if (is_master) {
    // only master gets to do this

    // get status reports every X milliseconds printed out on std::cout
    tasks.setStatusReportHandler([&](decltype(tasks)::FullStatusReportType report) {
        std::cout << report.getHumanReport() << "\n" ;
      });
    tasks.requestPeriodicStatusReport(2000) ;
  }

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

  if (!is_master) {
    logger.debug("not master, we're done here.");
    return 0;
  }

  // only master beyond this point

  const auto & task_results = tasks.collectedTaskResults();

  auto aggregated_histogram = taskcdat->aggregateResultHistograms(task_results) ;

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
          *taskcdat, // the cdata
          task_results, // the results
          aggregated_histogram // aggregated
          );
    });


  delete llh;
  delete taskcdat;

  logger.debug("Finally, all done.");

  // success.
  return 0;
}
