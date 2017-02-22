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



#include <tomographerpy/common.h>
#include <tomographerpy/pymhrwtasks.h>

#include "common_p.h"


struct DummyBinningAnalysisClass { };


struct ValueHistogramWithBinningMHRWStatsCollectorResult_pickle_suite : boost::python::pickle_suite
{
  static boost::python::tuple getinitargs(const Py::ValueHistogramWithBinningMHRWStatsCollectorResult & result)
  {
    return boost::python::make_tuple(result.hist,
                                     Py::RealMatrixType(result.error_levels),
                                     Eigen::VectorXi(result.converged_status));
  }
};

struct MHRandomWalkValueHistogramTaskResult_pickle_suite : boost::python::pickle_suite
{
  static boost::python::tuple getinitargs(const Py::MHRandomWalkValueHistogramTaskResult & result)
  {
    return boost::python::make_tuple(result.stats_collector_result,
                                     result.mhrw_params,
                                     result.acceptance_ratio);
  }
};


void py_tomo_mhrwtasks()
{
  auto logger = Tomographer::Logger::makeLocalLogger(TOMO_ORIGIN, tpy_logger);
  logger.debug("py_tomo_mhrwtasks() ...");

  logger.debug("tomographer.BinningAnalysis (dummy, just for convergence constants) ...");
  { typedef DummyBinningAnalysisClass Kl;
    boost::python::class_<Kl>(
        "BinningAnalysis",
        "A dummy class whose sole purpose is to expose the following constants to Python code. "
        "\n\n"
        ".. py:attribute:: CONVERGED\n\n"
        "    The error bar has converged over the different binning levels, "
        "the error bar can be considered reliable.\n\n"
        ".. py:attribute:: NOT_CONVERGED\n\n"
        "    The error bar has not converged over the different binning levels, "
        "and it should not be considered as reliable.\n\n"
        ".. py:attribute:: UNKNOWN_CONVERGENCE\n\n"
        "    The convergence of the error bar over the different binning levels "
        "is uknown, or couldn't be determined. It may or may not be reliable.\n\n"
        , boost::python::no_init
        )
      .add_static_property("CONVERGED",
                           +[]() -> int { return Tomographer::BinningAnalysisParams<double>::CONVERGED; })
      .add_static_property("NOT_CONVERGED",
                           +[]() -> int { return Tomographer::BinningAnalysisParams<double>::NOT_CONVERGED; })
      .add_static_property("UNKNOWN_CONVERGENCE",
                           +[]() -> int { return Tomographer::BinningAnalysisParams<double>::UNKNOWN_CONVERGENCE; })
      ;
  }

  logger.debug("ValueHistogramWithBinningMHRWStatsCollectorResult ...");
  { typedef Py::ValueHistogramWithBinningMHRWStatsCollectorResult Kl;
    boost::python::class_<Py::ValueHistogramWithBinningMHRWStatsCollectorResult>(
        "ValueHistogramWithBinningMHRWStatsCollectorResult",
        "Interfaces the corresponding C++ class :tomocxx:`"
        "Tomographer::ValueHistogramWithBinningMHRWStatsCollectorResult"
        " <struct_tomographer_1_1_value_histogram_with_binning_m_h_r_w_stats_collector_result.html>`."
        "\n\n"
        "|picklable|"
        "\n\n"
        ".. py:attribute:: hist"
        "\n\n"
        "    The resulting histogram, with the final error bars. "
        "The scaling of the histogram is chosen such that each bin value represents the "
        "fraction of sample data points whose value were inside this bin.  Note: "
        "This histogram is NOT normalized to a probability density."
        "\n\n"
        ".. py:attribute:: error_levels"
        "\n\n"
        "    Detailed error bars for all binning levels.  This is a matrix with a number "
        "of rows equal to the number of bins and a number of columns equal to the number of binning levels. "
        "Each entry corresponds to the error bar calculated after the given binning level, where the last "
        "(rightmost) entry corresponds to the final, hopefully converged error bar. "
        "\n\n"
        ".. py:attribute:: converged_status"
        "\n\n"
        "    The convergence status determined for each error bar. This is a vector of length equal to "
        " the number of histogram bins. Each element of the vector indicates that the corresponding histogram "
        "error bar has converged (:py:const:`BinningAnalysis.CONVERGED <tomographer.BinningAnalysis>`), "
        "has not converged (:py:const:`BinningAnalysis.NOT_CONVERGED <tomographer.BinningAnalysis>`), "
        "or whether the convergence status is unknown or couldn't be determined "
        " (:py:const:`BinningAnalysis.UNKNOWN_CONVERGENCE <tomographer.BinningAnalysis>`)."
        )
      .def(boost::python::init<Py::UniformBinsHistogramWithErrorBars,
           Py::RealMatrixType,
           Eigen::VectorXi>())
      .add_property("hist", +[](const Kl & x) { return x.hist; })
      .add_property("error_levels", +[](const Kl & x) -> Eigen::MatrixXd { return x.error_levels; })
      .add_property("converged_status", +[](const Kl & x) -> Eigen::VectorXi { return x.converged_status; })
      .def_pickle(ValueHistogramWithBinningMHRWStatsCollectorResult_pickle_suite())
      ;
  }


  logger.debug("mhrwtasks module ... ");

  boost::python::object mhrwtaskssubmod(boost::python::borrowed(PyImport_AddModule("tomographer.mhrwtasks")));
  boost::python::scope().attr("mhrwtasks") = mhrwtaskssubmod;
  {
    // now inside submodule
    boost::python::scope mhrwtasksmodule(mhrwtaskssubmod);

    mhrwtasksmodule.attr("__doc__") = std::string(
        "Utilities for tasks running Metropolis-Hastings random walks.  These classes shouldn't be used "
        "directly; rather, corresponding instances are returned by, e.g., "
        ":py:func:`tomographer.tomorun.tomorun()`."
        );

    logger.debug("mhrwtasks.MHRandomWalkValueHistogramTaskResult ...");
    { typedef Py::MHRandomWalkValueHistogramTaskResult Kl;
      boost::python::class_<Py::MHRandomWalkValueHistogramTaskResult>(
          "MHRandomWalkValueHistogramTaskResult",
          "The result of an executed Metropolis-Hastings random walk task."
          "\n\n"
          "This class interfaces the corresponding C++ class :tomocxx:`"
          "Tomographer::MHRWTasks::MHRandomWalkTaskResult"
          " <struct_tomographer_1_1_m_h_r_w_tasks_1_1_m_h_random_walk_task_result.html>`, "
          "when specialized to the :tomocxx:`"
          "Tomographer::ValueHistogramWithErrorBarsMHRWStatsCollector"
          " <class_tomographer_1_1_value_histogram_with_binning_m_h_r_w_stats_collector.html>` stats"
          " collector type."
          "\n\n"
          "|picklable|"
          "\n\n"
          ".. py:attribute:: stats_collector_result\n\n"
          "    An object of type :py:class:`~tomographer.ValueHistogramWithBinningMHRWStatsCollectorResult` "
          "detailing the result of the stats collecting class which is responsible for determining the final "
          "histogram, and carrying out the binning analysis to come up with error bars.\n\n"
          ".. py:attribute:: mhrw_params\n\n"
          "    The parameters of the executed random walk, as an :py:class:`~tomographer.MHRWParams` instance.\n\n"
          ".. py:attribute:: acceptance_ratio\n\n"
          "    The average acceptance ratio of the random walk (excluding the thermalization sweeps).\n\n")
        .def(boost::python::init<Py::ValueHistogramWithBinningMHRWStatsCollectorResult, Py::MHRWParams, double>())
        .add_property("stats_collector_result", +[](const Kl & x) { return x.stats_collector_result; })
        .add_property("mhrw_params", +[](const Kl & x) { return x.mhrw_params; })
        .add_property("acceptance_ratio", +[](const Kl & x) { return x.acceptance_ratio; })
        .def_pickle(MHRandomWalkValueHistogramTaskResult_pickle_suite())
        ;
    }
  }
}
