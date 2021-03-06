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
#include <typeinfo>

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
#define MKSTRING(x) #x
#define IDENT_TO_STRING(x) MKSTRING(x)
#ifdef TOMORUN_INT
  featconfig.push_back(Tomographer::Tools::fmts("int_type=%s", IDENT_TO_STRING(TOMORUN_INT)));
#endif
#ifdef TOMORUN_REAL
  featconfig.push_back(Tomographer::Tools::fmts("real_type=%s", IDENT_TO_STRING(TOMORUN_REAL)));
#endif
#ifdef EIGEN_DEFAULT_INDEX_TYPE
  featconfig.push_back(Tomographer::Tools::fmts("eigen_index_type=%s", IDENT_TO_STRING(EIGEN_DEFAULT_INDEX_TYPE)));
#endif
#if TOMORUN_DO_SLOW_POVM_CONSISTENCY_CHECKS
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
#ifdef TOMORUN_RNG_CLASS
  featconfig.push_back(Tomographer::Tools::fmts("rng=%s", IDENT_TO_STRING(TOMORUN_RNG_CLASS)));
#endif
#if TOMORUN_USE_DEVICE_SEED
  featconfig.push_back(Tomographer::Tools::fmts("rng-seed-from-device=%s", TOMORUN_RANDOM_DEVICE ""));
#endif
#if defined(TOMORUN_MAX_LOG_LEVEL)
  featconfig.push_back("max-log-level=" +
                       Tomographer::Logger::LogLevel(Tomographer::Logger:: TOMORUN_MAX_LOG_LEVEL ).levelName());
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

void validate(boost::any& v, const std::vector<std::string> & values,
              figure_of_merit_spec * /* target_type */,
              int)
{
  using namespace boost::program_options;

  // Make sure no previous assignment to 'v' was made.
  validators::check_first_occurrence(v);

  // Extract the first string from 'values'. If there is more than
  // one string, it's an error, and exception will be thrown.
  const std::string & s = validators::get_single_string(values);

  try {
    v = boost::any(figure_of_merit_spec(s));
  } catch (const std::invalid_argument & exc) {
    throw validation_error(validation_error::invalid_option_value);
  }
}


// ------------------------------------------------------------------------------


struct ProgOptions
{

  int nice_level{14};

  FILE * flog{stdout};

  std::string data_file_name{};

  TomorunReal step_size{TomorunReal(0.01)};

  TomorunInt Nsweep{100};
  TomorunInt Ntherm{512};
  TomorunInt Nrun{32768};

  figure_of_merit_spec valtype{"fidelity"};

  TomorunReal val_min{TomorunReal(0.9)};
  TomorunReal val_max{TomorunReal(1.0)};
  Eigen::Index val_nbins{50};

  bool light_jumps{false};

  bool binning_analysis_error_bars{true};
  int binning_analysis_num_levels{-1};

  bool control_step_size{true};
  TomorunInt control_step_size_moving_avg_samples{2048};

  bool control_binning_converged{true};
  Eigen::Index control_binning_converged_max_not_converged{0};
  Eigen::Index control_binning_converged_max_unknown{2};
  Eigen::Index control_binning_converged_max_unknown_notisolated{0};
  double control_binning_converged_max_add_run_iters{1.5};

  int Nrepeats{defaultNumRepeat()};
  int Nchunk{1};

  TomorunReal NMeasAmplifyFactor{TomorunReal(1.0)};

  Tomographer::Logger::LogLevel loglevel{Tomographer::Logger::INFO};
  bool verbose_log_info{false}; // whether to display origin in log messages

  std::string write_histogram{""};

  int periodic_status_report_ms{-1};
};




// ------------------------------------------------------------------------------


TOMOGRAPHER_DEFINE_MSG_EXCEPTION(bad_options, "Invalid input options: ") ;

TOMOGRAPHER_DEFINE_MSG_EXCEPTION(invalid_input, "Invalid input data: ") ;

inline void ensure_valid_input(bool condition, std::string msg) {
  Tomographer::Tools::tomographerEnsure<invalid_input>(condition, msg);
}




// -----------------------------------------------------------------------------




template<typename BaseLoggerType>
void parse_options(ProgOptions * opt, int argc, char **argv,
                   BaseLoggerType & baselogger, Tomographer::Logger::FileLogger & filelogger)
{
  // read the options
  using namespace boost::program_options;

  auto logger = Tomographer::Logger::makeLocalLogger("parse_options()", baselogger);

  std::string flogname;
  bool flogname_from_config_file_name = false;

  std::string valhiststr;

  std::string configfname;
  std::string configdir;
  std::string configbasename;
  bool write_histogram_from_config_file_name = false;

  bool light_jumps_set = false;
  bool no_light_jumps_set = false;

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
    ("value-type", value<figure_of_merit_spec>(& opt->valtype)->default_value(opt->valtype),
     "Which value to acquire histogram of, e.g. fidelity to MLE. Possible values are 'fidelity', "
     "'purif-dist', 'tr-dist' or 'obs-value'. The value type may be followed by ':ObjName' to refer "
     "to a particular object defined in the datafile. See below for more info.")
    ("value-hist", value<std::string>(&valhiststr),
     "Do a histogram of the figure of merit for different measured values. Format MIN:MAX/NUM_BINS")
    ("light-jumps", bool_switch(& light_jumps_set)->default_value(light_jumps_set),
     "Carry out the \"light\" version of the random walk, where instead of moving the "
     "bipartite purified state vector uniformly on the hypersphere, we apply a random "
     "qubit unitary in the 2-d subspace "
     "of two randomly picked basis vectors. This runs faster and samples the same distribution, so there "
     "should be no reason not to use it. This is an experimental option, disabled by default.")
    ("no-light-jumps", bool_switch(& no_light_jumps_set)->default_value(no_light_jumps_set),
     "Do not carry out the \"light\" version of the random walk, do the full random unitary instead "
     "(see --light-jumps)")
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
      "(say ~"+std::to_string(last_binning_level_warn_min_samples)+").  Set to a negative value to "
      "choose level automatically.").c_str())
    ("control-step-size", bool_switch(& control_step_size_set)->default_value(false),
     Tomographer::Tools::fmts(
         "Dynamically adjust the step size during thermalization runs in order to "
         "keep the acceptance ratio approximately within the range [%.2f,%.2f] (but "
         "in any case within [%.2f,%.2f]). The sweep size "
         "is automatically adjusted so that step_size*sweep_size remains constant. "
         "Furthermore the thermalization runs will be prolonged as necessary to "
         "ensure that at least %.2f*n_therm thermalization sweeps have passed after "
         "the last time the step size was adjusted. This option is enabled by "
         "default.",
         Tomographer::MHRWAcceptRatioWalkerParamsControllerDefaults::DesiredAcceptanceRatioMin,
         Tomographer::MHRWAcceptRatioWalkerParamsControllerDefaults::DesiredAcceptanceRatioMax,
         Tomographer::MHRWAcceptRatioWalkerParamsControllerDefaults::AcceptableAcceptanceRatioMin,
         Tomographer::MHRWAcceptRatioWalkerParamsControllerDefaults::AcceptableAcceptanceRatioMax,
         Tomographer::MHRWAcceptRatioWalkerParamsControllerDefaults::EnsureNThermFixedParamsFraction
         ).c_str())
    ("no-control-step-size", bool_switch(& no_control_step_size_set)->default_value(false),
     "Do not dynamically adjust the step size during thermalization.")
    ("control-step-size-moving-avg-samples", value<TomorunInt>(& opt->control_step_size_moving_avg_samples)
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
     value<Eigen::Index>(& opt->control_binning_converged_max_not_converged )
     ->default_value(opt->control_binning_converged_max_not_converged),
     "If control-binning-converged is set, then do not finish the random walk before there being "
     "no more than this number of error bars which have not converged.")
    ("control-binning-converged-max-unknown",
     value<Eigen::Index>(& opt->control_binning_converged_max_unknown )
     ->default_value(opt->control_binning_converged_max_unknown),
     "If control-binning-converged is set, then do not finish the random walk before there being "
     "no more than this number of error bars for which the convergence is uncertain.")
    ("control-binning-converged-max-unknown-notisolated",
     value<Eigen::Index>(& opt->control_binning_converged_max_unknown_notisolated )
     ->default_value(opt->control_binning_converged_max_unknown_notisolated),
     "If control-binning-converged is set, then do not finish the random walk before there being "
     "no more than this number of error bars for which the convergence is uncertain and which are "
     "adjacent to other error bars which have not converged or whose convergence status is unknown.")
    ("control-binning-converged-max-add-run-iters",
     value<double>(& opt->control_binning_converged_max_add_run_iters )
     ->default_value(opt->control_binning_converged_max_add_run_iters),
     "If control-binning-converged is set, then end the random walk after a certain amount runs "
     "regardless of bins error bars convergence status. Specify the amount as a fraction of the set "
     "number of run sweeps, e.g. a value of 1.5 prolongs the random walk by at most 50% of the run "
     "sweeps. Set to a negative value to run as long as needed to make error bars converge as requested.")

    ("step-size", value<TomorunReal>(& opt->step_size)->default_value(opt->step_size),
     "the step size for the region")
    ("n-sweep", value<TomorunInt>(& opt->Nsweep)->default_value(opt->Nsweep),
     "number of iterations per sweep")
    ("n-therm", value<TomorunInt>(& opt->Ntherm)->default_value(opt->Ntherm),
     "number of thermalizing sweeps")
    ("n-run", value<TomorunInt>(& opt->Nrun)->default_value(opt->Nrun),
     "number of running sweeps after thermalizing. If you're doing a binning analysis "
     "(default except if you give --no-binning-analysis-error-bars), use here a "
     "multiple of 2^(binning-analysis-num-levels).")
    ("n-repeats", value<int>(& opt->Nrepeats)->default_value(opt->Nrepeats),
     "number of times to repeat the metropolis procedure")
    ("n-chunk", value<int>(& opt->Nchunk)->default_value(opt->Nchunk),
     "OBSOLETE OPTION -- has no effect")
    ("n-meas-amplify-factor", value<TomorunReal>(& opt->NMeasAmplifyFactor)
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
    Tomographer::Tools::FmtFootnotes footnotes;
    // Reference [1]
    footnotes.addSilentFootNote(1, "Christandl and Renner, Phys. Rev. Lett. 12:120403 (2012), arXiv:1108.5329");
    // Reference [2] in options above
    footnotes.addSilentFootNote(2, "Ambegaokar and Troyer, Am. J. Phys., 78(2):150 (2010), arXiv:0906.0943");

    auto & stream = std::cout;
    stream
      << std::setprecision(5)
      << "\n" << prog_version_info <<
      "\n"
      "A toolbox for error analysis in quantum tomography.\n"
      "\n"
      "Usage: tomorun --data-file-name=<data-file-name> [options]\n"
      "       tomorun --config=<tomorun-config-file>\n"
      "\n"
//    |--------------------------------------------------------------------------------| 80 chars (col. 87)
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
      <<  desc	<<
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
      ;
    PrintFigsOfMerit<TomorunFiguresOfMerit>::print(stream, footnotes);
    stream << 
      "\n"
      "FOOTNOTES AND REFERENCES:\n"
           << footnotes.wrapped(80) <<
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
				    << configfname << ". Change to directory and run again."));
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
  filelogger.setLevel(opt->loglevel);
  filelogger.setDisplayOrigin(opt->verbose_log_info);
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

    filelogger.setFp(opt->flog);
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
#define SET_OPT_BOOL_SWITCH(OPTNAME, DASH_OPTNAME)                      \
  if (OPTNAME ## _set) {                                                \
    if (no_ ## OPTNAME ## _set) {                                       \
      throw bad_options("Cannot use both --" #DASH_OPTNAME " and --no-" #DASH_OPTNAME ); \
    }                                                                   \
    opt-> OPTNAME = true;                                               \
  } else if (no_ ## OPTNAME ## _set) {                                  \
    opt-> OPTNAME = false ;                                             \
  }


  SET_OPT_BOOL_SWITCH(light_jumps, light-jumps) ;

  SET_OPT_BOOL_SWITCH(binning_analysis_error_bars, binning-analysis-error-bars) ;
  if (no_binning_analysis_error_bars_set) {
    // disable control-binning-converged, which is incompatible with no error bars
    opt->control_binning_converged = false;
  }

  SET_OPT_BOOL_SWITCH(control_step_size, control-step-size) ;

  SET_OPT_BOOL_SWITCH(control_binning_converged, control-binning-converged) ;


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

    opt->val_min = (TomorunReal)fmin;
    opt->val_max = (TomorunReal)fmax;
    opt->val_nbins = (Eigen::Index)nbins;
    logger.debug([&](std::ostream & stream) {
        stream << "Histogram parameters parsed: min=" << opt->val_min << ", max=" << opt->val_max
               << ", num_bins=" << opt->val_nbins ;
      });
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
      "       random walk jumps :  %s\n"
      "       value type :         %s\n"
      "       val. histogram :     [%.2g, %.2g] (%s bins)\n"
      "       error bars :         %s\n"
      "       step size :          %-8.4g%s\n"
      "       sweep size :         %-8s%s\n"
      "       # therm sweeps :     %s\n"
      "       # run sweeps :       %-8s%s\n"
      "       # intgr. repeats :   %d\n"
      "       write histogram to : %s\n"
      "\n"
      "       --> total no. of live samples = %s  (%.2e)\n"
      "\n",
      opt->data_file_name.c_str(), (double)opt->NMeasAmplifyFactor,
      (opt->light_jumps ? "\"light\"" : "\"full\""),
      streamcstr(opt->valtype),
      (double)opt->val_min, (double)opt->val_max, streamcstr(opt->val_nbins),
      (opt->binning_analysis_error_bars
       ? Tomographer::Tools::fmts("binning analysis (%d levels)", opt->binning_analysis_num_levels).c_str()
       : "std. dev. of runs"),
      (double)opt->step_size,
      (opt->control_step_size ? "  (dyn. adjustment enabled)" : ""),
      streamcstr(opt->Nsweep),
      (opt->control_step_size ? "  (dyn. adjustment enabled)" : ""),
      streamcstr(opt->Ntherm),
      streamcstr(opt->Nrun),
      (opt->control_binning_converged ? "  (dyn. control of convergence)" : ""),
      (int)opt->Nrepeats,
      (opt->write_histogram.size()
       ? opt->write_histogram + std::string("-histogram.csv")
       : std::string("<don't write histogram>")).c_str(),
      streamcstr(opt->Nrun*(TomorunInt)opt->Nrepeats),
      (double)(opt->Nrun*(TomorunInt)opt->Nrepeats)
      );
}










#endif
