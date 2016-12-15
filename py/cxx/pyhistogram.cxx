
#include "tomographerpy/common.h"
#include "tomographerpy/pyhistogram.h"

#include "common_p.h"


void py_tomo_histogram()
{
  auto logger = Tomographer::Logger::makeLocalLogger(TOMO_ORIGIN, tpy_logger);

  using boost::python::arg;

  logger.debug("py_tomo_histogram() ...");

  logger.debug("UniformBinsHistogramParams...");

  // documentation docstrings are defined in tomographer._docstrings

  // CLASS: UniformBinsHistogramParams
  {
    typedef Py::UniformBinsHistogramParams Kl;
    boost::python::class_<Py::UniformBinsHistogramParams>("UniformBinsHistogramParams", Tomographer::Tools::fmts(
           "Specify histogram bins parameters: the minimum value, the maximum value, and the number "
           "of bins. The interval `[min,max[` is split into `num_bins` equally spaced bins."
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
      .def(boost::python::init<boost::python::optional<RealType,RealType,std::size_t> >())
      //   , (arg("min")=Kl().min, arg("max")=Kl().max, arg("num_bins")=Kl().num_bins))
      .add_property("min", +[](const Kl & p) { return p.min; }, +[](Kl & p, RealType min) { p.min = min; })
      .add_property("max", +[](const Kl & p) { return p.max; }, +[](Kl & p, RealType max) { p.max = max; })
      .add_property("num_bins", +[](const Kl & p) { return p.num_bins; },
                    +[](Kl & p, std::size_t n) { p.num_bins = n; })
      .add_property("values_center", +[](Kl & p) -> Eigen::VectorXd { return p.valuesCenter(); })
      .add_property("values_lower", +[](Kl & p) -> Eigen::VectorXd { return p.valuesLower(); })
      .add_property("values_upper", +[](Kl & p) -> Eigen::VectorXd { return p.valuesUpper(); })
      .def("isWithinBounds", &Kl::isWithinBounds, (arg("value")),
           "isWithinBounds(value)"
           "\n\n"
           "Check whether the given `value` is within the bounds of the histogram, that is, in the"
           " range `[min, max[`.")
      .def("binIndex", &Kl::binIndex, (arg("value")),
           "binIndex(value)"
           "\n\n"
           "Get the index of the bin in which the given value would be saved in. Indexes are of "
           "course zero-based.")
      .def("binLowerValue", &Kl::binLowerValue, (arg("index")),
           "binLowerValue(index)\n\n"
           "Returns the value which a given bin index represents (lower bin value limit).\n\n"
           "Raise a :py:exc:`TomographerCxxError` if the index is invalid.")
      .def("binCenterValue", &Kl::binCenterValue, (arg("index")),
           "binCenterValue(index)\n\n"
           "Returns the value which a given bin index represents (center bin value).")
      .def("binUpperValue", &Kl::binUpperValue, (arg("index")),
           "binUpperValue(index)\n\n"
           "Returns the value which a given bin index represents (upper bin value limit).")
      .def("binResolution", &Kl::binResolution,
           "binResolution()\n\n"
           "Returns the width of a bin.  This is simply :math:`(\\mathit{max} - \\mathit{min})/\\mathit{num_bins}`.")
      ;
  }

  logger.debug("UniformBinsHistogram...");

  // CLASS: UniformBinsHistogram
  {
    typedef Py::UniformBinsHistogram Kl;
    boost::python::scope cl = boost::python::class_<Py::UniformBinsHistogram>(
        "UniformBinsHistogram",  Tomographer::Tools::fmts(
            "A histogram object.  An interval `[min,max]` is divided into `num_bins` bins, each of same width. "
            "Each time a new value is to be recorded, the corresponding bin's counter is incremented."
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
            ).c_str()
        )
      .def(boost::python::init<boost::python::optional<Py::UniformBinsHistogramParams> >())
      .def(boost::python::init<RealType, RealType, std::size_t>())
      .add_property("params", boost::python::make_function(+[](Kl & h) -> const Kl::Params& { return h.params; },
                                                           boost::python::return_internal_reference<>()))
      .add_property("values_center", +[](Kl & p) -> Eigen::VectorXd { return p.params.valuesCenter(); })
      .add_property("values_lower", +[](Kl & p) -> Eigen::VectorXd { return p.params.valuesLower(); })
      .add_property("values_upper", +[](Kl & p) -> Eigen::VectorXd { return p.params.valuesUpper(); })
      .add_property("bins", +[](Kl & h) -> Eigen::VectorXi { return h.bins.matrix(); },
                    +[](Kl & h, const Eigen::VectorXi& v) { h.bins = v; })
      .add_property("off_chart", +[](Kl & h) { return h.off_chart; },
                    +[](Kl & h, CountIntType o) { h.off_chart = o; })
      .add_static_property("HasErrorBars", +[]() { return false; })
      .def("reset", &Kl::reset,
           "reset()\n\n"
           "Clears the current histogram counts (including `off_chart` counts) to zero.  The histogram "
           "parameters in `params` are kept intact.")
      //.def("load", +[](Kl & h, const Eigen::VectorXi& x) { h.load(x); })
      .def("load", +[](Kl & h, const Eigen::VectorXi& x, CountIntType o) { h.load(x, o); },
           (arg("bins"), arg("off_chart") = CountIntType(0)),
           "load(bins[, off_chart=0])\n\n"
           "Load bin values from the vector of values `bins`, which is expected to be a `NumPy` array. If "
           "`off_chart` is specified, the current `off_chart` count is also set to the given value; otherwise "
           "it is reset to zero.")
      //.def("add", +[](Kl & h, const Eigen::VectorXi& x) { h.add(x.array()); })
      .def("add", +[](Kl & h, const Eigen::VectorXi& x, CountIntType o) { h.add(x.array(), o); },
           (arg("bins"), arg("off_chart") = CountIntType(0)),
           "add(bins[, off_chart=0])\n\n"
           "Add a number of counts to each bin, specifed by a vector of values `bins` which is expected to be "
           "a `NumPy` array. If `off_chart` is specified, the current `off_chart` count is increased by this number, "
           "otherwise it is left to its current value.")
      .def("numBins", &Kl::numBins,
           "numBins()\n\n"
           "A shorthand for `params.num_bins`.  See :py:class:`UniformBinsHistogramParams`.")
      .def("count", &Kl::count,
           "count(index)\n\n"
           "Returns the number of counts in the bin indexed by `index`.  Indexes start at zero.  "
           "Raises :py:exc:`TomographerCxxError` if index is out of range.")
      //.def("record", +[](Kl & h, RealType x) { return h.record(x); })
      .def("record", +[](Kl & h, RealType x, CountIntType o) { return h.record(x, o); },
           (arg("value"), arg("weight") = CountIntType(1)),
           "record(value[, weight=1])\n\n"
           "Record a new data sample. This increases the corresponding bin count by one, or by `weight` if the "
           "latter argument is provided.")
      //.def("prettyPrint", +[](Kl & h) { return h.prettyPrint(); })
      .def("prettyPrint", &Kl::prettyPrint,
           (arg("max_width")=0),
           "prettyPrint([max_width=0])\n\n"
           "Produce a human-readable representation of the histogram, with horizontal visual bars, which is "
           "suitable to be displayed in a terminal (for instance).  The formatted histogram is returned as a "
           "string.  If `max_width` is specified and nonzero, the output is designed to fit into a terminal "
           "display of the given number of characters wide, otherwise a default width is used.")
      ;

    //    logger.debug("(A)");
  }

  logger.debug("UniformBinsRealHistogram...");

  // CLASS: UniformBinsRealHistogram
  {
    typedef Py::UniformBinsRealHistogram Kl;
    boost::python::scope cl = boost::python::class_<Py::UniformBinsRealHistogram>(
        "UniformBinsRealHistogram", Tomographer::Tools::fmts(
            "A histogram (with uniform bin size), with a real count type. This class is basically a copy of "
            ":py:class:`UniformBinsHistogram`, except that each bin's count is a real value.  (This allows, "
            "for example, the histogram to be normalized.) Every method documented in "
            ":py:class:`UniformBinsHistogram` is available to this class as well."
            "\n\n"
            "The corresponding C++ class is also :tomocxx:`Tomographer::UniformBinsHistogram "
            "<class_tomographer_1_1_uniform_bins_histogram.html>`, although the `CountType` template parameter "
            "is set to `%s` instead of `%s`.", boost::core::demangle(typeid(RealType).name()).c_str(),
            boost::core::demangle(typeid(CountIntType).name()).c_str()
            ).c_str()
        )
      .def(boost::python::init<boost::python::optional<Py::UniformBinsHistogramParams> >())
      .def(boost::python::init<RealType, RealType, std::size_t>())
      .def("reset", &Kl::reset)
      .def("load", +[](Kl & h, const Eigen::VectorXd& x) { h.load(x); })
      .def("load", +[](Kl & h, const Eigen::VectorXd& x, RealType o) { h.load(x, o); })
      .def("add", +[](Kl & h, const Eigen::VectorXd& x) { h.add(x.array()); })
      .def("add", +[](Kl & h, const Eigen::VectorXd& x, RealType o) { h.add(x.array(), o); })
      .def("numBins", &Kl::numBins)
      .def("count", &Kl::count)
      .def("record", +[](Kl & h, RealType x) { return h.record(x); })
      .def("record", +[](Kl & h, RealType x, RealType o) { return h.record(x, o); })
      .def("prettyPrint", &Kl::prettyPrint)
      .def("prettyPrint", +[](Kl & h) { return h.prettyPrint(); })
      .add_property("params", boost::python::make_function(+[](Kl & h) -> const Kl::Params& { return h.params; },
                                                           boost::python::return_internal_reference<>()))
      .add_property("values_center", +[](Kl & p) -> Eigen::VectorXd { return p.params.valuesCenter(); })
      .add_property("values_lower", +[](Kl & p) -> Eigen::VectorXd { return p.params.valuesLower(); })
      .add_property("values_upper", +[](Kl & p) -> Eigen::VectorXd { return p.params.valuesUpper(); })
      .add_property("bins", +[](Kl & h) -> Eigen::VectorXd { return h.bins.matrix(); },
                    +[](Kl & h, const Eigen::VectorXd& v) { h.bins = v; })
      .add_property("off_chart", +[](Kl & h) { return h.off_chart; },
                    +[](Kl & h, RealType o) { h.off_chart = o; })
      .add_static_property("HasErrorBars", +[]() { return false; })
      ;
  }
  
  logger.debug("UniformBinsHistogramWithErrorBars...");

  // UniformBinsHistogramWithErrorBars
  {
    typedef Py::UniformBinsHistogramWithErrorBars Kl;
    boost::python::scope cl =
      // seems we can't use bases<> because the class isn't polymorphic (no virtual destructor)
      boost::python::class_<Py::UniformBinsHistogramWithErrorBars>(
          "UniformBinsHistogramWithErrorBars",
          "A histogram (with uniform bin size), with a real count type and with error bars associated to"
          " each bin."
          "\n\n"
          "This class internally inherits :py:class:`UniformBinsRealHistogram`, and all those methods are "
          "exposed in this class, except for `add()`. In addition, the `reset()` method also clears the "
          "error bar values."
          "\n\n"
          ".. note:: The `load()` method currently leaves the error bar values unchanged, but this may "
          "change in a future version. In a future version, we will also add a "
          "`load()` overload which accepts also a vector of error bar values (TODO:!)."
          "\n\n"
          "In addition to the members inherited from :py:class:`UniformBinsRealHistogram`, the following "
          "members are available:"
          "\n\n"
          ".. py:attribute:: delta\n\n"
          "    The error bar values on each of the histogram bin counts, interfaced as a `NumPy` array object "
          "storing real values.  This attribute is readable and writable, although you may not change the "
          "size or type of the array.\n\n"
          )
      .def(boost::python::init<boost::python::optional<Py::UniformBinsHistogramParams> >())
      .def(boost::python::init<RealType, RealType, std::size_t>())
      .def("reset", &Kl::reset)
      .def("load", +[](Kl & h, const Eigen::VectorXd& x) { h.load(x); })
      .def("load", +[](Kl & h, const Eigen::VectorXd& x, RealType o) { h.load(x, o); })
      //.def("add", +[](Kl & h, const Eigen::VectorXd& x) { h.add(x.array()); })
      //.def("add", +[](Kl & h, const Eigen::VectorXd& x, RealType o) { h.add(x.array(), o); })
      .def("numBins", &Kl::numBins)
      .def("count", &Kl::count)
      .def("errorBar", &Kl::errorBar,
           (arg("index")),
          "errorBar(index)\n\n"
          "Get the error bar value associated to the bin of the given `index`. Raises "
           ":py:exc:`TomographerCxxError` if index is out of range.")
      .def("record", +[](Kl & h, RealType x) { return h.record(x); })
      .def("record", +[](Kl & h, RealType x, RealType o) { return h.record(x, o); })
      .def("prettyPrint", &Kl::prettyPrint)
      .def("prettyPrint", +[](Kl & h) { return h.prettyPrint(); })
      .add_property("params", boost::python::make_function(+[](Kl & h) -> const Kl::Params& { return h.params; },
                                                           boost::python::return_internal_reference<>()))
      .add_property("values_center", +[](Kl & p) -> Eigen::VectorXd { return p.params.valuesCenter(); })
      .add_property("values_lower", +[](Kl & p) -> Eigen::VectorXd { return p.params.valuesLower(); })
      .add_property("values_upper", +[](Kl & p) -> Eigen::VectorXd { return p.params.valuesUpper(); })
      .add_property("bins", +[](Kl & h) -> Eigen::VectorXd { return h.bins.matrix(); },
                    +[](Kl & h, const Eigen::VectorXd v) { h.bins = v; })
      .add_property("delta", +[](Kl & h) -> Eigen::VectorXd { return h.delta.matrix(); },
                    +[](Kl & h, const Eigen::VectorXd v) { h.delta = v; })
      .add_property("off_chart", +[](Kl & h) { return h.off_chart; },
                    +[](Kl & h, RealType o) { h.off_chart = o; })
      .add_static_property("HasErrorBars", +[]() { return true; })
      ;
  }

  logger.debug("AveragedSimpleHistogram...");
  // AveragedSimpleHistogram
  {
    typedef Py::AveragedSimpleHistogram Kl;
    boost::python::scope cl =
      boost::python::class_<Py::AveragedSimpleHistogram,
                            boost::python::bases<Py::UniformBinsHistogramWithErrorBars>
                            >("AveragedSimpleHistogram",
                              "A :py:class:`UniformBinsHistogramWithErrorBars` which results from the "
                              "averaging of several :py:class:`UniformBinsHistogram` histograms."
                              "\n\n"
                              "Add histograms to average together using the :py:meth:`addHistogram()` method, and "
                              "then call :py:meth:`finalize()`. Then, the data stored in the current object will "
                              "correspond to the averaged histogram. (See :tomocxx:`here the theory about how this is "
                              "implemented <pageTheoryAveragedHistogram.html>`.)"
                              "\n\n"
                              "This histogram object inherits :py:class:`UniformBinsHistogramWithErrorBars`, so all the "
                              "methods exposed in that class are available to access the averaged histogram data."
                              "\n\n"
                              ".. warning:: You must not forget to call `finalize()` before accessing the averaged "
                              "histogram data.  The data stored in the current "
                              "histogram object is UNDEFINED before having calling `finalize()`."
                                )
      .def(boost::python::init<boost::python::optional<Py::UniformBinsHistogramParams> >())
      .def("addHistogram",
           +[](Kl & h, const Py::UniformBinsHistogram & o) { h.addHistogram(o); },
           (arg("histogram")),
           "addHistogram(histogram)\n\n"
           "Add a new histogram to the average with the others.")
      .def("finalize", +[](Kl & h) { h.finalize(); },
           "finalize()\n\n"
           "Call this function after all the histograms have been added with calls to :py:meth:`addHistogram()`. Only "
           "after calling this function may you access the averaged histogram in the current histogram object.")
      ;
  }
  logger.debug("AveragedSimpleRealHistogram...");
  // AveragedSimpleRealHistogram
  {
    typedef Py::AveragedSimpleRealHistogram Kl;
    boost::python::scope cl =
      boost::python::class_<Py::AveragedSimpleRealHistogram,
                            boost::python::bases<Py::UniformBinsHistogramWithErrorBars>
                            >("AveragedSimpleRealHistogram",
                              "A :py:class:`UniformBinsHistogramWithErrorBars` which results from the "
                              "averaging of several :py:class:`UniformBinsRealHistogram` histograms."
                              "\n\n"
                              "This class is identical in functionality to :py:class:`AveragedSimpleHistogram`, except "
                              "that the histograms which are to be averaged are :py:class:`UniformBinsRealHistogram` "
                              "objects."
                              "\n\n"
                              ".. warning:: You must not forget to call `finalize()` before accessing the averaged "
                              "histogram data.  The data stored in the current "
                              "histogram object is UNDEFINED before having calling `finalize()`."
                                )
      .def(boost::python::init<boost::python::optional<Py::UniformBinsHistogramParams> >())
      .def("addHistogram",
           +[](Kl & h, const Py::UniformBinsRealHistogram & o) { h.addHistogram(o); },
           (arg("histogram")),
           "addHistogram(histogram)\n\n"
           "Add a new histogram to the average with the others.")
      .def("finalize", +[](Kl & h) { h.finalize(); },
           "finalize()\n\n"
           "Call this function after all the histograms have been added with calls to :py:meth:`addHistogram()`. Only "
           "after calling this function may you access the averaged histogram in the current histogram object.")
      ;
  }
  logger.debug("AveragedErrorBarHistogram...");
  // AveragedErrorBarHistogram
  {
    typedef Py::AveragedErrorBarHistogram Kl;
    boost::python::scope cl =
      boost::python::class_<Py::AveragedErrorBarHistogram,
                            boost::python::bases<Py::UniformBinsHistogramWithErrorBars>
                            >("AveragedErrorBarHistogram",
                              "A :py:class:`UniformBinsHistogramWithErrorBars` which results from the "
                              "averaging of several :py:class:`UniformBinsHistogramWithErrorBars` histograms."
                              "\n\n"
                              "This class is essentially identical in functionality to "
                              ":py:class:`AveragedSimpleHistogram` and :py:class:`AveragedSimpleRealHistogram`, except "
                              "that the histograms which are to be averaged are "
                              " :py:class:`UniformBinsHistogramWithErrorBars` "
                              "objects, i.e. each histogram added already has information about error bars.  Those "
                              "error bars are then combined appropriately, as described in :tomocxx:`the theory about "
                              "how this class is implemented <pageTheoryAveragedHistogram.html>`."
                              "\n\n"
                              ".. warning:: You must not forget to call `finalize()` before accessing the averaged "
                              "histogram data.  The data stored in the current "
                              "histogram object is UNDEFINED before having calling `finalize()`."
                                )
      .def(boost::python::init<boost::python::optional<Py::UniformBinsHistogramParams> >())
      .def("addHistogram",
           +[](Kl & h, const Py::UniformBinsHistogramWithErrorBars & o) {
             h.addHistogram(o);
           },
           (arg("histogram")),
           "addHistogram(histogram)\n\n"
           "Add a new histogram to the average with the others.")
      .def("finalize", +[](Kl & h) { h.finalize(); },
           "finalize()\n\n"
           "Call this function after all the histograms have been added with calls to :py:meth:`addHistogram()`. Only "
           "after calling this function may you access the averaged histogram in the current histogram object.")
      ;
  }

  logger.debug("py_tomo_histogram() completed.");
}
