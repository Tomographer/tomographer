
//#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <cerrno>

#include <signal.h>
#include <unistd.h>

#include <algorithm>
#include <complex>
#include <chrono>
#include <iostream>
#include <fstream>
#include <iomanip>

#include <boost/program_options.hpp>

#include <Eigen/Core>

#define EZMATIO_EIGEN_TYPES
#define EZMATIO_MORE_UTILS
#include "ezmatio.h"


//#ifdef TOMOGRAPHER_HAVE_OMP
// yes, OMP multithreading ROCKS!
#  include <omp.h>
//#endif

#include <tomographer/qit/matrq.h>
#include <tomographer/qit/util.h>
#include <tomographer/qit/param_herm_x.h>
#include <tomographer/loggers.h>
#include <tomographer/tomoproblem.h>
#include <tomographer/integrator.h>
#include <tomographer/dmintegrator.h>
#include <tomographer/multiprocomp.h>



// if defined, will make sure that all POVM effects read from the input file are positive
// semidefinite and nonzero.
//#define DO_SLOW_POVM_CONSISTENCY_CHECKS


//#define TimerClock std::chrono::high_resolution_clock
#define TimerClock std::chrono::system_clock




using namespace Tomographer;



struct ProgOptions
{
  ProgOptions()
    : // provide sensible defaults:
    nice_level(14),
    flog(stdout),
    data_file_name(),
    step_size(0.01),
    Nsweep(std::max(10, int(1/step_size))),
    Ntherm(500),
    Nrun(5000),
    fid_min(0.97),
    fid_max(1.0),
    fid_nbins(50),
    start_seed(std::chrono::system_clock::now().time_since_epoch().count()),
    Nrepeats(256),
    Nchunk(1),
    NMeasAmplifyFactor(1.0),
    loglevel(Logger::INFO),
    write_histogram("")
  {
  }

  int nice_level;

  FILE * flog;

  std::string data_file_name;

  double step_size;

  unsigned int Nsweep;
  unsigned int Ntherm;
  unsigned int Nrun;

  double fid_min;
  double fid_max;
  std::size_t fid_nbins;

  int start_seed;

  unsigned int Nrepeats;
  unsigned int Nchunk;

  double NMeasAmplifyFactor;

  int loglevel;

  std::string write_histogram;
};



SimpleFoutLogger logger(stdout, Logger::INFO, false);



// ------------------------------------------------------------------------------

void parse_options(ProgOptions * opt, int argc, char **argv)
{
  // read the options
  using namespace boost::program_options;

  std::string flogname;

  std::string fidhiststr;

  options_description desc("Allowed Options");
  desc.add_options()
    ("help", "Print Help Message")
    ("data-file-name", value<std::string>(& opt->data_file_name)->default_value("testfile.mat"),
     "specify .mat data file name")
    ("fidelity-hist", value<std::string>(&fidhiststr),
     "Do a histogram for different fidelity values, format MIN:MAX/NPOINTS")
    ("step-size", value<double>(& opt->step_size)->default_value(opt->step_size),
     "the step size for the region")
    ("n-sweep", value<unsigned int>(& opt->Nsweep)->default_value(opt->Nsweep),
     "number of iterations per sweep")
    ("n-therm", value<unsigned int>(& opt->Ntherm)->default_value(opt->Ntherm),
     "number of thermalizing sweeps")
    ("n-run", value<unsigned int>(& opt->Nrun)->default_value(opt->Nrun),
     "number of running sweeps after thermalizing")
    ("n-repeats", value<unsigned int>(& opt->Nrepeats)->default_value(opt->Nrepeats),
     "number of times to repeat the metropolis procedure")
    ("n-chunk", value<unsigned int>(& opt->Nchunk)->default_value(opt->Nchunk),
     "chunk the number of repeats by this number per OMP thread")
    ("n-meas-amplify-factor", value<double>(& opt->NMeasAmplifyFactor)->default_value(opt->NMeasAmplifyFactor),
     "Specify an integer factor by which to multiply number of measurements.")
    ("write-histogram", value<std::string>(& opt->write_histogram),
     "write the histogram to the given file in tabbed CSV values")
    ("verbose", value<int>(& opt->loglevel)->default_value(opt->loglevel)->implicit_value(Logger::DEBUG),
     "print iteration info. Not very readable unless n-repeats=1. You may also specify "
     "a specific verbosity level (integer); the higher the more verbose.")
    ("nice", value<int>(& opt->nice_level)->default_value(opt->nice_level),
     "Renice the process to the given level to avoid slowing down the whole system. Set to zero "
     "to avoid renicing.")
    ("log", value<std::string>(& flogname),
     "Redirect standard output (log) to the given file. Use '-' for stdout. If file exists, will append.")
    ;

  try {
    variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);

    if (vm.count("help")) {
      std::cout << desc;
      ::exit(1);
    }

    notify(vm);
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    ::exit(255);
  }

  // set up logging
  if (flogname.size() == 0 || flogname == "-") {
    opt->flog = stdout;
  } else {
    opt->flog = fopen(flogname.c_str(), "a");
    if (opt->flog == NULL) {
      logger.error("parse_options()", "Can't open file %s for logging: %s", flogname.c_str(), strerror(errno));
      ::exit(255);
    }
    logger.info("parse_options()", "Output is now being redirected to %s.", flogname.c_str());

    // write out header
    char curdtstr[128];
    std::time_t tt;
    std::time(&tt);
    std::tm * ptim = localtime(&tt);
    std::strftime(curdtstr, sizeof(curdtstr), "%c", ptim);
    std::fprintf(
        opt->flog,
        "\n\n\n"
        "================================================================================\n"
        "    tomorun -- NEW RUN   on %s\n"
        "================================================================================\n\n",
        curdtstr
        );

    logger.setFp(opt->flog);
  }

  // set up log level
  logger.setLevel(opt->loglevel);

  if (fidhiststr.size()) {
    double fmin, fmax;
    int nbins = 100;
    if (std::sscanf(fidhiststr.c_str(), "%lf:%lf/%d", &fmin, &fmax, &nbins) < 2) {
      logger.error("parse_options()",
                   "Error: --fidelity-hist expects an argument of format MIN:MAX[/NPOINTS]");
      ::exit(255);
    }

    opt->fid_min = fmin;
    opt->fid_max = fmax;
    opt->fid_nbins = nbins;
  }
}

// --------------------------------------------------

void display_parameters(ProgOptions * opt)
{
  logger.info(
      // origin
      "display_parameters()",
      // message
      "\n"
      "Using  data from file     = %s  (measurements x%.3g)\n"
      "       fid. histogram     = [%.2g, %.2g] (%lu bins)\n"
      "       step size          = %.6f\n"
      "       sweep size         = %lu\n"
      "       # therm sweeps     = %lu\n"
      "       # run sweeps       = %lu\n"
      "       # intgr. repeats   = %lu   (chunked by %lu/thread)\n"
      "       write histogram to = %s\n"
      "\n"
      "       --> total no. of live samples = %lu  (%.2e)\n"
      "\n",
      opt->data_file_name.c_str(),
      opt->NMeasAmplifyFactor,
      opt->fid_min, opt->fid_max, (unsigned long)opt->fid_nbins,
      opt->step_size,
      (unsigned long)opt->Nsweep,
      (unsigned long)opt->Ntherm,
      (unsigned long)opt->Nrun,
      (unsigned long)opt->Nrepeats,
      (unsigned long)opt->Nchunk,
      (opt->write_histogram.size()?opt->write_histogram:std::string("<don't write histogram>")).c_str(),
      (unsigned long)(opt->Nrun*opt->Nrepeats),
      (double)(opt->Nrun*opt->Nrepeats)
      );
}


// --------------------------------------------------

template<typename Duration>
std::string fmt_duration(Duration dt)
{
  // delta-time, in seconds and fraction of seconds
  double seconds = (double)dt.count() * Duration::period::num / Duration::period::den ;

  // split `seconds' into integral part and fractional part
  double dt_i_d;
  double dt_f = std::modf(seconds, &dt_i_d);
  int dt_i = (int)(dt_i_d+0.5);

  return fmts("%d:%02d:%02d.%03d", dt_i/3600, (dt_i/60)%60, dt_i%60, (int)(dt_f*1000+0.5));
};



// ------------------------------------------------------------------------------

template<int FixedDim, int FixedMaxPOVMEffects, typename Logger>
inline void tomorun(unsigned int dim, ProgOptions * opt, MAT::File * matf, Logger & logger);


int main(int argc, char **argv)
{
  ProgOptions opt;

  parse_options(&opt, argc, argv);

  logger.info(
      // origin
      "main()",
      // message
      "\n"
      "-------------------------------\n"
      "Welcome to tomorun.\n"
      "-------------------------------\n"
      );

  display_parameters(&opt);

  //
  // Renice the program, if requested
  //

  if (opt.nice_level != 0) {
    // nice up our process.
    errno = 0; // not std::errno, errno is a macro
    int niceret = nice(opt.nice_level);
    if (niceret == -1 && errno != 0) {
      logger.warning(
          "main()",
          "Failed to nice(%d) process: %s",
          opt.nice_level, strerror(errno)
          );
    } else {
      logger.debug("main()", "nice()'ed our process to priority %d", niceret);
    }
  }

  logger.debug(
      // origin
      "main()",
      // message
      "SIMD instructions set in use by Eigen: %s",
      Eigen::SimdInstructionSetsInUse()
      );

  //
  // ---------------------------------------------------------------------------
  // Read tomography data from MATLAB file
  // ---------------------------------------------------------------------------
  //

  MAT::File * matf = 0;
  unsigned int dim = 0;
  unsigned int n_povms = 0;
  try {
    matf = new MAT::File(opt.data_file_name);
    dim = MAT::value<int>(matf->var("dim"));
    n_povms = matf->var("Nm").numel();
  } catch(const std::exception& e) {
    logger.error("main()", [&opt, &e](std::ostream & str){
                   str << "Failed to read data from file "<< opt.data_file_name << "\n\t" << e.what() << "\n";
                 });
    ::exit(1);
  }

  auto delete_matf = finally([matf] {
                               logger.debug("main()", "Freeing input file resource");
                               delete matf;
                             });

  logger.debug("main()", "Data file opened, dim = %u", dim);

  //
  // ---------------------------------------------------------------------------
  // Now, run our main program.
  // ---------------------------------------------------------------------------
  //

  // Maybe use statically instantiated size for some predefined sizes.

  //  MinimumImportanceLogger<SimpleFoutLogger, Logger::INFO> mlog(logger);
  auto & mlog = logger;

  //  try {
    if (dim == 2 && n_povms <= 6) {
      tomorun<2, 6>(dim, &opt, matf, mlog);
    } else if (dim == 2 && n_povms <= 64) {
      tomorun<2, 64>(dim, &opt, matf, mlog);
    } else if (dim == 2) {
      tomorun<2, Eigen::Dynamic>(dim, &opt, matf, mlog);
    } else if (dim == 4 && n_povms <= 64) {
      tomorun<4, 64>(dim, &opt, matf, mlog);
    } else if (dim == 4) {
      tomorun<4, Eigen::Dynamic>(dim, &opt, matf, mlog);
    } else {
      tomorun<Eigen::Dynamic, Eigen::Dynamic>(dim, &opt, matf, mlog);
    }
    //  } catch (const std::exception& e) {
    //    logger.error("main()", "Caught exception: %s", e.what());
    //    return 2;
    //  }
}







//
// Some ugly workaround to be able to call code within the templated tomorun() from our
// signal handler.
//
struct SignalHandler
{
  SignalHandler() { }
  virtual ~SignalHandler() { }

  virtual void handle_signal(int) = 0;
};

static SignalHandler * signal_handler = NULL;


//
// A generic handler which requests a status report from an OMPTaskDispatcher
//
template<typename OMPTaskDispatcher, typename Logger>
struct SigHandlerStatusReporter : public SignalHandler
{
  SigHandlerStatusReporter(OMPTaskDispatcher * tasks_, Logger & logger_)
    : tasks(tasks_), logger(logger_), time_start()
  {
    tasks->set_status_report_handler(
        [this](const typename OMPTaskDispatcher::FullStatusReportType& report) {
          logger.debug("SigHandlerStatusReporter/lambda", "intermediate progress report lambda called");
          this->intermediate_progress_report(report);
        });
  }
  
  OMPTaskDispatcher * tasks;
  Logger & logger;

  TimerClock::time_point time_start;

  virtual void handle_signal(int /*sig*/)
  {
    tasks->request_status_report();
  }

  //
  // Format a nice intermediate progress report.
  //
  void intermediate_progress_report(const typename OMPTaskDispatcher::FullStatusReportType& report)
  {
    std::string elapsed = fmt_duration(TimerClock::now() - time_start);
    fprintf(stderr,
            "\n"
            "=========================== Intermediate Progress Report ============================\n"
            "                                              (hit Ctrl+C quickly again to interrupt)\n"
            "  Total Completed Runs: %d/%d: %5.2f%%\n"
            "  %s total elapsed\n"
            "Current Run(s) information (threads %d working / %d spawned):\n",
            report.num_completed, report.num_total_runs,
            (double)report.num_completed/report.num_total_runs*100.0,
            elapsed.c_str(), report.num_active_working_threads,
            report.num_threads
            );
    // calculate padding needed to align results
    //    typedef typename OMPTaskDispatcher::TaskStatusReportType TaskStatusReportType;
    //    auto elem = std::max_element(report.tasks_reports.begin(), report.tasks_reports.end(),
    //                                 [](const TaskStatusReportType& a, const TaskStatusReportType& b) -> bool {
    //                                   return a.msg.size() < b.msg.size();
    //                                 });
    //    std::size_t maxmsgwid = (*elem).msg.size();
    for (int k = 0; k < report.num_threads; ++k) {
      std::string msg = report.tasks_running[k] ? report.tasks_reports[k].msg : std::string("<idle>");
      //      if (msg.size() < maxmsgwid) {
      //        msg.insert(0, maxmsgwid - msg.size(), ' ');
      //      }
      fprintf(stderr, "=== Thread #%2d: %s\n", k, msg.c_str());
    }
    fprintf(stderr,
            "=====================================================================================\n\n");
  };

};

template<typename OMPTaskDispatcher, typename Logger>
SigHandlerStatusReporter<OMPTaskDispatcher, Logger>
makeSigHandlerStatusReporter(OMPTaskDispatcher * tasks, Logger & logger)
{
  // yes, RVO better kick in
  return SigHandlerStatusReporter<OMPTaskDispatcher, Logger>(tasks, logger);
}




//const std::time_t SIG_HIT_REPEAT_EXIT_DELAY = 0; // set this only for debugging the signal interrupt
const std::time_t SIG_HIT_REPEAT_EXIT_DELAY = 2;


static std::time_t last_sig_hit_time = 0;

void sig_int_handler(int signal)
{
  std::fprintf(stderr, "\n*** interrupt\n");
  std::time_t now;
  time(&now);
  if ( (now - last_sig_hit_time) < SIG_HIT_REPEAT_EXIT_DELAY ) {
    // two interrupts within two seconds --> exit
    std::fprintf(stderr, "\n*** Exit\n");
    ::exit(1);
    return;
  }
  last_sig_hit_time = now;

  if (signal_handler != NULL) {
    signal_handler->handle_signal(signal);
  } else {
    fprintf(stderr, "Warning: sig_handle: no signal handler set (got signal %d)\n", signal);
  }
}

//
// Here goes the actual program. This is templated, because then we can let Eigen use
// allocation on the stack rather than malloc'ing 2x2 matrices...
//
template<int FixedDim, int FixedMaxPOVMEffects, typename Logger>
inline void tomorun(unsigned int dim, ProgOptions * opt, MAT::File * matf, Logger & logger)
{
  logger.debug("tomorun()", "Running tomography program! FixedDim=%d and FixedMaxPOVMEffects=%d",
               FixedDim, FixedMaxPOVMEffects);

  //
  // Typedefs for tomography data types
  //

  typedef MatrQ<FixedDim, FixedMaxPOVMEffects, double, unsigned int> OurMatrQ;
  typedef IndepMeasTomoProblem<OurMatrQ, double> OurTomoProblem;

  //
  // Read data from file
  //

  OurMatrQ matq(dim);
  OurTomoProblem tomodat(matq);

  std::vector<typename OurMatrQ::MatrixType> Emn;
  MAT::getListOfEigenMatrices(matf->var("Emn"), & Emn, true);
  Eigen::VectorXi Nm;
  MAT::getEigenMatrix(matf->var("Nm"), & Nm);
  assert((int)Emn.size() == Nm.size());

  // convert to x-parameterization
  unsigned int k, j;
  int Npovmeffects = 0;
  for (k = 0; k < (unsigned int)Nm.size(); ++k) {
    if (Nm[k] > 0) {
      ++Npovmeffects;
    }
  }
  logger.debug("tomorun()", "Npovmeffects=%d", Npovmeffects);
  tomodat.Exn.resize(Npovmeffects, dim*dim);
  tomodat.Nx.resize(Npovmeffects);
  j = 0;
  for (k = 0; k < Emn.size(); ++k) {
    if (Nm[k] == 0) {
      continue;
    }
    // do some tests: positive semidefinite and non-zero
#ifdef DO_SLOW_POVM_CONSISTENCY_CHECKS
    logger.longdebug("tomorun()", [&](std::ostream & str) {
                       str << "Emn["<<k<<"] = \n" << Emn[k] << "\n"
                           << "\tEV=" << Emn[k].eigenvalues().transpose().real() << "\n"
                           << "\tnorm=" << double(Emn[k].norm()) << "\n" ;
                     });
    eigen_assert(double( (Emn[k] - Emn[k].adjoint()).norm() ) < 1e-8); // Hermitian
    eigen_assert(double(Emn[k].eigenvalues().real().minCoeff()) >= -1e-12); // Pos semidef
    eigen_assert(double(Emn[k].norm()) > 1e-6);
    logger.debug("tomorun()", "Consistency checks passed for Emn[%u].", k);
#endif

    // don't need to reset `row` to zero, param_herm_to_x doesn't require it
    auto Exn_row_as_col = tomodat.Exn.row(j).transpose();
    param_herm_to_x(Exn_row_as_col, Emn[k]);
    tomodat.Nx[j] = Nm[k];
    ++j;
  }
  // done.

  logger.debug("tomorun()", [&](std::ostream & ss) {
                 ss << "\n\nExn: size="<<tomodat.Exn.size()<<"\n"
                    << tomodat.Exn << "\n";
                 ss << "\n\nNx: size="<<tomodat.Nx.size()<<"\n"
                    << tomodat.Nx << "\n";
               });

  MAT::getEigenMatrix(matf->var("rho_MLE"), &tomodat.rho_MLE);

  tomodat.x_MLE = matq.initVectorParamType();
  param_herm_to_x(tomodat.x_MLE, tomodat.rho_MLE);

  tomodat.T_MLE = matq.initMatrixType();

  Eigen::LDLT<typename OurMatrQ::MatrixType> ldlt(tomodat.rho_MLE);
  typename OurMatrQ::MatrixType P = matq.initMatrixType();
  P = ldlt.transpositionsP().transpose() * OurMatrQ::MatrixType::Identity(dim,dim);
  tomodat.T_MLE.noalias() = P * ldlt.matrixL() * ldlt.vectorD().cwiseSqrt().asDiagonal();

  tomodat.NMeasAmplifyFactor = opt->NMeasAmplifyFactor;

  //
  // Data has now been successfully read. Now, create the OMP Task Manager and run.
  //

  typedef DMIntegratorTasks::CData<OurTomoProblem> OurCData;
  typedef MultiProc::OMPTaskLogger<Logger> OurTaskLogger;
  typedef DMIntegratorTasks::MHRandomWalkTask<OurTomoProblem,OurTaskLogger> OurMHRandomWalkTask;
  typedef DMIntegratorTasks::MHRandomWalkResultsCollector<
      typename OurMHRandomWalkTask::FidelityHistogramMHRWStatsCollectorType::HistogramType
      >
    OurResultsCollector;

  OurCData taskcdat(tomodat);
  // seed for random number generator
  taskcdat.base_seed = std::chrono::system_clock::now().time_since_epoch().count();
  // parameters for the fidelity histogram
  taskcdat.histogram_params = typename OurCData::HistogramParams(opt->fid_min, opt->fid_max, opt->fid_nbins);
  // parameters of the random walk
  taskcdat.n_sweep = opt->Nsweep;
  taskcdat.n_therm = opt->Ntherm;
  taskcdat.n_run = opt->Nrun;
  taskcdat.step_size = opt->step_size;

  OurResultsCollector results(taskcdat.histogram_params);

  auto tasks = MultiProc::makeOMPTaskDispatcher<OurMHRandomWalkTask>(
      &taskcdat, // constant data
      &results, // results collector
      logger, // the main logger object
      opt->Nrepeats, // num_runs
      opt->Nchunk // n_chunk
      );

  // set up signal handling

  auto srep = makeSigHandlerStatusReporter(&tasks, logger);

  signal_handler = &srep;

  signal(SIGINT, sig_int_handler);

  // and run our tomo process

  auto time_start = TimerClock::now();

  srep.time_start = time_start;

  tasks.run();

  auto time_end = TimerClock::now();

  logger.debug("tomorun()", "Random walks done.");

  // delta-time, in seconds and fraction of seconds
  std::string elapsed_s = fmt_duration(time_end - time_start);

  logger.info(
      "tomorun()", [&results](std::ostream & str) {
        unsigned int width = 80;
        // If the user provided a value for the terminal width, use it. Note that $COLUMNS is
        // not in the environment usually, so you have to set it manually with e.g.
        //    shell> export COLUMNS=$COLUMNS
        const char * cols_s = std::getenv("COLUMNS");
        if (cols_s != NULL) {
          width = std::atoi(cols_s);
        }
        str << "FINAL HISTOGRAM\n" << results.pretty_print(width) << "\n";
      });


  // save the histogram to a CSV file if the user required it
  if (opt->write_histogram.size()) {
    std::string csvfname = opt->write_histogram+"-histogram.csv";
    std::ofstream outf;
    outf.open(csvfname);
    outf << "Fidelity\tAvgCounts\tError\n"
         << std::scientific << std::setprecision(10);
    for (int kk = 0; kk < results.final_histogram.size(); ++kk) {
      outf << (double)results.params.bin_lower_value(kk) << "\t"
           << (double)results.final_histogram(kk) << "\t"
           << (double)results.std_dev(kk) << "\n";
    }
    logger.info("tomorun()", "Wrote histogram to CSV file %s", csvfname.c_str());
  }

  logger.info("tomorun()",
              "Computation time: %s\n\n",
              elapsed_s.c_str());

}
