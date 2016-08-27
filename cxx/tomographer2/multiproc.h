/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
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

#ifndef MULTIPROC_H
#define MULTIPROC_H

#include <string>

#include <tomographer2/tools/needownoperatornew.h>



/** \file multiproc.h
 *
 * \brief Some common definitions for multiprocessing interfaces.
 *
 * See also \ref pageTaskManagerDispatcher as well as \ref Tomographer::MultiProc::OMP.
 */


namespace Tomographer {

namespace MultiProc {



/** \brief Basic status report class
 *
 * This may serve as base class for a more detailed and specific status report.
 *
 * See also:
 *  - \ref pageTaskManagerDispatcher;
 *  - \ref OMPTaskDispatcher's status report mechanism;
 *  - \ref DMIntegratorTasks::MHRandomWalkTask::StatusReport for an example usage.
 *
 */
struct TaskStatusReport
{
  TaskStatusReport()
    : fraction_done(0), msg("<unknown>")
  { }
  TaskStatusReport(double fraction_done_, std::string msg_)
    : fraction_done(fraction_done_), msg(std::move(msg_))
  { }

  double fraction_done;
  std::string msg;
};

    
/** \brief A complete status report, abstract version
 *
 * Note: \a TaskStatusReportType must be default-constructible and copy-constructible.
 */
template<typename TaskStatusReportType>
struct FullStatusReport
{
  FullStatusReport()
    : num_completed(0),
      num_total_runs(0),
      workers_running(),
      workers_reports()
  {
  }

  //! Number of completed tasks
  int num_completed;

  //! Total number of tasks to perform
  int num_total_runs;
    
  /** \brief List specifying for each worker (e.g. a spawned thread) whether it is
   * active or not
   *
   * If <em>workers_running[k]</em> is \c true, then the worker number \a k (e.g. the \a
   * k -th thread) is currently running a task. Otherwise, it is inactive, for example
   * an idle thread waiting (because of chunking of tasks, for example, or because there
   * are are no new tasks to start)
   */
  std::vector<bool> workers_running;

  /** \brief List with the raw report submitted from each individual thread.
   *
   * The length of this list is always the same as that of \ref workers_running.
   *
   * If worker number \a k (e.g. the k-th thread) is active and currently running a
   * task, then <em>workers_reports[k]</em> is the raw status report that was submitted by
   * that thread. If the thread is not running a task, <em>tasks_reports[k]</em> is an
   * invalid, or default-constructed value.
   */
  std::vector<TaskStatusReportType,
              typename Tools::NeedOwnOperatorNew<TaskStatusReportType>::AllocatorType> workers_reports;
};




namespace Sequential {

/** \brief Executes multiple tasks sequentially
 *
 * This task dispatcher is useful mainly for test cases, when testing tasks (and to see if
 * a set of classes adhere well to the task manager/dispatcher type interfaces, see
 * \ref pageTaskManagerDispatcher).
 *
 * <ul>
 * <li> \a Task must be a \ref pageInterfaceTask compliant type 
 *
 * <li> \a TaskCData should conform to the \ref pageInterfaceTaskCData.
 *
 * <li> \a ResultsCollector must be a \ref pageInterfaceResultsCollector compliant type 
 *
 * <li> \a LoggerType is the type used for logging messages (derived from \ref LoggerBase)
 *
 * <li> \a CountIntType should be a type to use to count the number of tasks. Usually
 *      there's no reason not to use an \c int.
 *
 * </ul>
 */
template<typename TaskType_, typename TaskCData_, typename ResultsCollector_,
         typename LoggerType_, typename CountIntType_ = int>
class TaskDispatcher
{
public:
  typedef TaskType_ TaskType;
  typedef typename TaskType::StatusReportType TaskStatusReportType;
  typedef TaskCData_ TaskCData;
  typedef ResultsCollector_ ResultsCollector;
  typedef LoggerType_ LoggerType;
  typedef CountIntType_ CountIntType;

  // not directly needed, but make sure TaskType::ResultType exists as part of testing the
  // task, cdata and result-collectors's correct type interface implementation
  typedef typename TaskType::ResultType TaskResultType;

  typedef FullStatusReport<TaskStatusReportType> FullStatusReportType;

  typedef std::function<void(const FullStatusReportType&)> FullStatusReportCallbackType;

private:
  
  const TaskCData * pcdata;
  ResultsCollector * results;
  LoggerType & logger;

  CountIntType num_total_runs;
  
  /** \brief current executing task number == number of completed tasks
   *
   */
  CountIntType task_k;

  struct TaskMgrIface {
    TaskMgrIface(TaskDispatcher * dispatcher_)
      : dispatcher(dispatcher_),
        status_report_requested(false),
        status_report_user_fn()
    {
    }

    TaskDispatcher * dispatcher;

    bool status_report_requested;
    FullStatusReportCallbackType status_report_user_fn;
  
    inline void _request_status_report() { status_report_requested = true; }

    inline bool statusReportRequested() const
    {
      return status_report_requested;
    }

    inline void submitStatusReport(const TaskStatusReportType &statreport)
    {
      FullStatusReport<TaskStatusReportType> fullstatus;
      
      fullstatus.num_completed = dispatcher->task_k;
      fullstatus.num_total_runs = dispatcher->num_total_runs;
              
      // initialize task-specific reports
      // fill our lists with default-constructed values & set all running to false.
      fullstatus.workers_running.clear();
      fullstatus.workers_reports.clear();

      fullstatus.workers_running.resize(1, false);
      fullstatus.workers_running[0] = true;

      fullstatus.workers_reports.resize(1, false);
      fullstatus.workers_reports[0] = statreport;

      status_report_user_fn(fullstatus);

      status_report_requested = false;
    }

  };

  TaskMgrIface mgriface;
  
public:
  TaskDispatcher(TaskCData * pcdata_, ResultsCollector * results_, LoggerType & logger_,
                 CountIntType num_total_runs_)
    : pcdata(pcdata_), results(results_), logger(logger_), num_total_runs(num_total_runs_),
        mgriface(this)
  {
  }
  
  void run()
  {
    results->init(num_total_runs, 1, pcdata);
    
    logger.debug("MultiProc::Sequential::TaskDispatcher::run()", "preparing for sequential runs");
    
    for (task_k = 0; task_k < num_total_runs; ++task_k) {
      
      logger.debug("Tomographer::MultiProc::Sequential::TaskDispatcher::run()",
                   [&](std::ostream & stream) { stream << "Running task #" << task_k << " ..."; });
      
      auto input = pcdata->getTaskInput(task_k);
      
      // construct a new task instance
      TaskType t(input, pcdata, logger);

      // and run it
      t.run(pcdata, logger, &mgriface);
      
      // and collect the result
      results->collectResult(task_k, t.getResult(), pcdata);
    }
    
    results->runsFinished(num_total_runs, pcdata);
  }
  
  
  /** \brief assign a callable to be called whenever a status report is requested
   *
   * This function remembers the given \a fnstatus callable, so that each time that \ref
   * requestStatusReport() is called at any later point, then this callback will be
   * invoked.
   *
   * The callback, when invoked, will be called with a single parameter of type \ref
   * FullStatusReport<TaskStatusReportType>.
   *
   * \par How Tasks should handle status reports.
   * Task's must regularly check whether a status report has been requested as they run. 
   * This is done by regularly calling the function
   * <code>tmgriface->statusReportRequested()</code> on the \c tmgriface object
   * provided to <code>TaskType::run()</code>. This function call does not require a \c
   * critical section and is fast, so this check can be done often. The function
   * <code>tmgriface->statusReportRequested()</code> returns a \c bool indicating
   * whether such a report was requested or not. If such a report was requested, then
   * the thread should prepare its status report object (of type \c
   * TaskStatusReportType), and call <code>tmgriface->submitStatusReport(const
   * TaskStatusReportType & obj)</code>.
   *
   * \par
   * Note that the task should provide a member type named \c StatusReportType, which can be
   * for example a simple typedef to \ref MultiProc::StatusReport, which specifies the
   * type of its status reports.
   *
   */
  template<typename Fn>
  inline void setStatusReportHandler(Fn fnstatus)
  {
    mgriface.status_report_user_fn = fnstatus;
  }
  
  inline void requestStatusReport()
  {
    mgriface._request_status_report();
  }
  
}; // class TaskDispatcher


} // namespace Sequential

} // namespace MultiProc

} // namespace Tomographer

#endif
