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
#include <algorithm>

#include <boost/exception_ptr.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/serialization/serialization.hpp>

#include <tomographer/tools/fmt.h>
#include <tomographer/tools/needownoperatornew.h>
#include <tomographer/multiproc.h>
#include <tomographer/multiprocthreadcommon.h> // TOMOGRAPHER_SLEEP_FOR_MS()

#include <boost/mpi/environment.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/collectives.hpp>

namespace mpi = boost::mpi;



/** \file multiprocmpi.h
 *
 * \brief %MPI Implementation for multi-processing
 *
 * See \ref Tomographer::MultiProc::MPI.
 */


namespace Tomographer {

namespace MultiProc {

namespace MPI {

/** \brief Handles parallel execution of tasks using %MPI
 *
 * Plug into a given %MPI environment to run our tasks.
 *
 * The master process, i.e., the one with <code>comm.rank()==0</code>, is very
 * special.  Only this process has to provide the input data (the TaskCData
 * instance), and only that one will be able to retrieve the results at the end.
 * It's also the only one which can request interrupts, as well as request and
 * receive status reports.
 *
 * - \a TaskType must be a \ref pageInterfaceTask compliant type.  The types \a
 *   TaskType::ResultType and \a TaskType::StatusReportType must be serializable
 *   with \a Boost.Serialization.  In addition, \a TaskType::ResultType should
 *   be either move-constructible or copy-constructible.
 *
 * - \a TaskCData should conform to the \ref pageInterfaceTaskCData.  This class
 *   should be serializable with \a Boost.Serialization.
 *
 * - \a LoggerType is the type used for logging messages (derived from \ref
 *   Logger::LoggerBase)
 *
 * - \a TaskCountIntType should be a type to use to count the number of tasks. Usually
 *   there's no reason not to use an \c int.
 *
 *   <b>NOTE</b>: \a TaskCountIntType must be a \b signed integer type, because we might
 *   need to use the special value \a -1
 *
 */
template<typename TaskType_, typename TaskCData_, typename BaseLoggerType_,
         typename TaskCountIntType_ = int>
class TOMOGRAPHER_EXPORT TaskDispatcher
{
public:
  typedef TaskType_ TaskType;
  typedef typename TaskType::StatusReportType TaskStatusReportType;
  typedef TaskCData_ TaskCData;
  typedef BaseLoggerType_ BaseLoggerType;
  typedef TaskCountIntType_ TaskCountIntType;

  typedef typename TaskType::ResultType TaskResultType;

  typedef FullStatusReport<TaskStatusReportType, TaskCountIntType> FullStatusReportType;

  typedef std::function<void(const FullStatusReportType&)> FullStatusReportCallbackType;

  TOMO_STATIC_ASSERT_EXPR(std::is_signed<TaskCountIntType>::value) ;

private:
  
  typedef std::chrono::steady_clock StdClockType;

  struct FullTaskResult
  {
    FullTaskResult(TaskCountIntType task_id_ = -1, TaskResultType * t = NULL,
                   std::string errmsg = std::string())
      : task_id(task_id_), task_result(t), error_msg(errmsg) { }

    TaskCountIntType task_id;
    TaskResultType * task_result; // NULL if task was interrupted
    std::string error_msg;

  private:
    friend boost::serialization::access;
    template<typename Archive>
    void serialize(Archive & a, unsigned int /*version*/)
    {
      a & task_id;
      a & task_result;
      a & error_msg;
    }
  };

  struct MasterWorkersController {
    MasterWorkersController(TaskCountIntType num_total_runs_)
      : num_total_runs(num_total_runs_),
        num_tasks_completed(0),
        num_tasks_launched(0),
        num_workers_running(0),
        workers_running(),
        tasks_start_time(),
        interrupt_requested(0),
        interrupt_reacted(0),
        full_task_results(),
        task_results()
    {
    }

    inline void start(int num_workers)
    {
      // everything is left to do
      num_tasks_completed = 0;
      num_tasks_launched = 0;
      num_workers_running = num_workers;
      workers_running.resize((std::size_t)num_workers, 0);
      // interruption flag
      interrupt_requested = 0;
      interrupt_reacted = 0;
      // resize to # of processes
      full_task_results.resize((std::size_t)num_total_runs, NULL);
      task_results.resize((std::size_t)num_total_runs, NULL);
      // et ... top chrono
      tasks_start_time = StdClockType::now();
    }

    inline TaskCountIntType pop_task()
    {
      if (num_tasks_launched >= num_total_runs) {
        return -1;
      }
      TaskCountIntType task_id = num_tasks_launched;
      ++num_tasks_launched;
      return task_id; // or, as a C guy would say, return num_tasks_launched++
    }

    bool get_interrupt_event_and_react()
    {
      if (interrupt_requested) {
        interrupt_reacted = 1;
        return true;
      }
      return false;
    }

    const TaskCountIntType num_total_runs;
    TaskCountIntType num_tasks_completed;
    TaskCountIntType num_tasks_launched;

    int num_workers_running;
    std::vector<int> workers_running;

    StdClockType::time_point tasks_start_time;

    volatile std::sig_atomic_t interrupt_requested; // could be written to by signal handler
    std::sig_atomic_t interrupt_reacted;

    std::vector<FullTaskResult*> full_task_results;
    std::vector<TaskResultType*> task_results; // for convenience, same as full_task_results[k].task_result's
  };

  struct MasterStatusReportController {
    MasterStatusReportController()
      : event_counter(0),
        reacted_event_counter(0),
        in_preparation(false),
        full_report(),
        num_reports_waiting(0),
        last_report_time(StdClockType::now()),
        user_fn(),
        periodic_interval(0)
    {
    }

    bool get_event_and_react() {
      if (in_preparation) {
        return false;
      }
      if (event_counter != reacted_event_counter) {
        reacted_event_counter = event_counter;
        return true;
      }
      return false;
    }

    void reset() {
      in_preparation = false;
      full_report = FullStatusReportType();
      num_reports_waiting = 0;
    }

    volatile std::sig_atomic_t event_counter; // could be written to by signal handler
    std::sig_atomic_t reacted_event_counter;

    bool in_preparation;

    FullStatusReportType full_report;
    int num_reports_waiting;

    StdClockType::time_point last_report_time;

    FullStatusReportCallbackType user_fn;
    int periodic_interval;
  };

  struct TaskMgrIface {
    TaskMgrIface(TaskDispatcher * dispatcher_)
      : dispatcher(dispatcher_)
    {
      tomographer_assert(dispatcher != NULL) ;
    }

    inline bool statusReportRequested() const
    {
      return dispatcher->do_bookkeeping();
    }

    inline void submitStatusReport(const TaskStatusReportType &statreport)
    {
      dispatcher->submit_status_report(statreport);
    }

  private:
    TaskDispatcher * dispatcher;
    friend class TaskDispatcher;
  };

  enum {
    _TAG_offset_number = 199, // our tag #s start from 200

    TAG_WORKER_REQUEST_NEW_TASK_ID,
    TAG_MASTER_DELIVER_NEW_TASK_ID,

    TAG_MASTER_ORDER_INTERRUPT,
    TAG_MASTER_ORDER_STATUS_REPORT,

    TAG_WORKER_SUBMIT_STATUS_REPORT,
    TAG_WORKER_SUBMIT_IDLE_STATUS_REPORT,
    TAG_WORKER_SUBMIT_RESULT,

    TAG_WORKER_HELL_YEAH_IM_OUTTA_HERE
  };

  TaskCData * pcdata;
  mpi::communicator & comm;
  const bool is_master;

  Tomographer::Logger::LocalLogger<BaseLoggerType> llogger;

  MasterWorkersController * ctrl;
  MasterStatusReportController * ctrl_status_report;

  TaskMgrIface mgriface;
  
  class interrupt_tasks : public std::exception
  {
    std::string msg;
  public:
    interrupt_tasks(std::string msg_ = "Task Interrupted") : msg(msg_) { }
    virtual ~interrupt_tasks() throw() { }
    const char * what() const throw() { return msg.c_str(); }
  };

public:
  /** \brief Construct the task dispatcher around the given %MPI communicator
   *
   * The const data structure must have been initialized ONLY BY THE MASTER
   * PROCESS (defined as the one with <code>comm_.rank()==0</code>), and all
   * other processes are required to pass \a NULL to the \a pcdata_ argument
   * here.
   */
  TaskDispatcher(TaskCData * pcdata_, mpi::communicator & comm_, BaseLoggerType & logger_, int num_task_runs)
    : pcdata(pcdata_),
      comm(comm_),
      is_master(comm_.rank() == 0),
      llogger("Tomographer::MultiProc::MPI::TaskDispatcher", logger_),
      ctrl(NULL),
      ctrl_status_report(NULL),
      mgriface(this)
  {
    if (is_master) {
      ctrl = new MasterWorkersController(num_task_runs) ;
      ctrl_status_report = new MasterStatusReportController;
    }
  }
  ~TaskDispatcher()
  {
    if (ctrl != NULL) {
      for ( auto r : ctrl->full_task_results ) {
        if (r != NULL) {
          if (r->task_result != NULL) {
            delete r->task_result;
          }
          delete r;
        }
      }
    }
  }
  
  /** \brief Run the tasks
   *
   */
  void run()
  {
    auto logger = llogger.subLogger(TOMO_ORIGIN) ;

    // First, the pcdata should have been initialized by the master only.  Our
    // first job is to broadcast the data to all processes.
    if (is_master) {
      tomographer_assert(pcdata != NULL) ;
    } else {
      tomographer_assert(pcdata == NULL) ;
    }
    // broadcast pcdata...
    mpi::broadcast(comm, pcdata, 0);
    // now, pcdata is initialized everywhere.

    logger.longdebug("pcdata is now broadcast; is_master=%c", (is_master?'y':'n'));

    if (is_master) {
      tomographer_assert(ctrl != NULL) ;
      tomographer_assert(ctrl_status_report != NULL) ;
      ctrl->start(comm.size());
    } else {
      tomographer_assert(ctrl == NULL) ;
      tomographer_assert(ctrl_status_report == NULL) ;
    }

    const auto worker_id = comm.rank();

    logger.debug([&](std::ostream & stream) {
        stream << "Worker #" << worker_id << " up and running ...";
      });
    
    bool interrupted = false;
    std::string error_msg;

    try {

      run_worker();

    } catch (interrupt_tasks & e) {
      // tasks were interrupted, sorry.
      interrupted = true;
      error_msg = e.what();
    }
    
    //
    // gather the results to the master process
    //
    if (is_master) {

      logger.debug("master done here, waiting for other processes to finish");

      // we stopped working ourselves
      -- ctrl->num_workers_running;

      while ( ctrl->num_workers_running ) {
        // continue monitoring running processes and gathering results
        logger.longdebug("num_workers_running = %d", ctrl->num_workers_running);
        try {
          do_bookkeeping();
        } catch(interrupt_tasks & e) {
          // nothing to do but wait for others to finish...
        }
        // and sleep a bit
        TOMOGRAPHER_SLEEP_FOR_MS( 100 ) ;
      }

      for (std::size_t k = 0; k < ctrl->full_task_results.size(); ++k) {
        FullTaskResult * r = ctrl->full_task_results[k];
        if (r->error_msg.size()) {
          error_msg += "\nIn Worker #" + std::to_string(k) + ":\n" + r->error_msg;
        }
      }

    } else {

      // Notify master that we're outta here
      comm.send(0, TAG_WORKER_HELL_YEAH_IM_OUTTA_HERE);

    }

    if (interrupted) {
      throw TasksInterruptedException(error_msg);
    }

    // all done
    logger.debug("all done");
  }

private:
  friend struct TaskMgrIface;

  inline bool do_bookkeeping() // return TRUE if a status report was requested
  {
    auto logger = llogger.subLogger(TOMO_ORIGIN) ;

    bool status_report_requested = false;

    if (is_master) {
      
      if (ctrl->get_interrupt_event_and_react()) {

        master_order_interrupt();
        throw interrupt_tasks();

      }

      if (!ctrl->interrupt_requested) {

        // as master, we could still be asked to do some monitoring after an
        // interrupt was requested, so be careful about this.

        if ( ctrl_status_report->get_event_and_react()) {

          logger.longdebug("Status report requested, initiating");

          status_report_requested = master_initiate_status_report();

        }  else if ( ctrl_status_report->periodic_interval > 0 &&
                     ( std::chrono::duration_cast<std::chrono::milliseconds>(
                         StdClockType::now() - ctrl_status_report->last_report_time
                         ).count()  >  ctrl_status_report->periodic_interval ) ) {

          // enough time has passed since last status report, generate new one

          logger.longdebug("Time for a new status report, initiating");

          status_report_requested = master_initiate_status_report();

        }

      }

      master_regular_worker_monitoring();

    } else {

      // normal worker -- just check for interrupt or status reports from master
      // (source rank == 0)

      auto maybeorderinterruptmsg = comm.iprobe(0, TAG_MASTER_ORDER_INTERRUPT);
      if (maybeorderinterruptmsg) {
        // got an order -- react
        auto msg = *maybeorderinterruptmsg;
        tomographer_assert(msg.tag() == TAG_MASTER_ORDER_INTERRUPT);
        tomographer_assert(msg.source() == 0);
        logger.longdebug("Receiving an interrupt order from master ... ");
        comm.recv(msg.source(), msg.tag());

        // no error message, error message already set in other process
        throw interrupt_tasks("");
      }

      auto maybeorderstatreportmsg = comm.iprobe(0, TAG_MASTER_ORDER_STATUS_REPORT);
      if (maybeorderstatreportmsg) {
        // got an order -- react
        auto msg = *maybeorderstatreportmsg;
        tomographer_assert(msg.tag() == TAG_MASTER_ORDER_STATUS_REPORT);
        tomographer_assert(msg.source() == 0);
        logger.longdebug("Receiving an status report order from master ... ");
        comm.recv(msg.source(), msg.tag());

        // we need to send in a status report.
        status_report_requested = true;

      }

    }

    return status_report_requested;
  }

  inline void submit_status_report(const TaskStatusReportType &statreport)
  {
    if (is_master) {
      // just handle our own status report
      master_handle_incoming_worker_status_report(0, &statreport);
    } else {
      // with isend() we would have to monitor the MPI request (return value) I guess...
      comm.send(0, // report to master
                TAG_WORKER_SUBMIT_STATUS_REPORT,
                statreport) ;
    }
  }


  inline bool master_initiate_status_report()
  {
    auto logger = llogger.subLogger(TOMO_ORIGIN) ;
    tomographer_assert(is_master) ;

    if (ctrl_status_report->in_preparation) {
      logger.longdebug("Skipping this report, we're still working on the previous one");
      return false;
    }

    ctrl_status_report->in_preparation = true;
    ctrl_status_report->num_reports_waiting = 0;
    ctrl_status_report->full_report = FullStatusReportType();
    ctrl_status_report->full_report.num_completed = ctrl->num_tasks_completed;
    ctrl_status_report->full_report.num_total_runs = ctrl->num_total_runs;
    ctrl_status_report->full_report.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        StdClockType::now() - ctrl->tasks_start_time
        ).count() * 1e-3;
    std::size_t num_workers = (std::size_t)comm.size();
    // initialize task-specific reports
    // fill our lists with default-constructed values & set all running to false.
    ctrl_status_report->full_report.workers_running.clear();
    ctrl_status_report->full_report.workers_reports.clear();
    ctrl_status_report->full_report.workers_running.resize(num_workers, false);
    ctrl_status_report->full_report.workers_reports.resize(num_workers);

    // order all workers to report on their status
    for (int worker_id = 1; worker_id < comm.size(); ++worker_id) {
      if (ctrl->workers_running[(std::size_t)worker_id]) {
        // if this worker is running, send it a status report order
        comm.send(worker_id, TAG_MASTER_ORDER_STATUS_REPORT) ;
        ++ ctrl_status_report->num_reports_waiting;
      }
    }
    if (ctrl->workers_running[0]) {
      // make num_reports_waiting account for master's own status report
      ++ ctrl_status_report->num_reports_waiting;
    }

    return true;
  }

  inline void master_order_interrupt()
  {
    tomographer_assert(is_master) ;

    // we should be called in reaction to a requested interrupt
    tomographer_assert(ctrl->interrupt_requested) ;

    // We stopped because an interrupt exception was caught in the master
    // process, or a worker reported to the master process that they had failed.
    // Order all workers to stop now.
    for (int k = 1; k < comm.size(); ++k) {
      if (ctrl->workers_running[(std::size_t)k]) {
        comm.send(k, TAG_MASTER_ORDER_INTERRUPT) ;
      }
    }
  }

  inline void master_regular_worker_monitoring()
  {
    auto logger = llogger.subLogger(TOMO_ORIGIN) ;
    tomographer_assert(is_master) ;

    // see if we have to deliver a new task to someone
    { auto maybenewtaskmsg = comm.iprobe(mpi::any_source, TAG_WORKER_REQUEST_NEW_TASK_ID);
      if (maybenewtaskmsg) {
        logger.longdebug("Treating a new task id request message ... ");
        auto msg = *maybenewtaskmsg;
        tomographer_assert(msg.tag() == TAG_WORKER_REQUEST_NEW_TASK_ID);
        comm.recv(msg.source(), msg.tag());

        // send the worker a new task id
        TaskCountIntType task_id = master_get_new_task_id(msg.source());
        comm.send(msg.source(), TAG_MASTER_DELIVER_NEW_TASK_ID, task_id);
      }
    }

    // see if there is any task results incoming of tasks which have finished
    { auto mayberesultmsg = comm.iprobe(mpi::any_source, TAG_WORKER_SUBMIT_RESULT);
      if (mayberesultmsg) { // got something
        logger.longdebug("Treating a result message ... ");
        auto msg = *mayberesultmsg;
        tomographer_assert(msg.tag() == TAG_WORKER_SUBMIT_RESULT);

        FullTaskResult * result = new FullTaskResult;
        logger.longdebug("Receiving a worker's result from #%d ... ", msg.source());
        comm.recv(msg.source(), msg.tag(), *result);

        logger.longdebug("Got result.");

        master_store_task_result(msg.source(), result);
      }
    }

    // see if there is any task results incoming
    { auto maybestatmsg = comm.iprobe(mpi::any_source, TAG_WORKER_SUBMIT_STATUS_REPORT);
      if (maybestatmsg) { // got something
        logger.longdebug("Treating a status report message ... ");
        auto msg = *maybestatmsg;
        tomographer_assert(msg.tag() == TAG_WORKER_SUBMIT_STATUS_REPORT);

        TaskStatusReportType stat;
        logger.longdebug("Receiving a worker's status report from #%d ... ", msg.source());
        comm.recv(msg.source(), msg.tag(), stat);

        master_handle_incoming_worker_status_report(msg.source(), &stat);
      }
    }

    // see if there is any task results incoming reporting idle
    { auto maybeistatmsg = comm.iprobe(mpi::any_source, TAG_WORKER_SUBMIT_IDLE_STATUS_REPORT);
      if (maybeistatmsg) { // got something
        logger.longdebug("Treating a status report message ... ");
        auto msg = *maybeistatmsg;
        tomographer_assert(msg.tag() == TAG_WORKER_SUBMIT_IDLE_STATUS_REPORT);

        logger.longdebug("Receiving a worker's idle status report from #%d ... ", msg.source());
        comm.recv(msg.source(), msg.tag());

        master_handle_incoming_worker_status_report(msg.source(), NULL);
      }
    }

    // worker finished
    { auto mmsg = comm.iprobe(mpi::any_source, TAG_WORKER_HELL_YEAH_IM_OUTTA_HERE);
      if (mmsg) { // got something
        logger.longdebug("Treating a worker's farewell message ... ");
        auto msg = *mmsg;
        tomographer_assert(msg.tag() == TAG_WORKER_HELL_YEAH_IM_OUTTA_HERE);

        comm.recv(msg.source(), msg.tag());
        logger.debug("Received worker #%d's farewell message. Bye, you did a great job!", msg.source());

        // we got one less worker running
        -- ctrl->num_workers_running;
      }
    }


  }

  inline TaskCountIntType master_get_new_task_id(int worker_id)
  {
    auto logger = llogger.subLogger(TOMO_ORIGIN) ;

    tomographer_assert(is_master) ;

    TaskCountIntType task_id = ctrl->pop_task();

    if (task_id >= 0) {
      // worker got new task_id, will start running again
      ctrl->workers_running[(std::size_t)worker_id] = 1;
    }

    logger.debug([&](std::ostream & stream) {
        stream <<  "Got new task_id = " << task_id << " for worker #" << worker_id;
      }) ;

    logger.longdebug([&](std::ostream & stream) {
        stream << "num_workers_running now = " << ctrl->num_workers_running
               << ", all workers_running = ";
        for (std::size_t k = 0; k < ctrl->workers_running.size(); ++k) {
          if (k > 0) {
            stream << ", ";
          }
          stream << ctrl->workers_running[k];
        }
      }) ;

    return task_id;
  }

  // we're stealing the pointer here
  inline void master_store_task_result(int worker_id, FullTaskResult * result)
  {
    auto logger = llogger.subLogger(TOMO_ORIGIN) ;
    tomographer_assert(is_master) ;

    tomographer_assert(result != NULL) ;

    TaskCountIntType task_id = result->task_id;

    logger.debug([&](std::ostream & stream) {
        stream << "Got result from #" << worker_id << ", task_id=" << task_id;
      }) ;

    tomographer_assert(task_id >= 0 && (std::size_t)task_id < ctrl->full_task_results.size()) ;
    tomographer_assert(ctrl->full_task_results.size() == ctrl->task_results.size()) ;

    ctrl->full_task_results[(std::size_t)task_id] = result;
    ctrl->task_results[(std::size_t)task_id] = result->task_result;

    ++ ctrl->num_tasks_completed;

    // this worker is now (momentarily) no longer working
    ctrl->workers_running[(std::size_t)worker_id] = 0;

    logger.longdebug([&](std::ostream & stream) {
        stream << "num_workers_running now = " << ctrl->num_workers_running
               << ", all workers_running = ";
        for (std::size_t k = 0; k < ctrl->workers_running.size(); ++k) {
          if (k > 0) {
            stream << ", ";
          }
          stream << ctrl->workers_running[k];
        }
      }) ;

    if (result->error_msg.size()) {
      // task ran into an error, we should interrupt everything.  This flag will
      // be picked up by us later.
      ctrl->interrupt_requested = true;
    }

    logger.debug("Saved into results.");
  }

  inline void master_handle_incoming_worker_status_report(int worker_id, const TaskStatusReportType * stat)
  {
    auto logger = llogger.subLogger(TOMO_ORIGIN) ;
    tomographer_assert(is_master) ;

    logger.longdebug("incoming report from worker_id=%d", worker_id);

    --ctrl_status_report->num_reports_waiting;

    if (stat != NULL) {
      ctrl_status_report->full_report.workers_running[(std::size_t)worker_id] = 1;
      ctrl_status_report->full_report.workers_reports[(std::size_t)worker_id] = *stat;
    } else {
      // leave the data in the workers_[running|reports] to zero
      // value/default-initialized, meaning that the worker is in an IDLE state
      ctrl_status_report->full_report.workers_running[(std::size_t)worker_id] = 0;
    }
    
    if (ctrl_status_report->num_reports_waiting <= 0) {
      // report is ready, send it.
      logger.longdebug("Status report is ready to be sent");
      if (ctrl_status_report->user_fn) {
        logger.longdebug("Calling status report user function");
        ctrl_status_report->user_fn(ctrl_status_report->full_report) ;
      }
      // reset data
      ctrl_status_report->reset();
      ctrl_status_report->last_report_time = StdClockType::now();
      logger.longdebug("Status report finished");
    }
  }


  inline void run_worker()
  {
    auto logger = llogger.subLogger(TOMO_ORIGIN) ;

    auto worker_id = comm.rank();

    TaskCountIntType new_task_id = -1;
    for (;;) {

      if (is_master) {
        // we can get our task id directly
        new_task_id = master_get_new_task_id(0);
        logger.debug([&](std::ostream & stream) {
            stream << "Master worker: got new task id = " << new_task_id ;
          });
      } else {
        logger.debug("Requesting a new task id from master") ;
        // ask our master for a new task
        comm.send(0, // destination: master
                  TAG_WORKER_REQUEST_NEW_TASK_ID);
        comm.recv(0, // from master
                  TAG_MASTER_DELIVER_NEW_TASK_ID,
                  new_task_id);
        logger.debug([&](std::ostream & stream) {
            stream << "Worker #" << worker_id << ": got new task id = " << new_task_id ;
          });
      }

      if (new_task_id < 0) {
        // we're done, we shouldn't run any more tasks
        break;
      }

      // do some bookkeeping here as well -- just in case the task doesn't call
      // status-report-handling stuff (even though it's supposed to, you can't
      // ever trust those classes)
      do_bookkeeping();

      // run the given task
      run_task(new_task_id);
    }

    logger.debug("Worker #%d done treating tasks.", worker_id) ;

    if (!is_master) {
      // see if there are any stray interrupt orders
      auto mmsg = comm.iprobe(0, TAG_MASTER_ORDER_INTERRUPT);
      if (mmsg) {
        // got an order -- ignore
        auto msg = *mmsg;
        tomographer_assert(msg.tag() == TAG_MASTER_ORDER_INTERRUPT);
        tomographer_assert(msg.source() == 0);
        logger.longdebug("Receiving (belatedly) an interrupt order from master ... ");
        comm.recv(msg.source(), msg.tag());

        // no error message, error message already set in other process
        throw interrupt_tasks("");
      }
    }
  }

  inline void run_task(TaskCountIntType task_id)
  {
    auto logger = llogger.subLogger(TOMO_ORIGIN) ;

    std::string error_msg;

    TaskResultType * task_result = NULL;

    try {

      auto input = pcdata->getTaskInput(task_id);
    
      // construct a new task instance
      TaskType t(std::move(input), pcdata, logger.parentLogger());

      // and run it
      t.run(pcdata, logger.parentLogger(), &mgriface);

      // and collect result.
      task_result = new TaskResultType(t.stealResult());
    
    } catch (interrupt_tasks & e) {

      error_msg = e.what();

    } catch (...) {

      error_msg = std::string("Exception in task: ") +
        boost::diagnostic_information(boost::current_exception());
      
    }

    // collect the task result, and gather it to the master process

    if (is_master) {

      FullTaskResult * fullresult = new FullTaskResult(task_id, task_result, error_msg);

      // just store our own result
      master_store_task_result(0, fullresult);

    } else {

      int worker_id = comm.rank();

      // all good, task done.  Send our result to the master process.
      logger.debug("worker #%d done here, sending result to master", worker_id);

      FullTaskResult wresult(task_id, task_result, error_msg);

      comm.send(0, // destination
                TAG_WORKER_SUBMIT_RESULT,
                wresult);

      // we won't be needing this any longer, it's been serialized & transmitted
      // to master
      if (task_result != NULL) {
        delete task_result;
      }

      // make sure there is no pending status report order which we could pick up
      // when starting the next task
    
      auto maybeorderstatreportmsg = comm.iprobe(0, TAG_MASTER_ORDER_STATUS_REPORT);
      if (maybeorderstatreportmsg) {
        // got an order -- we can't ignore it, because the master is waiting for
        // our report
        auto msg = *maybeorderstatreportmsg;
        tomographer_assert(msg.tag() == TAG_MASTER_ORDER_STATUS_REPORT);
        tomographer_assert(msg.source() == 0);
        logger.longdebug("Receiving an status report order from master ... ");
        comm.recv(msg.source(), msg.tag());

        // report that we're idling for now
        comm.send(0, // report to master
                  TAG_WORKER_SUBMIT_IDLE_STATUS_REPORT);
      }
    }

    if (error_msg.size()) {
      throw interrupt_tasks(error_msg);
    }
  }


public:
  /** \brief Whether we are the master process
   *
   * Only the master process can query the task results.
   */
  inline bool isMaster() const { return is_master; }

  /** \brief The total number of task instances that were run
   *
   * \warning Only the master process can call this function.
   */
  inline TaskCountIntType numTaskRuns() const
  {
    tomographer_assert(is_master);
    tomographer_assert(ctrl != NULL);
    return ctrl->num_total_runs;
  }

  /** \brief Returns the results of all the tasks
   *
   * \warning Only the master process can call this function.
   *
   */
  inline const std::vector<TaskResultType*> & collectedTaskResults() const
  {
    tomographer_assert(is_master);
    return ctrl->task_results;
  }

  /** \brief Returns the result of the given task
   *
   * \warning Only the master process can call this function.
   *
   */
  inline const TaskResultType & collectedTaskResult(std::size_t k) const
  {
    tomographer_assert(is_master);
    tomographer_assert(ctrl != NULL) ;
    tomographer_assert(k >= 0 && k < ctrl->task_results.size()) ;
    return *ctrl->task_results[k];
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
   *
   * \warning Only the master process can call this function.
   */
  template<typename Fn>
  inline void setStatusReportHandler(Fn fnstatus)
  {
    tomographer_assert(is_master);
    ctrl_status_report->user_fn = fnstatus;
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
   *
   * \warning Only the master process can call this function.
   */
  inline void requestStatusReport()
  {
    tomographer_assert(is_master);

    ++ ctrl_status_report->event_counter; // will be picked up by master worker
  }

  /** \brief Request a status report periodically
   *
   * After this function is called, a status report will be automatically deliviered every
   * \a milliseconds milliseconds to the handler set by \ref setStatusReportHandler().
   *
   * Pass \a -1 to cancel the periodic status reporting.
   *
   * \warning Only the master process can call this function.
   */
  template<typename IntType>
  inline void requestPeriodicStatusReport(IntType milliseconds)
  {
    tomographer_assert(is_master);

    ctrl_status_report->periodic_interval = milliseconds; // will be picked up by master worker
  }

  /** \brief Interrupt all tasks as soon as possible
   *
   * As soon as the tasks notice this request, they will quit.  Any computation performed
   * until then is undefined, and the run() function throws a \ref
   * TasksInterruptedException.
   *
   * \warning Only the master process can call this function.
   */
  inline void requestInterrupt()
  {
    tomographer_assert(is_master);

    ctrl->interrupt_requested = 1; // will be picked up by master worker
  }
  
}; // class TaskDispatcher



template<typename TaskType_, typename TaskCData_,
         typename BaseLoggerType_, typename TaskCountIntType_ = int>
inline TaskDispatcher<TaskType_, TaskCData_, BaseLoggerType_, TaskCountIntType_>
mkTaskDispatcher(TaskCData_ * pcdata_, mpi::communicator & comm_, BaseLoggerType_ & baselogger_,
                 TaskCountIntType_ num_total_runs_)
{
  return TaskDispatcher<TaskType_, TaskCData_, BaseLoggerType_, TaskCountIntType_>(
      pcdata_, comm_, baselogger_, num_total_runs_
      );
}



} // namespace Sequential
} // namespace MultiProc
} // namespace Tomographer


#endif
