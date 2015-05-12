
//#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <cerrno>

#include <libgen.h> // dirname, basename
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

//#ifdef TOMOGRAPHER_HAVE_OMP
// yes, OMP multithreading ROCKS!
#  include <omp.h>
//#endif

#include <tomographer/tools/util.h>
#include <tomographer/tools/loggers.h>
#include <tomographer/tools/ezmatio.h>
#include <tomographer/tools/signal_status_report.h>
#include <tomographer/qit/matrq.h>
#include <tomographer/qit/util.h>
#include <tomographer/qit/param_herm_x.h>
#include <tomographer/tomoproblem.h>
#include <tomographer/integrator.h>
#include <tomographer/dmllhintegrator.h>
#include <tomographer/multiprocomp.h>



// if defined, will make sure that all POVM effects read from the input file are positive
// semidefinite and nonzero.
//#define DO_SLOW_POVM_CONSISTENCY_CHECKS


//typedef std::chrono::high_resolution_clock TimerClock;
typedef std::chrono::system_clock TimerClock;




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
    val_type("fidelity"),
    val_min(0.97),
    val_max(1.0),
    val_nbins(50),
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

  std::string val_type;

  double val_min;
  double val_max;
  std::size_t val_nbins;

  int start_seed;

  unsigned int Nrepeats;
  unsigned int Nchunk;

  double NMeasAmplifyFactor;

  int loglevel;

  std::string write_histogram;
};



SimpleFoutLogger logger(stdout, Logger::INFO, false);



// ------------------------------------------------------------------------------

class bad_options : public std::exception
{
  std::string _msg;
public:
  bad_options(const std::string& msg) : _msg(msg) { }
  virtual ~bad_options() throw() { }

  virtual const char * what() const throw() {
    return (std::string("Bad program options: ") + _msg).c_str();
  }
};

//
// see http://stackoverflow.com/a/8822627/1694896
// custom validation for types in boost::program_options
//
struct valtype
{
  valtype(std::string const& val):
    value(val)
  { }
  std::string value;
};

void validate(boost::any& v, std::vector<std::string> const& values,
              valtype* /* target_type */,
              int)
{
  using namespace boost::program_options;

  // Make sure no previous assignment to 'v' was made.
  validators::check_first_occurrence(v);

  // Extract the first string from 'values'. If there is more than
  // one string, it's an error, and exception will be thrown.
  std::string const& s = validators::get_single_string(values);

  if (s == "fidelity" || s == "tr-dist") {
    v = boost::any(valtype(s));
  } else {
    throw validation_error(validation_error::invalid_option_value);
  }
}

inline std::ostream & operator<<(std::ostream & str, const valtype & val)
{
  return str << val.value;
}

void parse_options(ProgOptions * opt, int argc, char **argv)
{
  // read the options
  using namespace boost::program_options;

  std::string flogname;
  bool flogname_from_config_file_name = false;

  std::string valhiststr;

  std::string configfname;
  std::string configdir;
  std::string configbasename;
  bool write_histogram_from_config_file_name = false;

  valtype val_type(opt->val_type);

  options_description desc("Options");
  desc.add_options()
    ("help", "Print Help Message")
    ("data-file-name", value<std::string>(& opt->data_file_name)->default_value("testfile.mat"),
     "specify .mat data file name")
    ("value-type", value<valtype>(& val_type)->default_value(val_type),
     "Which value to acquire histogram of, e.g. fidelity to MLE. Possible values are 'fidelity'"
     " or 'tr-dist'")
    ("value-hist", value<std::string>(&valhiststr),
     "Do a histogram for different measured values (e.g. fidelity to MLE), format MIN:MAX/NPOINTS")
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
    ("log-from-config-file-name", bool_switch(& flogname_from_config_file_name)->default_value(false),
     "Same as --log=tomorun-<config-file>.log, where <config-file> is the file name passed to the"
     "option --config. This option can only be used in conjunction with --config and may not be used"
     "with --log.")
    ("config", value<std::string>(),
     "Read options from the given file. Use lines with syntax \"key=value\".")
    ("write-histogram-from-config-file-name",
     bool_switch(&write_histogram_from_config_file_name)->default_value(false),
     "Same as --write-histogram=tomorun-<config-file>, where <config-file> is the file name passed to "
     "the option --config. This option can only be used in conjunction with --config and may not "
     "be used with --write-histogram.")
    ;

  try {
    variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);

    if (vm.count("help")) {
      std::cout << desc;
      ::exit(1);
    }

    // see http://www.boost.org/doc/libs/1_57_0/doc/html/program_options/howto.html#idp343356848
    if (vm.count("config")) {
      // load the file, and include options from that file
      configfname = vm["config"].as<std::string>();
      logger.info("parse_options()", "Loading options from file %s\n", configfname.c_str());
      // this will not overwrite options already given on the command line
      store(parse_config_file<char>(configfname.c_str(), desc), vm);

      // for options --write-histogram-from-config-file-name and --log-from-config-file-name:
      char *tmpbuf = new char[configfname.size()+1];
      strcpy(tmpbuf, configfname.c_str());
      configdir = std::string(dirname(tmpbuf)); // deep copy result to a std::string()
      strcpy(tmpbuf, configfname.c_str()); // dirname/basename may modify their argument.
      configbasename = std::string(basename(tmpbuf)); // deep copy result to a std::string()
      // clean up
      delete[] tmpbuf;

      /** \todo In a future version, maybe we will support running config files in
       * different directories. But for now, since a config file may refer to e.g. a data
       * file with a relative path, we'll just require that the config file be in the same
       * dir. makes things much easier, reduces the number of possible bugs and unexpected
       * behavior risks. Same thing for log file and histogram write file.
       */
      if (configdir != ".") {
	throw bad_options(streamstr("Config file must reside in current working directory: " << configfname));
      }

      // debug
      //logger.debug("parse_options()", [=](std::ostream& s) { s << "configdir=" << configdir; });
      //logger.debug("parse_options()", [=](std::ostream& s) { s << "configbasename=" << configbasename; });
    }

    notify(vm);
  } catch (const bad_options&) {
    throw;
  } catch (const std::exception& e) {
    throw bad_options(streamstr("Error parsing program options: " << e.what()));
  }

  // set up logging.
  // maybe set up log file name from config file name
  if (flogname_from_config_file_name) {
    if (!configfname.size()) {
      throw bad_options("--log-from-config-file-name may only be used with --config");
    }
    if (flogname.size()) {
      throw bad_options("--log-from-config-file-name may not be used with --log");
    }
    flogname = configdir + "/" + std::string("tomorun-") + configbasename + std::string(".log");
  }
  // prepare log file, and maybe write out header
  if (flogname.size() == 0 || flogname == "-") {
    opt->flog = stdout;
  } else {
    opt->flog = fopen(flogname.c_str(), "a");
    if (opt->flog == NULL) {
      throw bad_options(streamstr("Can't open file "<<flogname<<" for logging: " << strerror(errno)));
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

  opt->val_type = val_type.value;

  // set up write histogram file name from config file name
  if (write_histogram_from_config_file_name) {
    if (!configfname.size()) {
      throw bad_options("--write-histogram-from-config-file-name may only be used with --config");
    }
    if (opt->write_histogram.size()) {
      throw bad_options("--write-histogram-from-config-file-name may not be used with --write-histogram");
    }
    // "--histogram.csv" is appended later anyway
    opt->write_histogram = configdir + "/" + std::string("tomorun-") + configbasename;
  }

  // set up value histogram parameters
  if (valhiststr.size()) {
    double fmin, fmax;
    int nbins = 100;
    if (std::sscanf(valhiststr.c_str(), "%lf:%lf/%d", &fmin, &fmax, &nbins) < 2) {
      throw bad_options("--value-hist expects an argument of format MIN:MAX[/NPOINTS]");
    }

    opt->val_min = fmin;
    opt->val_max = fmax;
    opt->val_nbins = nbins;
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
      "       value type         = %s\n"
      "       val. histogram     = [%.2g, %.2g] (%lu bins)\n"
      "       step size          = %.6f\n"
      "       sweep size         = %lu\n"
      "       # therm sweeps     = %lu\n"
      "       # run sweeps       = %lu\n"
      "       # intgr. repeats   = %lu   (chunked by %lu/thread)\n"
      "       write histogram to = %s\n"
      "\n"
      "       --> total no. of live samples = %lu  (%.2e)\n"
      "\n",
      opt->data_file_name.c_str(), opt->NMeasAmplifyFactor,
      opt->val_type.c_str(),
      opt->val_min, opt->val_max, (unsigned long)opt->val_nbins,
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

template<int FixedDim, int FixedMaxPOVMEffects, typename Logger>
inline void tomorun_dispatch(unsigned int dim, ProgOptions * opt, Tomographer::MAT::File * matf, Logger & logger);


int main(int argc, char **argv)
{
  ProgOptions opt;

  try {
    parse_options(&opt, argc, argv);
  } catch (const bad_options& e) {
    fprintf(stderr, "%s\n", e.what());
    return 127;
  }

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

  auto delete_matf = Tools::finally([matf] {
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
      tomorun_dispatch<2, 6>(dim, &opt, matf, mlog);
      /*    } else if (dim == 2 && n_povms <= 64) {
	    tomorun<2, 64>(dim, &opt, matf, mlog); */
      /*    } else if (dim == 2) {
	    tomorun<2, Eigen::Dynamic>(dim, &opt, matf, mlog); */
      /*    } else if (dim == 4 && n_povms <= 64) {
	    tomorun<4, 64>(dim, &opt, matf, mlog); */
      /*    } else if (dim == 4) {
	    tomorun<4, Eigen::Dynamic>(dim, &opt, matf, mlog); */
    } else {
      tomorun_dispatch<Eigen::Dynamic, Eigen::Dynamic>(dim, &opt, matf, mlog);
    }
    //  } catch (const std::exception& e) {
    //    logger.error("main()", "Caught exception: %s", e.what());
    //    return 2;
    //  }
}


template<typename ValueCalculator, typename OurTomoProblem, typename Logger>
inline void tomorun(OurTomoProblem & tomodat, ProgOptions * opt, Logger & logger);


template<int FixedDim, int FixedMaxPOVMEffects, typename Logger>
inline void tomorun_dispatch(unsigned int dim, ProgOptions * opt, Tomographer::MAT::File * matf, Logger & logger)
{
  logger.debug("tomorun_dispatch()", "Running tomography program! FixedDim=%d and FixedMaxPOVMEffects=%d",
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

  typename Tools::eigen_std_vector<typename OurMatrQ::MatrixType>::type Emn;
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
  logger.debug("tomorun_dispatch()", "Npovmeffects=%d", Npovmeffects);
  tomodat.Exn.resize(Npovmeffects, dim*dim);
  tomodat.Nx.resize(Npovmeffects);
  j = 0;
  for (k = 0; k < Emn.size(); ++k) {
    if (Nm[k] == 0) {
      continue;
    }
    // do some tests: positive semidefinite and non-zero
#ifdef DO_SLOW_POVM_CONSISTENCY_CHECKS
    logger.longdebug("tomorun_dispatch()", [&](std::ostream & str) {
                       str << "Emn["<<k<<"] = \n" << Emn[k] << "\n"
                           << "\tEV=" << Emn[k].eigenvalues().transpose().real() << "\n"
                           << "\tnorm=" << double(Emn[k].norm()) << "\n" ;
                     });
    eigen_assert(double( (Emn[k] - Emn[k].adjoint()).norm() ) < 1e-8); // Hermitian
    eigen_assert(double(Emn[k].eigenvalues().real().minCoeff()) >= -1e-12); // Pos semidef
    eigen_assert(double(Emn[k].norm()) > 1e-6);
    logger.debug("tomorun_dispatch()", "Consistency checks passed for Emn[%u].", k);
#endif

    // don't need to reset `row` to zero, param_herm_to_x doesn't require it
    auto Exn_row_as_col = tomodat.Exn.row(j).transpose();
    param_herm_to_x(Exn_row_as_col, Emn[k]);
    tomodat.Nx[j] = Nm[k];
    ++j;
  }
  // done.

  logger.debug("tomorun_dispatch()", [&](std::ostream & ss) {
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
  // Data has now been successfully read. Now, dispatch to the correct template function
  // for futher processing.
  //

  if (opt->val_type == "fidelity") {
    tomorun<FidelityToRefCalculator<OurTomoProblem, double> >(tomodat, opt, logger);
  } else if (opt->val_type == "tr-dist") {
    tomorun<TrDistToRefCalculator<OurTomoProblem, double> >(tomodat, opt, logger);
  } else {
    throw std::logic_error((std::string("Unknown value type: ")+opt->val_type).c_str());
  }
}




//
// Here goes the actual program. This is templated, because then we can let Eigen use
// allocation on the stack rather than malloc'ing 2x2 matrices...
//
template<typename ValueCalculator, typename OurTomoProblem, typename Logger>
inline void tomorun(OurTomoProblem & tomodat, ProgOptions * opt, Logger & logger)
{
  //
  // create the OMP Task Manager and run.
  //

  typedef DMLLHIntegratorTasks::CData<OurTomoProblem, ValueCalculator> OurCData;
  typedef MultiProc::OMPTaskLogger<Logger> OurTaskLogger;
  typedef DMLLHIntegratorTasks::MHRandomWalkTask<OurTomoProblem,ValueCalculator,OurTaskLogger>
    OurMHRandomWalkTask;
  typedef DMLLHIntegratorTasks::MHRandomWalkResultsCollector<
      typename OurMHRandomWalkTask::ValueHistogramMHRWStatsCollectorType::HistogramType
      >
    OurResultsCollector;

  OurCData taskcdat(tomodat);
  // seed for random number generator
  taskcdat.base_seed = std::chrono::system_clock::now().time_since_epoch().count();
  // parameters for the value histogram
  taskcdat.histogram_params = typename OurCData::HistogramParams(opt->val_min, opt->val_max, opt->val_nbins);
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
  auto srep = Tools::makeSigHandlerTaskDispatcherStatusReporter(&tasks, logger);
  Tools::installSignalStatusReportHandler(SIGINT, &srep);
  //  auto srep = makeSigHandlerStatusReporter(&tasks, logger);
  //  signal_handler = &srep;
  //  signal(SIGINT, sig_int_handler);

  // and run our tomo process

  auto time_start = TimerClock::now();

  srep.time_start = time_start;

  tasks.run();

  auto time_end = TimerClock::now();

  logger.debug("tomorun()", "Random walks done.");

  // delta-time, in seconds and fraction of seconds
  std::string elapsed_s = Tools::fmt_duration(time_end - time_start);

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
    outf << "Value\tAvgCounts\tError\n"
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
