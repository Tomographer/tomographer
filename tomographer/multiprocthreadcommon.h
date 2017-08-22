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

#ifndef MULTIPROCTHREADCOMMON_H
#define MULTIPROCTHREADCOMMON_H

#include <chrono>
#include <stdexcept>

#include <boost/exception/diagnostic_information.hpp>

#include <tomographer/tools/loggers.h>
#include <tomographer/tools/cxxutil.h> // tomographer_assert()
#include <tomographer/tools/needownoperatornew.h>
#include <tomographer/multiproc.h>



/** \file multiprocthreadcommon.h
 *
 * \brief Basic multiprocessing templates for thread-based Tomographer::MultiProc
 * implementations.
 *
 * See \ref Tomographer::MultiProc::ThreadCommon, as well as the specific implementations
 * \ref Tomographer::MultiProc::CxxThreads and \ref Tomographer::MultiProc::OMP.
 *
 */


#ifdef TOMOGRAPHER_USE_WINDOWS_SLEEP
// use MS Window's Sleep() function
#  include <windows.h>
#  define TOMOGRAPHER_SLEEP_FOR_MS(x) Sleep((x))
#else
// normal C++11 API function, not available on mingw32 w/ win threads
#  include <thread>
#  ifdef TOMOGRAPHER_USE_MINGW_STD_THREAD
#    include <mingw.thread.h>
#  endif
#  define TOMOGRAPHER_SLEEP_FOR_MS(x)				\
  std::this_thread::sleep_for(std::chrono::milliseconds((x)))
#endif



namespace Tomographer {
namespace MultiProc {
namespace ThreadCommon {



/** \brief Provide common functionality to thread-based MultiProc implementations
 *
 * \since This class was introduced in %Tomographer 5.2
 *
 * See also \ref Tomographer::MultiProc::CxxThreads, \ref Tomographer::MultiProc::OMP.
 *
 * <ul>
 *
 * <li> \a TaskType must be a \ref pageInterfaceTask compliant type.  This type specifies
 *      the task which has to be run.  Objects of this type will be instantiated within
 *      separate threads to run the tasks.
 *
 * <li> \a TaskCountIntType should be a type to use to count the number of tasks. Usually
 *      there's no reason not to use an \c int.
 *
 * </ul>
 *
 */
template<typename TaskType_, typename TaskCountIntType_ = int>
class TOMOGRAPHER_EXPORT TaskDispatcherBase
{
public:
  //! The task type
  typedef TaskType_ TaskType;
  //! Integer type used to count the number of tasks to run (or running)
  typedef TaskCountIntType_ TaskCountIntType;

  //! The task result type
  typedef typename TaskType::ResultType TaskResultType;
  //! The type used by a single task when providing a status report
  typedef typename TaskType::StatusReportType TaskStatusReportType;

  //! The type to use to generate a full status report of all running tasks
  typedef FullStatusReport<TaskStatusReportType,TaskCountIntType> FullStatusReportType;

  /** \brief The relevant type for a callback function (or callable) which is provided
   *         with the full status report
   *
   * This is the type used as argument to a subclass' \a setStatusReportHandler()
   * method (see \ref pageInterfaceTaskDispatcher).
   */
  typedef std::function<void(const FullStatusReportType&)> FullStatusReportCallbackType;

  
protected:

  typedef
#if defined(__GNUC__) && __GNUC__ == 4 && __GNUC_MINOR__ <= 6 && !defined(__clang__)
    std::chrono::monotonic_clock // for GCC/G++ 4.6
#else
    std::chrono::steady_clock
#endif
    StdClockType;

  struct TaskInterruptedInnerException : public std::exception {
    std::string msg;
  public:
    TaskInterruptedInnerException() : msg("Task Interrupted") { }
    virtual ~TaskInterruptedInnerException() throw() { };
    const char * what() const throw() { return msg.c_str(); }
  };

  //! thread-shared variables
  template<typename TaskCData, typename LoggerType>
  struct ThreadSharedData {
    ThreadSharedData(const TaskCData * pcdata_, LoggerType & logger_,
                     TaskCountIntType num_total_runs, int num_threads)
      : pcdata(pcdata_),
        results((std::size_t)num_total_runs, NULL),
        logger(logger_),
        time_start(StdClockType::now()),
        schedule(num_total_runs, num_threads),
        status_report()
    {
    }

    ThreadSharedData(ThreadSharedData && x)
      : pcdata(x.pcdata),
        results(std::move(x.results)),
        logger(x.logger),
        time_start(std::move(x.time_start)),
        schedule(std::move(x.schedule)),
        status_report(std::move(x.status_report))
    { }

    ~ThreadSharedData()
    {
      for (auto r : results) {
        if (r != NULL) {
          delete r;
        }
      }
    }

    const TaskCData * pcdata;

    // Apparently it would be better if the elements are aligned in memory, not sure if
    // it's important here:  http://stackoverflow.com/a/41387941/1694896
    std::vector<TaskResultType*> results;

    LoggerType & logger;

    StdClockType::time_point time_start;

    struct Schedule {
      int num_threads;
      int num_active_working_threads;

      const TaskCountIntType num_total_runs;
      TaskCountIntType num_completed;
      TaskCountIntType num_launched;

      volatile std::sig_atomic_t interrupt_requested;
      std::vector<std::exception_ptr> inner_exception;

      Schedule(TaskCountIntType num_total_runs_, int num_threads_)
        : num_threads(num_threads_),
          num_active_working_threads(0),
          num_total_runs(num_total_runs_),
          num_completed(0),
          num_launched(0),
          interrupt_requested(0),
          inner_exception()
      {
      }
      Schedule(Schedule && x)
        : num_threads(std::move(x.num_threads)),
          num_active_working_threads(x.num_active_working_threads),
          num_total_runs(x.num_total_runs),
          num_completed(x.num_completed),
          num_launched(x.num_launched),
          interrupt_requested(x.interrupt_requested),
          inner_exception(std::move(x.inner_exception))
      {
      }
    };
    Schedule schedule;

    struct StatusReport {
      bool in_preparation;
      bool ready;
      int periodic_interval;
      int num_waiting_reports;
      
      FullStatusReportType full_report;
      FullStatusReportCallbackType user_fn;

      //! Gets incremented when user requests status report
      volatile std::sig_atomic_t event_counter_user;

      /** \brief Master thread increments this whenever other threads should provide status
       *         report -- use unsigned to wrap overflows
       */
      unsigned int event_counter_master;

      //! Only used by master thread to detect when to send periodic status reports
      StdClockType::time_point last_report_time;

      StatusReport()
        : in_preparation(false),
          ready(false),
          periodic_interval(-1),
          num_waiting_reports(0),
          full_report(),
          user_fn(),
          event_counter_user(0),
          event_counter_master(0u),
          last_report_time()
      {
      }
      StatusReport(StatusReport && x)
        : in_preparation(x.in_preparation),
          ready(x.ready),
          periodic_interval(x.periodic_interval),
          num_waiting_reports(x.num_waiting_reports),
          full_report(std::move(x.full_report)),
          user_fn(std::move(x.user_fn)),
          event_counter_user(x.event_counter_user),
          event_counter_master(x.event_counter_master),
          last_report_time(x.last_report_time)
      {
      }
    };
    StatusReport status_report;
  };

  //! thread-local variables and stuff &mdash; also serves as TaskManagerIface
  template<typename ThreadSharedDataType, typename TaskLocalLoggerType, typename CriticalExecutionLocker>
  struct ThreadPrivateData
  {
    const int thread_id;

    CriticalExecutionLocker & locker;

    ThreadSharedDataType * shared_data;

    TaskLocalLoggerType & llogger;

    TaskCountIntType task_id;

    // only used by master thread
    int local_status_report_event_counter_user;
    // used by all threads
    unsigned int local_status_report_event_counter;

    
    ThreadPrivateData(int thread_id_, ThreadSharedDataType * shared_data_,
                      TaskLocalLoggerType & llogger_, CriticalExecutionLocker & locker_)
      : thread_id(thread_id_),
        locker(locker_),
        shared_data(shared_data_),
        llogger(llogger_),
        task_id(-1),
        local_status_report_event_counter_user(0),
        local_status_report_event_counter(0u)
    {
    }
    ThreadPrivateData(const ThreadPrivateData & x)
      : thread_id(x.thread_id),
        locker(x.locker),
        shared_data(x.shared_data),
        llogger(x.llogger),
        task_id(x.task_id),
        local_status_report_event_counter_user(x.local_status_report_event_counter_user),
        local_status_report_event_counter(x.local_status_report_event_counter)
    {
    }
    
    inline bool statusReportRequested()
    {
      if (shared_data->schedule.interrupt_requested) {
        llogger.subLogger("/TaskManagerIface::statusReportRequested()")
          .longdebug("Tasks interrupt has been requested");
        throw TaskInterruptedInnerException();
      }

      //
      // if we're the master thread, we have some admin to do.
      //
      if (thread_id == 0) {
        // update event counters
        _master_update_event_counter();

        // if we're the master thread, then also check if there is a status report ready
        // to be sent.
        if (shared_data->status_report.ready) {
          _master_send_status_report();
        }
      } // master thread

      return local_status_report_event_counter != shared_data->status_report.event_counter_master;
    }

    // internal use only:
    inline void _master_update_event_counter()
    {
      if (local_status_report_event_counter_user != (int)shared_data->status_report.event_counter_user) {

        // user requested new status report -- need to do some initialization etc.

        // first, note that we have responded to this request
        local_status_report_event_counter_user = (int)shared_data->status_report.event_counter_user;

        _master_initiate_status_report();

      } else if ( shared_data->status_report.periodic_interval > 0 &&
                  ( std::chrono::duration_cast<std::chrono::milliseconds>(
                      StdClockType::now() - shared_data->status_report.last_report_time
                      ).count()  >  shared_data->status_report.periodic_interval ) ) {
        // enough time has passed since last status report, generate new one

        _master_initiate_status_report();

      }
    }

    inline void _master_initiate_status_report()
    {
      locker.critical_status_report([&]() {
          // a LocalLogger wrapping the unprotected shared_data->logger (we are
          // already in a critical section!!)
          auto logger = Tomographer::Logger::makeLocalLogger(
              llogger.originPrefix() + llogger.glue()
              + std::string("TaskManagerIface::statusReportRequested()"),
              shared_data->logger
              );
            
            // report already in preparation, ignore this request
            if (shared_data->status_report.in_preparation) {
              logger.debug("Still working on previous status report, ignoring new report due");
              return; // no new status report, we're still working on previous one
            }

            if (!shared_data->status_report.user_fn) {
              // no user handler set
              logger.warning("no user status report handler set! Call setStatusReportHandler() first.");
              return;
            }

            shared_data->status_report.in_preparation = true;
            shared_data->status_report.ready = false;

            // mark the last report time as the moment the report is
            // *initiated*, so that report interval does not get added the
            // overhead of preparing the report itself
            shared_data->status_report.last_report_time = StdClockType::now();
              
            // initialize status report object & overall data
            shared_data->status_report.full_report = FullStatusReportType();
            shared_data->status_report.full_report.num_completed = shared_data->schedule.num_completed;
            shared_data->status_report.full_report.num_total_runs = shared_data->schedule.num_total_runs;
            shared_data->status_report.full_report.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                StdClockType::now() - shared_data->time_start
                ).count() * 1e-3;
            int num_threads = shared_data->schedule.num_threads;
              
            // initialize task-specific reports
            // fill our lists with default-constructed values & set all running to false.
            shared_data->status_report.full_report.workers_running.clear();
            shared_data->status_report.full_report.workers_reports.clear();
            shared_data->status_report.full_report.workers_running.resize((std::size_t)num_threads, false);
            shared_data->status_report.full_report.workers_reports.resize((std::size_t)num_threads);

            shared_data->status_report.num_waiting_reports = shared_data->schedule.num_active_working_threads;

            logger.debug([&](std::ostream & stream) {
                stream << "vectors resized to workers_running.size()="
                       << shared_data->status_report.full_report.workers_running.size()
                       << " and workers_reports.size()="
                       << shared_data->status_report.full_report.workers_reports.size()
                       << ".";
              });
            
            // now update the master event counter, so that all threads provide their reports
            ++ shared_data->status_report.event_counter_master;
          }) ;
    }

    inline void _master_send_status_report()
    {
      auto logger = llogger.subLogger("/TaskManagerIface::statusReportRequested()");
      logger.longdebug("Status report is ready, sending to user function.");

      locker.critical_status_report_and_user_fn([&](){
          // call user-defined status report handler
          shared_data->status_report.user_fn(std::move(shared_data->status_report.full_report));
          // all reports received: done --> reset our status_report flags
          shared_data->status_report.in_preparation = false;
          shared_data->status_report.ready = false;
          shared_data->status_report.num_waiting_reports = 0;
          shared_data->status_report.full_report.workers_running.clear();
          shared_data->status_report.full_report.workers_reports.clear();
        }) ;
    }

    inline void submitStatusReport(const TaskStatusReportType & report)
    {
      local_status_report_event_counter = shared_data->status_report.event_counter_master;
          
      // use protected logger
      llogger.longdebug([&](std::ostream & stream) {
          stream << "status report received for thread #" << thread_id
                 << ", treating it ...  "
                 << "number of reports still expected="
                 << shared_data->status_report.num_waiting_reports
                 << " num_active_working_threads="
                 << shared_data->schedule.num_active_working_threads ;
        });

      //
      // Report the data corresponding to this thread.
      //
      llogger.debug([&](std::ostream & stream) {
          stream << "thread_id=" << thread_id << ", workers_reports.size()="
                 << shared_data->status_report.full_report.workers_reports.size();
        }) ;

      locker.critical_status_report_and_schedule([&]() {

          // do all of this inside critical section, to make sure the
          // worker_reports vector is memory-updated from the master thread

          if (thread_id < 0 ||
              thread_id >= (int)shared_data->status_report.full_report.workers_reports.size()) {
            fprintf(
                stderr,
                "Tomographer::MultiProc::ThreadCommon::TaskDispatcherBase::TaskPrivateData::submitStatusReport(): "
                "Internal inconsistency: thread_id=%d out of range [0,%d]\n",
                thread_id, (int)shared_data->status_report.full_report.workers_reports.size()
                ) ;
            -- shared_data->status_report.num_waiting_reports ;
            return;
          }

          shared_data->status_report.full_report.workers_running[(std::size_t)thread_id] = true;
          shared_data->status_report.full_report.workers_reports[(std::size_t)thread_id] = report;

          int num_waiting =  -- shared_data->status_report.num_waiting_reports ;

          if (num_waiting <= 0) {
            // The report is ready to be transmitted to the user.  But don't send it
            // directly quite yet, let the master thread send it.  We add this
            // guarantee so that the status report handler can do things which only
            // the master thread can do (e.g. in Python, call PyErr_CheckSignals()).
            shared_data->status_report.ready = true;
          }
        }) ;

    } // submitStatusReport()

    inline void _interrupt_with_inner_exception(std::exception_ptr exc)
    {
      locker.critical_schedule([&]() {
          shared_data->schedule.interrupt_requested = 1;
          shared_data->schedule.inner_exception.push_back(exc);
        });
    }

    
  }; // ThreadPrivateData

  

  /** \brief Basic constructor
   *
   */
  TaskDispatcherBase()
  {
  }

  TaskDispatcherBase(TaskDispatcherBase &&)
  {
  }

  ~TaskDispatcherBase()
  {
  }
  

  /** \brief New worker in the game
   */
  template<typename ThreadPrivateDataType, typename ThreadSharedDataType>
  void run_worker_enter(ThreadPrivateDataType & private_data, ThreadSharedDataType & shared_data)
  {
    private_data.locker.critical_status_report_and_schedule([&]() {
        ++ shared_data.schedule.num_active_working_threads;
        if (shared_data.status_report.in_preparation) {
          // we just entered the game in the middle of a status report preparation
          // moment -- make sure we send in ours later
          private_data.local_status_report_event_counter =
            shared_data.status_report.event_counter_master - 1;
        } else {
          private_data.local_status_report_event_counter =
            shared_data.status_report.event_counter_master;
        }
      });
  }
  
  /** \brief A worker exits the game
   */
  template<typename ThreadPrivateDataType, typename ThreadSharedDataType>
  void run_worker_exit(ThreadPrivateDataType & private_data, ThreadSharedDataType & shared_data)
  {
    private_data.locker.critical_status_report_and_schedule([&]() {
        -- shared_data.schedule.num_active_working_threads;
        if (shared_data.status_report.in_preparation) {
          // we're just leaving the game in the middle of a status report preparation
          // moment -- so make sure we don't mess up with the status reporting
          // accounting.
          if (private_data.local_status_report_event_counter
              != shared_data.status_report.event_counter_master) {
            // We haven't sent in our report yet.  No problem, but just flag the
            // full-status-report as ready if we're the last reporting thread
            // everyone's waiting for
            if (shared_data.status_report.num_waiting_reports == 1) {
              // it's our report they're waiting for -- leave the thread as idle
              shared_data.status_report.num_waiting_reports = 0;
              shared_data.status_report.ready = true;
            }
          } else {
            // Report has already been sent in, no problem, as
            // num_waiting_reports is still accurate
          }
        }
      });
  };



  /** \brief Run a given task
   *
   */
  template<typename ThreadPrivateDataType, typename ThreadSharedDataType>
  void TOMOGRAPHER_CXX_STACK_FORCE_REALIGN
  run_task(ThreadPrivateDataType & private_data, ThreadSharedDataType & shared_data)
  {
    auto & logger = private_data.llogger;
    //... = Tomographer::Logger::makeLocalLogger(TOMO_ORIGIN, private_data.llogger.parentLogger());

    try {

      // do not execute task if an interrupt was requested.
      if (shared_data.schedule.interrupt_requested) {
        throw TaskInterruptedInnerException();
      }

      logger.longdebug([&](std::ostream & stream) {
          stream << "Run #" << private_data.task_id << ": querying CData for task input";
        }) ;

      const auto input = shared_data.pcdata->getTaskInput(private_data.task_id);

      logger.longdebug([&](std::ostream & stream) {
          stream << "Running task #" << private_data.task_id;
        }) ;
      
      // construct a new task instance
      TaskType t(input, shared_data.pcdata, logger.parentLogger());
      
      // not sure an std::ostream would be safe here threadwise...?
      logger.longdebug([&](std::ostream & stream) {
          stream << "Task #" << private_data.task_id << " set up.";
        }) ;
      
      // and run it
      t.run(shared_data.pcdata, logger.parentLogger(), &private_data);

      logger.longdebug([&](std::ostream & stream) {
          stream << "Task #" << private_data.task_id << " finished, about to collect result.";
        }) ;
    
      // collect result -- no need for protected/locked/critical access because we
      // are the only thread which will write to this element; vector is pre-allocated
      shared_data.results[(std::size_t)private_data.task_id] = new TaskResultType(t.stealResult());

      private_data.locker.critical_schedule([&shared_data]() {
          // not sure we should make this into a full atomic type, because it
          // has custom type TaskCountIntType; plus this is a real small
          // overhead at 1x / task executed
          ++ shared_data.schedule.num_completed;
        }) ;

      logger.longdebug([&](std::ostream & stream) {
          stream << "Task #" << private_data.task_id << " done.";
        }) ;

    } catch (TaskInterruptedInnerException & ) {
      
      logger.debug("Task interrupted.") ;
      return;
      
    } catch (...) {

      private_data._interrupt_with_inner_exception(std::current_exception());
      return;

    }

  }

  /** \brief To be called by master thread only to continue monitoring for status reports.
   */
  template<typename ThreadPrivateDataType, typename ThreadSharedDataType>
  void master_continue_monitoring_status(ThreadPrivateDataType & private_data, ThreadSharedDataType & shared_data)
  {
    do {
      
      // check if we need to send a status report every 100ms
      TOMOGRAPHER_SLEEP_FOR_MS( 100 ) ;

      try {

        private_data.statusReportRequested();
        
      } catch (TaskInterruptedInnerException & ) {
      
        private_data.llogger.debug("[master] tasks were interrupted, returning") ;
        return;

      } catch (...) {

        private_data.llogger.debug("[master] Exception caught inside task!") ;
        private_data._interrupt_with_inner_exception( std::current_exception() );
        private_data.llogger.debug("[master] Exception caught inside task -- handled.") ;
        return;

      }
    
    } while (shared_data.schedule.num_active_working_threads > 0) ;

  }

  /** \brief To be called after all workers are done, to e.g. throw proper
   *         exception if an error occurred.
   */
  template<typename ThreadSharedDataType, typename LocalLoggerType>
  void run_epilog(ThreadSharedDataType & shared_data, LocalLoggerType & llogger)
  {
    if (shared_data.schedule.inner_exception.size()) {
      // interrupt was requested because of an inner exception, not an explicit interrupt request
      if (shared_data.schedule.inner_exception.size() > 1) {
        llogger.warning("Multiple exceptions caught in tasks, only the first one is re-thrown");
      }
      std::rethrow_exception(shared_data.schedule.inner_exception[0]);
    }
    
    // if tasks were interrupted, throw the corresponding exception
    if (shared_data.schedule.interrupt_requested) {
      throw TasksInterruptedException();
    }
  }

}; // class TaskDispatcherBase



} // namespace ThreadCommon
} // namespace MultiProc

} // namespace Tomographer





#endif
