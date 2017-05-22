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
#include <thread> // DEBUG!!

#include <tomographerpy/common.h>

#include <tomographer/mhrw.h>
#include <tomographer/mhrwtasks.h>

#include <tomographerpy/pymhrw.h>


namespace tpy {



struct TOMOGRAPHER_EXPORT WorkerStatusReport {
  int worker_id;
  py::float_ fraction_done;
  py::str msg;
  py::dict data;
};

struct TOMOGRAPHER_EXPORT FullStatusReport {
  FullStatusReport()
    : num_completed(-1), num_total_runs(-1), elapsed(0.0),
      workers(), total_fraction_done(), human_report()
  {
  }

  py::int_ num_completed;
  py::int_ num_total_runs;
  py::float_ elapsed; // elapsed time in seconds

  py::list workers; // list of [WorkerStatusReport or None (for idle)]

  py::float_ total_fraction_done;
  py::str human_report;
};

}

template<typename MHWalkerParams>
struct TOMOGRAPHER_EXPORT PyMHWalkerParamsToDict
{
  static inline py::dict makeDict(const MHWalkerParams & ) { return {}; }
};
template<typename StepRealType>
struct TOMOGRAPHER_EXPORT PyMHWalkerParamsToDict<Tomographer::MHWalkerParamsStepSize<StepRealType> >
{
  static inline py::dict makeDict(const Tomographer::MHWalkerParamsStepSize<StepRealType> & p) {
    return py::dict(py::arg("step_size") = p.step_size);
  }
};

template<typename MHWalkerParams>
inline py::dict pyMHWalkerParamsToDictInvoke(const MHWalkerParams & p)
{
  return PyMHWalkerParamsToDict<MHWalkerParams>::makeDict(p);
}


template<typename TaskType>
struct TOMOGRAPHER_EXPORT PyStatusReportAddWorkerDataFields
{
  static inline void addDataFields(py::dict & , const typename TaskType::StatusReportType & ) { }
};

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









#endif
