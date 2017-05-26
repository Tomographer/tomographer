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


class DummyBinningAnalysisClass {
private:
  DummyBinningAnalysisClass() { }
};

void py_tomo_mhrwtasks(py::module rootmodule)
{
  auto logger = Tomographer::Logger::makeLocalLogger(TOMO_ORIGIN, *tpy::logger);
  logger.debug("py_tomo_mhrwtasks() ...");

  logger.debug("tomographer BinningAnalysis (dummy, just for convergence constants) ...");
  { typedef DummyBinningAnalysisClass Kl;
    py::class_<Kl>(
        rootmodule,
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
        // , py::metaclass()  -- deprecated as of pybind 2.1
        )
      .def_property_readonly_static("CONVERGED",
                                    [](py::object) -> int {
                                      return Tomographer::BINNING_CONVERGED;
                                    })
      .def_property_readonly_static("NOT_CONVERGED",
                                    [](py::object) -> int {
                                      return Tomographer::BINNING_NOT_CONVERGED;
                                    })
      .def_property_readonly_static("UNKNOWN_CONVERGENCE",
                                    [](py::object) -> int {
                                      return Tomographer::BINNING_UNKNOWN_CONVERGENCE;
                                    })
      ;
  }

  logger.debug("BinningErrorBarConvergenceSummary  ...");
  { typedef Tomographer::BinningErrorBarConvergenceSummary Kl;
    py::class_<Kl>(
        rootmodule,
        "BinningErrorBarConvergenceSummary",
        "A summary of how many error bars have converged. "
        "\n\n"
        "Reflects the corresponding C++ class :tomocxx:`Tomographer::BinningErrorBarConvergenceSummary <"
        "struct_tomographer_1_1_binning_error_bar_convergence_summary.html>`."
        "\n\n"
        "|picklable|"
        "\n\n"
        ".. versionadded:: 5.0\n        This class was added in Tomographer v5.0."
        "\n\n"
        ".. py:attribute:: n_bins\n\n"
        "    The total number of histogram bins (to each histogram bin corresponds a binning analysis error bar).\n\n"
        ".. py:attribute:: n_converged\n\n"
        "    The number of binning analysis error bars which appear to have converged.\n\n"
        ".. py:attribute:: n_unknown\n\n"
        "    The number of binning analysis error bars for which convergence is uncertain.\n\n"
        ".. py:attribute:: n_unknown_isolated\n\n"
        "    The number of binning analysis error bars for which convergence is uncertain, which "
        "additionally are contiguous to error bars which appear to have converged.\n\n"
        ".. py:attribute:: n_not_converged\n\n"
        "    The number of histogram bins whose binning analysis error bar appears not to have converged.\n\n"
        )
      .def(py::init<Eigen::Index,Eigen::Index,Eigen::Index,Eigen::Index,Eigen::Index>(),
           "n_bins"_a=0,"n_converged"_a=0,"n_unknown"_a=0,"n_unknown_isolated"_a=0,"n_not_converged"_a=0)
      .def_readwrite("n_bins", & Kl::n_bins)
      .def_readwrite("n_converged", & Kl::n_converged)
      .def_readwrite("n_unknown", & Kl::n_unknown)
      .def_readwrite("n_unknown_isolated", & Kl::n_unknown_isolated)
      .def_readwrite("n_not_converged", & Kl::n_not_converged)
      .def("__getstate__", [](py::object p) {
          return py::make_tuple(p.attr("n_bins"), p.attr("n_converged"), p.attr("n_unknown"),
                                p.attr("n_unknown_isolated"), p.attr("n_not_converged"));
        })
      .def("__setstate__", [](Kl & p, py::tuple t) {
          tpy::internal::unpack_tuple_and_construct<Kl, Eigen::Index, Eigen::Index, Eigen::Index,
                                                    Eigen::Index, Eigen::Index>(p, t);
        })
      ;
      ;
  }

  logger.debug("ValueHistogramWithBinningMHRWStatsCollectorResult ...");
  { typedef tpy::ValueHistogramWithBinningMHRWStatsCollectorResult Kl;
    py::class_<tpy::ValueHistogramWithBinningMHRWStatsCollectorResult>(
        rootmodule,
        "ValueHistogramWithBinningMHRWStatsCollectorResult",
        "Interfaces the corresponding C++ class :tomocxx:`"
        "Tomographer::ValueHistogramWithBinningMHRWStatsCollectorResult"
        " <struct_tomographer_1_1_value_histogram_with_binning_m_h_r_w_stats_collector_result.html>`."
        "\n\n"
        "|picklable|"
        "\n\n"
        ".. py:attribute:: histogram"
        "\n\n"
        "    The resulting histogram, with the final error bars. "
        "The scaling of the histogram is chosen such that each bin value represents the "
        "fraction of sample data points whose value were inside this bin.  Note: "
        "This histogram is NOT normalized to a probability density."
        "\n\n"
        "    .. versionchanged:: 5.0\n        Renamed `hist` to `histogram`."
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
      .def(py::init<tpy::HistogramWithErrorBars, tpy::RealMatrixType, Eigen::VectorXi>(),
           "histogram"_a, "error_levels"_a, "converged_status"_a)
      .def_property_readonly("histogram", [](const Kl& x) -> tpy::HistogramWithErrorBars { return x.histogram; } )
      .def_property_readonly("error_levels", [](const Kl & x) -> tpy::RealMatrixType {
          return x.error_levels;
        })
      .def_property_readonly("converged_status", [](const Kl & x) -> Eigen::VectorXi {
          return x.converged_status;
        })
      .def("errorBarConvergenceSummary", & Kl::errorBarConvergenceSummary)
      // .def("__repr__", [](const Kl& p) {
      //     return streamstr("ValueHistogramWithBinningMHRWStatsCollectorResult("<<"..."<<")") ;
      //   })
      .def("__getstate__", [](py::object p) {
          return py::make_tuple(p.attr("histogram"), p.attr("error_levels"), p.attr("converged_status"));
        })
      .def("__setstate__", [](Kl & p, py::tuple t) {
          tpy::internal::unpack_tuple_and_construct<Kl,
                                                    tpy::HistogramWithErrorBars,
                                                    tpy::RealMatrixType, Eigen::VectorXi>(p, t);
        })
      ;
  }

  logger.debug("mhrwtasks module ... ");

  py::module mhrwtasksmodule = rootmodule.def_submodule(
      "mhrwtasks",
      ( "Utilities for tasks running Metropolis-Hastings random walks.  These classes shouldn't be used "
        "directly; rather, corresponding instances are returned by, e.g., "
        ":py:func:`tomographer.tomorun.tomorun()`." )
      ) ;
  
  logger.debug("mhrwtasks.MHRandomWalkTaskResult ...");
  { typedef tpy::MHRandomWalkTaskResult Kl;
    py::class_<tpy::MHRandomWalkTaskResult>(
        mhrwtasksmodule,
        "MHRandomWalkTaskResult",
        "The result of an executed Metropolis-Hastings random walk task."
        "\n\n"
        "This class interfaces the corresponding C++ class :tomocxx:`"
        "Tomographer::MHRWTasks::MHRandomWalkTaskResult"
        " <struct_tomographer_1_1_m_h_r_w_tasks_1_1_m_h_random_walk_task_result.html>` "
        "(the stats results type can be anything, represented in a Python object)."
        "\n\n"
        "|picklable|"
        "\n\n"
        ".. py:attribute:: stats_results\n\n"
        "    An object containing the results of the stats collected during the random walk. This can be\n"
        "    any Python object.\n\n"
        "    .. versionchanged:: 5.0\n"
        "        Previously, this attribute was called `stats_collector_result` and necessarily had the\n"
        "        type :py:class:`tomographer.ValueHistogramWithBinningMHRWStatsCollectorResult`."
        "\n\n"
        ".. py:attribute:: mhrw_params\n\n"
        "    The parameters of the executed random walk, as an :py:class:`~tomographer.MHRWParams` "
        "instance.\n\n"
        ".. py:attribute:: acceptance_ratio\n\n"
        "    The average acceptance ratio of the random walk (excluding the thermalization sweeps).\n\n")
      .def(py::init<py::object, tpy::MHRWParams, double>(),
           "stats_results"_a, "mhrw_params"_a, "acceptance_ratio"_a)
      .def_readonly("stats_results", & Kl::stats_results )
      .def_readonly("mhrw_params", & Kl::mhrw_params )
      .def_readonly("acceptance_ratio", & Kl::acceptance_ratio )
      .def("__repr__", [](py::object p) {
          return streamstr("<MHRandomWalkTaskResult with "
                           << py::repr(p.attr("mhrw_params")).cast<std::string>() << ">") ;
        })
      .def("__getstate__", [](py::object p) {
          return py::make_tuple(p.attr("stats_results"), p.attr("mhrw_params"), p.attr("acceptance_ratio"));
        })
      .def("__setstate__", [](Kl & p, py::tuple t) {
          tpy::internal::unpack_tuple_and_construct<
            Kl,
            py::object,
            tpy::MHRWParams,
            double
            >(p, t);
        })
      ;
  }

  // alias for backwards compatibility, e.g. for un-pickling data pickled with Tomographer < 5
  mhrwtasksmodule.attr("MHRandomWalkValueHistogramTaskResult") = mhrwtasksmodule.attr("MHRandomWalkTaskResult");
}
