/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
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


#ifndef TOMORUN_OPTS
#define TOMORUN_OPTS

#include <stdexcept>

#include <tomographer/tomographer_version.h>
#include <tomographer/tools/ezmatio.h>


static const std::string prog_version_info_1 =
  "Tomographer/Tomorun " TOMOGRAPHER_VERSION "\n"
  ;

static const std::string prog_version_info_2 =
  "by Philippe Faist, Institute for Theoretical Physics, ETH Zurich\n"
  "Copyright (c) 2015 ETH Zurich\n"
  "Released under the terms of the MIT License (see LICENSE.txt)\n";

static const std::string prog_version_info = prog_version_info_1 + prog_version_info_2;


static std::string prog_version_info_features()
{
  static std::string features_str = std::string();

  if (features_str.size() == 0) {
    // detect the program features. 

    // MatIO
    int major, minor, release;
    Mat_GetLibraryVersion(&major,&minor,&release);
    features_str += Tomographer::Tools::fmts("MatIO %d.%d.%d\n", major, minor, release);

    // OpenMP
    features_str +=
#ifdef TOMOGRAPHER_HAVE_OMP
      "+OpenMP [Tomorun was compiled with OpenMP support.]\n"
#else
      "-OpenMP [Tomorun was compiled without OpenMP support.]\n"
#endif
      ;

    // Eigen
    features_str += Tomographer::Tools::fmts("Eigen %d.%d.%d (SIMD: %s)\n", EIGEN_WORLD_VERSION,
					     EIGEN_MAJOR_VERSION, EIGEN_MINOR_VERSION,
					     Eigen::SimdInstructionSetsInUse());
  }

  return features_str;
}




static const int last_binning_level_warn_min_samples = 128;



//
// see http://stackoverflow.com/a/8822627/1694896
// custom validation for types in boost::program_options
//
struct val_type_spec
{
  val_type_spec(const std::string & str)
    : valtype(INVALID),
      ref_obj_name()
  {
    set_value_string(str);
  }

  enum ValueType {
    INVALID = 0,
    OBS_VALUE,
    TR_DIST,
    FIDELITY,
    PURIF_DIST
  };


  ValueType valtype;

  std::string ref_obj_name;

  inline void set_value_string(const std::string & str)
  {
    std::size_t k = str.find(':');
    std::string valtype_str;
    std::string ref_obj_name_str;
    if (k == std::string::npos) {
      // not found
      valtype_str = str;
    } else {
      valtype_str = str.substr(0, k);
      ref_obj_name_str = str.substr(k+1); // the rest of the string
    }

    // validate the given value
    if (valtype_str == "obs-value") {
      valtype = OBS_VALUE;
      ref_obj_name = ref_obj_name_str;
      return;
    } else if (valtype_str == "tr-dist") {
      valtype = TR_DIST;
      ref_obj_name = ref_obj_name_str;
      return;
    } else if (valtype_str == "fidelity") {
      valtype = FIDELITY;
      ref_obj_name = ref_obj_name_str;
      return;
    } else if (valtype_str == "purif-dist") {
      valtype = PURIF_DIST;
      ref_obj_name = ref_obj_name_str;
      return;
    } else {
      throw std::invalid_argument(std::string("Invalid argument to val_type_spec: '") + str + std::string("'"));
    }
  }
};

inline std::ostream & operator<<(std::ostream & str, const val_type_spec & val)
{
  switch (val.valtype) {
  case val_type_spec::OBS_VALUE:
    str << "obs-value";
    break;
  case val_type_spec::TR_DIST:
    str << "tr-dist";
    break;
  case val_type_spec::FIDELITY:
    str << "fidelity";
    break;
  case val_type_spec::PURIF_DIST:
    str << "purif-dist";
    break;
  case val_type_spec::INVALID:
    str << "<invalid>";
    break;
  default:
    str << "<unknown/invalid>";
    break;
  }

  if (val.ref_obj_name.size()) {
    str << ":" << val.ref_obj_name;
  }

  return str;
}

void validate(boost::any& v, const std::vector<std::string> & values,
              val_type_spec * /* target_type */,
              int)
{
  using namespace boost::program_options;

  // Make sure no previous assignment to 'v' was made.
  validators::check_first_occurrence(v);

  // Extract the first string from 'values'. If there is more than
  // one string, it's an error, and exception will be thrown.
  const std::string & s = validators::get_single_string(values);

  try {
    v = boost::any(val_type_spec(s));
  } catch (const std::invalid_argument & exc) {
    throw validation_error(validation_error::invalid_option_value);
  }
}


// ------------------------------------------------------------------------------


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
    valtype("fidelity"),
    val_min(0.97),
    val_max(1.0),
    val_nbins(50),
    binning_analysis_error_bars(true), // error bars from binning analysis
    binning_analysis_num_levels(8),
    start_seed(std::chrono::system_clock::now().time_since_epoch().count()),
    Nrepeats(256),
    Nchunk(1),
    NMeasAmplifyFactor(1.0),
    loglevel(Tomographer::Logger::INFO),
    verbose_log_info(false), // display origins in log messages
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

  val_type_spec valtype;

  double val_min;
  double val_max;
  std::size_t val_nbins;

  bool binning_analysis_error_bars;
  int binning_analysis_num_levels;

  int start_seed;

  unsigned int Nrepeats;
  unsigned int Nchunk;

  double NMeasAmplifyFactor;

  Tomographer::Logger::LogLevel loglevel;
  bool verbose_log_info;

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





// -----------------------------------------------------------------------------




template<typename LoggerType>
void parse_options(ProgOptions * opt, int argc, char **argv, LoggerType & baselogger)
{
  // read the options
  using namespace boost::program_options;

  Tomographer::Logger::LocalLogger<LoggerType> logger("parse_options()", baselogger);

  std::string flogname;
  bool flogname_from_config_file_name = false;

  std::string valhiststr;

  std::string configfname;
  std::string configdir;
  std::string configbasename;
  bool write_histogram_from_config_file_name = false;

  bool no_binning_analysis_error_bars = ! opt->binning_analysis_error_bars;

  const int line_width = 80;
  options_description desc("OPTIONS", line_width, line_width*2/3);
  desc.add_options()
    ("data-file-name", value<std::string>(& opt->data_file_name),
     "specify MATLAB (.mat) file to read data from")
    ("value-type", value<val_type_spec>(& opt->valtype)->default_value(opt->valtype),
     "Which value to acquire histogram of, e.g. fidelity to MLE. Possible values are 'fidelity', "
     "'purif-dist', 'tr-dist' or 'obs-value'. The value type may be followed by ':ObjName' to refer "
     "to a particular object defined in the datafile. See below for more info.")
    ("value-hist", value<std::string>(&valhiststr),
     "Do a histogram of the figure of merit for different measured values. Format MIN:MAX/NUM_BINS")
    ("no-binning-analysis-error-bars", bool_switch(& no_binning_analysis_error_bars)
     ->default_value(no_binning_analysis_error_bars),
     // REFERENCE [2]
     "Don't produce error bars from a binning analysis [2] for each histogram bin")
    ("binning-analysis-num-levels", value<int>(& opt->binning_analysis_num_levels)
     ->default_value(opt->binning_analysis_num_levels),
     ("Number of levels of coarse-graining in the binning analysis. See --binning-analysis-error-bars. "
      "Choose this number such that (n-run)/(2^(<binning-num-levels>)) is a sufficiently decent sample size "
      "(say ~"+std::to_string(last_binning_level_warn_min_samples)+").").c_str())
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
    ("verbose", value<Tomographer::Logger::LogLevel>(& opt->loglevel)->default_value(opt->loglevel)
     ->implicit_value(Tomographer::Logger::DEBUG),
     "print verbose information. Not very readable unless n-repeats=1. You may also specify "
     "as argument 'longdebug', 'debug', 'info', 'warning' or 'error', or a numerical verbosity "
     "level 0-4.")
    ("verbose-log-info", bool_switch(& opt->verbose_log_info)->default_value(opt->verbose_log_info),
     "[For Developers.] If specified, log messages are more verbose; they display e.g. at which point "
     "in the code they were emitted.")
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
    ("version", "Print Tomographer/Tomorun version information as well as information about enabled features.")
    ("help", "Print Help Message")
    ;

  // no positional options accepted
  positional_options_description p;

  try {
    variables_map vm;
    store(command_line_parser(argc, argv).options(desc).positional(p).run(), vm);

    if (vm.count("help")) {
      std::cout
	<< "\n" << prog_version_info <<
	"\n"
	"A toolbox for error analysis in quantum tomography.\n"
	"\n"
	"Usage: tomorun --data-file-name=<data-file-name> [options]\n"
	"       tomorun --config=<tomorun-config-file>\n"
	"\n"
//      |--------------------------------------------------------------------------------| 80 chars (col. 89)
	"Produce a histogram of a figure of merit during a random walk in quantum state\n"
	// REFERENCE [1]
	"space according to the distribution \\mu_{B^n}(.) defined in Ref. [1]. The\n"
	"likelihood function is specified with independent POVM effects (see below).\n"
	"\n"
	"Input data is given as a MATLAB file (--data-file-name). See below for exact\n"
	"format. Options may be specified in a separate file and referred to (option\n"
	"--config).\n"
	"\n"
	"Hit CTRL-C while `tomorun` is running to inquire about progress information.\n"
	"\n"
	<< desc	<<
	"\n"
	"DATA FILE CONTENTS:\n"
	"The data file must contain the following MATLAB variables:\n"
	"\n"
	"    - dim\n"
	"      An integer scalar: the dimension of the quantum system\n"
	"\n"
	"    - Emn\n"
	"      A list of all the POVM effects. This is a complex matrix of shape\n"
	"      (dim,dim,K) where dim is the dimension of the system and K the total\n"
	"      number of POVM effects.\n"
	"\n"
	"    - Nm\n"
	"      A list of (integer) frequencies. Nm(k) is the number of times the POVM\n"
	"      effect Emn(:,:,k) was observed.\n"
	"\n"
	"    - rho_MLE\n"
	"      (Required now, but in the future might not be required.) The maximum\n"
	"      likelihood estimate corresponding to the given data. Used mostly as the\n"
	"      default reference state if none other is specified for some figures of\n"
	"      merit.\n"
	"\n"
	"    - <any other variable name>\n"
	"      The MATLAB data file may contain further variables for use in some\n"
	"      figures of merit. See below.\n"
	"\n"
	"Note: if the MatIO library was compiled without HDF5/MATLAB-7.3 file format\n"
	"support, you must save your MATLAB data files in MATLAB v6 file format, e.g.:\n"
	"\n"
	"    (Matlab)>> save('datafile.mat', ..., '-v6')\n"
	"\n"
	"OUTPUT HISTOGRAM:\n"
	"The histogram data is output to a text file in tab-separated values format with\n"
	"a single-line header. There are three or four columns, depending on whether a\n"
	"binning analysis is performed. Each row corresponds to a histogram bin. The\n"
	"columns are:\n"
	"\n"
	"    - The first column is the X-axis value, given as the *left edge* of the bin.\n"
	"      For example, if the range [0, 1[ is divided into 10 bins, then the first\n"
	"      column will display the values 0.0, 0.1, 0.2, ..., 0.9; the first bin\n"
	"      covers samples in the range [0.0, 0.1[, the second collects samples in the\n"
	"      range [0.1, 0.2[, and so on.\n"
	"\n"
	"    - The second column gives the average counts in the histogram bin. The value\n"
	"      here is the average of the characteristic function \"is the point in this\n"
	"      bin\" over the samples of the random walk.\n"
	"\n"
	"    - The third column gives an error bar on the figure in the second column. If\n"
	"      binning analysis is enabled, then the third column is the corresponding\n"
	"      error bar obtained by combining error bars from the binning analyses of\n"
	"      each random walk. If binning analysis is disabled, this column is the\n"
	"      statistical standard deviation of the results of the different random\n"
	"      walks (make sure to have enough independent runs for this figure to make\n"
	"      sense in this case).\n"
	"\n"
	"    - If binning analysis is enabled, then the fourth column is the statistical\n"
	"      standard deviation of the results of the different random walks,\n"
	"      regardless of error bars from the binning analysis (this figure might be\n"
	"      irrelevant or misleading if too few independent random walks are\n"
	"      instanciated). There is no fourth column if binning analysis is disabled.\n"
	"\n"
	"FIGURES OF MERIT:\n"
	"The argument to the option --value-type should be specified as \"keyword\" or\n"
	"\"keyword:<RefObject>\". <RefObject> should be the name of a MATLAB variable\n"
	"present in the data file provided to --data-file-name. The possible keywords and\n"
	"corresponding possible reference variables are:\n"
	"\n"
	"    - \"obs-value\": the expectation value of an observable. <RefObject> should\n"
	"      be the name of a MATLAB variable present in the MATLAB data file. This\n"
	"      object should be a complex dim x dim matrix which represents the\n"
	"      observable in question. If no <RefObject> is specified, the variable named\n"
	"      \"Observable\" is looked up in the data file.\n"
	"\n"
	"    - \"tr-dist\": the trace distance to a reference state. <RefObject> should\n"
	"      be the name of a MATLAB variable present in the MATLAB data file. This\n"
	"      object should be a complex dim x dim matrix, the density matrix of the\n"
	"      reference state. If no <RefObject> is specified, then rho_MLE is used.\n"
	"\n"
	// REFERENCE [3]
	"    - \"fidelity\": the (root) fidelity to a reference state [3]. <RefObject>\n"
	"      should be the name of a MATLAB variable present in the MATLAB data file.\n"
	"      This object should be a complex dim x dim matrix, the density matrix of\n"
	"      the reference state. If no <RefObject> is specified, then rho_MLE is used.\n"
	"\n"
	// REFERENCE [4]
	"    - \"purif-dist\": the purified distance to a reference state [4].\n"
	"      <RefObject> should be the name of a MATLAB variable present in the MATLAB\n"
	"      data file. This object should be a complex dim x dim matrix, the density\n"
	"      matrix of the reference state. If no <RefObject> is specified, then\n"
	"      rho_MLE is used.\n"
	"\n"
	"Note: For the (squared) fidelity to a pure state (usually preferred in\n"
	"experimental papers), you should use \"obs-value\" with the observable being\n"
	// REFERENCE [5]
	"the density matrix of the reference state [5].\n"
	"\n"
	"REFERENCES:\n"
	" [1] Christandl and Renner, Phys. Rev. Lett. 12:120403 (2012), arXiv:1108.5329\n"
	" [2] Ambegaokar and Troyer, Am. J. Phys., 78(2):150 (2010), arXiv:0906.0943\n"
	" [3] The root fidelity is defined as F(rho,sigma)=|| rho^{1/2} sigma^{1/2} ||_1,\n"
	"     as in Nielsen and Chuang, \"Quantum Computation and Quantum Information\".\n"
	" [4] The purified distance, also called \"infidelity\" in the literature, is\n"
	"     defined as P(rho,sigma) = \\sqrt{1 - F^2(rho,sigma)}.\n"
	" [5] Indeed, for pure rho_ref, F^2(rho,rho_ref) = tr(rho*rho_ref).\n"
	"\n"
	"CITATION:\n"
	"If you use this program in your research, we strongly encourage you to cite the\n"
	"following works:\n"
	"\n"
	"  1. Philippe Faist and Renato Renner. Practical, Reliable Error Bars in Quantum\n"
	"     Tomography (2015). arXiv:XXXX.XXXXX\n"
	"\n"
	"  2. Philippe Faist. The Tomographer Project. Available at\n"
	"     https://github.com/Tomographer/tomographer/.\n"
	"\n"
	"FEEDBACK:\n"
	"Please report issues, wishlists and bugs by following instructions at:\n"
	"\n"
	"    https://github.com/Tomographer/tomographer/\n"
	"\n"
	"Have a lot of fun!\n"
	"\n"
	;

      ::exit(1);
    }

    if (vm.count("version")) {
      std::cout << prog_version_info
		<< "----\n"
		<< prog_version_info_features();
      ::exit(2);
    }

    // see http://www.boost.org/doc/libs/1_57_0/doc/html/program_options/howto.html#idp343356848
    if (vm.count("config")) {
      // load the file, and include options from that file
      configfname = vm["config"].as<std::string>();
      // avoid log messages at this point before having parsed --verbose. We'll issue this
      // message later.
      //logger.info("Loading options from file %s\n", configfname.c_str());

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
	throw bad_options(streamstr("Sorry, config file must reside in current working directory: "
				    << configfname));
      }
    }

    notify(vm);
  } catch (const bad_options&) {
    throw;
  } catch (const std::exception& e) {
    throw bad_options(streamstr("Error parsing program options: " << e.what()));
  }

  // First thing: set up logging, so that we can issue log messages.
  // --------------------

  // set up level and verbosity
  baselogger.setLevel(opt->loglevel);
  baselogger.setDisplayOrigin(opt->verbose_log_info);
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

    baselogger.setFp(opt->flog);
    logger.info("Output is now being redirected to %s.", flogname.c_str());
  }

  // issue any delayed log messages
  // --------------------

  if (configfname.size()) {
    logger.debug("Options were loaded from file %s\n", configfname.c_str());
  }

  // Further Settings
  // --------------------

  // set up the "false-type" boolean switche(s)

  opt->binning_analysis_error_bars = ! no_binning_analysis_error_bars;


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


  // make sure we have a data file
  if (!opt->data_file_name.size()) {
    logger.error("No data file specified. Please specify a MATLAB file with --data-file-name.");
    ::exit(3);
  }

  // set up value histogram parameters
  if (valhiststr.size()) {
    double fmin, fmax;
    int nbins = 100;
    if (std::sscanf(valhiststr.c_str(), "%lf:%lf/%d", &fmin, &fmax, &nbins) < 2) {
      throw bad_options("--value-hist expects an argument of format MIN:MAX[/NUM_BINS]");
    }

    opt->val_min = fmin;
    opt->val_max = fmax;
    opt->val_nbins = nbins;
    logger.debug("Histogram parameters parsed: min=%g, max=%g, num_bins=%d",
		 (double)opt->val_min, (double)opt->val_max, (int)opt->val_nbins);
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
      "Using  data from file :     %s  (measurements x%.3g)\n"
      "       value type :         %s\n"
      "       val. histogram :     [%.2g, %.2g] (%lu bins)\n"
      "       error bars :         %s\n"
      "       step size :          %.6f\n"
      "       sweep size :         %lu\n"
      "       # therm sweeps :     %lu\n"
      "       # run sweeps :       %lu\n"
      "       # intgr. repeats :   %lu   (chunked by %lu/thread)\n"
      "       write histogram to : %s\n"
      "\n"
      "       --> total no. of live samples = %lu  (%.2e)\n"
      "\n",
      opt->data_file_name.c_str(), opt->NMeasAmplifyFactor,
      streamcstr(opt->valtype),
      opt->val_min, opt->val_max, (unsigned long)opt->val_nbins,
      (opt->binning_analysis_error_bars
       ? Tomographer::Tools::fmts("binning analysis (%d levels)", opt->binning_analysis_num_levels).c_str()
       : "std. dev. of runs"),
      opt->step_size,
      (unsigned long)opt->Nsweep,
      (unsigned long)opt->Ntherm,
      (unsigned long)opt->Nrun,
      (unsigned long)opt->Nrepeats,
      (unsigned long)opt->Nchunk,
      (opt->write_histogram.size()
       ? opt->write_histogram + std::string("-histogram.csv")
       : std::string("<don't write histogram>")).c_str(),
      (unsigned long)(opt->Nrun*opt->Nrepeats),
      (double)(opt->Nrun*opt->Nrepeats)
      );
}










#endif
