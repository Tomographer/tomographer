
#include <tomographerpy/common.h>
#include <tomographerpy/pymultiproc.h>

#include "common_p.h"


void py_tomo_multiproc()
{
  auto logger = Tomographer::Logger::makeLocalLogger(TOMO_ORIGIN, tpy_logger);
  logger.debug("py_tomo_multiproc() ...");

  logger.debug("multiproc module ... ");

  boost::python::object multiprocsubmod(boost::python::borrowed(PyImport_AddModule("tomographer.multiproc")));
  boost::python::scope().attr("multiproc") = multiprocsubmod;
  {
    // now inside submodule
    boost::python::scope multiprocmodule(multiprocsubmod);

    logger.debug("multiproc.FullStatusReport ...");
    { typedef Py::FullStatusReport Kl;
      boost::python::class_<Py::FullStatusReport>("FullStatusReport")
        .add_property("num_completed", +[](const Kl& r) { return r.num_completed; })
        .add_property("num_total_runs", +[](const Kl& r) { return r.num_total_runs; })
        .add_property("elapsed", +[](const Kl& r) { return r.elapsed; })
        .add_property("workers", +[](const Kl& r) { return r.workers; })
        .add_property("total_fraction_done", +[](const Kl & r) { return r.total_fraction_done; })
        .def("getHumanReport", +[](const Kl & r) { return r.human_report; })
        ;
    }
    logger.debug("multiproc.WorkerStatusReport ...");
    { typedef Py::WorkerStatusReport Kl;
      boost::python::class_<Py::WorkerStatusReport>("WorkerStatusReport")
        .add_property("worker_id", +[](const Kl& r) { return r.worker_id; })
        .add_property("fraction_done", +[](const Kl& r) { return r.fraction_done; })
        .add_property("msg", +[](const Kl& r) { return r.msg; })
        .add_property("data", +[](const Kl& r) { return r.data; });
    }
  }
}
