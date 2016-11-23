
#include "common.h"

#include "pyhistogram.h"



void py_tomo_histogram()
{
  std::cerr << "py_tomo_histogram() ...\n";

  std::cerr << "UniformBinsHistogramParams...\n";

  // CLASS: UniformBinsHistogramParams
  {
    typedef Py::UniformBinsHistogramParams Kl;
    boost::python::class_<Py::UniformBinsHistogramParams>("UniformBinsHistogramParams")
      .def(boost::python::init<boost::python::optional<RealType,RealType,std::size_t> >())
      .add_property("min", +[](const Kl & p) { return p.min; }, +[](Kl & p, RealType min) { p.min = min; })
      .add_property("max", +[](const Kl & p) { return p.max; }, +[](Kl & p, RealType max) { p.max = max; })
      .add_property("num_bins", +[](const Kl & p) { return p.num_bins; }, +[](Kl & p, std::size_t n) { p.num_bins = n; })
      ;/*      .def("isWithinBounds", &Kl::isWithinBounds)
      .def("binIndex", &Kl::binIndex)
      .def("binLowerValue", &Kl::binLowerValue)
      .def("binCenterValue", &Kl::binCenterValue)
      .def("binUpperValue", &Kl::binUpperValue)
      .def("binResolution", &Kl::binResolution)
      ;*/
  }

  std::cerr << "UniformBinsHistogram...\n";

  // CLASS: UniformBinsHistogram
  {
    typedef Py::UniformBinsHistogram Kl;
    boost::python::scope cl = boost::python::class_<Py::UniformBinsHistogram>("UniformBinsHistogram")
      .def(boost::python::init<boost::python::optional<Py::UniformBinsHistogramParams> >())
      .def(boost::python::init<RealType, RealType, std::size_t>())
      .def("reset", &Kl::reset)
      .def("load", +[](Kl & h, const Eigen::MatrixXi& x) { h.load(x); })
      .def("load", +[](Kl & h, const Eigen::MatrixXi& x, CountIntType o) { h.load(x, o); })
      .def("add", +[](Kl & h, const Eigen::MatrixXi& x) { h.add(x.array()); })
      .def("add", +[](Kl & h, const Eigen::MatrixXi& x, CountIntType o) { h.add(x.array(), o); })
      .def("numBins", &Kl::numBins)
      .def("count", &Kl::count)
      .def("record", +[](Kl & h, RealType x) { return h.record(x); })
      .def("record", +[](Kl & h, RealType x, CountIntType o) { return h.record(x, o); })
      .def("prettyPrint", &Kl::prettyPrint)
      .add_property("params", boost::python::make_function(+[](Kl & h) -> const Kl::Params& { return h.params; },
                                                           boost::python::return_internal_reference<>()))
      .add_property("bins", +[](Kl & h) { return h.bins; },
                    +[](Kl & h, const Eigen::MatrixXi& v) { h.bins = v; })
      .add_property("off_chart", +[](Kl & h) { return h.off_chart; },
                    +[](Kl & h, CountIntType o) { h.off_chart = o; })
      .add_static_property("HasErrorBars", +[](Kl &) { return false; })
      ;

    //    std::cerr << "(A)\n";
  }

  std::cerr << "UniformBinsRealHistogram...\n";

  // CLASS: UniformBinsRealHistogram
  {
    typedef Py::UniformBinsRealHistogram Kl;
    boost::python::scope cl = boost::python::class_<Py::UniformBinsRealHistogram>("UniformBinsRealHistogram")
      .def(boost::python::init<boost::python::optional<Py::UniformBinsHistogramParams> >())
      .def(boost::python::init<RealType, RealType, std::size_t>())
      .def("reset", &Kl::reset)
      .def("load", +[](Kl & h, const Eigen::MatrixXd& x) { h.load(x); })
      .def("load", +[](Kl & h, const Eigen::MatrixXd& x, RealType o) { h.load(x, o); })
      .def("add", +[](Kl & h, const Eigen::MatrixXd& x) { h.add(x.array()); })
      .def("add", +[](Kl & h, const Eigen::MatrixXd& x, RealType o) { h.add(x.array(), o); })
      .def("numBins", &Kl::numBins)
      .def("count", &Kl::count)
      .def("record", +[](Kl & h, RealType x) { return h.record(x); })
      .def("record", +[](Kl & h, RealType x, RealType o) { return h.record(x, o); })
      .def("prettyPrint", &Kl::prettyPrint)
      .add_property("params", boost::python::make_function(+[](Kl & h) -> const Kl::Params& { return h.params; },
                                                           boost::python::return_internal_reference<>()))
      .add_property("bins", +[](Kl & h) { return h.bins; },
                    +[](Kl & h, const Eigen::MatrixXd& v) { h.bins = v; })
      .add_property("off_chart", +[](Kl & h) { return h.off_chart; },
                    +[](Kl & h, RealType o) { h.off_chart = o; })
      .add_static_property("HasErrorBars", +[](Kl &) { return false; })
      ;
  }
  
  std::cerr << "UniformBinsHistogramWithErrorBars...\n";

  // UniformBinsHistogramWithErrorBars
  {
    typedef Py::UniformBinsHistogramWithErrorBars Kl;
    boost::python::scope cl =
      // seems we can't use bases<> because the class isn't polymorphic (no virtual destructor)
      boost::python::class_<Py::UniformBinsHistogramWithErrorBars>("UniformBinsHistogramWithErrorBars")
      .def(boost::python::init<boost::python::optional<Py::UniformBinsHistogramParams> >())
      .def(boost::python::init<RealType, RealType, std::size_t>())
      .def("reset", &Kl::reset)
      .def("load", +[](Kl & h, const Eigen::MatrixXd& x) { h.load(x); })
      .def("load", +[](Kl & h, const Eigen::MatrixXd& x, RealType o) { h.load(x, o); })
      .def("add", +[](Kl & h, const Eigen::MatrixXd& x) { h.add(x.array()); })
      .def("add", +[](Kl & h, const Eigen::MatrixXd& x, RealType o) { h.add(x.array(), o); })
      .def("numBins", &Kl::numBins)
      .def("count", &Kl::count)
      .def("errorBar", &Kl::errorBar)
      .def("record", +[](Kl & h, RealType x) { return h.record(x); })
      .def("record", +[](Kl & h, RealType x, RealType o) { return h.record(x, o); })
      .def("prettyPrint", &Kl::prettyPrint)
      .add_property("params", boost::python::make_function(+[](Kl & h) -> const Kl::Params& { return h.params; },
                                                           boost::python::return_internal_reference<>()))
      .add_property("bins", +[](Kl & h) { return h.bins; },
                    +[](Kl & h, const Eigen::MatrixXd v) { h.bins = v; })
      .add_property("delta", +[](Kl & h) { return h.delta; },
                    +[](Kl & h, const Eigen::MatrixXd v) { h.delta = v; })
      .add_property("off_chart", +[](Kl & h) { return h.off_chart; },
                    +[](Kl & h, RealType o) { h.off_chart = o; })
      .add_static_property("HasErrorBars", +[](Kl &) { return true; })
      ;
  }

  std::cerr << "AveragedSimpleHistogram...\n";
  // AveragedSimpleHistogram
  {
    typedef Py::AveragedSimpleHistogram Kl;
    boost::python::scope cl =
      boost::python::class_<Py::AveragedSimpleHistogram,
                            boost::python::bases<Py::UniformBinsHistogramWithErrorBars>
                            >("AveragedSimpleHistogram")
      .def(boost::python::init<boost::python::optional<Py::UniformBinsHistogramParams> >())
      .def("addHistogram",
           +[](Kl & h, const Py::UniformBinsHistogram & o) { h.addHistogram(o); })
      .def("finalize", +[](Kl & h) { h.finalize(); })
      ;
  }
  std::cerr << "AveragedSimpleRealHistogram...\n";
  // AveragedSimpleRealHistogram
  {
    typedef Py::AveragedSimpleRealHistogram Kl;
    boost::python::scope cl =
      boost::python::class_<Py::AveragedSimpleRealHistogram,
                            boost::python::bases<Py::UniformBinsHistogramWithErrorBars>
                            >("AveragedSimpleRealHistogram")
      .def(boost::python::init<boost::python::optional<Py::UniformBinsHistogramParams> >())
      .def("addHistogram",
           +[](Kl & h, const Py::UniformBinsRealHistogram & o) { h.addHistogram(o); })
      .def("finalize", +[](Kl & h) { h.finalize(); })
      ;
  }
  std::cerr << "AveragedErrorBarHistogram...\n";
  // AveragedErrorBarHistogram
  {
    typedef Py::AveragedErrorBarHistogram Kl;
    boost::python::scope cl =
      boost::python::class_<Py::AveragedErrorBarHistogram,
                            boost::python::bases<Py::UniformBinsHistogramWithErrorBars>
                            >("AveragedErrorBarHistogram")
      .def(boost::python::init<boost::python::optional<Py::UniformBinsHistogramParams> >())
      .def("addHistogram",
           +[](Kl & h, const Py::UniformBinsHistogramWithErrorBars & o) {
             h.addHistogram(o);
           })
      .def("finalize", +[](Kl & h) { h.finalize(); })
      ;
  }

  std::cerr << "py_tomo_histogram() completed.\n";
}
