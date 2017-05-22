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

#include <tomographer/tools/cxxutil.h>
#include <tomographer/tools/loggers.h>
#include <tomographer/tools/ezmatio.h>
#include <tomographer/tools/signal_status_report.h>
#include <tomographer/tools/eigenutil.h>
#include <tomographer/densedm/dmtypes.h>
#include <tomographer/densedm/param_herm_x.h>
#include <tomographer/densedm/indepmeasllh.h>
#include <tomographer/densedm/tspacefigofmerit.h>
#include <tomographer/mhrw.h>
#include <tomographer/mhrwstepsizecontroller.h>
#include <tomographer/mhrwvalueerrorbinsconvergedcontroller.h>
#include <tomographer/mhrwtasks.h>
#include <tomographer/multiproc.h>
#include <tomographer/densedm/tspacellhwalker.h>
#include <tomographer/tomographer_version.h>

#if defined(TOMORUN_MULTIPROC_OPENMP)

// use OpenMP if it is available.
#  if defined(_OPENMP)
#    include <tomographer/multiprocomp.h>
#    define TomorunMultiProcTaskDispatcher Tomographer::MultiProc::OMP::TaskDispatcher
#    define TomorunMultiProcTaskDispatcherTitle "OpenMP"
#  else
#    error "OpenMP multiprocessing scheme requested, but OpenMP is not enabled"
#  endif

inline int defaultNumRepeat() { return omp_get_num_procs(); }

#define TOMORUN_THREAD_CRITICAL_SECTION         \
  _Pragma("omp critical")

#elif defined(TOMORUN_MULTIPROC_SEQUENTIAL)

// Don't parallelize.
#  include <tomographer/multiproc.h>
#  define TomorunMultiProcTaskDispatcher Tomographer::MultiProc::Sequential::TaskDispatcher
#  define TomorunMultiProcTaskDispatcherTitle "Sequential (parallelization deactivated!)"

inline int defaultNumRepeat() { return 1; }

#define TOMORUN_THREAD_CRITICAL_SECTION

#else // also if defined(TOMORUN_MULTIPROC_CXXTHREADS)

// use C++11 threads otherwise
#  include <tomographer/multiprocthreads.h>
#  define TomorunMultiProcTaskDispatcher Tomographer::MultiProc::CxxThreads::TaskDispatcher
#  define TomorunMultiProcTaskDispatcherTitle "C++11 Threads"

inline int defaultNumRepeat() { return (int)std::thread::hardware_concurrency(); }

#define TOMORUN_THREAD_CRITICAL_SECTION                 \
  static std::mutex _tomorun_critical_section_mutex;                       \
  std::lock_guard<std::mutex> _tomorun_lock_guard(_tomorun_critical_section_mutex);

#endif


#include "tomorun_config.h"
#include "tomorun_opts.h"
#include "tomorun_dispatch.h"


Tomographer::Logger::FileLogger rootlogger(stdout, Tomographer::Logger::INFO, false);


// ------------------------------------------------------------------------------


int main(int argc, char **argv)
{
  ProgOptions opt;

  Tomographer::Logger::LocalLogger<decltype(rootlogger)> logger("main()", rootlogger);

  try {
    parse_options(&opt, argc, argv, logger.parentLogger());
  } catch (const bad_options& e) {
    fprintf(stderr, "%s\n", e.what());
    return 127;
  }

  logger.info([&](std::ostream & str) {
      str << "\n"
	  << "-----------------------------------------------------------------\n"
	  << "Welcome to " << prog_version_info_1
	  << "-----------------------------------------------------------------\n"
	  << prog_version_info_2
	  << "-----------------------------------------------------------------\n"
	;
    });

  display_parameters(&opt, logger.parentLogger());

  //
  // Renice the program, if requested
  //

  if (opt.nice_level != 0) {
    // nice up our process.
#ifndef TOMORUN_NOT_HAVE_NICE
    errno = 0; // not std::errno, errno is a macro
    int niceret = nice(opt.nice_level);
    if (niceret == -1 && errno != 0) {
      logger.warning(
          "Failed to nice(%d) process: %s",
          opt.nice_level, strerror(errno)
          );
    } else {
      logger.debug("nice()'ed our process to priority %d", niceret);
    }
#else
    logger.warning("nice() is not supported on this system, and option --nice-level was ignored.");
#endif
  }

  logger.debug([](std::ostream & str) {
      str << "Features activated:\n"
	  << prog_version_info_features();
    });


  //
  // ---------------------------------------------------------------------------
  // Do some preliminary checks
  // ---------------------------------------------------------------------------
  //

  // warn the user if the last binning level comprises too few samples.
  //
  // # of samples at last level is = Nrun/(2^{num_binning_levels}).
  // [note: std::ldexp(x,e) := x * 2^{e} ]
  //
  const TomorunInt last_level_num_samples
    = (TomorunInt)std::ldexp((TomorunReal)opt.Nrun, - opt.binning_analysis_num_levels);
  logger.debug([&](std::ostream & stream) {
      stream << "last_level_num_samples = " << last_level_num_samples;
    }) ;
  //
  if ( opt.binning_analysis_error_bars &&
       ( last_level_num_samples < (TomorunInt)last_binning_level_warn_min_samples ) ) {
    logger.warning([&](std::ostream & stream) {
        stream << "Few samples in the last binning level of binning analysis : "
               << "Nrun=" << opt.Nrun << ", # of levels=" << opt.binning_analysis_num_levels
               << " --> " << last_level_num_samples
               << " samples. [Recommended >= " << last_binning_level_warn_min_samples << "]";
      });
  }

  // warn the user if they specified control-binning-convergence but don't have binning
  // analysis enabled
  if (opt.control_binning_converged && ! opt.binning_analysis_error_bars) {
    logger.warning([&](std::ostream & stream) {
        stream << "--control-binning-converged cannot be used if --binning-analysis-error-bars "
               << "is not set. --control-binning-converged will be ignored.";
      });
  }

  //
  // ---------------------------------------------------------------------------
  // Read tomography data from MATLAB file
  // ---------------------------------------------------------------------------
  //

  Tomographer::MAT::File * matf = 0;
  int dim = 0;
  int n_povms = 0;
  try {
    matf = new Tomographer::MAT::File(opt.data_file_name);
    dim = Tomographer::MAT::value<int>(matf->var("dim"));
    n_povms = matf->var("Nm").numel();
  } catch (const std::exception& e) {
    logger.error([&opt, &e](std::ostream & str){
                   str << "Failed to read data from file "<< opt.data_file_name << "\n\t" << e.what() << "\n";
                 });
    ::exit(1);
  }

  auto delete_matf = Tomographer::Tools::finally([matf,&logger] {
      logger.debug("Freeing input file resource");
      delete matf;
    });

  logger.debug([&](std::ostream & stream) {
      stream << "Data file opened, found dim = " << dim;
    }) ;

  //
  // ---------------------------------------------------------------------------
  // Now, run our main program.
  // ---------------------------------------------------------------------------
  //

  // Maybe use statically instantiated size for some predefined sizes.

  //  Logger::MinimumSeverityLogger<Logger::FileLogger, Logger::INFO> mlog(logger);
  auto & mlog = logger.parentLogger();

  try {
    // some special cases where we can avoid dynamic memory allocation for Eigen matrices
    // by using compile-time sizes
    
#if defined(TOMORUN_CUSTOM_FIXED_DIM) && defined(TOMORUN_CUSTOM_FIXED_MAX_DIM) && defined(TOMORUN_CUSTOM_MAX_POVM_EFFECTS)
    
    //
    // We want a single customized case, with a fixed dimension of
    // TOMORUN_CUSTOM_FIXED_DIM (which may be "Eigen::Dynamic"), and a fixed maximum
    // number of POVM effects TOMORUN_CUSTOM_MAX_POVM_EFFECTS (which may also be
    // "Eigen::Dynamic").
    //
    // These macros can be defined in  "tomorun_config.h"
    //
    logger.debug("Using custom fixed dim = %d, custom fixed max dim = %d, "
                 " and fixed max POVM effects = %d  (%d=dynamic)",
		 TOMORUN_CUSTOM_FIXED_DIM, TOMORUN_CUSTOM_FIXED_MAX_DIM,
                 TOMORUN_CUSTOM_MAX_POVM_EFFECTS, Eigen::Dynamic);
    tomorun_dispatch_st<TOMORUN_CUSTOM_FIXED_DIM,TOMORUN_CUSTOM_FIXED_MAX_DIM,
                        TOMORUN_CUSTOM_MAX_POVM_EFFECTS>(dim, &opt, matf, mlog);

    (void)n_povms; // silence unused variable warning

#else
    
    (void)n_povms; // silence unused variable warning

    //
    // Provide some standard fixed-size cases, in order to avoid dynamic memory allocation
    // for small matrices for common system sizes (e.g. a single qubit)
    //
    if (dim == 2) {
      tomorun_dispatch_st<2, 2, Eigen::Dynamic>(dim, &opt, matf, mlog);
    } else if (dim == 4) { // two-qubit systems are also common
      tomorun_dispatch_st<4, 4, Eigen::Dynamic>(dim, &opt, matf, mlog);
    } else {
      tomorun_dispatch_st<Eigen::Dynamic, Eigen::Dynamic, Eigen::Dynamic>(dim, &opt, matf, mlog);
    }

#endif

  } catch (const std::exception& e) {
    logger.error("Exception: %s", e.what());
    throw;
  }

  return 0;
}
