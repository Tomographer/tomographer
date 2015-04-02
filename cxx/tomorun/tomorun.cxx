
//#include <csignal>
#include <cstddef>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <cerrno>

#include <unistd.h>

#include <complex>
#include <chrono>

#include <boost/program_options.hpp>

#include <Eigen/Core>

#define EZMATIO_EIGEN_TYPES
#define EZMATIO_MORE_UTILS
#include "ezmatio.h"

#include <tomographer/loggers.h>
#include <tomographer/tomoproblem.h>

#ifdef TOMOGRAPHER_HAVE_OMP
// yes, OMP multithreading ROCKS!
#  include <omp.h>
#endif



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
    Nchunk(10),
    NMeasAmplifyFactor(1.0),
    loglevel(Logger::INFO),
    write_histogram("")
  {
  }

  int nice_level;

  FILE * flog;

  std::string data_file_name;

  double step_size;

  std::size_t Nsweep;
  std::size_t Ntherm;
  std::size_t Nrun;

  double fid_min;
  double fid_max;
  std::size_t fid_nbins;

  int start_seed;

  std::size_t Nrepeats;
  std::size_t Nchunk;

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
    ("n-sweep", value<std::size_t>(& opt->Nsweep)->default_value(opt->Nsweep),
     "number of iterations per sweep")
    ("n-therm", value<std::size_t>(& opt->Ntherm)->default_value(opt->Ntherm),
     "number of thermalizing sweeps")
    ("n-run", value<std::size_t>(& opt->Nrun)->default_value(opt->Nrun),
     "number of running sweeps after thermalizing")
    ("n-repeats", value<std::size_t>(& opt->Nrepeats)->default_value(opt->Nrepeats),
     "number of times to repeat the metropolis procedure")
    ("n-chunk", value<std::size_t>(& opt->Nchunk)->default_value(opt->Nchunk),
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
     "Redirect standard output (log) to the given file. Use '-' for stdout.")
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
      logger.error("parse_options()", "Can't open file %s: %s", flogname.c_str(), strerror(errno));
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
    if (sscanf(fidhiststr.c_str(), "%lf:%lf/%d", &fmin, &fmax, &nbins) < 2) {
      logger.error("parse_options()",
                   "Error: --fidelity-hist expects an argument of format MIN:MAX[/NPOINTS]");
      ::exit(255);
    }

    opt->fid_min = fmin;
    opt->fid_max = fmax;
    opt->fid_nbins = nbins;
  }
}


void display_parameters(ProgOptions * opt)
{
  logger.info(
      // origin
      "display_parameters()",
      // message
      "\n"
      "Using  data from file     = %s  (measurements x%.3g)\n"
      "       fid. histogram     = %.2g--%.2g (%lu bins)\n"
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



// ------------------------------------------------------------------------------


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

  logger.info(
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

//   UserData our_data;
//   /* Read the data for the llh function & rho_MLE. */
//   try {
//     read_user_data(&our_data, data_file_name);
//   } catch (const MAT::Exception& e) {
//     fprintf(stderr, "Failed to read data: got exception:\n\t%s\n", e.what());
//     return 1;
//   }

  //fprintf(flog, "Loaded data. dim=%d\n", our_data.dim);
  
}
