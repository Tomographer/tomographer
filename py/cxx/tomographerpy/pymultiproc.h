/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
 * Copyright (c) 2017 Caltech, Institute for Quantum Information and Matter, Philippe Faist
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef PYMULTIPROC_H
#define PYMULTIPROC_H

#include <string>

#include <tomographerpy/common.h>

#include <tomographer/mhrw.h>
#include <tomographer/mhrwtasks.h>


namespace tpy {

struct WorkerStatusReport {
  int worker_id;
  double fraction_done;
  std::string msg;
  py::dict data;
};

struct FullStatusReport {
  FullStatusReport() : num_completed(-1), num_total_runs(-1), elapsed(0), workers(), total_fraction_done(), human_report() { }
  int num_completed;
  int num_total_runs;
  double elapsed; // elapsed time in seconds

  py::list workers; // list of [WorkerStatusReport or None (for idle)]

  double total_fraction_done;
  std::string human_report;
};

}

template<typename TaskType>
struct PyStatusReportAddWorkerDataFields
{
  static inline void addDataFields(py::dict & , const typename TaskType::StatusReportType & ) { }
};

template<typename CData, typename Rng>
struct PyStatusReportAddWorkerDataFields< Tomographer::MHRWTasks::MHRandomWalkTask<CData, Rng> >
{
  typedef typename Tomographer::MHRWTasks::MHRandomWalkTask<CData, Rng>::StatusReportType TaskStatusReportType;
  static inline void addDataFields(py::dict & d, const TaskStatusReportType & wr) {
    d["mhrw_params"] = wr.mhrw_params;
    d["acceptance_ratio"] = wr.acceptance_ratio;
    d["kstep"] = wr.kstep;
    d["n_total_iters"] = wr.n_total_iters;
  }
};

template<typename TaskType>
inline tpy::FullStatusReport preparePyTaskStatusReport(
    const Tomographer::MultiProc::FullStatusReport<typename TaskType::StatusReportType> & report
    )
{
  tpy::FullStatusReport r;
  r.num_completed = report.num_completed;
  r.num_total_runs = report.num_total_runs;
  r.elapsed = report.elapsed;
  r.total_fraction_done = report.totalFractionDone();
  r.human_report = report.getHumanReport();
  for (std::size_t k = 0; k < report.workers_reports.size(); ++k) {
    if (!report.workers_running[k]) {
      r.workers.append(py::object());
      continue;
    }
    // and prepare the report object
    const auto& rr = report.workers_reports[k];
    tpy::WorkerStatusReport wreport;
    // generic worker status report fields
    wreport.worker_id = (int)k;
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
inline void setTasksStatusReportPyCallback(TaskDispatcher & tasks, py::object progress_fn,
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
          throw py::error_already_set();
        }
        // call the python progress callback:
        if (!progress_fn.is_none()) {
          auto r = preparePyTaskStatusReport<TaskType>(report);
          progress_fn(py::cast(r));
        }
        // borrowed from tomographer/tools/signal_status_handler.h: --->  FOR DEBUGGING::
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
