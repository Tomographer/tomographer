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


static void check_pickle_tuple_size(std::size_t expect, int given)
{
  if ((int)expect != given) {
    throw std::runtime_error(std::string("Invalid pickle state: expected ")+std::to_string(expect)+", got "
                             +std::to_string(given));
  }
}


// inspired by http://www.boost.org/doc/libs/1_46_1/libs/python/test/pickle3.cpp

template<typename HistogramType, bool HasErrorBars = HistogramType::HasErrorBars>
struct histogram_pickle
{
  // nothing by default, only specializations
};
// HasErrorBars=false
template<typename HistogramType>
struct histogram_pickle<HistogramType, false>
{
  static py::tuple getstate(py::object self)
  {
    return py::make_tuple(
        self.attr("params"),
        self.attr("bins"),
        self.attr("off_chart")
        ) ;
  }
  static void setstate(py::object self, py::tuple t)
  {
    check_pickle_tuple_size(py::len(t), 3);
    HistogramType & histogram = self.cast<HistogramType&>();

    new (&histogram) HistogramType(t[0].cast<tpy::UniformBinsHistogramParams>()) ;

    // restore bins & off_chart
    histogram.bins = t[1].cast< Eigen::Matrix<typename HistogramType::CountType, Eigen::Dynamic, 1> >();
    histogram.off_chart = t[2].cast<typename HistogramType::CountType>();
  }
};
// HasErrorBars=true
template<typename HistogramType>
struct histogram_pickle<HistogramType, true>
{
  static py::tuple getstate(const py::object & self)
  {
    return py::make_tuple( self.attr("params"), self.attr("bins"), self.attr("delta"), self.attr("off_chart") );
  }
  static void setstate(py::object self, py::tuple t)
  {
    check_pickle_tuple_size(py::len(t), 4);
    HistogramType & histogram = self.cast<HistogramType&>();

    new (&histogram) HistogramType(t[0].cast<tpy::UniformBinsHistogramParams>()) ;

    // restore bins, delta & off_chart
    histogram.bins = t[1].cast< Eigen::Matrix<typename HistogramType::CountType, Eigen::Dynamic, 1> >() ;
    histogram.delta = t[2].cast<Eigen::Matrix<typename HistogramType::CountType, Eigen::Dynamic, 1> >() ;
    histogram.off_chart = t[3].cast<typename HistogramType::CountType>() ;
  }
};
// for averaged types
template<typename HistogramType>
struct avghistogram_pickle
{
  static py::tuple getstate(py::object self)
  {
    return py::make_tuple(
        self.attr("params"),
        self.attr("bins"),
        self.attr("delta"),
        self.attr("off_chart"),
        self.attr("num_histograms")
        ) ;
  }
  static void setstate(py::object self, py::tuple t)
  {
    check_pickle_tuple_size(py::len(t), 5);
    HistogramType & histogram = self.cast<HistogramType&>();

    new (&histogram) HistogramType(t[0].cast<tpy::UniformBinsHistogramParams>()) ;

    // restore bins, delta & off_chart
    histogram.bins = t[1].cast<Eigen::Matrix<typename HistogramType::CountType, Eigen::Dynamic, 1> >();
    histogram.delta = t[2].cast<Eigen::Matrix<typename HistogramType::CountType, Eigen::Dynamic, 1> >();
    histogram.off_chart = t[3].cast<typename HistogramType::CountType>();
    histogram.num_histograms = t[4].cast<int>();
  }
};



static inline std::string fmt_hist_param_float(RealType val)
{
  return Tomographer::Tools::fmts("%.3g", (double)val);
}



void py_tomo_histogram(py::module rootmodule)
{
  auto logger = Tomographer::Logger::makeLocalLogger(TOMO_ORIGIN, *tpy::logger);

  logger.debug("py_tomo_histogram() ...");

  logger.debug("UniformBinsHistogramParams...");

  // CLASS: UniformBinsHistogramParams
  {
    typedef tpy::UniformBinsHistogramParams Kl;
    py::class_<tpy::UniformBinsHistogramParams>(
        rootmodule,
        "UniformBinsHistogramParams",
        Tomographer::Tools::fmts(
            "Specify histogram bins parameters: the minimum value, the maximum value, and the number "
            "of bins. The interval `[min,max[` is split into `num_bins` equally spaced bins."
            "\n\n"
            "|picklable|"
            "\n\n"
            ".. seealso:: This python class interfaces its corresponding "
            ":tomocxx:`C++ class Tomographer::UniformBinsHistogramParams "
            "<struct_tomographer_1_1_uniform_bins_histogram_params.html>`, with the template parameter "
            "`Scalar=%s`.\n\n"
            "\n\n"
            ".. py:function:: UniformBinsHistogramParams(min=%.1f, max=%.1f, num_bins=%d)"
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
            "    Read-only attribute returning a vector (numpy array) of values corresponding to each bin center value."
            "\n\n"
            ".. py:attribute:: values_lower"
            "\n\n"
            "    Read-only attribute returning a vector (numpy array) of values corresponding to each bin lower value."
            "\n\n"
            ".. py:attribute:: values_upper"
            "\n\n"
            "    Read-only attribute returning a vector (numpy array) of values corresponding to each bin upper value."
            "\n\n"
            "\n\n", boost::core::demangle(typeid(RealType).name()).c_str(),
            Kl().min, Kl().max, (int)Kl().num_bins).c_str())
      .def(py::init<RealType,RealType,std::size_t>(),
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
          return streamstr("UniformBinsHistogramParams(min="
                           << fmt_hist_param_float(p.min) << ",max="
                           << fmt_hist_param_float(p.max) << ",num_bins=" << p.num_bins << ")");
        })
      .def("__getstate__", [](const Kl& p) {
          return py::make_tuple(p.min, p.max, p.num_bins) ;
        })
      .def("__setstate__", [](Kl & p, py::tuple t) {
          tpy::internal::unpack_tuple_and_construct<Kl, RealType,RealType,std::size_t>(p, t);
        })
      ;
  }

  logger.debug("UniformBinsHistogram...");

  // CLASS: UniformBinsHistogram
  {
    typedef tpy::UniformBinsHistogram Kl;
    py::class_<tpy::UniformBinsHistogram>(
        rootmodule,
        "UniformBinsHistogram",
        Tomographer::Tools::fmts(
            "A histogram object.  An interval `[min,max]` is divided into `num_bins` bins, each of same width. "
            "Each time a new value is to be recorded, the corresponding bin's counter is incremented."
            "\n\n"
            "|picklable|"
            "\n\n"
            ".. seealso:: This python class interfaces the C++ class :tomocxx:`Tomographer::UniformBinsHistogramParams "
            "<class_tomographer_1_1_uniform_bins_histogram.html>`, with the template parameters "
            "`Scalar=%s` and `CountType=%s`.  See the C++ class documentation for more information."
            "\n\n"
            ".. py:function:: UniformBinsHistogram([params=UniformBinsHistogramParams()])\n\n"
            "    Construct a new histogram object with the given histogram parameters.\n\n"
            ".. py:function:: UniformBinsHistogram(min, max, num_bins)\n\n"
            "    Alternative call syntax; the effect is the same as the other constructor.\n\n"
            ".. py:attribute:: params\n\n"
            "    The :py:class:`UniformBinsHistogramParams` object which stores the current histogram "
            "parameters.  This attribute is read-only.  The parameters are specified to the constructor, and "
            "cannot be changed.\n\n"
            ".. py:attribute:: values_center\n\n    A shorthand for `params.values_center`. "
            "See :py:class:`UniformBinsHistogramParams`.\n\n"
            ".. py:attribute:: values_lower\n\n    A shorthand for `params.values_lower`. "
            "See :py:class:`UniformBinsHistogramParams`.\n\n"
            ".. py:attribute:: values_upper\n\n    A shorthand for `params.values_upper`. "
            "See :py:class:`UniformBinsHistogramParams`.\n\n"
            ".. py:attribute:: bins\n\n"
            "    The histogram bin counts, interfaced as a `NumPy` array object storing integers.  This attribute "
            "is readable and writable, although you may not change the size or type of the array.\n\n"
            ".. py:attribute:: off_chart\n\n"
            "    The number of recorded data points which were beyond the histogram range `[params.min, params.max[`.\n\n"
            ".. py:attribute:: UniformBinsHistogram.HasErrorBars\n\n"
            "    This is a class attribute, i.e. is accessed as `UniformBinsHistogram.HasErrorBars`, and is set to the "
            "constant value `False`.\n\n",
            boost::core::demangle(typeid(RealType).name()).c_str(),
            boost::core::demangle(typeid(CountIntType).name()).c_str()
            ).c_str(),
        py::metaclass()
        )
      .def(py::init<tpy::UniformBinsHistogramParams>(), "params"_a = tpy::UniformBinsHistogramParams())
      .def(py::init<RealType, RealType, std::size_t>(),
          "min"_a, "max"_a, "num_bins"_a)
      .def_property_readonly("params", [](const Kl & h) -> Kl::Params { return h.params; })
      .def_property_readonly("values_center", [](const Kl & p) -> tpy::RealVectorType { return p.params.valuesCenter(); })
      .def_property_readonly("values_lower", [](const Kl & p) -> tpy::RealVectorType { return p.params.valuesLower(); })
      .def_property_readonly("values_upper", [](const Kl & p) -> tpy::RealVectorType { return p.params.valuesUpper(); })
      .def_property("bins", [](const Kl & h) -> tpy::CountIntVectorType { return h.bins.matrix(); },
                    [](Kl & h, const tpy::CountIntVectorType& v) { h.bins = v; })
      .def_readwrite("off_chart", & Kl::off_chart )
      .def_property_readonly_static("HasErrorBars", [](py::object) { return false; })
      .def("reset", & Kl::reset,
           "reset()\n\n"
           "Clears the current histogram counts (including `off_chart` counts) to zero.  The histogram "
           "parameters in `params` are kept intact.")
      .def("load", [](Kl & h, const tpy::CountIntVectorType& x, CountIntType o) { h.load(x, o); },
           "bins"_a, "off_chart"_a = CountIntType(0),
           "load(bins[, off_chart=0])\n\n"
           "Load bin values from the vector of values `bins`, which is expected to be a `NumPy` array. If "
           "`off_chart` is specified, the current `off_chart` count is also set to the given value; otherwise "
           "it is reset to zero.")
      .def("add", [](Kl & h, const tpy::CountIntVectorType& x, CountIntType o) { h.add(x.array(), o); },
           "bins"_a, "off_chart"_a = CountIntType(0),
           "add(bins[, off_chart=0])\n\n"
           "Add a number of counts to each bin, specifed by a vector of values `bins` which is expected to be "
           "a `NumPy` array. If `off_chart` is specified, the current `off_chart` count is increased by this number, "
           "otherwise it is left to its current value.")
      .def("numBins", & Kl::numBins,
           "numBins()\n\n"
           "A shorthand for `params.num_bins`.  See :py:class:`UniformBinsHistogramParams`.")
      .def("count", & Kl::count, "index"_a,
           "count(index)\n\n"
           "Returns the number of counts in the bin indexed by `index`.  Indexes start at zero.  "
           "Raises :py:exc:`TomographerCxxError` if index is out of range.")
      .def("record", [](Kl & h, RealType x, CountIntType o) { return h.record(x, o); },
           "value"_a, "weight"_a = CountIntType(1),
           "record(value[, weight=1])\n\n"
           "Record a new data sample. This increases the corresponding bin count by one, or by `weight` if the "
           "latter argument is provided.")
      .def("normalization", [](const Kl & h) { return h.normalization<RealType>(); },
           "normalization()\n\n"
           "Calculate the normalization factor for the histogram.  This corresponds to the total number "
           "of weight-1 data points, where the weight of a data point may be specified as a second argument "
           "to :py:meth:`record()`.")
      .def("normalized", [](const Kl & h) -> tpy::UniformBinsRealHistogram { return h.normalized<RealType>(); },
           "normalized()\n\n"
           "Returns a normalized version of this histogram. The bin counts as well as the off_chart counts "
           "are divided by :py:meth:`normalization()`.  The returned object is a "
           ":py:class:`UniformBinsRealHistogram` instance.")
      .def("prettyPrint", & Kl::prettyPrint, "max_width"_a = 0,
           "prettyPrint([max_width=0])\n\n"
           "Produce a human-readable representation of the histogram, with horizontal visual bars, which is "
           "suitable to be displayed in a terminal (for instance).  The formatted histogram is returned as a "
           "string.  If `max_width` is specified and nonzero, the output is designed to fit into a terminal "
           "display of the given number of characters wide, otherwise a default width is used.")
      .def("__repr__", [](const Kl& p) {
          return streamstr("UniformBinsHistogram(min="
                           << fmt_hist_param_float(p.params.min) << ",max="
                           << fmt_hist_param_float(p.params.max) << ",num_bins=" << p.params.num_bins << ")");
        })
      .def("__getstate__", histogram_pickle<Kl>::getstate)
      .def("__setstate__", histogram_pickle<Kl>::setstate)
      ;
  }

  logger.debug("UniformBinsRealHistogram...");

  // CLASS: UniformBinsRealHistogram
  {
    typedef tpy::UniformBinsRealHistogram Kl;
    py::class_<tpy::UniformBinsRealHistogram>(
        rootmodule,
        "UniformBinsRealHistogram",
        Tomographer::Tools::fmts(
            "A histogram (with uniform bin size), with a real count type. This class is basically a copy of "
            ":py:class:`UniformBinsHistogram`, except that each bin's count is a real value.  (This allows, "
            "for example, the histogram to be normalized.) Every method documented in "
            ":py:class:`UniformBinsHistogram` is available to this class as well."
            "\n\n"
            "|picklable|"
            "\n\n"
            "The corresponding C++ class is also :tomocxx:`Tomographer::UniformBinsHistogram "
            "<class_tomographer_1_1_uniform_bins_histogram.html>`, although the `CountType` template parameter "
            "is set to `%s` instead of `%s`.", boost::core::demangle(typeid(RealType).name()).c_str(),
            boost::core::demangle(typeid(CountIntType).name()).c_str()
            ).c_str(),
        py::metaclass()
        )
      .def(py::init<tpy::UniformBinsHistogramParams>(), "params"_a = tpy::UniformBinsHistogramParams())
      .def(py::init<RealType, RealType, std::size_t>(),
          "min"_a, "max"_a, "num_bins"_a)
      .def_property_readonly("params", [](const Kl & h) -> Kl::Params { return h.params; })
      .def_property_readonly("values_center", [](const Kl & p) -> tpy::RealVectorType { return p.params.valuesCenter(); })
      .def_property_readonly("values_lower", [](const Kl & p) -> tpy::RealVectorType { return p.params.valuesLower(); })
      .def_property_readonly("values_upper", [](const Kl & p) -> tpy::RealVectorType { return p.params.valuesUpper(); })
      .def_property("bins", [](const Kl & h) -> tpy::RealVectorType { return h.bins.matrix(); },
                    [](Kl & h, const tpy::RealVectorType& v) { h.bins = v; })
      .def_readwrite("off_chart", & Kl::off_chart)
      .def_property_readonly_static("HasErrorBars", [](py::object) { return false; })
      .def("reset", & Kl::reset)
      .def("load", [](Kl & h, const tpy::RealVectorType& x, RealType o) { h.load(x, o); },
           "bins"_a, "off_chart"_a = CountIntType(0))
      .def("add", [](Kl & h, const tpy::RealVectorType& x, RealType o) { h.add(x.array(), o); },
           "bins"_a, "off_chart"_a = CountIntType(0))
      .def("numBins", & Kl::numBins)
      .def("count", & Kl::count, "index"_a)
      .def("record", [](Kl & h, RealType x, RealType o) { return h.record(x, o); },
           "value"_a, "weight"_a=RealType(1))
      .def("normalization", [](const Kl & h) { return h.normalization<RealType>(); })
      .def("normalized", [](const Kl & h) -> tpy::UniformBinsRealHistogram { return h.normalized<RealType>(); })
      .def("prettyPrint", &Kl::prettyPrint, "max_width"_a=0)
      .def("__repr__", [](const Kl& p) {
          return streamstr("UniformBinsRealHistogram(min="
                           << fmt_hist_param_float(p.params.min) << ",max="
                           << fmt_hist_param_float(p.params.max) << ",num_bins=" << p.params.num_bins << ")");
        })
      .def("__getstate__", histogram_pickle<Kl>::getstate)
      .def("__setstate__", histogram_pickle<Kl>::setstate)
      ;
  }
  
  logger.debug("UniformBinsHistogramWithErrorBars...");

  // UniformBinsHistogramWithErrorBars
  {
    typedef tpy::UniformBinsHistogramWithErrorBars Kl;
    py::class_<tpy::UniformBinsHistogramWithErrorBars,tpy::UniformBinsRealHistogram>(
        rootmodule,
        "UniformBinsHistogramWithErrorBars",
        "A histogram (with uniform bin size), with a real count type and with error bars associated to"
        " each bin."
        "\n\n"
        "This class internally inherits :py:class:`UniformBinsRealHistogram`, and all those methods are "
        "exposed in this class, except for `add()`. In addition, the `reset()` method also clears the "
        "error bar values, and the `normalized()` method returns a histogram with the appropriate error "
        " bars on the normalized histogram."
        "\n\n"
        "|picklable|"
        "\n\n"
        "In addition to the members inherited from :py:class:`UniformBinsRealHistogram`, the following "
        "members are available:"
        "\n\n"
        ".. py:attribute:: delta\n\n"
        "    The error bar values on each of the histogram bin counts, interfaced as a `NumPy` array object "
        "storing real values.  This attribute is readable and writable, although you may not change the "
        "size or type of the array.\n\n",
        py::metaclass()
        )
      .def(py::init<tpy::UniformBinsHistogramParams>(), "params"_a=tpy::UniformBinsHistogramParams())
      .def(py::init<RealType, RealType, std::size_t>())
      .def_property_readonly("params", [](const Kl & h) -> Kl::Params { return h.params; })
      .def_property_readonly("values_center", [](const Kl & p) -> tpy::RealVectorType { return p.params.valuesCenter(); })
      .def_property_readonly("values_lower", [](const Kl & p) -> tpy::RealVectorType { return p.params.valuesLower(); })
      .def_property_readonly("values_upper", [](const Kl & p) -> tpy::RealVectorType { return p.params.valuesUpper(); })
      .def_property("bins", [](const Kl & h) -> tpy::RealVectorType { return h.bins.matrix(); },
                    [](Kl & h, const tpy::RealVectorType v) { h.bins = v; })
      .def_property("delta", [](const Kl & h) -> tpy::RealVectorType { return h.delta.matrix(); },
                    [](Kl & h, const tpy::RealVectorType v) { h.delta = v; })
      .def_readwrite("off_chart", & Kl::off_chart)
      .def_property_readonly_static("HasErrorBars", [](py::object) { return true; })
      .def("reset", & Kl::reset)
      .def("load",
           [](Kl & h, const tpy::RealVectorType& x, const tpy::RealVectorType& err, RealType o) {
             h.load(x, err, o);
           },
           "y"_a, "yerr"_a, "off_chart"_a = RealType(0),
           "load(y, yerr[, off_chart=0])"
           "\n\n"
           "Load data into the histogram. The array `y` specifies the bin counts, and `yerr` specifies "
           "the error bars on those bin counts.  The off-chart counter is set to `off_chart`."
          )
      .def("add", [](Kl & , py::args, py::kwargs) {
          throw std::runtime_error("May not call add() on UniformBinsHistogramWithErrorBars");
        })
// add() makes no sense, user should use AveragedHistogram for that.
      .def("numBins", & Kl::numBins)
      .def("count", & Kl::count, "index"_a)
      .def("errorBar", & Kl::errorBar, "index"_a,
          "errorBar(index)\n\n"
          "Get the error bar value associated to the bin of the given `index`. Raises "
           ":py:exc:`TomographerCxxError` if index is out of range.")
      .def("record", [](Kl & h, RealType x, RealType o) { return h.record(x, o); },
           "value"_a, "weight"_a=RealType(1))
      .def("normalization", [](const Kl & h) { return h.normalization<RealType>(); })
      .def("normalized",
           [](const Kl & h) -> tpy::UniformBinsHistogramWithErrorBars { return h.normalized<RealType>(); },
           "normalized()\n\n"
           "Returns a normalized version of this histogram, including the error bars. The bin counts, the "
           "error bars and the off_chart counts are divided by :py:meth:`~UniformBinsHistogram.normalization()`. "
           " The returned object is again a :py:class:`UniformBinsHistogramWithErrorBars` instance.")
      .def("prettyPrint", & Kl::prettyPrint, "max_width"_a = 0)
      .def("__repr__", [](const Kl& p) {
          return streamstr("UniformBinsHistogramWithErrorBars(min="
                           << fmt_hist_param_float(p.params.min) << ",max="
                           << fmt_hist_param_float(p.params.max) << ",num_bins=" << p.params.num_bins << ")");
        })
      .def("__getstate__", histogram_pickle<Kl>::getstate)
      .def("__setstate__", histogram_pickle<Kl>::setstate)
      ;
  }

  logger.debug("AveragedSimpleHistogram...");
  // AveragedSimpleHistogram
  {
    typedef tpy::AveragedSimpleHistogram Kl;
    py::class_<tpy::AveragedSimpleHistogram, tpy::UniformBinsHistogramWithErrorBars>(
        rootmodule,
        "AveragedSimpleHistogram",
        "A :py:class:`UniformBinsHistogramWithErrorBars` which results from the "
        "averaging of several :py:class:`UniformBinsHistogram` histograms."
        "\n\n"
        "Add histograms to average together using the :py:meth:`addHistogram()` method, and "
        "then call :py:meth:`finalize()`. Then, the data stored in the current object will "
        "correspond to the averaged histogram. (See :tomocxx:`here the theory about how this is "
        "implemented <page_theory_averaged_histogram.html>`.)"
        "\n\n"
        "This histogram object inherits :py:class:`UniformBinsHistogramWithErrorBars`, so all the "
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
      .def(py::init<tpy::UniformBinsHistogramParams>(), "params"_a = tpy::UniformBinsHistogramParams())
      .def(py::init<RealType, RealType, CountIntType>(),
           "min"_a, "max"_a, "num_bins"_a)
      .def_property_readonly("num_histograms", [](const Kl & h) { return h.num_histograms; })
      .def("addHistogram",
           [](Kl & h, const tpy::UniformBinsHistogram & o) { h.addHistogram(o); },
           "histogram"_a,
           "addHistogram(histogram)\n\n"
           "Add a new histogram to the average with the others.")
      .def("reset",
           [](Kl & h) { h.reset(); }
          )
      .def("reset",
           [](Kl & h, const tpy::UniformBinsHistogramParams & param) { h.reset(param); },
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
    py::class_<tpy::AveragedSimpleRealHistogram,tpy::UniformBinsHistogramWithErrorBars>(
        rootmodule,
        "AveragedSimpleRealHistogram",
        "A :py:class:`UniformBinsHistogramWithErrorBars` which results from the "
        "averaging of several :py:class:`UniformBinsRealHistogram` histograms."
        "\n\n"
        "This class is identical in functionality to :py:class:`AveragedSimpleHistogram`, except "
        "that the histograms which are to be averaged are :py:class:`UniformBinsRealHistogram` "
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
      .def(py::init<tpy::UniformBinsHistogramParams>(), "params"_a = tpy::UniformBinsHistogramParams())
      .def(py::init<RealType, RealType, CountIntType>(),
          "min"_a, "max"_a, "num_bins"_a)
      .def_property_readonly("num_histograms", [](const Kl & h) { return h.num_histograms; })
      .def("addHistogram",
           [](Kl & h, const tpy::UniformBinsRealHistogram & o) { h.addHistogram(o); },
           "histogram"_a,
           "addHistogram(histogram)\n\n"
           "Add a new histogram to the average with the others.")
      .def("reset",
           [](Kl & h) { h.reset(); }
          )
      .def("reset",
           [](Kl & h, const tpy::UniformBinsHistogramParams & param) { h.reset(param); },
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
    py::class_<tpy::AveragedErrorBarHistogram,tpy::UniformBinsHistogramWithErrorBars>(
        rootmodule,
        "AveragedErrorBarHistogram",
        "A :py:class:`UniformBinsHistogramWithErrorBars` which results from the "
        "averaging of several :py:class:`UniformBinsHistogramWithErrorBars` histograms."
        "\n\n"
        "This class is essentially identical in functionality to "
        ":py:class:`AveragedSimpleHistogram` and :py:class:`AveragedSimpleRealHistogram`, except "
        "that the histograms which are to be averaged are "
        " :py:class:`UniformBinsHistogramWithErrorBars` "
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
      .def(py::init<tpy::UniformBinsHistogramParams>(), "params"_a = tpy::UniformBinsHistogramParams())
      .def(py::init<RealType, RealType, CountIntType>(),
          "min"_a, "max"_a, "num_bins"_a)
      .def_property_readonly("num_histograms", [](const Kl & h) { return h.num_histograms; })
      .def("addHistogram",
           [](Kl & h, const tpy::UniformBinsHistogramWithErrorBars & o) {
             h.addHistogram(o);
           },
           "histogram"_a,
           "addHistogram(histogram)\n\n"
           "Add a new histogram to the average with the others.")
      .def("reset",
           [](Kl & h) { h.reset(); }
          )
      .def("reset",
           [](Kl & h, const tpy::UniformBinsHistogramParams & param) { h.reset(param); },
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

  logger.debug("py_tomo_histogram() completed.");
}
