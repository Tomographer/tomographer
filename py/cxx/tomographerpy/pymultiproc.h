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

#include <tomographerpy/pymhrw.h>


namespace tpy {


/** \brief Report of the status of a single worker
 *
 * Provides basic information, such as \a worker_id, fraction of the work done
 * (\a fraction_done), and a human-readable message (\a msg), but also
 * task-specific data stored in the dictionary \a data. Anything can be stored
 * in \a data, this should be documented by the function or class which emits
 * the status reports.
 */
struct TOMOGRAPHER_EXPORT WorkerStatusReport
{
  //! Unique identifier of the worker
  int worker_id;

  //! Fraction of the job done for this worker (0.0 to 1.0)
  py::float_ fraction_done;

  //! Human-readable message summarizing the status of this worker
  py::str msg;

  //! Additional task-specific data; see also \ref PyStatusReportAddWorkerDataFields
  py::dict data;
};

/** \brief Complete status report for multiple tasks running in parallel
 *
 * Reports the number of tasks completed, the number of total tasks to run, the
 * time elapsed, the total fraction done, as well as individual status reports
 * for each running worker, and a global human-readable summary.
 */
struct TOMOGRAPHER_EXPORT FullStatusReport {
  FullStatusReport()
    : num_completed(-1), num_total_runs(-1), elapsed(0.0),
      workers(), total_fraction_done(), human_report()
  {
  }

  //! Number of tasks which have already completed
  py::int_ num_completed;
  //! Total number of tasks which have been or will be run
  py::int_ num_total_runs;
  //! Elapsed time in seconds since the launching of the tasks
  py::float_ elapsed;

  /** \brief A Python list of worker status; either a \ref WorkerStatusReport,
   *         or \a py::none() if the worker is idle
   */
  py::list workers; // list of [WorkerStatusReport or None (for idle)]

  //! Total fraction of work done, as a fraction between 0 and 1
  py::float_ total_fraction_done;

  //! Complete, human-readable summary of the current status of everything
  py::str human_report;
};


/** \brief C++ utility to populate the \a data field of a \ref
 *         WorkerStatusReport for a given \a TaskType
 *
 * Specialize this class template to all requested \ref pageInterfaceTask types
 * in order to provide meaningful status reports.
 *
 * This class should provide a single static member, which should add fields to
 * a dictionary based on a status report emanating from that task type.
 */
template<typename TaskType>
struct TOMOGRAPHER_EXPORT PyStatusReportAddWorkerDataFields
{
  /** \brief Add fields to the given dict, from a status report sent in by a \a
   *         TaskType. The default implementation does nothing
   */
  static inline void addDataFields(py::dict & , const typename TaskType::StatusReportType & ) { }
};

/** \brief Add fields to the given dict, from a status report sent in by a \a
 *         Tomographer::MHRWTasks::MHRandomWalkTask
 *
 * The fields \c "mhrw_params", \c "acceptance_ratio", \c "kstep" (iteration
 * number), and \c "n_total_iters" are added to the dictionary with
 * corresponding values.
 */
template<typename CData, typename Rng>
struct TOMOGRAPHER_EXPORT PyStatusReportAddWorkerDataFields< Tomographer::MHRWTasks::MHRandomWalkTask<CData, Rng> >
{
  typedef typename Tomographer::MHRWTasks::MHRandomWalkTask<CData, Rng>::StatusReportType TaskStatusReportType;
  static inline void addDataFields(py::dict & d, const TaskStatusReportType & wr) {
    d["mhrw_params"] = tpy::MHRWParams(
        pyMHWalkerParamsToDictInvoke(wr.mhrw_params.mhwalker_params),
        wr.mhrw_params.n_sweep,
        wr.mhrw_params.n_therm,
        wr.mhrw_params.n_run);
    d["acceptance_ratio"] = wr.acceptance_ratio;
    d["kstep"] = wr.kstep;
    d["n_total_iters"] = wr.n_total_iters;
  }
};

/** \brief Utility to prepare a Python status report (\ref FullStatusReport)
 *         from a task's status report
 *
 * If you write C++ code using a task dispatcher, and would want to provide
 * status reporting to your caller, then consider using the higher-level \ref
 * setTasksStatusReportPyCallback(), which does everything for you, including
 * checking for signals (e.g. keyboard interrupts).
 */
template<typename TaskType, typename IntType = int>
inline tpy::FullStatusReport preparePyTaskStatusReport(
    const Tomographer::MultiProc::FullStatusReport<typename TaskType::StatusReportType, IntType> & report
    )
{
  tpy::FullStatusReport r;
  r.num_completed = report.num_completed;
  r.num_total_runs = report.num_total_runs;
  r.elapsed = report.elapsed;
  r.total_fraction_done = report.totalFractionDone();
  r.human_report = report.getHumanReport();
  r.workers = py::list();
  for (std::size_t k = 0; k < report.workers_reports.size(); ++k) {
    if (!report.workers_running[k]) {
      r.workers.append(py::none());
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


/** \brief Set up status reporting for a task dispatcher, using a Python callback for status reports
 *
 * Sets up the given \ref pageInterfaceTaskDispatcher "task dispatcher" \a tasks
 * to provide status reports every \a progress_interval_ms milliseconds, by
 * calling the Python callback \a progress_fn.
 *
 * Also, when we recieve progress reports, we check for signals (e.g. keyboard
 * interrupts) or other Python exceptions.  If such an exception occurred, we
 * interrupt the tasks.
 *
 * The argument \a progress_fn can be \a None, in which case no callback is
 * performed; however we still check for signals including keyboard interrupts
 * and for other Python exceptions.
 *
 * If \a require_gil_acquisition is \a true, then the GIL (Global Interpreter
 * Lock) is acquired before any Python API call.  Otherwise, we assume that we
 * already hold the GIL and don't attempt to acquire it.
 */
template<typename TaskDispatcher>
inline void setTasksStatusReportPyCallback(TaskDispatcher & tasks, py::object progress_fn,
                                           int progress_interval_ms, bool require_gil_acquisition = false)
{
  typedef typename TaskDispatcher::TaskType TaskType;

  //
  // NOTE: always set a progress report handler, even if progress_fn is None.  Indeed, we
  // use this callback to e.g. check for keyboard interrupt signals.
  //

  auto fn = [progress_fn](const typename TaskDispatcher::FullStatusReportType & report) {

    if (PyErr_Occurred() != NULL || PyErr_CheckSignals() == -1) {
      //fprintf(stderr, "DEBUG:: error set, throwing\n") ;
      throw py::error_already_set();
    }
    // call the python progress callback:
    if (!progress_fn.is_none()) {
      auto r = preparePyTaskStatusReport<TaskType>(report);
      //fprintf(stderr, "DEBUG:: about to call py callback\n") ;
      progress_fn(py::cast(r));
      if (PyErr_Occurred() != NULL || PyErr_CheckSignals() == -1) {
        fprintf(stderr, "DEBUG:: error set, throwing\n") ;
        throw py::error_already_set();
      }
      //fprintf(stderr, "DEBUG:: py callback done\n") ;
    }
  };

  auto fn_with_gil = [fn](const typename TaskDispatcher::FullStatusReportType & report) {
    py::gil_scoped_acquire gil_acquire;
    fn(report);
  };

  if (require_gil_acquisition) {
    tasks.setStatusReportHandler(fn_with_gil);
  } else {
    tasks.setStatusReportHandler(fn);
  }
  tasks.requestPeriodicStatusReport(progress_interval_ms);
}


} // namespace tpy






#endif
