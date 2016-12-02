
#ifndef PYMULTIPROC_H
#define PYMULTIPROC_H

#include <string>

#include <tomographerpy/common.h>

#include <tomographer2/mhrw.h>
#include <tomographer2/mhrwtasks.h>


namespace Py {

typedef Tomographer::MHRWParams<CountIntType, RealType> MHRWParams;

struct WorkerStatusReport {
  int worker_id;
  double fraction_done;
  std::string msg;
  boost::python::dict data;
};

struct FullStatusReport {
  FullStatusReport() : num_completed(-1), num_total_runs(-1), elapsed(0), workers() { }
  int num_completed;
  int num_total_runs;
  double elapsed; // elapsed time in seconds

  boost::python::list workers; // list of [WorkerStatusReport or None (for idle)]

  double total_fraction_done() const {
    double f = num_completed;
    for (long k = 0; k < boost::python::len(workers); ++k) {
      if (!boost::python::object(workers[k]).is_none()) {
        f += boost::python::extract<WorkerStatusReport>(workers[k])().fraction_done;
      }
    }
    return f / num_total_runs;
  }
};

}

template<typename TaskType>
struct PyStatusReportAddWorkerDataFields
{
  static inline void addDataFields(boost::python::dict & , const typename TaskType::StatusReportType & ) { }
};

template<typename CData, typename Rng>
struct PyStatusReportAddWorkerDataFields< Tomographer::MHRWTasks::MHRandomWalkTask<CData, Rng> >
{
  typedef typename Tomographer::MHRWTasks::MHRandomWalkTask<CData, Rng>::StatusReportType TaskStatusReportType;
  static inline void addDataFields(boost::python::dict & d, const TaskStatusReportType & wr) {
    d["mhrw_params"] = wr.mhrw_params;
    d["acceptance_ratio"] = wr.acceptance_ratio;
    d["kstep"] = wr.kstep;
    d["n_total_iters"] = wr.n_total_iters;
  }
};

template<typename TaskType>
inline Py::FullStatusReport preparePyTaskStatusReport(
    const Tomographer::MultiProc::FullStatusReport<typename TaskType::StatusReportType> & report
    )
{
  Py::FullStatusReport r;
  r.num_completed = report.num_completed;
  r.num_total_runs = report.num_total_runs;
  for (std::size_t k = 0; k < report.workers_reports.size(); ++k) {
    if (!report.workers_running[k]) {
      r.workers.append(boost::python::object());
      continue;
    }
    // and prepare the report object
    const auto& rr = report.workers_reports[k];
    Py::WorkerStatusReport wreport;
    // generic worker status report fields
    wreport.worker_id = k;
    wreport.fraction_done = rr.fraction_done;
    wreport.msg = rr.msg;
    // fields specific to MHRWValueHistogramTasks
    PyStatusReportAddWorkerDataFields<TaskType>::addDataFields(wreport.data, rr);
    // add this report
    r.workers.append(wreport);
  }
  return r;
}


template<typename TaskDispatcher>
inline void setTasksStatusReportPyCallback(TaskDispatcher & tasks, boost::python::object progress_fn,
                                           int progress_interval_ms)
{
  typedef typename TaskDispatcher::TaskType TaskType;
  typedef typename TaskType::StatusReportType TaskStatusReportType;

  typedef
#if defined(__GNUC__) && __GNUC__ == 4 && __GNUC_MINOR__ <= 6 && !defined(__clang__)
    std::chrono::monotonic_clock // for GCC/G++ 4.6
#else
    std::chrono::steady_clock
#endif
    StdClockType;

  auto time_start = StdClockType::now();

  tasks.setStatusReportHandler(
      [progress_fn,time_start](const Tomographer::MultiProc::FullStatusReport<TaskStatusReportType> & report) {
        // check to see if we got any KeyboardInterrupt
        // PyErr_CheckSignals() returns -1 if an exception was raised
        if (PyErr_Occurred() != NULL || PyErr_CheckSignals() == -1) {
          throw boost::python::error_already_set();
        }
        // call the python progress callback:
        if (!progress_fn.is_none()) {
          auto r = preparePyTaskStatusReport<TaskType>(report);
          r.elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(StdClockType::now() - time_start).count()
            / 1000.0;
          progress_fn(boost::python::object(r));
        }
        // borrowed from tomographer2/tools/signal_status_handler.h: --->  FOR DEBUGGING::
        /*
          std::string elapsed = Tomographer::Tools::fmtDuration(StdClockType::now() - time_start);
          fprintf(stderr,
          "\n"
          "=========================== Intermediate Progress Report ============================\n"
          "  Total Completed Runs: %d/%d: %5.2f%%\n"
          "  %s total elapsed\n",
          report.num_completed, report.num_total_runs,
          (double)report.num_completed/report.num_total_runs*100.0,
          elapsed.c_str());
          if (report.workers_running.size() == 1) {
          if (report.workers_running[0]) {
          fprintf(stderr, "--> %s\n", report.workers_reports[0].msg.c_str());
          }
          } else if (report.workers_running.size() > 1) {
          fprintf(stderr,
          "Current Run(s) information (workers working/spawned %d/%d):\n",
          (int)std::count(report.workers_running.begin(), report.workers_running.end(), true),
          (int)report.workers_running.size()
          );
          for (unsigned int k = 0; k < report.workers_running.size(); ++k) {
          std::string msg = report.workers_running[k] ? report.workers_reports[k].msg : std::string("<idle>");
          fprintf(stderr, "=== #%2u: %s\n", k, msg.c_str());
          }
          } else {
          // no info. (workers_running.size() == 0)
          }
          fprintf(stderr,
          "=====================================================================================\n\n");
        */
        // <----
      });
  tasks.requestPeriodicStatusReport(progress_interval_ms);
}








#endif
