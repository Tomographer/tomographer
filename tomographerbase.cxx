

#include <Python/Python.h>
#include <boost/python.hpp>

#include <Eigen/Eigen>

#include "eigpyconv.h"

#include <tomographer2/histogram.h>


// -- global-use C++ types --

typedef double RealType;
typedef int CountIntType;


// -- Python interface --



// ------ histograms --------



// ------ random walks -----------

/*

class ValueCalculatorDblPyFn {
  typedef RealType ValueType;
  ValueType getValue(const boost::python::object & fn) {
    return fn();
  }
};





class MHRWStatsCollectorEmpty {
  void init() { }
  void thermalizingDone() { }
  void done() { }

  void processSample(CountIntType k, CountIntType n, const boost::python::object & pt,
                     RealType fnval, boost::python::object & rw) { }

  void rawMove(CountIntType k, bool is_thermalizing, bool is_live_iter, bool accepted,
               double a, const boost::python::object & newpt, RealType newptval,
               const boost::python::object & curpt, RealType curptval,
               boost::python::object & rw) { }
};

class MHRWStatsCollectorBase : boost::python::wrapper<MHRWStatsCollectorEmpty>
{
  void init() {
    this->get_override("init")();
  }

  void thermalizingDone() {
    this->get_override("thermalizingDone")();
  }

  void done() {
    this->get_override("done")();
  }

  void processSample(CountIntType k, CountIntType n, const boost::python::object & pt,
                     RealType fnval, boost::python::object & rw) {
    this->get_override("processSample")(k, n, pt, fnval, rw);
  }

  void rawMove(CountIntType k, bool is_thermalizing, bool is_live_iter, bool accepted,
               double a, const boost::python::object & newpt, RealType newptval,
               const boost::python::object & curpt, RealType curptval,
               boost::python::object & rw)
  {
    this->get_override("rawMove")(k, is_thermalizing, is_live_iter, accepted, a,
                                  newpt, newptval, curpt, curptval, rw);
  }
  
};



*/




// ===============================================================

// export everything.

namespace Py {

typedef Tomographer::UniformBinsHistogram<RealType, CountIntType> UniformBinsHistogram;

// UniformBinsWithErrorBars
typedef Tomographer::UniformBinsHistogramWithErrorBars<RealType, RealType> UniformBinsHistogramWithErrorBars;


// AveragedSimpleHistogram (underlying histogram w/o error bars)
typedef Tomographer::AveragedHistogram<UniformBinsHistogram, RealType> AveragedSimpleHistogram;
// void hlp_ash_addHistogram(AveragedSimpleHistogram & h, const UniformBinsHistogram & other) {
//   h.addHistogram(other);
// }
//void hlp_ash_finalize(AveragedSimpleHistogram & h) { h.finalize(); }
// AveragedErrorBarHistogram (underlying histogram with error bars)
typedef Tomographer::AveragedHistogram<UniformBinsHistogramWithErrorBars, RealType> AveragedErrorBarHistogram;
// void hlp_aebh_addHistogram(AveragedErrorBarHistogram & h, const UniformBinsHistogramWithErrorBars & other) {
//   h.addHistogram(other);
// }
//void hlp_aebh_finalize(AveragedErrorBarHistogram & h) { h.finalize(); }

} // namespace Py


// tests .... ---------------
double test_eigen(const Eigen::MatrixXd & x)
{
  std::ostringstream s;
  s << "x = \n" << x << ";\n";
  std::string ss(s.str());
  fprintf(stderr, "%s", ss.c_str());
  //
  return x.sum();//rows() * 10000 + x.cols() * 100 + x(1,0);
}
boost::python::object test_eigen2(const Eigen::MatrixXcd & x)
{
  boost::python::dict d;
  d["rows"] = x.rows();
  d["cols"] = x.cols();
  d["(0,0)"] = x(0,0);
  d["(1,0)"] = ((2 <= x.rows()) ? x(1,0) : -1);
  d["(0,1)"] = ((2 <= x.cols()) ? x(0,1) : -1);
  d["sum"] = x.sum();
  return d;
}

Eigen::MatrixXd testgetmatrix_d(const Eigen::MatrixXd & x)
{
  return x * 3;
}
Eigen::MatrixXcd testgetmatrix_cd(const Eigen::MatrixXcd & x)
{
  return x * 3;
}
Eigen::MatrixXi testgetmatrix_i(const Eigen::MatrixXi & x)
{
  return x * 3;
}
// --------------



BOOST_PYTHON_MODULE(tomographer)
{
  std::cerr << "INIT TOMOGRAPHER\n";

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

    std::cerr << "(A)\n";

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

  std::cerr << "(B)\n";

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

  std::cerr << "(C)\n";

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

  std::cerr << "DONE 1\n";

  // Eigen converters ---- tests ---------
  register_eigen_converter();
  boost::python::def("test_eigen", test_eigen);
  boost::python::def("test_eigen2", test_eigen2);
  boost::python::def("testgetmatrix_d", testgetmatrix_d);
  boost::python::def("testgetmatrix_cd", testgetmatrix_cd);
  boost::python::def("testgetmatrix_i", testgetmatrix_i);

  std::cerr << "DONE 2\n";
}
