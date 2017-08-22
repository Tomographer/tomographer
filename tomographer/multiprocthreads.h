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

#ifndef MULTIPROCTHREADS_H
#define MULTIPROCTHREADS_H

#include <chrono>
#include <stdexcept>

#include <boost/exception/diagnostic_information.hpp>

#include <tomographer/tools/loggers.h>
#include <tomographer/tools/cxxutil.h> // tomographer_assert()
#include <tomographer/tools/needownoperatornew.h>
#include <tomographer/multiproc.h>
#include <tomographer/multiprocthreadcommon.h>

#include <thread>
#include <mutex>
// Include these here only, because on MinGW with mingw-std-threads, this
// includes windows headers which messes up tomographer/tools/logger.h by
// defining ERROR preprocessor symbols and other sh***t...
#ifdef TOMOGRAPHER_USE_MINGW_STD_THREAD
#  include <mingw.thread.h>
#  include <mingw.mutex.h>
#endif



/** \file multiprocthreads.h
 *
 * \brief Multiprocessing with native C++11 threads.
 *
 * See \ref Tomographer::MultiProc::CxxThreads.
 *
 */


namespace Tomographer {
namespace MultiProc {
namespace CxxThreads {


/** \brief Wrapper logger to call non-thread-safe loggers from a multithreaded environment.
 *
 * Wraps calls to emit log messages with a protective C++11 thread mutex, which ensure
 * thread-safety of the logging. Of course don't log too often, as this will drastically
 * slow down the execution of your program!!
 *
 * \note If the base logger is already thread-safe (as defined by \ref
 *       Logger::DefaultLoggerTraits::IsThreadSafe "LoggerTraits::IsThreadSafe"), then the
 *       call to emit the log is not wrapped in a critical section, but directly called.
 *
 * \warning The runtime level of this logger is fixed to the level of the base logger at
 *   the moment of instanciation. Any changes to the level of the base logger afterwards
 *   will not be reflected here. This is for thread-safety/consistency reasons.
 *
 * \warning If your base logger has a \a filterByOrigin() mechanism and is not
 *   thread-safe, this might be very slow because a OMP critical section is opened on each
 *   log message which needs to be tested for its origin.
 *
 * Example usage:
 * \code
 *   SomeLogger logger;
 *
 *   ... start threads with ... []() {
 *
 *     ... // parallel code
 *
 *     // it may not be safe to log to `logger`, because it might not be
 *     // thread-safe. So create a ThreadSanitizerLogger to which we can
 *     // safely log and pass to sub-routines that want a logger.
 *     ThreadSanitizerLogger<SomeLogger> threadsafelogger(logger);
 *
 *     threadsafelogger.longdebug( ... ); // safe
 *
 *     // the logger may be passed to subtasks
 *     FidelityHistogramStatsCollector<..., double, ThreadSanitizerLogger<SomeLogger> >
 *       fidelityhistogramcollector(..., threadsafelogger);
 *
 *     ... // more parallel code
 *
 *   }
 * \endcode
 * 
 */
template<typename BaseLogger>
class TOMOGRAPHER_EXPORT ThreadSanitizerLogger
  : public Logger::LoggerBase<ThreadSanitizerLogger<BaseLogger> >
{
public:
  static constexpr bool IsBaseLoggerThreadSafe = Logger::LoggerTraits<BaseLogger>::IsThreadSafe;
private:
  BaseLogger & _baselogger;

  std::mutex * _mutex;
public:

  /** \brief Constructor
   *
   * This constructor accepts arbitrary more arguments and ignores them.  The
   * reason is because the task dispatcher does not know for sure which type the
   * task-logger is (you can specify your custom type), and will always invoke
   * the constructor with additional parameters such as a pointer to the \a
   * TaskCData.  Here we don't need those so we can just ignore any additional
   * args.
   */
  ThreadSanitizerLogger(BaseLogger & logger, std::mutex * mutex)
    // NOTE: pass the baselogger's level on here. The ThreadSanitizerLogger's
    // level is this one, and is fixed and cannot be changed while running.
    : Logger::LoggerBase<ThreadSanitizerLogger<BaseLogger> >(logger.level()),
    _baselogger(logger),
    _mutex(mutex)
  {
  }
    
  ~ThreadSanitizerLogger()
  {
  }

    
  //! Implementation of Logger::LoggerBase::emitLog() for a base logger which is thread-safe
  TOMOGRAPHER_ENABLED_IF(IsBaseLoggerThreadSafe)
  inline void emitLog(int level, const char * origin, const std::string& msg)
  {
    _baselogger.emitLog(level, origin, msg);
  }

  //! Implementation of Logger::LoggerBase::filterByOrigin() for a base logger which is thread-safe
  TOMOGRAPHER_ENABLED_IF(Logger::LoggerTraits<BaseLogger>::HasFilterByOrigin &&
                         IsBaseLoggerThreadSafe)
  bool filterByOrigin(int level, const char * origin) const
  {
    return _baselogger.filterByOrigin(level, origin);
  }

  //! Implementation of Logger::LoggerBase::emitLog() for a base logger which is not thread-safe
  TOMOGRAPHER_ENABLED_IF(!IsBaseLoggerThreadSafe)
  inline void emitLog(int level, const char * origin, const std::string& msg)
  {
    std::lock_guard<std::mutex> lock(*_mutex);
    _baselogger.emitLog(level, origin, msg);
  }

  //! Implementation of Logger::LoggerBase::filterByOrigin() for a base logger which is not thread-safe
  TOMOGRAPHER_ENABLED_IF(Logger::LoggerTraits<BaseLogger>::HasFilterByOrigin &&
                         !IsBaseLoggerThreadSafe)
  bool filterByOrigin(int level, const char * origin) const
  {
    std::lock_guard<std::mutex> lock(*_mutex);
    return _baselogger.filterByOrigin(level, origin);
  }

};

} // namespace CxxThreads
} // namespace MultiProc

namespace Logger {
/** \brief Specialized Traits for \ref
 *         Tomographer::MultiProc::OMP::ThreadSanitizerLogger<BaseLogger> -- see \ref
 *         Tomographer::Logger::LoggerTraits<LoggerType>
 *
 * Logger traits for \ref MultiProc::OMP::ThreadSanitizerLogger.
 */
template<typename BaseLogger>
struct TOMOGRAPHER_EXPORT LoggerTraits<MultiProc::CxxThreads::ThreadSanitizerLogger<BaseLogger> >
  : public LoggerTraits<BaseLogger>
{
  /** \brief Special flags for this logger */
  enum {
    /** \brief explicitly require our logger instance to store its level. The
     *         level cannot be changed */
    HasOwnGetLevel = 0,
    /** \brief Obviously this logger is now always thread-safe */
    IsThreadSafe = 1
  };
};
} // namespace Logger


namespace MultiProc {
namespace CxxThreads {
    

/** \brief Dispatches tasks to parallel threads using C++11 native threads
 *
 * This multiprocessing implementation uses <a
 * href="http://en.cppreference.com/w/cpp/thread">the C++11 standard's API for
 * multithreading</a>.
 *
 * This task dispatcher is very similar to \ref
 * Tomographer::MultiProc::OMP::TaskDispatcher and conforms to the \ref
 * pageInterfaceTaskDispatcher type interface.
 *
 * \since Changed in %Tomographer 5.0: removed results collector, introduced
 *        collectedTaskResults() and friends
 *
 * <ul>
 *
 * <li> \a TaskType must be a \ref pageInterfaceTask compliant type.  This type
 *      specifies the task which has to be run.  Objects of this type will be
 *      instantiated within separate threads to run the tasks.
 *
 * <li> \a TaskCData should conform to the \ref pageInterfaceTaskCData.
 *
 *      \a TaskCData may be any struct which contains all the information which
 *      needs to be accessed by the task. It should be read-only, i.e. the task
 *      should not need to write to this information. (This typically encodes
 *      the data of the problem, ie. experimental measurement results.)
 *
 * <li> \a LoggerType is a logger type derived from \ref Logger::LoggerBase, for
 *      example \ref Logger::FileLogger. This is the type of a logger defined in
 *      the caller's scope (and given as constructor argument here) to which
 *      messages should be logged to.
 *
 * <li> \a TaskCountIntType should be a type to use to count the number of
 *      tasks. Usually there's no reason not to use an \c int.
 *
 * </ul>
 *
 */
template<typename TaskType_, typename TaskCData_,
         typename LoggerType_, typename TaskCountIntType_ = int>
class TOMOGRAPHER_EXPORT TaskDispatcher
  : public Tomographer::MultiProc::ThreadCommon::TaskDispatcherBase<
      TaskType_,
      TaskCountIntType_
    >
{
public:
  //! Base class, provides common functionality to all thread-based MutliProc implementations
  typedef Tomographer::MultiProc::ThreadCommon::TaskDispatcherBase<TaskType_, TaskCountIntType_> Base;

  //! The task type
  using typename Base::TaskType;
  //! The task result type
  using typename Base::TaskResultType;
  //! The type used by a single task when providing a status report
  using typename Base::TaskStatusReportType;
  //! Integer type used to count the number of tasks to run (or running)
  using typename Base::TaskCountIntType;
  //! The type to use to generate a full status report of all running tasks
  using typename Base::FullStatusReportType;

  //! The type which stores constant, shared data for all tasks to access
  typedef TaskCData_ TaskCData;

  //! The logger type specified to the dispatcher (not necessarily thread-safe)
  typedef LoggerType_ LoggerType;

  //! A thread-safe logger type which is passed on to the child tasks
  typedef ThreadSanitizerLogger<LoggerType_> TaskLoggerType;

  /** \brief The relevant type for a callback function (or callable) which is provided
   *         with the full status report
   *
   * See \ref setStatusReportHandler().
   */
  using typename Base::FullStatusReportCallbackType;

private:

  typedef typename Base::template ThreadSharedData<TaskCData, LoggerType>
    ThreadSharedDataType;

  ThreadSharedDataType shared_data;

  struct CriticalSectionManager {
    /** \brief Mutex for IO, as well as interface user interaction (status
     *         report callback fn, etc.) */
    std::mutex user_mutex;
    
    /** \brief Mutex for the \a schedule part of \a shared_data */
    std::mutex schedule_mutex;
    
    /** \brief Mutex for the \a status_report part of \a shared_data */
    std::mutex status_report_mutex;

    template<typename Fn>
    inline void critical_status_report(Fn && fn) {
      std::lock_guard<std::mutex> lck(status_report_mutex);
      fn();
    }
    template<typename Fn>
    inline void critical_status_report_and_user_fn(Fn && fn) {
      std::lock(status_report_mutex, user_mutex);
      std::lock_guard<std::mutex> lck1(status_report_mutex, std::adopt_lock);
      std::lock_guard<std::mutex> lck2(user_mutex, std::adopt_lock);
      fn();
    }
    template<typename Fn>
    inline void critical_status_report_and_schedule(Fn && fn) {
      std::lock(status_report_mutex, schedule_mutex);
      std::lock_guard<std::mutex> lck1(status_report_mutex, std::adopt_lock);
      std::lock_guard<std::mutex> lck2(schedule_mutex, std::adopt_lock);
      fn();
    }
    template<typename Fn>
    inline void critical_schedule(Fn && fn) {
      std::lock_guard<std::mutex> lck(schedule_mutex);
      fn();
    }
  };

  CriticalSectionManager critical;
  
  typedef typename Base::template ThreadPrivateData<
    ThreadSharedDataType,
    Tomographer::Logger::LocalLogger<TaskLoggerType>,
    CriticalSectionManager
    >
    ThreadPrivateDataType;

public:
  /** \brief Task dispatcher constructor
   *
   * \param pcdata  The constant shared data, which will be accessible by all
   *                tasks
   *
   * \param logger  The logger instance to use to log messages.  This logger
   *                does not need to be thread safe.
   *
   * \param num_total_runs The number of tasks to run in total.  Recall that
   *                the inputs to the different task instances are provided by
   *                the TaskCData's getTaskInput() method (see \ref
   *                pageInterfaceTaskCData).
   *
   * \param num_threads The number of parallel threads to use as workers.
   *
   */
  TaskDispatcher(TaskCData * pcdata, LoggerType & logger,
                 TaskCountIntType num_total_runs,
                 int num_threads = (int)std::thread::hardware_concurrency())
    : shared_data(pcdata, logger, num_total_runs, num_threads)
  {
  }

  TaskDispatcher(TaskDispatcher && other)
    : shared_data(std::move(other.shared_data))
    // critical(std::move(other.critical)) -- mutexes are not movable, so just
    //                                        use new ones...  ugly :(
  {
  }

  ~TaskDispatcher()
  {
  }

  /** \brief Run the specified tasks
   *
   * Do everything, run tasks, collect results etc.
   */
  void run()
  {
    auto logger = Tomographer::Logger::makeLocalLogger(TOMO_ORIGIN,
                                                       shared_data.logger);
    logger.debug("Let's go!");

    shared_data.time_start = Base::StdClockType::now();
    
    logger.debug("Preparing for parallel runs");

    auto worker_fn_id = [&](const int thread_id) noexcept(true) {
      
      // construct a thread-safe logger we can use
      TaskLoggerType threadsafelogger(shared_data.logger, & critical.user_mutex);

      Tomographer::Logger::LocalLogger<TaskLoggerType> locallogger(
          logger.originPrefix()+logger.glue()+"worker",
          threadsafelogger);
      
      ThreadPrivateDataType private_data(thread_id, & shared_data, locallogger,
                                         critical);

      locallogger.longdebug([&](std::ostream & stream) {
          stream << "Thread #" << thread_id
                 << ": thread-safe logger and private thread data set up";
        }) ;
      
      {
        // active working region. This thread now takes care of handling tasks.
        this->run_worker_enter(private_data, shared_data);
        auto _f0 = Tools::finally([&]() {
            this->run_worker_exit(private_data, shared_data);
          });

        for ( ;; ) {
          // continue doing stuff until we stop

          if (shared_data.schedule.interrupt_requested) {
            break;
          }

          // get new task to perform
          critical.critical_schedule([&]() {
              if (shared_data.schedule.num_launched ==
                  shared_data.schedule.num_total_runs) {
                private_data.task_id = -1; // all tasks already launched ->
                                           // nothing else to do
                return;
              }
              private_data.task_id = shared_data.schedule.num_launched;
              ++ shared_data.schedule.num_launched ;
            }) ;

          if ( private_data.task_id < 0 ) {
            // all tasks already launched -> nothing else to do
            break;
          }

          // run this task.

          this->run_task(private_data, shared_data) ;

        } // for(;;)

      } // end of active working region, thread on longer serves to run tasks
        // (--num_active_working_threads is executed at this point)

      // only master thread should make sure it continues to serve status report
      // requests
      if (thread_id == 0 && !shared_data.schedule.interrupt_requested) {

        this->master_continue_monitoring_status(private_data, shared_data) ;

      }
      
    } ; // worker_fn_id

    //
    // now, prepare & launch the workers
    //

    logger.debug("About to launch threads");

    std::vector<std::thread> threads;

    // thread_id = 0 is reserved for ourselves.
    for (int thread_id = 1; thread_id < shared_data.schedule.num_threads;
         ++thread_id) {
      // NOTE: do NOT capture thread_id by reference!
      threads.push_back( std::thread( [thread_id,worker_fn_id]() {
            worker_fn_id(thread_id);
          } ) );
    }

    // also run stuff as master thread
    worker_fn_id(0);

    std::for_each(threads.begin(), threads.end(),
                  [](std::thread & thread) { thread.join(); }) ;

    logger.debug("Threads finished");

    this->run_epilog(shared_data, logger);

    logger.debug("All done.");

  } // run()

  

  /** \brief Total number of task run instances
   *
   */
  inline TaskCountIntType numTaskRuns() const {
    return shared_data.schedule.num_total_runs;
  }

  /** \brief Get all the task results
   *
   */
  inline const std::vector<TaskResultType*> & collectedTaskResults() const {
    return shared_data.results;
  }

  /** \brief Get the result of a specific given task
   *
   */
  inline const TaskResultType & collectedTaskResult(std::size_t k) const {
    return *shared_data.results[(std::size_t)k];
  }


  /** \brief assign a callable to be called whenever a status report is
   *         requested
   *
   * This function remembers the given \a fnstatus callable, so that each time
   * that \ref requestStatusReport() is called at any later point, then this
   * callback will be invoked.
   *
   * The callback, when invoked, will be called with a single parameter of type
   * \ref FullStatusReport "FullStatusReport<TaskStatusReportType>".  It is
   * guaranteed to be called from within the main thread.
   */
  inline void setStatusReportHandler(FullStatusReportCallbackType fnstatus)
  {
    std::lock_guard<std::mutex> lck(critical.status_report_mutex) ;
    shared_data.status_report.user_fn = fnstatus;
  }

  /** \brief Request a one-time status report
   *
   * This function makes a note that a status report has been requested.
   * Subsequently, the tasks should notice it (provided they regularly query for
   * status report requests as described on the page \ref pageInterfaceTask),
   * and provide status reports.  When all the reports have been received from
   * all running threads, the full status report is passed on to the callback
   * set with \ref setStatusReportHandler().
   *
   * \note This function is safe to be called from within a signal handler.
   */
  inline void requestStatusReport()
  {
    //
    // This function can be called from a signal handler. We essentially can't
    // do anything here because the state of the program can be pretty much
    // anything, including inside a malloc() or thread lock.
    //
    // So just increment an atomic int.
    //

    ++ shared_data.status_report.event_counter_user;
  }

  /** \brief Request a periodic status report
   *
   * The status report function callback set with \ref setStatusReportHandler()
   * will be called every \a milliseconds milliseconds with a status report.
   *
   * Pass \a -1 as argument to milliseconds to disable periodic status reports.
   */
  inline void requestPeriodicStatusReport(int milliseconds)
  {
    std::lock_guard<std::mutex> lck(critical.status_report_mutex) ;
    shared_data.status_report.periodic_interval = milliseconds;
  }

  /** \brief Request an immediate interruption of the tasks.
   *
   * Execution inside the function \ref run() will stop as soon as each workers
   * notices the interrupt request, and will emit the \ref
   * TasksInterruptedException.
   *
   * The periodic check on the tasks' side is implemented in each tasks' check
   * for a status report, so that any \ref pageInterfaceTask -compliant type
   * which periodically checks for status reports is automatically
   * interruptible.
   *
   * \note This function is safe to be called from within a signal handler.
   */
  inline void requestInterrupt()
  {
    // set the atomic int
    shared_data.schedule.interrupt_requested = 1;
  }
    
}; // class TaskDispatcher



template<typename TaskType_, typename TaskCData_,
         typename LoggerType_, typename TaskCountIntType_ = int>
inline
TaskDispatcher<TaskType_, TaskCData_, LoggerType_, TaskCountIntType_>
mkTaskDispatcher(TaskCData_ * pcdata_,
                 LoggerType_ & logger_,
                 TaskCountIntType_ num_total_runs_,
                 int num_threads_ = (int)std::thread::hardware_concurrency())
{
  return TaskDispatcher<TaskType_, TaskCData_, LoggerType_, TaskCountIntType_>(
      pcdata_, logger_, num_total_runs_, num_threads_
      ) ;
}



} // namespace CxxThreads
} // namespace MultiProc

} // namespace Tomographer





#endif
