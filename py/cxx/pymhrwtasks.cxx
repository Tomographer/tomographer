

#include <tomographerpy/common.h>
#include <tomographerpy/pymhrwtasks.h>

#include "common_p.h"



void py_tomo_mhrwtasks()
{
  auto logger = Tomographer::Logger::makeLocalLogger(TOMO_ORIGIN, tpy_logger);
  logger.debug("py_tomo_mhrwtasks() ...");

  logger.debug("mhrwtasks module ... ");

  boost::python::object mhrwtaskssubmod(boost::python::borrowed(PyImport_AddModule("tomographer.mhrwtasks")));
  boost::python::scope().attr("mhrwtasks") = mhrwtaskssubmod;
  {
    // now inside submodule
    boost::python::scope mhrwtasksmodule(mhrwtaskssubmod);

    logger.debug("mhrwtasks.ValueHistogramWithBinningMHRWStatsCollectorResult ...");
    { typedef Py::ValueHistogramWithBinningMHRWStatsCollectorResult Kl;
      boost::python::class_<Py::ValueHistogramWithBinningMHRWStatsCollectorResult>("ValueHistogramWithBinningMHRWStatsCollectorResult")
        .add_property("hist", +[](const Kl & x) { return x.hist; })
        .add_property("error_levels", +[](const Kl & x) -> Eigen::MatrixXd { return x.error_levels; })
        .add_property("converged_status", +[](const Kl & x) -> Eigen::VectorXi { return x.converged_status; })
        ;
    }

    logger.debug("mhrwtasks.MHRandomWalkValueHistogramTaskResult ...");
    { typedef Py::MHRandomWalkValueHistogramTaskResult Kl;
      boost::python::class_<Py::MHRandomWalkValueHistogramTaskResult>("MHRandomWalkValueHistogramTaskResult")
        .add_property("mhrw_params", +[](const Kl & x) { return x.mhrw_params; })
        .add_property("acceptance_ratio", +[](const Kl & x) { return x.acceptance_ratio; })
        .add_property("stats_collector_result", +[](const Kl & x) { return x.stats_collector_result; })
        ;
    }
  }
}
