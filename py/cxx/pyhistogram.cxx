
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
           "    The number of bins the range `[min,max]` is divided into, defining the bins."
           " (Read-write attribute)"
           "\n\n"
           , Kl().min, Kl().max, (int)Kl().num_bins).c_str())
      .def(boost::python::init<boost::python::optional<RealType,RealType,std::size_t> >())
      //   , (arg("min")=Kl().min, arg("max")=Kl().max, arg("num_bins")=Kl().num_bins))
      .add_property("min", +[](const Kl & p) { return p.min; }, +[](Kl & p, RealType min) { p.min = min; })
      .add_property("max", +[](const Kl & p) { return p.max; }, +[](Kl & p, RealType max) { p.max = max; })
      .add_property("num_bins", +[](const Kl & p) { return p.num_bins; },
                    +[](Kl & p, std::size_t n) { p.num_bins = n; })
      .def("isWithinBounds", &Kl::isWithinBounds, (arg("value")),
           "isWithinBounds(value)"
           "\n\n"
           "Check whether the given `value` is within the bounds of the histogram, that is, in the"
           " range `[min, max[`."
           "\n\n"
           ":param value: the requested value on the x-axis")
      .def("binIndex", &Kl::binIndex, (arg("value")),
           "UniformBinsHistogramParams.binIndex(value)"
           "\n\n"
           "Get the index of the bin in which the given value would be saved in. Indexes are of "
           "course zero-based.")
      .def("binLowerValue", &Kl::binLowerValue, (arg("index")))
      .def("binCenterValue", &Kl::binCenterValue, (arg("index")))
      .def("binUpperValue", &Kl::binUpperValue, (arg("index")))
      .def("binResolution", &Kl::binResolution)
      .add_property("values_center", +[](Kl & p) -> Eigen::VectorXd { return p.valuesCenter(); })
      .add_property("values_lower", +[](Kl & p) -> Eigen::VectorXd { return p.valuesLower(); })
      .add_property("values_upper", +[](Kl & p) -> Eigen::VectorXd { return p.valuesUpper(); })
      ;
  }

  logger.debug("UniformBinsHistogram...");

  // CLASS: UniformBinsHistogram
  {
    typedef Py::UniformBinsHistogram Kl;
    boost::python::scope cl = boost::python::class_<Py::UniformBinsHistogram>(
        "UniformBinsHistogram",
        "A histogram (with uniform bin size).  An interval `[min,max]` is divided into `num_bins` uniform "
        "bins. Each time a new value is to be recorded, the corresponding bin's counter is incremented. "
        "See the C++ class doc for more information."
        )
      .def(boost::python::init<boost::python::optional<Py::UniformBinsHistogramParams> >())
      .def(boost::python::init<RealType, RealType, std::size_t>())
      .def("reset", &Kl::reset)
      .def("load", +[](Kl & h, const Eigen::VectorXi& x) { h.load(x); })
      .def("load", +[](Kl & h, const Eigen::VectorXi& x, CountIntType o) { h.load(x, o); })
      .def("add", +[](Kl & h, const Eigen::VectorXi& x) { h.add(x.array()); })
      .def("add", +[](Kl & h, const Eigen::VectorXi& x, CountIntType o) { h.add(x.array(), o); })
      .def("numBins", &Kl::numBins)
      .def("count", &Kl::count)
      .def("record", +[](Kl & h, RealType x) { return h.record(x); })
      .def("record", +[](Kl & h, RealType x, CountIntType o) { return h.record(x, o); })
      .def("prettyPrint", &Kl::prettyPrint)
      .def("prettyPrint", +[](Kl & h) { return h.prettyPrint(); })
      .add_property("params", boost::python::make_function(+[](Kl & h) -> const Kl::Params& { return h.params; },
                                                           boost::python::return_internal_reference<>()))
      .add_property("values_center", +[](Kl & p) -> Eigen::VectorXd { return p.params.valuesCenter(); })
      .add_property("values_lower", +[](Kl & p) -> Eigen::VectorXd { return p.params.valuesLower(); })
      .add_property("values_upper", +[](Kl & p) -> Eigen::VectorXd { return p.params.valuesUpper(); })
      .add_property("bins", +[](Kl & h) -> Eigen::VectorXi { return h.bins.matrix(); },
                    +[](Kl & h, const Eigen::VectorXi& v) { h.bins = v; })
      .add_property("off_chart", +[](Kl & h) { return h.off_chart; },
                    +[](Kl & h, CountIntType o) { h.off_chart = o; })
      .add_static_property("HasErrorBars", +[](Kl &) { return false; })
      ;

    //    logger.debug("(A)");
  }

  logger.debug("UniformBinsRealHistogram...");

  // CLASS: UniformBinsRealHistogram
  {
    typedef Py::UniformBinsRealHistogram Kl;
    boost::python::scope cl = boost::python::class_<Py::UniformBinsRealHistogram>(
        "UniformBinsRealHistogram",
        "A histogram (with uniform bin size), with a real count type. This class is similar to "
        ":py:class:`UniformBinsHistogram`, except that each bin's count is a real value, allowing "
        "e.g. the histogram to be normalized."
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
      .add_static_property("HasErrorBars", +[](Kl &) { return false; })
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
      .def("errorBar", &Kl::errorBar)
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
      .add_static_property("HasErrorBars", +[](Kl &) { return true; })
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
                                )
      .def(boost::python::init<boost::python::optional<Py::UniformBinsHistogramParams> >())
      .def("addHistogram",
           +[](Kl & h, const Py::UniformBinsHistogram & o) { h.addHistogram(o); })
      .def("finalize", +[](Kl & h) { h.finalize(); })
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
                                )
      .def(boost::python::init<boost::python::optional<Py::UniformBinsHistogramParams> >())
      .def("addHistogram",
           +[](Kl & h, const Py::UniformBinsRealHistogram & o) { h.addHistogram(o); })
      .def("finalize", +[](Kl & h) { h.finalize(); })
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
                                )
      .def(boost::python::init<boost::python::optional<Py::UniformBinsHistogramParams> >())
      .def("addHistogram",
           +[](Kl & h, const Py::UniformBinsHistogramWithErrorBars & o) {
             h.addHistogram(o);
           })
      .def("finalize", +[](Kl & h) { h.finalize(); })
      ;
  }

  logger.debug("py_tomo_histogram() completed.");
}
