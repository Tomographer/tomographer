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

#include <thread>
#include <mutex>

#include <boost/exception/diagnostic_information.hpp>

#include <tomographer/tools/loggers.h>
#include <tomographer/tools/cxxutil.h> // tomographer_assert()
#include <tomographer/tools/needownoperatornew.h>
#include <tomographer/multiproc.h>




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
TOMOGRAPHER_EXPORT class ThreadSanitizerLogger
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
   * This constructor accepts arbitrary more arguments and ignores them.  The reason is
   * because the task dispatcher does not know for sure which type the task-logger is (you
   * can specify your custom type), and will always invoke the constructor with additional
   * parameters such as a pointer to the \a TaskCData.  Here we don't need those so we can
   * just ignore any additional args.
   */
  template<typename... MoreArgs>
  ThreadSanitizerLogger(BaseLogger & logger, std::mutex * mutex)
    // NOTE: pass the baselogger's level on here. The ThreadSanitizerLogger's level is
    // this one, and is fixed and cannot be changed while running.
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
 *         Tomographer::MultiProc::OMP::ThreadSanitizerLogger<typename BaseLogger> --
 *         see \ref Tomographer::Logger::LoggerTraits<typename LoggerType>
 *
 * Logger traits for \ref MultiProc::OMP::ThreadSanitizerLogger.
 */
template<typename BaseLogger>
struct LoggerTraits<MultiProc::CxxThreads::ThreadSanitizerLogger<BaseLogger> > : public LoggerTraits<BaseLogger>
{
  /** \brief Special flags for this logger */
  enum {
    /** \brief explicitly require our logger instance to store its level. The level cannot be
     *         changed */
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
 * <ul>
 *
 * <li> \a TaskType must be a \ref pageInterfaceTask compliant type.  This type specifies
 *      the task which has to be run.  Objects of this type will be instantiated within
 *      separate threads to run the tasks.
 *
 * <li> \a TaskCData should conform to the \ref pageInterfaceTaskCData.
 *
 *      \a TaskCData may be any struct which contains all the information which
 *      needs to be accessed by the task. It should be read-only, i.e. the task should
 *      not need to write to this information. (This typically encodes the data of the
 *      problem, ie. experimental measurement results.)
 *
 * <li> \a ResultsCollector must be a \ref pageInterfaceResultsCollector compliant type 
 *
 * <li> \a LoggerType is a logger type derived from \ref Logger::LoggerBase, for example
 *      \ref Logger::FileLogger. This is the type of a logger defined in the caller's
 *      scope (and given as constructor argument here) to which messages should be logged
 *      to.
 *
 * <li> \a TaskLoggerType is the type of the logger which will be provided to tasks inside
 *      the parallel section. Such logger should ensure that the logging is
 *      thread-safe. By default \a TaskLoggerType is nothing else than an appropriate \ref
 *      ThreadSanitizerLogger.
 *
 *      (Note that if the originally given \c logger is thread-safe (see \ref
 *      Logger::LoggerTraits), then \ref ThreadSanitizerLogger directly relays calls
 *      without wrapping them into mutex locking blocks.)
 *
 *      For each task, a new \a TaskLoggerType will be created. The constructor is expected
 *      to accept the following arguments:
 *      \code
 *         TaskLoggerType(LoggerType & baselogger, const TaskCData * pcdata, CountIntType thread_id)
 *      \endcode
 *      where \a baselogger is the logger given to the \a TaskDispatcher constructor,
 *      \a pcdata is the constant shared data pointer also given to the constructor, and
 *      \a k is the thread ID number (which may range from 0 to the total number of
 *      threads minus one). The task logger is NOT constructed in a thread-safe code
 *      region.
 *
 * <li> \a CountIntType should be a type to use to count the number of tasks. Usually
 *      there's no reason not to use an \c int.
 *
 * </ul>
 *
 */
template<typename TaskType_, typename TaskCData_, typename ResultsCollector_,
         typename LoggerType_, typename CountIntType_ = int>
TOMOGRAPHER_EXPORT class TaskDispatcher
{
public:
  //! The task type
  typedef TaskType_ TaskType;
  //! The type used by a single task when providing a status report
  typedef typename TaskType::StatusReportType TaskStatusReportType;
  //! The type which stores constant, shared data for all tasks to access
  typedef TaskCData_ TaskCData;
  //! The type which is responsible to collect the final results of the individual tasks
  typedef ResultsCollector_ ResultsCollector;
  //! The logger type specified to the dispatcher (not necessarily thread-safe)
  typedef LoggerType_ LoggerType;
  //! Integer type used to count the number of tasks to run (or running)
  typedef CountIntType_ CountIntType;
  //! A thread-safe logger type which is passed on to the child tasks
  typedef ThreadSanitizerLogger<LoggerType_> TaskLoggerType;
  //! The type to use to generate a full status report of all running tasks
  typedef FullStatusReport<TaskStatusReportType> FullStatusReportType;

  /** \brief The relevant type for a callback function (or callable) which is provided
   *         with the full status report
   *
   * See \ref setStatusReportHandler().
   */
  typedef std::function<void(const FullStatusReportType&)> FullStatusReportCallbackType;

private:

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
  struct TaskInnerException : public std::exception {
    std::string msg;
  public:
    TaskInnerException(std::string msgexc) : msg("Task raised an exception: "+msgexc) { }
    virtual ~TaskInnerException() throw() { };
    const char * what() const throw() { return msg.c_str(); }
  };

  //! thread-shared variables
  struct thread_shared_data {
    thread_shared_data(const TaskCData * pcdata_, ResultsCollector * results_, LoggerType & logger_,
                       CountIntType num_total_runs, CountIntType num_threads)
      : pcdata(pcdata_),
        results(results_),
        logger(logger_),
        time_start(),
        schedule(num_total_runs, num_threads),
        status_report()
    { }

    const TaskCData * pcdata;
    std::mutex user_mutex; // mutex for IO, as well as interface user interaction (results collector etc.)

    ResultsCollector * results;
    LoggerType & logger;

    StdClockType::time_point time_start;

    struct Schedule {
      const CountIntType num_threads;
      CountIntType num_active_working_threads;

      const CountIntType num_total_runs;
      CountIntType num_completed;
      CountIntType num_launched;

      std::sig_atomic_t interrupt_requested;
      std::string inner_exception;

      std::mutex mutex;

      Schedule(CountIntType num_total_runs_, CountIntType num_threads_)
        : num_threads(num_threads_),
          num_active_working_threads(0),
          num_total_runs(num_total_runs_),
          num_completed(0),
          num_launched(0),
          interrupt_requested(0),
          inner_exception(),
          mutex()
      {
      }
    };
    Schedule schedule;

    struct StatusReport {
      bool underway;
      bool initialized;
      bool ready;
      int periodic_interval;
      CountIntType numreportsrecieved;
      FullStatusReportType full_report;
      FullStatusReportCallbackType user_fn;

      std::sig_atomic_t counter;

      std::mutex mutex;

      StatusReport()
        : underway(false),
          initialized(false),
          ready(false),
          periodic_interval(-1),
          numreportsrecieved(0),
          full_report(),
          user_fn(),
          counter(0),
          mutex()
      {
      }
    };
    StatusReport status_report;

    template<typename Struct, typename Fn>
    void with_lock(Struct & s, Fn fn) {
      std::lock_guard<std::mutex> lock(s.mutex);
      fn(s);
    }
  };

  //! thread-local variables and stuff &mdash; also serves as TaskManagerIface
  struct thread_private_data
  {
    const CountIntType thread_id;

    thread_shared_data * shared_data;

    TaskLoggerType & logger;

    CountIntType task_id;
    CountIntType local_status_report_counter;

    thread_private_data(CountIntType thread_id_, thread_shared_data * shared_data_, TaskLoggerType & logger_)
      : thread_id(thread_id_),
        shared_data(shared_data_),
        logger(logger_),
        task_id(-1),
        local_status_report_counter(0)
    {
    }

    inline bool statusReportRequested() const
    {
      if (shared_data->schedule.interrupt_requested) {
        logger.longdebug("CxxThreads::thread_private_data::statusReportRequested()",
                         "tasks interrupt has been requested");
        throw TaskInterruptedInnerException();
      }

      //
      // if we're the master thread, we have some admin to do.
      //      
      if (thread_id == 0) {
        // Update the status_report_counter according to whether
        // we should provoke a periodic status report
        if (shared_data->status_report.periodic_interval > 0 && shared_data->status_report.user_fn) {
          _master_thread_update_status_report_periodic_interval_counter();
        }

        // if we're the master thread, then also check if there is a status report ready
        // to be sent.
        if (shared_data->status_report.ready) {
          logger.longdebug("Tomographer::MultiProc::CxxThreads::thread_private_data::statusReportRequested()",
                           "Status report is ready.");

          // guard this block for status_report access
          std::lock(shared_data->status_report.mutex, shared_data->user_mutex);
          std::lock_guard<std::mutex> lck1(shared_data->status_report.mutex, std::adopt_lock);
          std::lock_guard<std::mutex> lck2(shared_data->user_mutex, std::adopt_lock);

          // call user-defined status report handler
          shared_data->status_report.user_fn(shared_data->status_report.full_report);
          // all reports recieved: done --> reset our status_report flags
          shared_data->status_report.numreportsrecieved = 0;
          shared_data->status_report.underway = false;
          shared_data->status_report.initialized = false;
          shared_data->status_report.ready = false;
          shared_data->status_report.full_report.workers_running.clear();
          shared_data->status_report.full_report.workers_reports.clear();
        }
      } // master thread

      return (int)local_status_report_counter != (int)shared_data->status_report.counter;
    }

    // internal use only:
    inline void _master_thread_update_status_report_periodic_interval_counter() const
    {
      shared_data->status_report.counter = (uint32_t)(
          (std::chrono::duration_cast<std::chrono::milliseconds>(
              StdClockType::now().time_since_epoch()
              ).count()  /  shared_data->status_report.periodic_interval) & 0x00FFFFFF
          ) << 6;
      // the (x << 6) (equivalent to (x * 64)) allows individual increments from
      // unrelated additional requestStatusReport() to be taken into account (allows 64
      // such additional requests per periodic status report)
    }

    inline void submitStatusReport(const TaskStatusReportType &statreport)
    {
      if ((int)local_status_report_counter == (int)shared_data->status_report.counter) {
        // error: task submitted unsollicited report
        logger.warning("CxxThreads TaskDispatcher/taskmanageriface", "Task submitted unsollicited status report");
        return;
      }

      std::lock_guard<std::mutex> lockguard(shared_data->status_report.mutex) ;

      // we've reacted to the given "signal"
      local_status_report_counter = shared_data->status_report.counter;
          
      // access to the local logger is fine as a different mutex is used
      logger.longdebug("CxxThreads TaskDispatcher/taskmanageriface", [&](std::ostream & stream) {
          stream << "status report received for thread #" << thread_id << ", treating it ...  "
                 << "numreportsrecieved=" << shared_data->status_report.numreportsrecieved
                 << " num_active_working_threads=" << shared_data->schedule.num_active_working_threads ;
        });

      //
      // If we're the first reporting thread, we need to initiate the status reporing
      // procedure and initialize the general data
      //
      if (!shared_data->status_report.initialized) {

        //
        // Check that we indeed have to submit a status report.
        //
        if (shared_data->status_report.underway) {
          // status report already underway!
          logger.warning("CxxThreads TaskDispatcher/taskmanageriface", "status report already underway!");
          return;
        }
        if (!shared_data->status_report.user_fn) {
          // no user handler set
          logger.warning("CxxThreads TaskDispatcher/taskmanageriface",
                         "no user status report handler set! Call setStatusReportHandler() first.");
          return;
        }

        shared_data->status_report.underway = true;
        shared_data->status_report.initialized = true;
        shared_data->status_report.ready = false;
              
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
        shared_data->status_report.full_report.workers_running.resize(num_threads, false);
        shared_data->status_report.full_report.workers_reports.resize(num_threads);

        shared_data->status_report.numreportsrecieved = 0;

        logger.debug("CxxThreads TaskDispatcher/taskmanageriface", [&](std::ostream & stream) {
            stream << "vectors resized to workers_running.size()="
                   << shared_data->status_report.full_report.workers_running.size()
                   << " and workers_reports.size()="
                   << shared_data->status_report.full_report.workers_reports.size()
                   << ".";
          });
      } // status_report.initialized

      //
      // Report the data corresponding to this thread.
      //
      logger.debug("CxxThreads TaskDispatcher/taskmanageriface", "thread_id=%ld, workers_reports.size()=%ld",
                   (long)thread_id, (long)shared_data->status_report.full_report.workers_reports.size());

      tomographer_assert(0 <= thread_id &&
                         (std::size_t)thread_id < shared_data->status_report.full_report.workers_reports.size());

      shared_data->status_report.full_report.workers_running[thread_id] = true;
      shared_data->status_report.full_report.workers_reports[thread_id] = statreport;

      ++ shared_data->status_report.numreportsrecieved;

      if (shared_data->status_report.numreportsrecieved == shared_data->schedule.num_active_working_threads) {
        //
        // the report is ready to be transmitted to the user: go!
        //
        // Don't send it quite yet, queue it for the master thread to send.  We add
        // this guarantee so that the status report handler can do things which only
        // the master thread can do (e.g. in Python, call PyErr_CheckSignals()).
        shared_data->status_report.ready = true;
      }

    } // submitStatusReport()

  }; // thread_private_data

  thread_shared_data shared_data;

public:
  /** \brief Task dispatcher constructor
   *
   * \param pcdata_  The constant shared data, which will be accessible by all tasks
   *
   * \param results_ The results collector instance, responsible for collecting the
   *                 results of all individual tasks
   *
   * \param logger_  The logger instance to use to log messages.  This logger does not need
   *                 to be thread safe.
   *
   * \param num_total_runs_ The number of tasks to run in total.  Recall that the inputs
   *                 to the different task instances are provided by the TaskCData's
   *                 getTaskInput() method (see \ref pageInterfaceTaskCData).
   */
  TaskDispatcher(TaskCData * pcdata_, ResultsCollector * results_, LoggerType & logger_,
                 CountIntType num_total_runs_,
                 CountIntType num_threads_ = std::thread::hardware_concurrency())
    : shared_data(pcdata_, results_, logger_, num_total_runs_, num_threads_)
  {
  }

  /** \brief Run the specified tasks
   *
   * Do everything, run tasks, collect results etc.
   */
  void run()
  {
    shared_data.logger.debug("MultiProc::CxxThreads::TaskDispatcher::run()", "Let's go!");
    shared_data.time_start = StdClockType::now();

    shared_data.results->init(shared_data.schedule.num_total_runs, 1, shared_data.pcdata);
    
    shared_data.logger.debug("MultiProc::CxxThreads::TaskDispatcher::run()", "preparing for parallel runs");

    typedef typename thread_shared_data::Schedule Schedule;

    auto worker_fn_id = [&](const int thread_id) noexcept(true) {
      
      // construct a thread-safe logger we can use
      TaskLoggerType threadsafelogger(shared_data.logger, & shared_data.user_mutex);
      
      thread_private_data privdat(thread_id, & shared_data, threadsafelogger);

      // not sure an std::ostream would be safe here threadwise...?
      privdat.logger.longdebug("Tomographer::MultiProc::CxxThreads::TaskDispatcher::run()",
                               "Thread #%d: thread-safe logger and private thread data set up", thread_id);
      
      {
        // active working region. This thread takes care of handling tasks.
        
        shared_data.with_lock(shared_data.schedule, [](Schedule & schedule) {
            ++ schedule.num_active_working_threads;
          });
        auto _f0 = Tools::finally([&](){
            shared_data.with_lock(shared_data.schedule, [](Schedule & schedule) {
                -- schedule.num_active_working_threads;
              });
          });

        for ( ;; ) {
          // continue doing stuff until we stop

          if (shared_data.schedule.interrupt_requested) {
            break;
          }

          // get new task to perform
          shared_data.with_lock(shared_data.schedule, [&privdat](Schedule & schedule) {
              if (schedule.num_launched == schedule.num_total_runs) {
                privdat.task_id = -1; // all tasks already launched -> nothing else to do
                return;
              }
              privdat.task_id = schedule.num_launched;
              ++ schedule.num_launched ;
            }) ;

          if ( privdat.task_id < 0 ) {
            // all tasks already launched -> nothing else to do
            break;
          }

          // run this task.

          {
            std::lock_guard<std::mutex> lk2(shared_data.status_report.mutex);
            privdat.local_status_report_counter = shared_data.status_report.counter;
          }

          try {

            _run_task(privdat, shared_data) ;

          } catch (TaskInterruptedInnerException & exc) {
            privdat.logger.debug("CxxThreads::run()/worker", "Tasks interrupted.") ;
            std::lock_guard<std::mutex> lk(shared_data.schedule.mutex) ;
            shared_data.schedule.interrupt_requested = true;
            break;
          } catch (...) {
            privdat.logger.debug("CxxThreads::run()/worker", "Exception caught inside task! "
                                 + boost::current_exception_diagnostic_information()) ;
            std::lock_guard<std::mutex> lk(shared_data.schedule.mutex) ;
            shared_data.schedule.interrupt_requested = true;
            shared_data.schedule.inner_exception += std::string("Exception caught inside task: ")
              + boost::current_exception_diagnostic_information() + "\n";
            break;
          }

          { std::lock_guard<std::mutex> lk(shared_data.schedule.mutex) ;
            ++ shared_data.schedule.num_completed;
          }

        } // for(;;)

      } // end of active working region, thread on longer serves to run tasks
        // (--num_active_working_threads is executed at this point)

      // only master thread should make sure it continues to serve status report requests
      if (thread_id == 0 && !shared_data.schedule.interrupt_requested) {

        const int sleep_val = std::max(shared_data.status_report.periodic_interval, 200);

        while (shared_data.schedule.num_active_working_threads > 0) {

          std::this_thread::sleep_for(std::chrono::milliseconds(sleep_val)) ;
          try {
            privdat.statusReportRequested();
          } catch (...) {
            privdat.logger.debug("CxxThreads::run()", "[master] Exception caught inside task!") ;
            std::lock_guard<std::mutex> lk(shared_data.schedule.mutex) ;
            shared_data.schedule.interrupt_requested = true;
            shared_data.schedule.inner_exception += std::string("Exception caught inside task: ")
              + boost::current_exception_diagnostic_information() + "\n";
            privdat.logger.debug("CxxThreads::run()", "[master] Exception caught inside task -- handled.") ;
            break;
          }

        }
      }
      
    } ; // worker_fn_id

    //
    // now, prepare & launch the workers
    //

    shared_data.logger.debug("MultiProc::CxxThreads::TaskDispatcher::run()", "About to launch threads");

    std::vector<std::thread> threads;

    // thread_id = 0 is reserved for ourselves.
    for (CountIntType thread_id = 1; thread_id < shared_data.schedule.num_threads; ++thread_id) {
      threads.push_back( std::thread( [thread_id,worker_fn_id]() { // do NOT capture thread_id by reference!
            worker_fn_id(thread_id);
          } ) );
    }

    // also run stuff as master thread
    worker_fn_id(0);

    std::for_each(threads.begin(), threads.end(), [](std::thread & thread) { thread.join(); }) ;

    shared_data.logger.debug("MultiProc::CxxThreads::TaskDispatcher::run()", "Threads finished");

    if (shared_data.schedule.inner_exception.size()) {
      // interrupt was requested because of an inner exception, not an explicit interrupt request
      throw std::runtime_error(shared_data.schedule.inner_exception);
    }
    
    // if tasks were interrupted, throw the corresponding exception
    if (shared_data.schedule.interrupt_requested) {
      throw TasksInterruptedException();
    }

    shared_data.results->runsFinished(shared_data.schedule.num_total_runs, shared_data.pcdata);
    
    shared_data.logger.debug("MultiProc::CxxThreads::TaskDispatcher::run()", "Done.");
  } // run()

  

private:
  void _run_task(thread_private_data & privdat, thread_shared_data & shared_data)
    TOMOGRAPHER_CXX_STACK_FORCE_REALIGN
  {

    // do not execute task if an interrupt was requested.
    if (shared_data.schedule.interrupt_requested) {
      return;
    }

    // not sure an std::ostream would be safe here threadwise...?
    privdat.logger.longdebug("Tomographer::MultiProc::CxxThreads::TaskDispatcher::_run_task()",
                             "Run #%lu: querying CData for task input", (unsigned long)privdat.task_id);

    // See OMP implementation: getTaskInput() is not thread protected
    //
    // const auto input = [](thread_shared_data & x, task_id) {
    //   std::lock_guard<std::mutex> lck(x.user_mutex);
    //   return x.pcdata->getTaskInput(task_id);
    // } (shared_data, privdat.task_id);
    const auto input = shared_data.pcdata->getTaskInput(privdat.task_id);

    // not sure an std::ostream would be safe here threadwise...?
    privdat.logger.debug("Tomographer::MultiProc::CxxThreads::TaskDispatcher::_run_task()",
                         "Running task #%lu ...", (unsigned long)privdat.task_id);
      
    // construct a new task instance
    TaskType t(input, shared_data.pcdata, privdat.logger);
      
    // not sure an std::ostream would be safe here threadwise...?
    privdat.logger.longdebug("Tomographer::MultiProc::CxxThreads::TaskDispatcher::_run_task()",
                             "Task #%lu set up.", (unsigned long)privdat.task_id);
      
    // and run it
    t.run(shared_data.pcdata, privdat.logger, &privdat);

    privdat.logger.longdebug("Tomographer::MultiProc::CxxThreads::TaskDispatcher::_run_task()",
                             "Task #%lu finished, about to collect result.", (unsigned long)privdat.task_id);
      
    { std::lock_guard<std::mutex> lck(shared_data.user_mutex);
      shared_data.results->collectResult(privdat.task_id, t.getResult(), shared_data.pcdata);
    }
    
    // {
    //   std::lock_guard<std::mutex> lck(shared_data.status_report.mutex) ;
    //   if ((int)privdat.local_status_report_counter != (int)shared_data.status_report.counter) {
    //     // status report request missed by task... do as if we had provided a
    //     // report, but don't provide report.
    //     ++ shared_data.status_report.numreportsrecieved;
    //   }
    // }

    privdat.logger.longdebug("Tomographer::MultiProc::CxxThreads::TaskDispatcher::_run_task()", "task done") ;
  }

public:

  /** \brief assign a callable to be called whenever a status report is requested
   *
   * This function remembers the given \a fnstatus callable, so that each time that \ref
   * requestStatusReport() is called at any later point, then this callback will be
   * invoked.
   *
   * The callback, when invoked, will be called with a single parameter of type \ref
   * FullStatusReport "FullStatusReport<TaskStatusReportType>".  It is guaranteed to be
   * called from within the main thread.
   */
  inline void setStatusReportHandler(FullStatusReportCallbackType fnstatus)
  {
    std::lock_guard<std::mutex> lck(shared_data.status_report.mutex) ;
    shared_data.status_report.user_fn = fnstatus;
  }

  /** \brief Request a status report
   *
   * This function makes a note that a status report has been requested.  Subsequently,
   * the tasks should notice it (provided they regularly query for status report requests
   * as described on the page \ref pageInterfaceTask), and provide status reports.  When
   * all the reports have been received from all running threads, the full status report
   * is passed on to the callback set with \ref setStatusReportHandler().
   *
   * \note This function is safe to be called from within a signal handler.
   */
  inline void requestStatusReport()
  {
    //
    // This function can be called from a signal handler. We essentially can't do
    // anything here because the state of the program can be pretty much anything,
    // including inside a malloc() or thread lock.
    //
    // So just increment an atomic int.
    //

    shared_data.status_report.counter = (shared_data.status_report.counter + 1) & 0x7f;
  }

  /** \brief Request a periodic status report
   *
   * The status report function callback set with \ref setStatusReportHandler() will be
   * called every \a milliseconds milliseconds with a status report.
   *
   * Pass \a -1 as argument to milliseconds to disable periodic status reports.
   */
  inline void requestPeriodicStatusReport(int milliseconds)
  {
    std::lock_guard<std::mutex> lck(shared_data.status_report.mutex) ;
    shared_data.status_report.periodic_interval = milliseconds;
  }

  /** \brief Request an immediate interruption of the tasks.
   *
   * Execution inside the function \ref run() will stop as soon as each workers notices
   * the interrupt request, and will emit the \ref TasksInterruptedException.
   *
   * The periodic check on the tasks' side is implemented in each tasks' check for a
   * status report, so that any \ref pageInterfaceTask-compliant type which periodically
   * checks for status reports is automatically interruptible.
   *
   * \note This function is safe to be called from within a signal handler.
   */
  inline void requestInterrupt()
  {
    // set the atomic int
    shared_data.interrupt_requested = 1;
  }

    
}; // class TaskDispatcher




} // namespace CxxThreads
} // namespace MultiProc

} // namespace Tomographer





#endif
