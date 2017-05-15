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


#ifndef TOMORUN_OPTS
#define TOMORUN_OPTS

#include <stdexcept>

#include <boost/version.hpp>

#include <tomographer/tomographer_version.h>
#include <tomographer/tools/ezmatio.h>


static const std::string prog_version_info_1 =
  "Tomographer/Tomorun " TOMOGRAPHER_VERSION "\n"
  ;

static const std::string prog_version_info_2 =
  "by Philippe Faist, Institute for Quantum Information and Matter, Caltech\n"
  "Copyright (c) 2016 ETH Zurich, Institute for Theoretical Physics, Philippe Faist\n"
  "Copyright (c) 2017 Caltech, Institute for Quantum Information and Matter, Philippe Faist\n"
  "Released under the terms of the MIT License (see LICENSE.txt)\n";

static const std::string prog_version_info = prog_version_info_1 + prog_version_info_2;


static std::string prog_version_info_features()
{
  std::string features_str = std::string();

  // detect the program features. 

  // Eigen
  features_str += Tomographer::Tools::fmts("Eigen %d.%d.%d (SIMD: %s)\n", EIGEN_WORLD_VERSION,
                                           EIGEN_MAJOR_VERSION, EIGEN_MINOR_VERSION,
                                           Eigen::SimdInstructionSetsInUse());

  // Boost
  features_str += std::string("Boost headers ") + std::string(BOOST_LIB_VERSION) + std::string("\n");

  // Parallelization
  features_str += "MultiProc: " TomorunMultiProcTaskDispatcherTitle "\n";
  // --replaces:
  // OpenMP
  //     features_str +=
  // #ifdef _OPENMP
  //       "+OpenMP [Tomorun was compiled with OpenMP support.]\n"
  // #else
  //       "-OpenMP [Tomorun was compiled without OpenMP support.]\n"
  // #endif
  ;

  // MatIO
  int major, minor, release;
  Mat_GetLibraryVersion(&major,&minor,&release);
  features_str += Tomographer::Tools::fmts("MatIO %d.%d.%d\n", major, minor, release);

  //
  // Compiler
  //
  features_str += std::string("Compiler: ") + TOMOGRAPHER_COMPILER_INFO_STR + "\n";

  //
  // features: assertions, fixed TOMORUN_* sizes etc., ...
  //
  std::vector<std::string> featconfig;
  // assertions
#ifdef EIGEN_NO_DEBUG
  featconfig.push_back("no assertions");
#else
  featconfig.push_back("with assertions");
#endif
  // tomorun-specifics
#define IDENT_TO_STRING(x) #x
#ifdef TOMORUN_INT
  featconfig.push_back(Tomographer::Tools::fmts("int_type=%s", IDENT_TO_STRING(TOMORUN_INT)));
#endif
#ifdef TOMORUN_REAL
  featconfig.push_back(Tomographer::Tools::fmts("real_type=%s", IDENT_TO_STRING(TOMORUN_REAL)));
#endif
#ifdef TOMORUN_DO_SLOW_POVM_CONSISTENCY_CHECKS
  featconfig.push_back("with povm_consistency_checks");
#endif
#ifdef TOMORUN_CUSTOM_FIXED_DIM
  featconfig.push_back(Tomographer::Tools::fmts("custom_fixed_dim=%d", TOMORUN_CUSTOM_FIXED_DIM));
#endif
#ifdef TOMORUN_CUSTOM_FIXED_MAX_DIM
  featconfig.push_back(Tomographer::Tools::fmts("custom_fixed_max_dim=%d", TOMORUN_CUSTOM_FIXED_DIM));
#endif
#ifdef TOMORUN_CUSTOM_MAX_POVM_EFFECTS
  featconfig.push_back(Tomographer::Tools::fmts("custom_max_povm_effects=%d", TOMORUN_CUSTOM_MAX_POVM_EFFECTS));
#endif
#ifdef TOMORUN_USE_MULTIPLEXORVALUECALCULATOR
  featconfig.push_back("with multiplexor-value-calculator");
#else
  featconfig.push_back("no multiplexor-value-calculator");
#endif
  // join feature items together into a config string
  features_str += "Config: ";
  for (std::size_t k = 0; k < featconfig.size(); ++k) {
    if (k != 0) { features_str += ", "; }
    features_str += featconfig[k];
  }
  features_str += "\n";

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
    //
    // INSERT CUSTOM FIGURE OF MERIT HERE:
    // See instructions in API documentation, page 'Adding a new figure of merit to the tomorun program'
    //
    // <MY_CUSTOM_FIGURE_OF_MERIT_INTERNAL_CODE>
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
    }
    if (valtype_str == "tr-dist") {
      valtype = TR_DIST;
      ref_obj_name = ref_obj_name_str;
      return;
    }
    if (valtype_str == "fidelity") {
      valtype = FIDELITY;
      ref_obj_name = ref_obj_name_str;
      return;
    }
    if (valtype_str == "purif-dist") {
      valtype = PURIF_DIST;
      ref_obj_name = ref_obj_name_str;
      return;
    }
    //
    // INSERT CUSTOM FIGURE OF MERIT HERE:
    // See instructions in API documentation, page 'Adding a new figure of merit to the tomorun program'
    //
    // if (valtype_str == "<my-figure-of-merit-code-in-config-file>") {
    //   valtype = <MY_CUSTOM_FIGURE_OF_MERIT_INTERNAL_CODE>;
    //   ref_obj_name = ref_obj_name_str; // in case there's a reference object name (eg. reference state)
    //   return;
    // }
    //
    
    throw std::invalid_argument(std::string("Invalid argument to val_type_spec: '") + str + std::string("'"));
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

  //
  // INSERT CUSTOM FIGURE OF MERIT HERE:
  // See instructions in API documentation, page 'Adding a new figure of merit to the tomorun program'
  //
  // case val_type_spec::<MY_CUSTOM_FIGURE_OF_MERIT_INTERNAL_CODE>:
  //   str << "<my-figure-of-merit-code-in-config-file>";
  //   break;

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
    Nrun(32768),
    valtype("fidelity"),
    val_min(0.97),
    val_max(1.0),
    val_nbins(50),
    binning_analysis_error_bars(true), // error bars from binning analysis
    binning_analysis_num_levels(8),
    control_step_size(true),
    control_step_size_moving_avg_samples(2048),
    control_binning_converged(true),
    control_binning_converged_max_not_converged(0),
    control_binning_converged_max_unknown(2),
    control_binning_converged_max_unknown_notisolated(0),
    start_seed(std::chrono::system_clock::now().time_since_epoch().count()),
    Nrepeats(12),
    Nchunk(1),
    NMeasAmplifyFactor(1.0),
    loglevel(Tomographer::Logger::INFO),
    verbose_log_info(false), // display origins in log messages
    write_histogram(""),
    periodic_status_report_ms(-1)
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

  bool control_step_size;
  int control_step_size_moving_avg_samples;
  bool control_binning_converged;
  int control_binning_converged_max_not_converged;
  int control_binning_converged_max_unknown;
  int control_binning_converged_max_unknown_notisolated;

  int start_seed;

  unsigned int Nrepeats;
  unsigned int Nchunk;

  double NMeasAmplifyFactor;

  Tomographer::Logger::LogLevel loglevel;
  bool verbose_log_info;

  std::string write_histogram;

  int periodic_status_report_ms;
};




// ------------------------------------------------------------------------------


TOMOGRAPHER_DEFINE_MSG_EXCEPTION(bad_options, "Invalid input: ") ;

TOMOGRAPHER_DEFINE_MSG_EXCEPTION(invalid_input, "Bad program options: ") ;

inline void ensure_valid_input(bool condition, std::string msg) {
  Tomographer::Tools::tomographerEnsure<invalid_input>(condition, msg);
}




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

  bool binning_analysis_error_bars_set = false;
  bool no_binning_analysis_error_bars_set = false;

  bool control_step_size_set = false;
  bool no_control_step_size_set = false;

  bool control_binning_converged_set = false;
  bool no_control_binning_converged_set = false;

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
    ("binning-analysis-error-bars", bool_switch(& binning_analysis_error_bars_set)
     ->default_value(false),
     // REFERENCE [2] is here
     "Produce error bars from a binning analysis [2] for each histogram bin. This option is enabled "
     "by default.")
    ("no-binning-analysis-error-bars", bool_switch(& no_binning_analysis_error_bars_set)
     ->default_value(false),
     "Don't produce error bars from a binning analysis for each histogram bin")
    ("binning-analysis-num-levels", value<int>(& opt->binning_analysis_num_levels)
     ->default_value(opt->binning_analysis_num_levels),
     ("Number of levels of coarse-graining in the binning analysis. See --binning-analysis-error-bars. "
      "Choose this number such that (n-run)/(2^(<binning-num-levels>)) is a sufficiently decent sample size "
      "(say ~"+std::to_string(last_binning_level_warn_min_samples)+").").c_str())
    ("control-step-size", bool_switch(& control_step_size_set)->default_value(false),
     Tomographer::Tools::fmts("Dynamically adjust the step size during thermalization runs in order to "
                              "keep the acceptance ratio approximately within the range [%.2f,%.2f] (but "
                              "in any case within [%.2f,%.2f]). The sweep size "
                              "is automatically adjusted so that step_size*sweep_size remains constant. "
                              "Furthermore the thermalization runs will be prolonged as necessary to "
                              "ensure that at least %.2f*n_therm thermalization sweeps have passed after "
                              "the last time the step size was adjusted. This option is enabled by "
                              "default.",
                              Tomographer::MHRWStepSizeControllerDefaults::DesiredAcceptanceRatioMin,
                              Tomographer::MHRWStepSizeControllerDefaults::DesiredAcceptanceRatioMax,
                              Tomographer::MHRWStepSizeControllerDefaults::AcceptableAcceptanceRatioMin,
                              Tomographer::MHRWStepSizeControllerDefaults::AcceptableAcceptanceRatioMax,
                              Tomographer::MHRWStepSizeControllerDefaults::EnsureNThermFixedParamsFraction
         ).c_str())
    ("no-control-step-size", bool_switch(& no_control_step_size_set)->default_value(false),
     "Do not dynamically adjust the step size during thermalization.")
    ("control-step-size-moving-avg-samples", value<int>(& opt->control_step_size_moving_avg_samples)
     ->default_value(opt->control_step_size_moving_avg_samples),
     "The number of most recent samples to use when calculating the moving average of "
     "the acceptance ratio.  Used only for dynamically adjusting the step size when "
     "--control-step-size is set.")
    ("control-binning-converged", bool_switch(& control_binning_converged_set)->default_value(false),
     "When calculating error bars via a binning analysis, ensure that the error bars have "
     "converged for (almost) all bins (see fine-tuning options below) before terminating "
     "the random walk. The random walk will continue past 100% until the required "
     "convergence criteria are met. This option is enabled by default.")
    ("no-control-binning-converged", bool_switch(& no_control_binning_converged_set)->default_value(false),
     "Do not dynamically control the random walk length, you might need to manually inspect "
     "the convergence status of the error bars resulting from the binning analysis.")
    ("control-binning-converged-max-not-converged",
     value<int>(& opt->control_binning_converged_max_not_converged )
     ->default_value(opt->control_binning_converged_max_not_converged),
     "If control-binning-converged is set, then do not finish the random walk before there being "
     "no more than this number of error bars which have not converged.")
    ("control-binning-converged-max-unknown",
     value<int>(& opt->control_binning_converged_max_unknown )
     ->default_value(opt->control_binning_converged_max_unknown),
     "If control-binning-converged is set, then do not finish the random walk before there being "
     "no more than this number of error bars for which the convergence is uncertain.")
    ("control-binning-converged-max-unknown-notisolated",
     value<int>(& opt->control_binning_converged_max_unknown_notisolated )
     ->default_value(opt->control_binning_converged_max_unknown_notisolated),
     "If control-binning-converged is set, then do not finish the random walk before there being "
     "no more than this number of error bars for which the convergence is uncertain and which are "
     "adjacent to other error bars which have not converged or whose convergence status is unknown.")

    ("step-size", value<double>(& opt->step_size)->default_value(opt->step_size),
     "the step size for the region")
    ("n-sweep", value<unsigned int>(& opt->Nsweep)->default_value(opt->Nsweep),
     "number of iterations per sweep")
    ("n-therm", value<unsigned int>(& opt->Ntherm)->default_value(opt->Ntherm),
     "number of thermalizing sweeps")
    ("n-run", value<unsigned int>(& opt->Nrun)->default_value(opt->Nrun),
     "number of running sweeps after thermalizing. If you're doing a binning analysis "
     "(default except if you give --no-binning-analysis-error-bars), use here a "
     "multiple of 2^(binning-analysis-num-levels).")
    ("n-repeats", value<unsigned int>(& opt->Nrepeats)->default_value(opt->Nrepeats),
     "number of times to repeat the metropolis procedure")
    ("n-chunk", value<unsigned int>(& opt->Nchunk)->default_value(opt->Nchunk),
     "OBSOLETE OPTION -- has no effect")
    ("n-meas-amplify-factor", value<double>(& opt->NMeasAmplifyFactor)
     ->default_value(opt->NMeasAmplifyFactor),
     "Specify an integer factor by which to multiply number of measurements. "
     "Don't use this. It's unphysical, and meant just for debugging Tomographer itself.")
    ("write-histogram", value<std::string>(& opt->write_histogram),
     "write the histogram to the given file in tabbed CSV values")
    ("verbose", value<Tomographer::Logger::LogLevel>(& opt->loglevel)->default_value(opt->loglevel)
     ->implicit_value(Tomographer::Logger::DEBUG),
     "print verbose information. Not very readable unless n-repeats=1. You may also specify "
     "as argument 'longdebug', 'debug', 'info', 'warning' or 'error'.")
    ("verbose-log-info", bool_switch(& opt->verbose_log_info)->default_value(opt->verbose_log_info),
     "[For Developers.] If specified, log messages are more verbose; they display e.g. at which point "
     "in the code they were emitted.")
    ("nice", value<int>(& opt->nice_level)->default_value(opt->nice_level),
     "Renice the process to the given level to avoid slowing down the whole system. Set to zero "
     "to avoid renicing.")
    ("log", value<std::string>(& flogname),
     "Redirect standard output (log) to the given file. Use '-' for stdout. If file exists, will append.")
    ("log-from-config-file-name", bool_switch(& flogname_from_config_file_name)->default_value(false),
     "Same as --log=<config-file>.log, where <config-file> is the file name passed to the "
     "option --config. This option can only be used in conjunction with --config and may not be used "
     "with --log.")
    ("config", value<std::string>(),
     "Read options from the given file. Use lines with syntax \"key=value\".")
    ("write-histogram-from-config-file-name",
     bool_switch(&write_histogram_from_config_file_name)->default_value(false),
     "Same as --write-histogram=<config-file>, where <config-file> is the file name passed to "
     "the option --config. This option can only be used in conjunction with --config and may not "
     "be used with --write-histogram.")
    ("periodic-status-report-ms",
     value<int>(& opt->periodic_status_report_ms)->default_value(opt->periodic_status_report_ms),
     "If set to a value > 0, then tomorun will produce a status report every so many milliseconds. "
     "The format of the status report is the same as when you hit Ctrl+C. You can still get reports "
     "anytime by hitting Ctrl+C.")
    ("version",
     "Print Tomographer/Tomorun version information as well as information about enabled features.")
    ("help", "Print this help message")
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
	// REFERENCE [1] is here
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
	"      reference state. If no <RefObject> is specified, then 'rho_MLE' is used.\n"
	"\n"
	// REFERENCE [3] is here
	"    - \"fidelity\": the (root) fidelity to a reference state [3]. <RefObject>\n"
	"      should be the name of a MATLAB variable present in the MATLAB data file.\n"
	"      This object should be a complex dim x dim matrix, the density matrix of\n"
	"      the reference state. If no <RefObject> is specified, then 'rho_MLE' is\n"
        "      used.\n"
	"      \n"
	"      Note: For the squared fidelity to a pure state (usually preferred in\n"
	"      experimental papers), you should use \"obs-value\" with the observable\n"
	// REFERENCE [4] is here
        "      being the density matrix of the reference state [4].\n"
	"\n"
	// REFERENCE [5] is here
	"    - \"purif-dist\": the purified distance to a reference state [5].\n"
	"      <RefObject> should be the name of a MATLAB variable present in the MATLAB\n"
	"      data file. This object should be a complex dim x dim matrix, the density\n"
	"      matrix of the reference state. If no <RefObject> is specified, then\n"
	"      'rho_MLE' is used.\n"
        //
        // INSERT CUSTOM FIGURE OF MERIT HERE:
        // Please don't forget to document the option value corresponding to your figure
        // of merit. Document also any possible string that is understood after the colon
        // in the option value.
        //
	"\n"
	"FOOTNOTES AND REFERENCES:\n"
	" [1] Christandl and Renner, Phys. Rev. Lett. 12:120403 (2012), arXiv:1108.5329\n"
	" [2] Ambegaokar and Troyer, Am. J. Phys., 78(2):150 (2010), arXiv:0906.0943\n"
	" [3] The root fidelity is defined as F(rho,sigma)=|| rho^{1/2} sigma^{1/2} ||_1,\n"
	"     as in Nielsen and Chuang, \"Quantum Computation and Quantum Information\".\n"
	" [4] Indeed, for pure rho_ref, F^2(rho,rho_ref) = tr(rho*rho_ref).\n"
	" [5] The purified distance, also called \"infidelity\" in the literature, is\n"
	"     defined as P(rho,sigma) = \\sqrt{1 - F^2(rho,sigma)}.\n"
	"\n"
	"CITATION:\n"
	"If you use this program in your research, we warmly encourage you to cite the\n"
	"following works:\n"
	"\n"
	"  1. Philippe Faist and Renato Renner. Practical and Reliable Error Bars in\n"
        "     Quantum Tomography. Physical Review Letters 117:1, 010404 (2016).\n"
        "     arXiv:1509.06763.\n"
	"\n"
	"  2. Philippe Faist. The Tomographer Project. Available at\n"
	"     https://github.com/Tomographer/tomographer/.\n"
	"\n"
	"FEEDBACK:\n"
	"Please report bugs, issues, and wishes at:\n"
	"\n"
	"    https://github.com/Tomographer/tomographer/issues\n"
	"\n"
	"Have a lot of fun!\n"
	"\n"
	;

      ::exit(1);
    }

    if (vm.count("version")) {
      std::cout << prog_version_info
		<< "----\n"
		<< "using:\n"
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

  // set up the boolean switche(s)

  if (binning_analysis_error_bars_set) {
    if (no_binning_analysis_error_bars_set) {
      throw bad_options("Cannot use both --binning_analysis-error-bars and "
                        "--no-binning-analysis-error-bars");
    }
    opt->binning_analysis_error_bars = true;
  } else if (no_binning_analysis_error_bars_set) {
    opt->binning_analysis_error_bars = false;
    // disable control-binning-converged, which is incompatible
    opt->control_binning_converged = false;
  }

  if (control_step_size_set) {
    if (no_control_step_size_set) {
      throw bad_options("Cannot use both --control-step-size and "
                        "--no-control-step-size");
    }
    opt->control_step_size = true;
  } else if (no_control_step_size_set) {
    opt->control_step_size = false;
  }

  if (control_binning_converged_set) {
    if (no_control_binning_converged_set) {
      throw bad_options("Cannot use both --control-binning-converged and "
                        "--no-control-binning-converged");
    }
    opt->control_binning_converged = true;
  } else if (no_control_binning_converged_set) {
    opt->control_binning_converged = false;
  }


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
      "       step size :          %-8.4g%s\n"
      "       sweep size :         %-8lu%s\n"
      "       # therm sweeps :     %lu\n"
      "       # run sweeps :       %-8lu%s\n"
      "       # intgr. repeats :   %lu\n"
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
      (opt->control_step_size ? "  (dyn. adjustment enabled)" : ""),
      (unsigned long)opt->Nsweep,
      (opt->control_step_size ? "  (dyn. adjustment enabled)" : ""),
      (unsigned long)opt->Ntherm,
      (unsigned long)opt->Nrun,
      (opt->control_binning_converged ? "  (dyn. control of convergence)" : ""),
      (unsigned long)opt->Nrepeats,
      (opt->write_histogram.size()
       ? opt->write_histogram + std::string("-histogram.csv")
       : std::string("<don't write histogram>")).c_str(),
      (unsigned long)(opt->Nrun*opt->Nrepeats),
      (double)(opt->Nrun*opt->Nrepeats)
      );
}










#endif
