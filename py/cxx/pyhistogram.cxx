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


#include <ios>
#include <iomanip>

#include "tomographerpy/common.h"
#include "tomographerpy/pyhistogram.h"

#include <tomographer/tools/fmt.h>

#include "common_p.h"

#include <pybind11/operators.h>


using namespace pybind11::literals;



static void check_pickle_tuple_size(std::size_t expect, int given)
{
  if ((int)expect != given) {
    throw tpy::TomographerCxxError(std::string("Invalid pickle state: expected ")+std::to_string(expect)+", got "
                                  +std::to_string(given));
  }
}

static inline std::string fmt_hist_param_float(tpy::RealType val)
{
  return Tomographer::Tools::fmts("%.3g", (double)val);
}



void py_tomo_histogram(py::module rootmodule)
{
  auto logger = Tomographer::Logger::makeLocalLogger(TOMO_ORIGIN, *tpy::logger);

  logger.debug("py_tomo_histogram() ...");

  logger.debug("HistogramParams...");

  // CLASS: HistogramParams
  {
    typedef tpy::HistogramParams Kl;
    py::class_<tpy::HistogramParams>(
        rootmodule,
        "HistogramParams",
        Tomographer::Tools::fmts(
            "Specify histogram bins parameters: the minimum value, the maximum value, and the number "
            "of bins. The interval `[min,max[` is split into `num_bins` equally spaced bins."
            "\n\n"
            "|picklable|"
            "\n\n"
            ".. seealso:: This python class interfaces its corresponding "
            ":tomocxx:`C++ class Tomographer::HistogramParams "
            "<struct_tomographer_1_1_uniform_bins_histogram_params.html>`, with the template parameter "
            "`Scalar=%s`.\n\n"
            "\n\n"
            ".. py:function:: HistogramParams(min=%.1f, max=%.1f, num_bins=%ld)"
            "\n\n"
            "    Construct a histogram parameters configuration."
            "\n\n"
            ".. py:attribute:: min"
            "\n\n"
            "    The lower bound on the range of values covered by the histogram. (Read-write attribute)"
            "\n\n"
            ".. py:attribute:: max"
            "\n\n"
            "    The (strict) upper bound on the range of values covered by the histogram. (Read-write attribute)"
            "\n\n"
            ".. py:attribute:: num_bins"
            "\n\n"
            "    The number of bins the range `[min,max]` is divided into, defining the bins. (Read-write attribute)\n\n"
            ".. py:attribute:: values_center"
            "\n\n"
            "    Read-only attribute returning a vector (NumPy array) of values corresponding to each bin center value."
            "\n\n"
            ".. py:attribute:: values_lower"
            "\n\n"
            "    Read-only attribute returning a vector (NumPy array) of values corresponding to each bin lower value."
            "\n\n"
            ".. py:attribute:: values_upper"
            "\n\n"
            "    Read-only attribute returning a vector (NumPy array) of values corresponding to each bin upper value."
            "\n\n"
            "\n\n", boost::core::demangle(typeid(tpy::RealType).name()).c_str(),
            Kl().min, Kl().max, (long)Kl().num_bins).c_str())
      .def(py::init<tpy::RealType,tpy::RealType,Eigen::Index>(),
           "min"_a=Kl().min, "max"_a=Kl().max, "num_bins"_a=Kl().num_bins)
      .def_readwrite("min", & Kl::min)
      .def_readwrite("max", & Kl::max)
      .def_readwrite("num_bins", & Kl::num_bins)
      .def_property_readonly("values_center", [](Kl & p) -> tpy::RealVectorType { return p.valuesCenter(); })
      .def_property_readonly("values_lower", [](Kl & p) -> tpy::RealVectorType { return p.valuesLower(); })
      .def_property_readonly("values_upper", [](Kl & p) -> tpy::RealVectorType { return p.valuesUpper(); })
      .def("isWithinBounds", & Kl::isWithinBounds, "value"_a,
           "isWithinBounds(value)"
           "\n\n"
           "Check whether the given `value` is within the bounds of the histogram, that is, in the"
           " range `[min, max[`.")
      .def("binIndex", & Kl::binIndex, "value"_a,
           "binIndex(value)"
           "\n\n"
           "Get the index of the bin in which the given value would be saved in. Indexes are of "
           "course zero-based.")
      .def("binLowerValue", & Kl::binLowerValue, "index"_a,
           "binLowerValue(index)\n\n"
           "Returns the value which a given bin index represents (lower bin value limit).\n\n"
           "Raise a :py:exc:`TomographerCxxError` if the index is invalid.")
      .def("binCenterValue", & Kl::binCenterValue, "index"_a,
           "binCenterValue(index)\n\n"
           "Returns the value which a given bin index represents (center bin value).")
      .def("binUpperValue", & Kl::binUpperValue, "index"_a,
           "binUpperValue(index)\n\n"
           "Returns the value which a given bin index represents (upper bin value limit).")
      .def("binResolution", & Kl::binResolution,
           "binResolution()\n\n"
           "Returns the width of a bin.  This is simply :math:`(\\mathit{max} - \\mathit{min})/\\mathit{num_bins}`.")
      .def("__repr__", [](const Kl& p) {
          return streamstr("HistogramParams(min="
                           << fmt_hist_param_float(p.min) << ",max="
                           << fmt_hist_param_float(p.max) << ",num_bins=" << p.num_bins << ")");
        })
      .def("__getstate__", [](const Kl& p) {
          return py::make_tuple(p.min, p.max, p.num_bins) ;
        })
      .def("__setstate__", [](Kl & p, py::tuple t) {
          tpy::internal::unpack_tuple_and_construct<Kl, tpy::RealType, tpy::RealType, Eigen::Index>(p, t);
        })
      ;
  }

  logger.debug("Histogram...");

  // CLASS: Histogram
  {
    typedef tpy::Histogram Kl;
    py::class_<tpy::Histogram>(
        rootmodule,
        "Histogram",
        // doc
        "A histogram object.  An interval `[min,max]` is divided into `num_bins` bins, each of same width. "
        "Each time a new value is to be recorded, the corresponding bin's counter is incremented."
        "\n\n"
        "|picklable|"
        "\n\n"
        ".. seealso:: This python class mirrors the C++ class :tomocxx:`Tomographer::HistogramParams "
        "<class_tomographer_1_1_uniform_bins_histogram.html>`.  See the C++ class documentation for "
        "more information.  However, the Python implementation uses NumPy arrays internally, so it can "
        "transparently store different bin count types."
        "\n\n"
        ".. versionchanged:: 4.1\n"
        "    Replaced the class static attribute `HasErrorBars` by the property `has_error_bars`."
        "\n\n"
        ".. versionchanged:: 5.0\n"
        "    This class no longer has a fixed storage type, now using a NumPy array internally. The "
        "    `HistogramReal` has been deprecated."
        "\n\n"
        ".. py:function:: Histogram([params=HistogramParams()])\n\n"
        "    Construct a new histogram object with the given histogram parameters.\n\n"
        ".. py:function:: Histogram(min, max, num_bins)\n\n"
        "    Alternative call syntax; the effect is the same as the other constructor.\n\n"
        ".. py:attribute:: params\n\n"
        "    The :py:class:`HistogramParams` object which stores the current histogram "
        "parameters.  This attribute is read-only.  The parameters are specified to the constructor, and "
        "cannot be changed.\n\n"
        ".. py:attribute:: values_center\n\n    A shorthand for `params.values_center`. "
        "See :py:class:`HistogramParams`.\n\n"
        ".. py:attribute:: values_lower\n\n    A shorthand for `params.values_lower`. "
        "See :py:class:`HistogramParams`.\n\n"
        ".. py:attribute:: values_upper\n\n    A shorthand for `params.values_upper`. "
        "See :py:class:`HistogramParams`.\n\n"
        ".. py:attribute:: bins\n\n"
        "    The histogram bin counts, interfaced as a `NumPy` array object storing integers.  This attribute "
        "is readable and writable, although you may not change the size or type of the array.\n\n"
        ".. py:attribute:: off_chart\n\n"
        "    The number of recorded data points which were beyond the histogram range `[params.min, params.max[`.\n\n"
        ".. py:attribute:: has_error_bars\n\n"
        "    The constant `False`.  Note that, by contrast to the correpsonding C++ class, this is "
        "an instance property and not a class property.  (This is because in Python, you don't usually "
        "have natural direct access to the type, only to the instance.)\n\n"
        // , py::metaclass()  -- deprecated as of pybind 2.1
        )
      .def(py::init<tpy::HistogramParams>(), "params"_a = tpy::HistogramParams())
      .def(py::init<tpy::RealType, tpy::RealType, Eigen::Index>(),
          "min"_a, "max"_a, "num_bins"_a)
      .def_property_readonly("params", [](const Kl & h) -> tpy::HistogramParams { return h.params; })
      .def_property_readonly("values_center", [](const Kl & p) -> tpy::RealVectorType { return p.params.valuesCenter(); })
      .def_property_readonly("values_lower", [](const Kl & p) -> tpy::RealVectorType { return p.params.valuesLower(); })
      .def_property_readonly("values_upper", [](const Kl & p) -> tpy::RealVectorType { return p.params.valuesUpper(); })
      .def_property("bins", [](const Kl & h) { return h.bins; }, & Kl::set_bins )
      .def_property("off_chart", [](const Kl& h) { return h.off_chart; }, & Kl::set_off_chart )
      .def_property_readonly("has_error_bars", [](py::object) { return false; })
      .def("reset", [](Kl & h) {
          h.bins[py::slice(0,h.bins.attr("size").cast<py::ssize_t>(),1)] = py::cast(0);
          h.off_chart = py::cast(0);
        },
        "reset()\n\n"
        "Clears the current histogram counts (including `off_chart` counts) to zero.  The histogram "
        "parameters in `params` are kept intact.")
      .def("load", & Kl::load,
           "bins"_a, "off_chart"_a = tpy::CountIntType(0),
           // doc
           "load(bins[, off_chart=0])\n\n"
           "Load bin values from the vector of values `bins`, which is expected to be a `NumPy` array. If "
           "`off_chart` is specified, the current `off_chart` count is also set to the given value; otherwise "
           "it is reset to zero.")
      .def("add", [](Kl & h, py::object x, py::object o) {
          auto np = py::module::import("numpy");
          np.attr("add")(h.bins, x, "out"_a=h.bins);
          h.off_chart = np.attr("add")(h.off_chart, o);
        },
        "bins"_a, "off_chart"_a = py::cast(0),
        // doc
        "add(bins[, off_chart=0])\n\n"
        "Add a number of counts to each bin, specifed by a vector of values `bins` which is expected to be "
        "a `NumPy` array. If `off_chart` is specified, the current `off_chart` count is increased by this number, "
        "otherwise it is left to its current value.")
      .def("numBins", [](Kl & h) { return h.params.num_bins; },
           "numBins()\n\n"
           "A shorthand for `params.num_bins`.  See :py:class:`HistogramParams`.")
      .def("count", [](Kl & h, Eigen::Index i) {
          return h.bins[py::cast(i)];
        },
        "index"_a,
        // doc
        "count(index)\n\n"
        "Returns the number of counts in the bin indexed by `index`.  Indexes start at zero.  "
        "Raises :py:exc:`TomographerCxxError` if index is out of range.")
      .def("record", [](Kl & h, py::object value, py::object w) {
          auto np = py::module::import("numpy");
          if ( ! h.params.isWithinBounds(value.cast<tpy::RealType>()) ) {
            h.off_chart = np.attr("add")(h.off_chart, w);
            return (Eigen::Index)(-1);
          }
          // calling bin_index_unsafe because we have already checked that value is in range.
          const Eigen::Index index = h.params.binIndexUnsafe(value.cast<tpy::RealType>());
          np.attr("add").attr("at")(h.bins, py::cast(index), w);
          return index;
        },
        "value"_a, "weight"_a = py::cast(1),
        // doc
        "record(value[, weight=1])\n\n"
        "Record a new data sample. This increases the corresponding bin count by one, or by `weight` if the "
        "latter argument is provided.")
      .def("normalization", [](const Kl & h) {
          return h.normalization();
        },
        "normalization()\n\n"
        "Calculate the normalization factor for the histogram.  This corresponds to the total number "
        "of weight-1 data points, where the weight of a data point may be specified as a second argument "
        "to :py:meth:`record()`.")
      .def("normalized", [](const Kl & h) {
          tpy::Histogram newh(h.params);
          py::object f = h.normalization();
          auto np = py::module::import("numpy");
          newh.load(np.attr("true_divide")(h.bins, f), np.attr("true_divide")(h.off_chart, f));
          return newh;
        },
        "normalized()\n\n"
        "Returns a normalized version of this histogram. The bin counts as well as the off_chart counts "
        "are divided by :py:meth:`normalization()`.  The returned object is a "
        ":py:class:`Histogram` instance.")
      .def("totalCounts", & Kl::totalCounts,
           "totalCounts()\n\n"
           "Return the total number of histogram counts (no normalization)\n\n"
           "This returns the sum of all `bins` contents, plus the `off_chart` counts. This does not "
           "account for the normalization and completely ignores the x-axis scaling.")
      .def("normalizedCounts", [](const Kl & h) {
          tpy::Histogram newh(h.params);
          py::object f = h.totalCounts();
          auto np = py::module::import("numpy");
          newh.load(np.attr("true_divide")(h.bins, f), np.attr("true_divide")(h.off_chart, f));
          return newh;
        },
        "normalizedCounts()\n\n"
        "Get a version of this histogram, normalized by total counts.  See :tomocxx:`the doc for the "
        "corresponding C++ function <class_tomographer_1_1_histogram.html#a51e60cfa1f7f111787ee015805aac93c>`.")
      .def("prettyPrint", [](const Kl & h, int max_width) {
          return Tomographer::histogramPrettyPrint(
              h.toCxxHistogram<tpy::RealType,tpy::RealType>(),
              max_width
              );
        },
        "max_width"_a = 0,
        // doc
        "prettyPrint([max_width=0])\n\n"
        "Produce a human-readable representation of the histogram, with horizontal visual bars, which is "
        "suitable to be displayed in a terminal (for instance).  The formatted histogram is returned as a "
        "string.  If `max_width` is specified and nonzero, the output is designed to fit into a terminal "
           "display of the given number of characters wide, otherwise a default width is used.")
      .def("__repr__", [](const Kl& p) {
          return streamstr("Histogram(min="
                           << fmt_hist_param_float(p.params.min) << ",max="
                           << fmt_hist_param_float(p.params.max) << ",num_bins=" << p.params.num_bins
                           << ",bins=" << py::repr(p.bins).cast<std::string>()
                           << ",off_chart=" << py::repr(p.off_chart).cast<std::string>() << ")");
        })
      .def("__getstate__", [](py::object self) {
          return py::make_tuple(
              self.attr("params"),
              self.attr("bins"),
              self.attr("off_chart")
              ) ;
        })
      .def("__setstate__", [](py::object self, py::tuple t) {
          check_pickle_tuple_size(py::len(t), 3);
          tpy::Histogram & histogram = self.cast<tpy::Histogram&>();

          new (&histogram) tpy::Histogram(t[0].cast<tpy::HistogramParams>()) ;

          // restore bins & off_chart
          histogram.bins = t[1];
          histogram.off_chart = t[2];
        })
      ;
  }

  logger.debug("HistogramWithErrorBars...");

  // HistogramWithErrorBars
  {
    typedef tpy::HistogramWithErrorBars Kl;
    py::class_<tpy::HistogramWithErrorBars,tpy::Histogram>(
        rootmodule,
        "HistogramWithErrorBars",
        "A histogram (with uniform bin size), with error bars associated to each bin."
        "\n\n"
        "This class internally inherits :py:class:`Histogram`, and all those methods are "
        "exposed in this class, except for `add()`. In addition, the `reset()` method also clears the "
        "error bar values, and the `normalized()` method returns a histogram with the appropriate error "
        " bars on the normalized histogram."
        "\n\n"
        "|picklable|"
        "\n\n"
        ".. versionchanged:: 4.1\n"
        "    Replaced the class static attribute `HasErrorBars` by the property `has_error_bars`."
        "\n\n"
        ".. versionchanged:: 5.0\n"
        "    This class no longer has a fixed storage type, now using NumPy arrays internally."
        "\n\n"
        "In addition to the members inherited from :py:class:`Histogram`, the following "
        "members are available:"
        "\n\n"
        ".. py:attribute:: delta\n\n"
        "    The error bar values on each of the histogram bin counts, interfaced as a `NumPy` array object "
        "storing real values.  This attribute is readable and writable, although you may not change the "
        "size or type of the array.\n\n"
        ".. py:attribute:: has_error_bars\n\n"
        "    The constant `True`.  Note that, by contrast to the correpsonding C++ class, this is "
        "an instance property and not a class property.  (This is because in Python, you don't usually "
        "have natural direct access to the type, only to the instance.)\n\n"
        // , py::metaclass()  -- deprecated as of pybind 2.1
        )
      .def(py::init<tpy::HistogramParams>(), "params"_a=tpy::HistogramParams())
      .def(py::init<tpy::RealType, tpy::RealType, Eigen::Index>())
      .def_property("delta", [](const Kl & h) { return h.delta; }, & Kl::set_delta )
      .def_property_readonly("has_error_bars", [](py::object) { return true; })
      .def("reset", [](Kl & h) {
          h.bins[py::slice(0,h.bins.attr("size").cast<py::ssize_t>(),1)] = py::cast(0);
          h.delta[py::slice(0,h.bins.attr("size").cast<py::ssize_t>(),1)] = py::cast(0);
          h.off_chart = py::cast(0);
        },
        "reset()\n\n"
        "Clears the current histogram counts (including `off_chart` counts) to zero.  The histogram "
        "parameters in `params` are kept intact.")
      .def("load", & Kl::load,
           "y"_a, "yerr"_a, "off_chart"_a = tpy::RealType(0),
           // doc
           "load(y, yerr[, off_chart=0])"
           "\n\n"
           "Load data into the histogram. The array `y` specifies the bin counts, and `yerr` specifies "
           "the error bars on those bin counts.  The off-chart counter is set to `off_chart`."
          )
      // add() and record() make no sense
      .def("add", [](Kl & , py::args, py::kwargs) {
          throw tpy::TomographerCxxError("May not call add() on HistogramWithErrorBars");
        })
      .def("record", [](Kl & , py::args, py::kwargs) {
          throw tpy::TomographerCxxError("May not call record() on HistogramWithErrorBars");
        })
      .def("errorBar", [](Kl & h, Eigen::Index i) {
          return h.delta[py::cast(i)];
          },
        "index"_a,
        // doc
        "errorBar(index)\n\n"
        "Get the error bar value associated to the bin of the given `index`. Raises "
        ":py:exc:`TomographerCxxError` if index is out of range.")
      .def("normalized",
           [](const Kl & h) {
             tpy::HistogramWithErrorBars newh(h.params);
             py::object f = h.normalization();
             auto np = py::module::import("numpy");
             newh.load(np.attr("true_divide")(h.bins, f), np.attr("true_divide")(h.delta, f),
                       np.attr("true_divide")(h.off_chart, f));
             return newh;
           },
           "normalized()\n\n"
           "Returns a normalized version of this histogram, including the error bars. The bin counts, the "
           "error bars and the off_chart counts are divided by :py:meth:`~Histogram.normalization()`. "
           " The returned object is again a :py:class:`HistogramWithErrorBars` instance.")
      .def("normalizedCounts", [](const Kl & h) {
          tpy::HistogramWithErrorBars newh(h.params);
          py::object f = h.totalCounts();
          auto np = py::module::import("numpy");
          newh.load(np.attr("true_divide")(h.bins, f), np.attr("true_divide")(h.delta, f),
                    np.attr("true_divide")(h.off_chart, f));
          return newh;
        },
        "normalizedCounts()\n\n"
        "Get a version of this histogram, normalized by total counts.  See :tomocxx:`the doc for the "
        "corresponding C++ function <class_tomographer_1_1_histogram.html#a51e60cfa1f7f111787ee015805aac93c>`.")
      .def("prettyPrint", [](const Kl & h, int max_width) {
          return Tomographer::histogramPrettyPrint(
              h.toCxxHistogram<tpy::RealType,tpy::RealType>(),
              max_width
              );          
        },
        "max_width"_a = 0,
        // doc
        "prettyPrint([max_width=0])\n\n"
        "Produce a human-readable representation of the histogram, with horizontal visual bars, which is "
        "suitable to be displayed in a terminal (for instance).  The formatted histogram is returned as a "
        "string.  If `max_width` is specified and nonzero, the output is designed to fit into a terminal "
           "display of the given number of characters wide, otherwise a default width is used.")
      .def("__repr__", [](const Kl& p) {
          return streamstr("HistogramWithErrorBars(min="
                           << fmt_hist_param_float(p.params.min) << ",max="
                           << fmt_hist_param_float(p.params.max) << ",num_bins=" << p.params.num_bins
                           << ",bins=" << py::repr(p.bins).cast<std::string>()
                           << ",delta=" << py::repr(p.delta).cast<std::string>()
                           << ",off_chart=" << py::repr(p.off_chart).cast<std::string>() << ")");
        })
      .def("__getstate__", [](py::object self) {
          return py::make_tuple( self.attr("params"), self.attr("bins"), self.attr("delta"), self.attr("off_chart") );
        })
      .def("__setstate__", [](py::object self, py::tuple t) {
          // allow 5
          if (py::len(t) < 4 || py::len(t) > 5) {
            throw tpy::TomographerCxxError(streamstr("Invalid pickle state: expected 4, got "
                                                     << py::len(t)));
          }
          tpy::HistogramWithErrorBars & histogram = self.cast<tpy::HistogramWithErrorBars&>();

          new (&histogram) tpy::HistogramWithErrorBars(t[0].cast<tpy::HistogramParams>()) ;

          // restore bins, delta & off_chart
          histogram.bins = t[1];
          histogram.delta = t[2];
          histogram.off_chart = t[3];
          // It's possible we get passed a 5th parameter (t[4]), if we are unpickling an
          // AveragedXXXXXHistogram from an earlier version of Tomographer.  Just ignore it.
        })
       ;
  }

  /* These won't be useful -- they are deprecated.  If anything, we should
     expose the AggregatedHistogram classes .......

  logger.debug("AveragedSimpleHistogram...");
  // AveragedSimpleHistogram
  {
    typedef tpy::AveragedSimpleHistogram Kl;
    py::class_<tpy::AveragedSimpleHistogram, tpy::HistogramWithErrorBars>(
        rootmodule,
        "AveragedSimpleHistogram",
        "A :py:class:`HistogramWithErrorBars` which results from the "
        "averaging of several :py:class:`Histogram` histograms."
        "\n\n"
        "Add histograms to average together using the :py:meth:`addHistogram()` method, and "
        "then call :py:meth:`finalize()`. Then, the data stored in the current object will "
        "correspond to the averaged histogram. (See :tomocxx:`here the theory about how this is "
        "implemented <page_theory_averaged_histogram.html>`.)"
        "\n\n"
        "This histogram object inherits :py:class:`HistogramWithErrorBars`, so all the "
        "methods exposed in that class are available to access the averaged histogram data."
        "\n\n"
        "|picklable|"
        "\n\n"
        ".. warning:: You must not forget to call `finalize()` before accessing the averaged "
        "histogram data.  The data stored in the current "
        "histogram object is UNDEFINED before having calling `finalize()`."
        "\n\n"
        ".. py:attribute:: num_histograms\n\n"
        "    The number of histograms currently stored (read-only). This property may be "
        "    accessed at any time, also before having called :py:meth:`finalize()`."
        )
      .def(py::init<tpy::HistogramParams>(), "params"_a = tpy::HistogramParams())
      .def(py::init<tpy::RealType, tpy::RealType, tpy::CountIntType>(),
           "min"_a, "max"_a, "num_bins"_a)
      .def_property_readonly("num_histograms", [](const Kl & h) { return h.num_histograms; })
      .def("addHistogram",
           [](Kl & h, const tpy::Histogram & o) { h.addHistogram(o); },
           "histogram"_a,
           "addHistogram(histogram)\n\n"
           "Add a new histogram to the average with the others.")
      .def("reset",
           [](Kl & h) { h.reset(); }
          )
      .def("reset",
           [](Kl & h, const tpy::HistogramParams & param) { h.reset(param); },
           "reset([param])\n\n"
           "Clear all stored histograms and start a new averaging sequence. The "
           "histogram parameters are changed to `param` if you specify `param`, otherwise "
           "they are left unchanged. After calling this function, the averaged histogram class "
           "is in the same state as a freshly constructed averaged histogram object: you may call "
           ":py:meth:`addHistogram()` to add histograms to the average, after which you must call "
           ":py:meth:`finalize()` to finalize the averaging procedure."
           )
      .def("finalize", [](Kl & h) { h.finalize(); },
           "finalize()\n\n"
           "Call this function after all the histograms have been added with calls to :py:meth:`addHistogram()`. Only "
           "after calling this function may you access the averaged histogram in the current histogram object.")
      .def("__repr__", [](const Kl& p) {
          return streamstr("AveragedSimpleHistogram(min="
                           << fmt_hist_param_float(p.params.min) << ",max="
                           << fmt_hist_param_float(p.params.max) << ",num_bins=" << p.params.num_bins << ")");
        })
      .def("__getstate__", avghistogram_pickle<Kl>::getstate)
      .def("__setstate__", avghistogram_pickle<Kl>::setstate)
      ;
  }
  logger.debug("AveragedSimpleRealHistogram...");
  // AveragedSimpleRealHistogram
  {
    typedef tpy::AveragedSimpleRealHistogram Kl;
    py::class_<tpy::AveragedSimpleRealHistogram,tpy::HistogramWithErrorBars>(
        rootmodule,
        "AveragedSimpleRealHistogram",
        "A :py:class:`HistogramWithErrorBars` which results from the "
        "averaging of several :py:class:`HistogramReal` histograms."
        "\n\n"
        "This class is identical in functionality to :py:class:`AveragedSimpleHistogram`, except "
        "that the histograms which are to be averaged are :py:class:`HistogramReal` "
        "objects."
        "\n\n"
        "|picklable|"
        "\n\n"
        ".. warning:: You must not forget to call `finalize()` before accessing the averaged "
        "histogram data.  The data stored in the current "
        "histogram object is UNDEFINED before having calling `finalize()`."
        "\n\n"
        ".. py:attribute:: num_histograms\n\n"
        "    The number of histograms currently stored (read-only). This property may be "
        "    accessed at any time, also before having called :py:meth:`finalize()`."
        )
      .def(py::init<tpy::HistogramParams>(), "params"_a = tpy::HistogramParams())
      .def(py::init<tpy::RealType, tpy::RealType, tpy::CountIntType>(),
          "min"_a, "max"_a, "num_bins"_a)
      .def_property_readonly("num_histograms", [](const Kl & h) { return h.num_histograms; })
      .def("addHistogram",
           [](Kl & h, const tpy::HistogramReal & o) { h.addHistogram(o); },
           "histogram"_a,
           "addHistogram(histogram)\n\n"
           "Add a new histogram to the average with the others.")
      .def("reset",
           [](Kl & h) { h.reset(); }
          )
      .def("reset",
           [](Kl & h, const tpy::HistogramParams & param) { h.reset(param); },
           "reset([param])\n\n"
           "Clear all stored histograms and start a new averaging sequence. The "
           "histogram parameters are changed to `param` if you specify `param`, otherwise "
           "they are left unchanged. After calling this function, the averaged histogram class "
           "is in the same state as a freshly constructed averaged histogram object: you may call "
           ":py:meth:`addHistogram()` to add histograms to the average, after which you must call "
           ":py:meth:`finalize()` to finalize the averaging procedure."
           )
      .def("finalize", [](Kl & h) { h.finalize(); },
           "finalize()\n\n"
           "Call this function after all the histograms have been added with calls to :py:meth:`addHistogram()`. Only "
           "after calling this function may you access the averaged histogram in the current histogram object.")
      .def("__repr__", [](const Kl& p) {
          return streamstr("AveragedSimpleRealHistogram(min="
                           << fmt_hist_param_float(p.params.min) << ",max="
                           << fmt_hist_param_float(p.params.max) << ",num_bins=" << p.params.num_bins << ")");
        })
      .def("__getstate__", avghistogram_pickle<Kl>::getstate)
      .def("__setstate__", avghistogram_pickle<Kl>::setstate)
      ;
  }
  logger.debug("AveragedErrorBarHistogram...");
  // AveragedErrorBarHistogram
  {
    typedef tpy::AveragedErrorBarHistogram Kl;
    py::class_<tpy::AveragedErrorBarHistogram,tpy::HistogramWithErrorBars>(
        rootmodule,
        "AveragedErrorBarHistogram",
        "A :py:class:`HistogramWithErrorBars` which results from the "
        "averaging of several :py:class:`HistogramWithErrorBars` histograms."
        "\n\n"
        "This class is essentially identical in functionality to "
        ":py:class:`AveragedSimpleHistogram` and :py:class:`AveragedSimpleRealHistogram`, except "
        "that the histograms which are to be averaged are "
        " :py:class:`HistogramWithErrorBars` "
        "objects, i.e. each histogram added already has information about error bars.  Those "
        "error bars are then combined appropriately, as described in :tomocxx:`the theory about "
        "how this class is implemented <page_theory_averaged_histogram.html>`."
        "\n\n"
        "|picklable|"
        "\n\n"
        ".. warning:: You must not forget to call `finalize()` before accessing the averaged "
        "histogram data.  The data stored in the current "
        "histogram object is UNDEFINED before having calling `finalize()`."
        "\n\n"
        ".. py:attribute:: num_histograms\n\n"
        "    The number of histograms currently stored (read-only). This property may be "
        "    accessed at any time, also before having called :py:meth:`finalize()`."
        )
      .def(py::init<tpy::HistogramParams>(), "params"_a = tpy::HistogramParams())
      .def(py::init<tpy::RealType, tpy::RealType, tpy::CountIntType>(),
          "min"_a, "max"_a, "num_bins"_a)
      .def_property_readonly("num_histograms", [](const Kl & h) { return h.num_histograms; })
      .def("addHistogram",
           [](Kl & h, const tpy::HistogramWithErrorBars & o) {
             h.addHistogram(o);
           },
           "histogram"_a,
           "addHistogram(histogram)\n\n"
           "Add a new histogram to the average with the others.")
      .def("reset",
           [](Kl & h) { h.reset(); }
          )
      .def("reset",
           [](Kl & h, const tpy::HistogramParams & param) { h.reset(param); },
           "reset([param])\n\n"
           "Clear all stored histograms and start a new averaging sequence. The "
           "histogram parameters are changed to `param` if you specify `param`, otherwise "
           "they are left unchanged. After calling this function, the averaged histogram class "
           "is in the same state as a freshly constructed averaged histogram object: you may call "
           ":py:meth:`addHistogram()` to add histograms to the average, after which you must call "
           ":py:meth:`finalize()` to finalize the averaging procedure."
           )
      .def("finalize", [](Kl & h) { h.finalize(); },
           "finalize()\n\n"
           "Call this function after all the histograms have been added with calls to :py:meth:`addHistogram()`. Only "
           "after calling this function may you access the averaged histogram in the current histogram object.")
      .def("__repr__", [](const Kl& p) {
          return streamstr("AveragedErrorBarHistogram(min="
                           << fmt_hist_param_float(p.params.min) << ",max="
                           << fmt_hist_param_float(p.params.max) << ",num_bins=" << p.params.num_bins << ")");
        })
      .def("__getstate__", avghistogram_pickle<Kl>::getstate)
      .def("__setstate__", avghistogram_pickle<Kl>::setstate)
      ;
  }
  */


  // deprecated aliases
  auto & m = rootmodule;
  m.attr("AveragedSimpleHistogram") = m.attr("HistogramWithErrorBars");
  m.attr("AveragedSimpleRealHistogram") = m.attr("HistogramWithErrorBars");
  m.attr("AveragedErrorBarHistogram") = m.attr("HistogramWithErrorBars");
  m.attr("HistogramReal") = m.attr("Histogram");
  m.attr("UniformBinsHistogramParams") = m.attr("HistogramParams");
  m.attr("UniformBinsHistogram") = m.attr("Histogram");
  m.attr("UniformBinsRealHistogram") = m.attr("HistogramReal");
  m.attr("UniformBinsHistogramWithErrorBars") = m.attr("HistogramWithErrorBars");

  logger.debug("py_tomo_histogram() completed.");
}
