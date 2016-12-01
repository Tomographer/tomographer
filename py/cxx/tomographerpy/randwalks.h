

#include <tomographer2/mhrw.h>



/*

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
