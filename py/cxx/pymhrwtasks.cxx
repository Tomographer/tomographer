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

  logger.debug("tomographer.BinningAnalysis (dummy, just for convergence constants) ...");
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
        ,
        py::metaclass()
        )
      .def_property_readonly_static("CONVERGED",
                                    [](py::object) -> int {
                                      return Tomographer::BinningAnalysisParams<double>::CONVERGED;
                                    })
      .def_property_readonly_static("NOT_CONVERGED",
                                    [](py::object) -> int {
                                      return Tomographer::BinningAnalysisParams<double>::NOT_CONVERGED;
                                    })
      .def_property_readonly_static("UNKNOWN_CONVERGENCE",
                                    [](py::object) -> int {
                                      return Tomographer::BinningAnalysisParams<double>::UNKNOWN_CONVERGENCE;
                                    })
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
      .def(py::init<tpy::UniformBinsHistogramWithErrorBars,tpy::RealMatrixType,Eigen::VectorXi>(),
           "hist"_a, "error_levels"_a, "converged_status"_a)
      .def_readonly("hist", & Kl::hist )
      .def_property_readonly("error_levels", [](const Kl & x) -> tpy::RealMatrixType { return x.error_levels; })
      .def_property_readonly("converged_status", [](const Kl & x) -> Eigen::VectorXi { return x.converged_status; })
      // .def("__repr__", [](const Kl& p) {
      //     return streamstr("ValueHistogramWithBinningMHRWStatsCollectorResult("<<"..."<<")") ;
      //   })
      .def("__getstate__", [](py::object p) {
          return py::make_tuple(p.attr("hist"), p.attr("error_levels"), p.attr("converged_status"));
        })
      .def("__setstate__", [](Kl & p, py::tuple t) {
          tpy::internal::unpack_tuple_and_construct<Kl, tpy::UniformBinsHistogramWithErrorBars,
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
  
  logger.debug("mhrwtasks.MHRandomWalkValueHistogramTaskResult ...");
  { typedef tpy::MHRandomWalkValueHistogramTaskResult Kl;
    py::class_<tpy::MHRandomWalkValueHistogramTaskResult>(
        mhrwtasksmodule,
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
      .def(py::init<tpy::ValueHistogramWithBinningMHRWStatsCollectorResult, tpy::MHRWParams, double>(),
           "stats_collector_result"_a, "mhrw_params"_a, "acceptance_ratio"_a)
      .def_readonly("stats_collector_result", & Kl::stats_collector_result )
      .def_readonly("mhrw_params", & Kl::mhrw_params )
      .def_readonly("acceptance_ratio", & Kl::acceptance_ratio )
      .def("__repr__", [](py::object p) {
          return streamstr("<MHRandomWalkValueHistogramTaskResult with "
                           << py::repr(p.attr("mhrw_params")).cast<std::string>() << ">") ;
        })
      .def("__getstate__", [](py::object p) {
          return py::make_tuple(p.attr("stats_collector_result"), p.attr("mhrw_params"), p.attr("acceptance_ratio"));
        })
      .def("__setstate__", [](Kl & p, py::tuple t) {
          tpy::internal::unpack_tuple_and_construct<Kl, tpy::ValueHistogramWithBinningMHRWStatsCollectorResult,
                                                    tpy::MHRWParams, double>(p, t);
        })
      ;
  }
}
