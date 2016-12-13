
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

    multiprocmodule.attr("__doc__") = std::string(
        "Classes and utilities for handling parallel tasks and for status reporting."
        );

    logger.debug("multiproc.FullStatusReport ...");
    { typedef Py::FullStatusReport Kl;
      boost::python::class_<Py::FullStatusReport>(
          "FullStatusReport",
          "A full status report of running tasks.  This is typically passed on to a callback for displaying "
          "progressive status reports.\n\n"
          ".. seealso:: This Python class interfaces :tomocxx:`the C++ class Tomographer::MultiProc::FullStatusReport "
          "<struct_tomographer_1_1_multi_proc_1_1_full_status_report.html>`."
          "\n\n"
          "There is a slight difference between the C++ and Python API's on how the individual workers' running/idle "
          " are reported: here, we don't expose any `workers_running` list, rather, if a worker is idle, the corresponding "
          "object in `workers` is `None`."
          "\n\n"
          "All attributes of this class are read-only.\n\n"
          "\n\n"
          ".. py:attribute:: num_completed\n\n"
          "    The number of task instances that have already been entirely completed.\n\n"
          ".. py:attribute:: num_total_runs\n\n"
          "    The total number of task instances that we have to run\n\n"
          ".. py:attribute:: elapsed\n\n"
          "    How many seconds have elapsed since starting to run tasks\n\n"
          ".. py:attribute:: workers\n\n"
          "    A list of individual reports for each running worker.  The length of this list is the number of workers "
          "which can handle running tasks (e.g. parallel threads).  Each item of the list is either `None` if the "
          "worker is idle, or a `WorkerStatusReport` providing the worker's current status report."
          "\n\n"
          )
        .add_property("num_completed", +[](const Kl& r) { return r.num_completed; })
        .add_property("num_total_runs", +[](const Kl& r) { return r.num_total_runs; })
        .add_property("elapsed", +[](const Kl& r) { return r.elapsed; })
        .add_property("workers", +[](const Kl& r) { return r.workers; })
        .add_property("total_fraction_done", +[](const Kl & r) { return r.total_fraction_done; })
        .def("getHumanReport", +[](const Kl & r) { return r.human_report; },
             "getHumanReport()\n\n"
             "A convenience function which formats the data contained in this structure in a human-readable form, "
             "providing an overview of the current status of the tasks.  The report is suitable to be displayed "
             "in a terminal, for example.\n\n"
             "The formatted report is returned as a string.")
        ;
    }
    logger.debug("multiproc.WorkerStatusReport ...");
    { typedef Py::WorkerStatusReport Kl;
      boost::python::class_<Py::WorkerStatusReport>(
          "WorkerStatusReport",
          "Provides a summary of the current status of a worker processing a task."
          "\n\n"
          "This Python class is a wrapper for a corresponding C++ subclass class of "
          ":tomocxx:`Tomographer::MultiProc::TaskStatusReport "
          "<struct_tomographer_1_1_multi_proc_1_1_task_status_report.html>`, for example "
          ":tomocxx:`Tomographer::MHRWTasks::MHRandomWalkTask::StatusReport "
          "<struct_tomographer_1_1_m_h_r_w_tasks_1_1_m_h_random_walk_task_1_1_status_report.html>`. "
          "Note that the data is rearranged from the C++ API; only the attributes `fraction_done` and `msg` are "
          "exposed directly; the remaining data is exposed via a general-purpose dictionary `data`."
          "\n\n"
          ".. py:attribute:: worker_id\n\n"
          "    The identification number for this worker, typically the thread number.\n\n"
          ".. py:attribute:: fraction_done\n\n"
          "    The estimated fraction of the current task which is completed, given as a real number between 0 and 1.\n\n"
          ".. py:attribute:: msg\n\n"
          "    A message (provided as a string) which summarizes the currents status of the task\n\n"
          ".. py:attribute:: data\n\n"
          "    Additional data which is available, depending on the task type.\n\n"
          "\n\n"
          "    Functions which provide status reports using :py:class:`FullStatusReport` and "
          ":py:class:`WorkerStatusReport` should properly document which additional information is "
          "available in the `data` attribute. (See, for example, :py:func:`tomographer.tomorun.tomorun()`.)"
          )
        .add_property("worker_id", +[](const Kl& r) { return r.worker_id; })
        .add_property("fraction_done", +[](const Kl& r) { return r.fraction_done; })
        .add_property("msg", +[](const Kl& r) { return r.msg; })
        .add_property("data", +[](const Kl& r) { return r.data; });
    }
  }
}
