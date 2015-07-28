
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

#include <tomographer/tools/util.h>
#include <tomographer/tools/loggers.h>
#include <tomographer/tools/ezmatio.h>
#include <tomographer/tools/signal_status_report.h>
#include <tomographer/qit/matrq.h>
#include <tomographer/qit/util.h>
#include <tomographer/qit/param_herm_x.h>
#include <tomographer/tomoproblem.h>
#include <tomographer/mhrw.h>
#include <tomographer/mhrwtasks.h>
#include <tomographer/dmmhrw.h>
#include <tomographer/multiprocomp.h>

#include "tomorun_config.h"
#include "tomorun_opts.h"
#include "tomorun_dispatch.h"


Tomographer::Logger::FileLogger logger(stdout, Tomographer::Logger::INFO, false);


// ------------------------------------------------------------------------------


int main(int argc, char **argv)
{
  ProgOptions opt;

  try {
    parse_options(&opt, argc, argv, logger);
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

  display_parameters(&opt, logger);

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

  Tomographer::MAT::File * matf = 0;
  unsigned int dim = 0;
  unsigned int n_povms = 0;
  try {
    matf = new Tomographer::MAT::File(opt.data_file_name);
    dim = Tomographer::MAT::value<int>(matf->var("dim"));
    n_povms = matf->var("Nm").numel();
  } catch (const std::exception& e) {
    logger.error("main()", [&opt, &e](std::ostream & str){
                   str << "Failed to read data from file "<< opt.data_file_name << "\n\t" << e.what() << "\n";
                 });
    ::exit(1);
  }

  auto delete_matf = Tomographer::Tools::finally([matf] {
      logger.debug("main()", "Freeing input file resource");
      delete matf;
    });

  logger.debug("main()", "Data file opened, found dim = %u", dim);

  //
  // ---------------------------------------------------------------------------
  // Now, run our main program.
  // ---------------------------------------------------------------------------
  //

  // Maybe use statically instantiated size for some predefined sizes.

  //  Logger::MinimumSeverityLogger<Logger::FileLogger, Logger::INFO> mlog(logger);
  auto & mlog = logger;

  //  try {
  if (dim == 2 && n_povms <= 6) {
    tomorun_dispatch_eb<2, 6>(dim, &opt, matf, mlog);
/* speed up a bit compilation times by removing some cases here */
/*} else if (dim == 2 && n_povms <= 64) {
    tomorun_dispatch<2, 64>(dim, &opt, matf, mlog); */
  } else if (dim == 2) {
    tomorun_dispatch_eb<2, Eigen::Dynamic>(dim, &opt, matf, mlog);
/*} else if (dim == 4 && n_povms <= 64) {
    tomorun_dispatch<4, 64>(dim, &opt, matf, mlog); */
  } else if (dim == 4) {
    tomorun_dispatch_eb<4, Eigen::Dynamic>(dim, &opt, matf, mlog);
  } else {
    tomorun_dispatch_eb<Eigen::Dynamic, Eigen::Dynamic>(dim, &opt, matf, mlog);
  }
  //  } catch (const std::exception& e) {
  //    logger.error("main()", "Caught exception: %s", e.what());
  //    return 2;
  //  }
}
