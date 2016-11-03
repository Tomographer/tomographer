

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
RealType hlp_ubhparams_min_get(const UniformBinsHistogram::Params & p) { return p.min; }
void hlp_ubhparams_min_set(UniformBinsHistogram::Params & p, RealType min) { p.min = min; }
RealType hlp_ubhparams_max_get(const UniformBinsHistogram::Params & p) { return p.max; }
void hlp_ubhparams_max_set(UniformBinsHistogram::Params & p, RealType max) { p.max = max; }
CountIntType hlp_ubhparams_num_bins_get(const UniformBinsHistogram::Params & p) { return p.num_bins; }
void hlp_ubhparams_num_bins_set(UniformBinsHistogram::Params & p, RealType num_bins) { p.num_bins = num_bins; }
const UniformBinsHistogram::Params & hlp_ubh_params_get(const UniformBinsHistogram & p) { return p.params; }
CountIntType hlp_ubh_off_chart_get(const UniformBinsHistogram & p) { return p.off_chart; }
void hlp_ubh_off_chart_set(UniformBinsHistogram & p, CountIntType off_chart) { p.off_chart = off_chart; }
bool hlp_ubh_HasErrorBars_get(const UniformBinsHistogram & ) { return false; }
std::size_t (UniformBinsHistogram::*hlp_ubh_record1)(RealType) = &UniformBinsHistogram::record;
std::size_t (UniformBinsHistogram::*hlp_ubh_record2)(RealType,CountIntType) = &UniformBinsHistogram::record;
}

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

BOOST_PYTHON_MODULE(tomographer)
{
  // CLASS: UniformBinsHistogram
  {
    boost::python::scope cl = boost::python::class_<Py::UniformBinsHistogram>("UniformBinsHistogram")
      .def(boost::python::init<boost::python::optional<Py::UniformBinsHistogram::Params> >())
      .def(boost::python::init<RealType, RealType, std::size_t>())
      .def("reset", &Py::UniformBinsHistogram::reset)
      //      .def("load", &Py::UniformBinsHistogram::load)
      //      .def("add", &Py::UniformBinsHistogram::add)
      .def("numBins", &Py::UniformBinsHistogram::numBins)
      .def("count", &Py::UniformBinsHistogram::count)
      .def("record", Py::hlp_ubh_record1)
      .def("record", Py::hlp_ubh_record2)
      .def("prettyPrint", &Py::UniformBinsHistogram::prettyPrint)
      .add_property("params",
                    boost::python::make_function(&Py::hlp_ubh_params_get,
                                                 boost::python::return_internal_reference<>()))
      .add_property("off_chart", &Py::hlp_ubh_off_chart_get, &Py::hlp_ubh_off_chart_set)
      .add_static_property("HasErrorBars", &Py::hlp_ubh_HasErrorBars_get)
      ;


    // CLASS: UniformBinsHistogram::Params
    boost::python::class_<Py::UniformBinsHistogram::Params>("Params")
      .def(boost::python::init<boost::python::optional<RealType,RealType,std::size_t> >())
      .add_property("min", &Py::hlp_ubhparams_min_get, &Py::hlp_ubhparams_min_set)
      .add_property("max", &Py::hlp_ubhparams_max_get, &Py::hlp_ubhparams_max_set)
      .add_property("num_bins", &Py::hlp_ubhparams_num_bins_get, &Py::hlp_ubhparams_num_bins_set)
      .def("isWithinBounds", &Py::UniformBinsHistogram::Params::isWithinBounds)
      .def("binIndex", &Py::UniformBinsHistogram::Params::binIndex)
      .def("binLowerValue", &Py::UniformBinsHistogram::Params::binLowerValue)
      .def("binCenterValue", &Py::UniformBinsHistogram::Params::binCenterValue)
      .def("binUpperValue", &Py::UniformBinsHistogram::Params::binUpperValue)
      .def("binResolution", &Py::UniformBinsHistogram::Params::binResolution);
  }

  // Eigen converters
  register_eigen_converter();
  boost::python::def("test_eigen", test_eigen);
  boost::python::def("test_eigen2", test_eigen2);
}
