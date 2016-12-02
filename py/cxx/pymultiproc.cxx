
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
        .add_property("total_fraction_done", &Kl::total_fraction_done)
        .add_property("num_completed", +[](const Kl& r) { return r.num_completed; })
        .add_property("num_total_runs", +[](const Kl& r) { return r.num_total_runs; })
        .add_property("elapsed", +[](const Kl& r) { return r.elapsed; })
        .add_property("workers", +[](const Kl& r) { return r.workers; })
        // a shortcut to get scripts up&running real fast:
        .def("getHumanReport", +[](const Kl& r) {
            // adapted from tomographer2/tools/signal_status_handler.h: -->
            std::string elapsed = Tomographer::Tools::fmtDuration(std::chrono::milliseconds(int(r.elapsed*1000)));
            std::stringstream ss;
            ss << "=========================== Intermediate Progress Report ============================\n"
               << "  Total Completed Runs: " << Tomographer::Tools::fmts(
                   "%d/%d: %5.2f%%", r.num_completed, r.num_total_runs, r.num_completed*100.0/r.num_total_runs
                   ) << "\n"
               << "  " << elapsed << "s total elapsed\n";
            if (boost::python::len(r.workers) == 1) {
              if (!boost::python::object(r.workers[0]).is_none()) {
                ss << "--> " << boost::python::extract<Py::WorkerStatusReport>(r.workers[0])().msg << "\n";
              }
            } else if (boost::python::len(r.workers) > 1) {
              const int numtotal = boost::python::len(r.workers);
              int numworking = 0;
              int k;
              for (k = 0; k < numtotal; ++k) { numworking += (boost::python::object(r.workers[k]).is_none() ? 0 : 1); }
              ss << "Current Run(s) information (workers working/spawned " << numworking << "/" << numtotal << "):\n";
              for (k = 0; k < numtotal; ++k) {
                ss << "=== " << std::setw(2) << k << ": ";
                if (boost::python::object(r.workers[k]).is_none()) {
                  ss << "<idle>\n";
                } else {
                  ss << boost::python::extract<Py::WorkerStatusReport>(r.workers[k])().msg << "\n";
                }
              }
            } else {
              // no info. (workers_running.size() == 0)
            }
            ss << "=====================================================================================\n";
            return ss.str();
          })
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
