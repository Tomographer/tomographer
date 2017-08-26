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

#ifndef MPI_MULTIPROCMPI_H
#define MPI_MULTIPROCMPI_H

#include <csignal>

#include <string>
#include <chrono>
#include <exception>

#include <tomographer/tools/fmt.h>
#include <tomographer/tools/needownoperatornew.h>
#include <tomographer/multiproc.h>

#include <boost/mpi/environment.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/collectives.hpp>

namespace mpi = boost::mpi;



/** \file multiprocmpi.h
 *
 * \brief MPI Implementation for multi-processing
 *
 * See \ref Tomographer::MultiProc::MPI.
 */


namespace Tomographer {

namespace MultiProc {

namespace MPI {

/** \brief Handles parallel execution of tasks using MPI
 *
 * Plug into a given MPI communicator to run our tasks.
 *
 * The master process, i.e., the one with <code>comm.rank()==0</code>, is very
 * special.  Only this process has to provide the input data (the TaskCData
 * instance), and only that one will be able to retrieve the results at the end.
 * It's also the only one which can request interrupts, as well as request and
 * receive status reports.
 *
 * - \a TaskType must be a \ref pageInterfaceTask compliant type.  The types \a
 *   TaskType::ResultType and \a TaskType::StatusReportType must be \a
 *   boost::serialize-able, have associated MPI types, and be
 *   default-constructible.  In addition, \a TaskType::ResultType should be
 *   either move-constructible or copy-constructible.
 *
 * - \a TaskCData should conform to the \ref pageInterfaceTaskCData.  This class
 *   should be serializable with \a boost::serialize and have an associated MPI
 *   type; it should be default-constructible.
 *
 * - \a LoggerType is the type used for logging messages (derived from \ref
 *   Logger::LoggerBase)
 *
 * - \a TaskCountIntType should be a type to use to count the number of tasks. Usually
 *   there's no reason not to use an \c int.
 *
 */
template<typename TaskType_, typename TaskCData_, typename LoggerType_,
         typename TaskCountIntType_ = int>
class TOMOGRAPHER_EXPORT TaskDispatcher
{
public:
  typedef TaskType_ TaskType;
  typedef typename TaskType::StatusReportType TaskStatusReportType;
  typedef TaskCData_ TaskCData;
  typedef LoggerType_ LoggerType;
  typedef TaskCountIntType_ TaskCountIntType;

  typedef typename TaskType::ResultType TaskResultType;

  typedef FullStatusReport<TaskStatusReportType, int> FullStatusReportType;

  typedef std::function<void(const FullStatusReportType&)> FullStatusReportCallbackType;

private:
  
  typedef std::chrono::steady_clock StdClockType;

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

    inline void _request_status_report()
    {
      tomographer_assert(dispatcher != NULL);
      tomographer_assert(dispatcher->is_master);
      status_report_requested = 1;
      // TODO: communicate to the world that we want a status report
    }
    inline void _request_interrupt()
    {
      tomographer_assert(dispatcher != NULL);
      tomographer_assert(dispatcher->is_master);
      interrupt_requested = 1;
      // TODO: communicate to the world that we need to interrupt
    }
    template<typename IntType>
    inline void _request_periodic_status_report(IntType milliseconds)
    {
      tomographer_assert(dispatcher != NULL);
      tomographer_assert(dispatcher->is_master);
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
      // TODO: communicate report to master process...
      status_report_requested = false;
      _last_status_report = StdClockType::now();
    }

  };

  enum {
    TAG_RESULT = 100,
    TAG_REQUEST_STATUS_REPORT,
    TAG_REQUEST_INTERRUPT,
    TAG_SUBMIT_STATUS_REPORT
  };

  TaskCData * pcdata;
  mpi::communicator & comm;
  const bool is_master;
  std::vector<TaskResultType*> results;
  LoggerType & logger;

  TaskMgrIface mgriface;
  
public:
  /** \brief Construct the task dispatcher around the given MPI communicator
   *
   * The const data structure must have been initialized ONLY BY THE MASTER
   * PROCESS (defined as the one with <code>comm_.rank()==0</code>), and all
   * other processes are required to pass \a NULL to the \a pcdata_ argument
   * here.
   */
  TaskDispatcher(TaskCData * pcdata_, mpi::communicator & comm_, LoggerType & logger_, int num_task_runs)
    : pcdata(pcdata_),
      comm(comm_),
      is_master(comm_.rank() == 0),
      results(),
      logger(logger_),
      mgriface(this)
  {
    // First, the pcdata should have been initialized by the master only.  Our
    // first job is to broadcast the data to all processes.
    if (is_master) {
      tomographer_assert(pcdata != NULL) ;
    } else {
      tomographer_assert(pcdata == NULL) ;
      pcdata = new TaskCData;
    }
    // broadcast pcdata...
    mpi::broadcast(comm, *pcdata, 0);
    // now, pcdata is initialized everywhere.

  }
  ~TaskDispatcher()
  {
    for ( auto r : results ) {
      if (r != NULL) {
        delete r;
      }
    }
  }
  
  /** \brief Run the tasks
   *
   */
  void run()
  {
    if (is_master) {
      results.resize((std::size_t)comm.size(), NULL); // resize to # of processes
    }

    const auto mpi_rank = comm.rank();

    logger.debug("Tomographer::MultiProc::MPI::TaskDispatcher::run()",
                 [&](std::ostream & stream) { stream << "Task running in process #" << mpi_rank << " ..."; });
    
    auto input = pcdata->getTaskInput(mpi_rank);
    
    // construct a new task instance
    TaskType t(std::move(input), pcdata, logger);

    // and run it
    t.run(pcdata, logger, &mgriface);
    
    //
    // gather the results to the master process
    //
    if (is_master) {

      logger.debug("Tomographer::MultiProc::MPI::TaskDispatcher::run()",
                   "master done here, waiting for other processes to finish");

      tomographer_assert(results.size() > 0);
      
      results[0] = new TaskResultType(std::move(t.stealResult()));

      while ( results.find(NULL) != results.end() ) {
        // continue monitoring running processes and gathering results
        _master_regular_bookkeeping();
        // and sleep a bit
        TOMOGRAPHER_SLEEP_FOR_MS( 100 ) ;
      }

    } else {
      
      // all good, task done.  Send our result to the master process.
      logger.debug("Tomographer::MultiProc::MPI::TaskDispatcher::run()",
                   "worker done here, sending result to master");

      auto task_result = t.stealResult();

      comm.send(0, // destination
                TAG_RESULT,
                task_result);
    }

    // all done
    logger.debug("MultiProc::Sequential::TaskDispatcher::run()", "all done");
  }

private:
  void _master_regular_bookkeeping()
  {
    // see if there is any task results incoming of tasks which have finished
    auto mayberesultmsg = comm.iprobe(mpi::any_source, TAG_RESULT);
    if (mayberesultmsg) { // got something
      auto msg = std::move(*mayberesultmsg);
      tomographer_assert(msg.tag() == TAG_RESULT);

      TaskResultType * taskresult = new TaskResultType;
      comm.recv(msg.source(), m.tag(), *taskresult);

      tomographer_assert(msg.source() > 0 && msg.source() < results.size()) ;
      results[msg.source()] = taskresult;
    }

    // see if there is any status reports incoming
    // TODO......
  }


public:
  /** \brief Whether we are the master process
   *
   * Only the master process can query the task results.
   */
  inline bool isMaster() const { return is_master; }

  /** \brief The total number of task instances that were run
   *
   */
  inline TaskCountIntType numTaskRuns() const
  {
    tomographer_assert(is_master);
    return num_total_runs;
  }

  /** \brief Returns the results of all the tasks
   *
   */
  inline const std::vector<TaskResultType*> & collectedTaskResults() const
  {
    tomographer_assert(is_master);
    return results;
  }

  /** \brief Returns the result of the given task
   *
   */
  inline const TaskResultType & collectedTaskResult(std::size_t k) const
  {
    tomographer_assert(is_master);
    return *results[k];
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
    tomographer_assert(is_master);
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
  template<typename IntType>
  inline void requestPeriodicStatusReport(IntType milliseconds)
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



template<typename TaskType_, typename TaskCData_,
         typename LoggerType_, typename TaskCountIntType_ = int>
inline TaskDispatcher<TaskType_, TaskCData_, LoggerType_, TaskCountIntType_>
mkTaskDispatcher(TaskCData_ * pcdata_, LoggerType_ & logger_, TaskCountIntType_ num_total_runs_)
{
  return TaskDispatcher<TaskType_, TaskCData_, LoggerType_, TaskCountIntType_>(
      pcdata_, logger_, num_total_runs_
      );
}





} // namespace Sequential
} // namespace MultiProc
} // namespace Tomographer

#endif
