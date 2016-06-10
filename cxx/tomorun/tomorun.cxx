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

#include <tomographer2/tools/cxxutil.h>
#include <tomographer2/tools/loggers.h>
#include <tomographer2/tools/ezmatio.h>
#include <tomographer2/tools/signal_status_report.h>
#include <tomographer2/tools/eigenutil.h>
#include <tomographer2/densedm/dmtypes.h>
#include <tomographer2/densedm/param_herm_x.h>
#include <tomographer2/densedm/indepmeasllh.h>
#include <tomographer2/densedm/tspacefigofmerit.h>
#include <tomographer2/mhrw.h>
#include <tomographer2/mhrwtasks.h>
#include <tomographer2/multiprocomp.h>
#include <tomographer2/densedm/tspacellhwalker.h>
#include <tomographer2/tomographer_version.h>

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
    parse_options(&opt, argc, argv, logger.baselogger());
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

  display_parameters(&opt, logger.baselogger());

  //
  // Renice the program, if requested
  //

  if (opt.nice_level != 0) {
    // nice up our process.
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
  const unsigned long last_level_num_samples = std::ldexp((double)opt.Nrun, - opt.binning_analysis_num_levels);
  logger.debug("last_level_num_samples = %lu", last_level_num_samples);
  //
  if ( opt.binning_analysis_error_bars &&
       ( last_level_num_samples < (unsigned long)last_binning_level_warn_min_samples ) ) {
    logger.warning(
	"Few samples in the last binning level of binning analysis : "
	"Nrun=%lu, # of levels=%lu --> %lu samples. [Recommended >= %lu]",
	(unsigned long)opt.Nrun,
	(unsigned long)opt.binning_analysis_num_levels,
	(unsigned long)last_level_num_samples,
	(unsigned long)last_binning_level_warn_min_samples
	);
  }


  //
  // ---------------------------------------------------------------------------
  // Read tomography data from MATLAB file
  // ---------------------------------------------------------------------------
  //

  Tomographer::MAT::File * matf = 0;
  unsigned int dim = 0;
  unsigned int n_povms = 0;
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

  logger.debug("Data file opened, found dim = %u", dim);

  //
  // ---------------------------------------------------------------------------
  // Now, run our main program.
  // ---------------------------------------------------------------------------
  //

  // Maybe use statically instantiated size for some predefined sizes.

  //  Logger::MinimumSeverityLogger<Logger::FileLogger, Logger::INFO> mlog(logger);
  auto & mlog = logger.baselogger();

  try {
    // some special cases where we can avoid dynamic memory allocation for Eigen matrices
    // by using compile-time sizes
    
#if defined(TOMORUN_CUSTOM_FIXED_DIM) && defined(TOMORUN_CUSTOM_MAX_POVM_EFFECTS)
    
    //
    // We want a single customized case, with a fixed dimension of
    // TOMORUN_CUSTOM_FIXED_DIM (which may be "Eigen::Dynamic"), and a fixed maximum
    // number of POVM effects TOMORUN_CUSTOM_MAX_POVM_EFFECTS (which may also be
    // "Eigen::Dynamic").
    //
    // These macros can be defined in  "tomorun_config.h"
    //
    logger.debug("Using custom fixed dim = %d and fixed max POVM effects = %d  (%d=dynamic)",
		 TOMORUN_CUSTOM_FIXED_DIM, TOMORUN_CUSTOM_MAX_POVM_EFFECTS, Eigen::Dynamic);
    tomorun_dispatch_eb<TOMORUN_CUSTOM_FIXED_DIM,TOMORUN_CUSTOM_MAX_POVM_EFFECTS>(dim, &opt, matf, mlog);

#else
    
    //
    // Provide some standard fixed-size cases, in order to avoid dynamic memory allocation
    // for small matrices for common system sizes (e.g. a single qubit)
    //
    if (dim == 2 && n_povms <= 6) { // qubit problems are really common
      tomorun_dispatch_eb<2, 6>(dim, &opt, matf, mlog);
    } else if (dim == 2) {
      tomorun_dispatch_eb<2, Eigen::Dynamic>(dim, &opt, matf, mlog);
    } else if (dim == 4) { // two-qubit systems are also common
      tomorun_dispatch_eb<4, Eigen::Dynamic>(dim, &opt, matf, mlog);
    } else {
      tomorun_dispatch_eb<Eigen::Dynamic, Eigen::Dynamic>(dim, &opt, matf, mlog);
    }

#endif

  } catch (const std::exception& e) {
    logger.error("Exception: %s", e.what());
    throw;
  }

  return 0;
}
