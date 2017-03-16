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

#ifndef MULTIPROC_H
#define MULTIPROC_H

#include <csignal>

#include <string>
#include <chrono>
#include <exception>

#include <tomographer/tools/fmt.h>
#include <tomographer/tools/needownoperatornew.h>



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
 *  - \ref OMP::TaskDispatcher's status report mechanism;
 *  - \ref MHRWTasks::MHRandomWalkTask::StatusReport for an example usage.
 *
 */
TOMOGRAPHER_EXPORT struct TaskStatusReport
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
TOMOGRAPHER_EXPORT struct FullStatusReport
{
  FullStatusReport()
    : num_completed(0),
      num_total_runs(0),
      workers_running(),
      workers_reports(),
      elapsed(std::numeric_limits<double>::quiet_NaN())
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

  /** \brief Number of seconds elapsed since launching the tasks
   *
   */
  double elapsed;


  /** \brief  The total fraction of the job completed
   *
   * Returns the total fraction of the tasks completed, taking into account the number of
   * tasks fully completed and any partially completed tasks.
   *
   * This is the figure you want to report in a global progress bar, for example.
   */
  inline double totalFractionDone() const
  {
    // idea: cumulate in f one unit per task completed.
    double f = num_completed;
    for (std::size_t k = 0; k < workers_running.size(); ++k) {
      if (workers_running[k]) {
        // partially completed tasks contribute a fraction
        f += workers_reports[k].fraction_done;
      }
    }
    // and f goes from zero to num_total_runs
    return f / num_total_runs;
  }

  /** \brief Produce a text-based human-readable short representation of the status report
   *
   */
  inline std::string getHumanReport() const
  {
    std::string elapsed_s = Tomographer::Tools::fmtDuration(std::chrono::milliseconds(int(elapsed*1000)));
    std::stringstream ss;
    ss << "=========================== Intermediate Progress Report ============================\n"
       << "  "
       << elapsed_s << "s elapsed"
       << "  -  "
       << num_completed << "/" << num_total_runs << " runs completed"
       << "  -  "
       << std::fixed << std::setw(5) << std::setprecision(2) << totalFractionDone() * 100.0 << "% total done"
       << "\n";

    if (workers_running.size() == 0) {
      // no info
    } else if (workers_running.size() == 1) {
      if (workers_running[0]) {
        ss << "--> " << workers_reports[0].msg << "\n";
      }
    } else {
      ss << "Current Run(s) information (workers working/spawned "
         << (int)std::count(workers_running.begin(), workers_running.end(), true)
         << "/" << workers_running.size() << "):\n";
      for (std::size_t k = 0; k < workers_running.size(); ++k) {
        ss << "=== " << std::setw(2) << k << ": ";
        if (!workers_running[k]) {
          ss << "<idle>\n";
        } else {
          ss << workers_reports[k].msg << "\n";
        }
      }
    }
    ss << "=====================================================================================\n";
    return ss.str();
  }
};



TOMOGRAPHER_EXPORT class TasksInterruptedException : public std::exception
{
  std::string msg_;
public:
  TasksInterruptedException(std::string msg = "Tasks Interrupted.") : msg_(msg) { }
  virtual ~TasksInterruptedException() throw() { }
  const char * what() const throw() { return msg_.c_str(); }
};



namespace Sequential {

/** \brief Executes multiple tasks sequentially
 *
 * This task dispatcher is useful mainly for test cases, when testing tasks (and to see if
 * a set of classes adhere well to the task dispatcher type interfaces, see \ref
 * pageTaskManagerDispatcher).
 *
 * <ul>
 * <li> \a Task must be a \ref pageInterfaceTask compliant type 
 *
 * <li> \a TaskCData should conform to the \ref pageInterfaceTaskCData.
 *
 * <li> \a ResultsCollector must be a \ref pageInterfaceResultsCollector compliant type 
 *
 * <li> \a LoggerType is the type used for logging messages (derived from \ref Logger::LoggerBase)
 *
 * <li> \a CountIntType should be a type to use to count the number of tasks. Usually
 *      there's no reason not to use an \c int.
 *
 * </ul>
 */
template<typename TaskType_, typename TaskCData_, typename ResultsCollector_,
         typename LoggerType_, typename CountIntType_ = int>
TOMOGRAPHER_EXPORT class TaskDispatcher
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

  typedef
#if defined(__GNUC__) && __GNUC__ == 4 && __GNUC_MINOR__ <= 6 && !defined(__clang__)
    std::chrono::monotonic_clock // for GCC/G++ 4.6
#else
    std::chrono::steady_clock
#endif
    StdClockType;


  struct TaskMgrIface {
    TaskMgrIface(TaskDispatcher * dispatcher_)
      : dispatcher(dispatcher_),
        interrupt_requested(0),
        status_report_requested(0),
        status_report_user_fn(),
        _tasks_start_time(StdClockType::now()),
        _last_status_report(StdClockType::now()),
        _status_report_periodic_interval(0)
    {
    }

  private:
    TaskDispatcher * dispatcher;

    volatile std::sig_atomic_t interrupt_requested; // could be written to by signal handler
    volatile std::sig_atomic_t status_report_requested; // could be written to by signal handler
    FullStatusReportCallbackType status_report_user_fn;

    const StdClockType::time_point _tasks_start_time;
    StdClockType::time_point _last_status_report;
    StdClockType::duration _status_report_periodic_interval;

    friend class TaskDispatcher;

    inline void _request_status_report() {
      status_report_requested = 1;
    }
    inline void _request_interrupt() {
      interrupt_requested = 1;
    }
    inline void _request_periodic_status_report(int milliseconds) {
      if ( milliseconds >= 0 ) {
        _status_report_periodic_interval = std::chrono::duration_cast<StdClockType::duration>(
            std::chrono::milliseconds(1+milliseconds)
            );
      } else {
        _status_report_periodic_interval = StdClockType::duration(0);
      }
    }

  public:
    inline bool statusReportRequested() const
    {
      if (interrupt_requested) {
        throw TasksInterruptedException();
      }
      if (_status_report_periodic_interval.count() > 0
          && (StdClockType::now() - (_last_status_report + _status_report_periodic_interval)).count() > 0) {
        return true;
      }
      return (bool) status_report_requested;
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

      fullstatus.workers_reports.resize(1);
      fullstatus.workers_reports[0] = statreport;

      fullstatus.elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
          StdClockType::now() - _tasks_start_time
          ).count() * 1e-6;

      status_report_user_fn(fullstatus);

      status_report_requested = false;
      _last_status_report = StdClockType::now();
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
  
  /** \brief Run the tasks
   *
   *
   */
  void run()
  {
    results->init(num_total_runs, CountIntType(1), pcdata);
    
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
   * FullStatusReport "FullStatusReport<TaskStatusReportType>".
   *
   */
  template<typename Fn>
  inline void setStatusReportHandler(Fn fnstatus)
  {
    mgriface.status_report_user_fn = fnstatus;
  }
  
  /** \brief Request a status report
   *
   * This function makes a note that a status report has been requested.  Subsequently,
   * the currently running task should notice it (provided it regularly queries for status
   * report requests as described on the page \ref pageInterfaceTask), and provides a
   * status report.  This status report, along with some additional information such as
   * overall progress in number of task forms the full status report which is passed on to
   * the callback set with \ref setStatusReportHandler().
   *
   * \note This function is safe to be called from within a signal handler.
   */
  inline void requestStatusReport()
  {
    mgriface._request_status_report();
  }

  /** \brief Request a status report periodically
   *
   * After this function is called, a status report will be automatically deliviered every
   * \a milliseconds milliseconds to the handler set by \ref setStatusReportHandler().
   *
   * Pass \a -1 to cancel the periodic status reporting.
   */
  inline void requestPeriodicStatusReport(int milliseconds)
  {
    mgriface._request_periodic_status_report(milliseconds);
  }

  /** \brief Interrupt all tasks as soon as possible
   *
   * As soon as the tasks notice this request, they will quit.  Any computation performed
   * until then is undefined, and the run() function throws a \ref
   * TasksInterruptedException.
   */
  inline void requestInterrupt()
  {
    mgriface._request_interrupt();
  }
  
}; // class TaskDispatcher


} // namespace Sequential

} // namespace MultiProc

} // namespace Tomographer

#endif
