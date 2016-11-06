
#include "pyhistogram.h"



void py_tomo_histogram()
{
  // CLASS: UniformBinsHistogram
  {
    typedef Py::UniformBinsHistogram Kl;
    boost::python::scope cl = boost::python::class_<Py::UniformBinsHistogram>("UniformBinsHistogram")
      .def(boost::python::init<boost::python::optional<Py::UniformBinsHistogram::Params> >())
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
                    +[](Kl & h, const Eigen::MatrixXi v) { h.bins = v; })
      .add_property("off_chart", +[](Kl & h) { return h.off_chart; },
                    +[](Kl & h, CountIntType o) { h.off_chart = o; })
      .add_static_property("HasErrorBars", +[](Kl &) { return false; })
      ;

    //    std::cerr << "(A)\n";

    {
      typedef Py::UniformBinsHistogram::Params Kl;
      // CLASS: UniformBinsHistogram::Params
      boost::python::class_<Py::UniformBinsHistogram::Params>("Params")
        .def(boost::python::init<boost::python::optional<RealType,RealType,std::size_t> >())
        .add_property("min", +[](Kl & p) { return p.min; }, +[](Kl & p, RealType min) { p.min = min; })
        .add_property("max", +[](Kl & p) { return p.max; }, +[](Kl & p, RealType max) { p.max = max; })
        .add_property("num_bins", +[](Kl & p) { return p.num_bins; }, +[](Kl & p, std::size_t n) { p.num_bins = n; })
        .def("isWithinBounds", &Kl::isWithinBounds)
        .def("binIndex", &Kl::binIndex)
        .def("binLowerValue", &Kl::binLowerValue)
        .def("binCenterValue", &Kl::binCenterValue)
        .def("binUpperValue", &Kl::binUpperValue)
        .def("binResolution", &Kl::binResolution)
        ;
    }
  }

  //  std::cerr << "(B)\n";

  // UniformBinsWithErrorBars
  {
    typedef Py::UniformBinsHistogramWithErrorBars Kl;
    boost::python::scope cl =
      // seems we can't use bases<> because the class isn't polymorphic (no virtual destructor)
      boost::python::class_<Py::UniformBinsHistogramWithErrorBars>("UniformBinsHistogramWithErrorBars")
      .def(boost::python::init<boost::python::optional<Py::UniformBinsHistogram::Params> >())
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

  //  std::cerr << "(C)\n";

  // AveragedSimpleHistogram
  {
    typedef Py::AveragedSimpleHistogram Kl;
    boost::python::scope cl =
      boost::python::class_<Py::AveragedSimpleHistogram,
                            boost::python::bases<Py::UniformBinsHistogramWithErrorBars>
                            >("AveragedSimpleHistogram")
      .def(boost::python::init<boost::python::optional<Py::UniformBinsHistogram::Params> >())
      .def("addHistogram",
           +[](Kl & h, const Py::UniformBinsHistogram & o) { h.addHistogram(o); })
      .def("finalize", +[](Kl & h) { h.finalize(); })
      ;
  }
  // AveragedErrorBarHistogram
  {
    typedef Py::AveragedSimpleHistogram Kl;
    boost::python::scope cl =
      boost::python::class_<Py::AveragedErrorBarHistogram,
                            boost::python::bases<Py::UniformBinsHistogramWithErrorBars>
                            >("AveragedErrorBarHistogram")
      .def(boost::python::init<boost::python::optional<Py::UniformBinsHistogram::Params> >())
      .def("addHistogram",
           +[](Kl & h, const Py::UniformBinsHistogramWithErrorBars & o) {
             h.addHistogram(o);
           })
      .def("finalize", +[](Kl & h) { h.finalize(); })
      ;
  }

}
