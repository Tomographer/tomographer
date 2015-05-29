#ifndef TOMORUN_OPTS
#define TOMORUN_OPTS



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
    use_ref_state(false), // use MLE by default
    val_min(0.97),
    val_max(1.0),
    val_nbins(50),
    start_seed(std::chrono::system_clock::now().time_since_epoch().count()),
    Nrepeats(256),
    Nchunk(1),
    NMeasAmplifyFactor(1.0),
    loglevel(Tomographer::Logger::INFO),
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
  //! If \c true, use \a rho_ref; if \c false, use \a rho_MLE .
  bool use_ref_state;

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

class invalid_input : public std::exception
{
  std::string _msg;
public:
  invalid_input(const std::string& msg) : _msg(msg) { }
  virtual ~invalid_input() throw() { }

  virtual const char * what() const throw() {
    return (std::string("Invalid Input: ") + _msg).c_str();
  }
};

#define ensure_valid_input(condition, msg)	\
  do { if (!(condition)) {			\
    throw invalid_input(msg);	\
  } } while (0)



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

  if (s == "fidelity" || s == "purif-dist" || s == "tr-dist" || s == "obs-value") {
    v = boost::any(valtype(s));
  } else {
    throw validation_error(validation_error::invalid_option_value);
  }
}

inline std::ostream & operator<<(std::ostream & str, const valtype & val)
{
  return str << val.value;
}



// -----------------------------------------------------------------------------


template<typename LoggerType>
void parse_options(ProgOptions * opt, int argc, char **argv, LoggerType & logger)
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
     "Which value to acquire histogram of, e.g. fidelity to MLE. Possible values are 'fidelity', "
     "'purif-dist', 'tr-dist' or 'obs-value'. If 'obs-value' (hermitian observable value)  is "
     "specified, the given \".mat\" data-file (see --data-file-name) must also define a variable "
     "Observable as a dim x dim hermitian matrix.")
    ("use-ref-state", bool_switch(& opt->use_ref_state)->default_value(opt->use_ref_state),
     "If --value-type is 'fidelity', 'purif-dist' or 'tr-dist', then read the variable named 'rho_ref' "
     "in the data file (--data-file-name) and calculate the histogram of fidelity or trace distance to "
     "that state, instead of the MLE estimate (as by default).")
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
    ("verbose", value<int>(& opt->loglevel)->default_value(opt->loglevel)->implicit_value(Tomographer::Logger::DEBUG),
     "print iteration info. Not very readable unless n-repeats=1. You may also specify "
     "a specific verbosity level (integer); the higher the more verbose.")
    ("nice", value<int>(& opt->nice_level)->default_value(opt->nice_level),
     "Renice the process to the given level to avoid slowing down the whole system. Set to zero "
     "to avoid renicing.")
    ("log", value<std::string>(& flogname),
     "Redirect standard output (log) to the given file. Use '-' for stdout. If file exists, will append.")
    ("log-from-config-file-name", bool_switch(& flogname_from_config_file_name)->default_value(false),
     "Same as --log=<config-file>.log, where <config-file> is the file name passed to the"
     "option --config. This option can only be used in conjunction with --config and may not be used"
     "with --log.")
    ("config", value<std::string>(),
     "Read options from the given file. Use lines with syntax \"key=value\".")
    ("write-histogram-from-config-file-name",
     bool_switch(&write_histogram_from_config_file_name)->default_value(false),
     "Same as --write-histogram=<config-file>, where <config-file> is the file name passed to "
     "the option --config. This option can only be used in conjunction with --config and may not "
     "be used with --write-histogram.")
    ;

  // no positional options accepted
  positional_options_description p;

  try {
    variables_map vm;
    store(command_line_parser(argc, argv).options(desc).positional(p).run(), vm);

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
    flogname = configdir + "/" + configbasename + std::string(".log");
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
    opt->write_histogram = configdir + "/" + configbasename;
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



template<typename LoggerType>
void display_parameters(ProgOptions * opt, LoggerType & logger)
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










#endif
