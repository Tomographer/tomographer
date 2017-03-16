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

#ifndef MULTIPROCOMP_H
#define MULTIPROCOMP_H

#include <csignal>
#include <chrono>
#include <thread>
#include <stdexcept>

#ifdef _OPENMP
#include <omp.h>
#else
inline constexpr int omp_get_thread_num() { return 0; }
inline constexpr int omp_get_num_threads() { return 1; }
#endif

#include <boost/exception/diagnostic_information.hpp>

#include <tomographer/tools/loggers.h>
#include <tomographer/tools/cxxutil.h> // tomographer_assert()
#include <tomographer/tools/needownoperatornew.h>
#include <tomographer/multiproc.h>

#ifdef TOMOGRAPHER_USE_WINDOWS_SLEEP
// use MS Window's Sleep() function
#include <windows.h>
#define TOMOGRAPHER_SLEEP_FOR_MS(x) Sleep((x))
#else
// normal C++11 API function, not available on mingw32 w/ win threads
#define TOMOGRAPHER_SLEEP_FOR_MS(x) std::this_thread::sleep_for(std::chrono::milliseconds((x)))
#endif


/** \file multiprocomp.h
 *
 * \brief Multiprocessing with OpenMP parallel multithreading.
 *
 * See \ref Tomographer::MultiProc::OMP.
 *
 */


namespace Tomographer {
namespace MultiProc {
namespace OMP {

namespace tomo_internal {

/** \brief Internal. Helper for \ref ThreadSanitizerLogger
 *
 * Emits the log to \c baselogger. This template is specialized for
 * <code>baseLoggerIsThreadSafe=true</code>, in which case the call to baselogger's
 * \ref emitLog() is not wrapped into a critical section.
 *
 * This helper is meant to be called as
 * \code
 *      ThreadSanitizerLoggerHelper<BaseLogger, Logger::LoggerTraits<BaseLogger>::IsThreadSafe>
 *        ::emit_log(
 *            baselogger, level, origin, msg
 *          );
 * \endcode
 */
template<typename BaseLogger, bool baseLoggerIsThreadSafe>
struct ThreadSanitizerLoggerHelper
{
  static inline void emitLog(BaseLogger & baselogger, int level, const char * origin, const std::string & msg)
  {
    bool got_exception = false;
    std::string exception_str;
#pragma omp critical
    {
      //fprintf(stderr, "ThreadSanitizerLoggerHelper::emitLog(%d, %s, %s) -- OMP CRITICAL\n", level, origin, msg.c_str());
      try {
        baselogger.emitLog(level, origin, msg);
      } catch (...) {
        got_exception = true;
        exception_str = std::string("Caught exception in emitLog: ") + boost::current_exception_diagnostic_information();
      }
    }
    if (got_exception) {
      throw std::runtime_error(exception_str);
    }
  }
  static inline bool filterByOrigin(BaseLogger & baselogger, int level, const char * origin)
  {
    bool got_exception = false;
    std::string exception_str;

    bool ok = true;
#pragma omp critical
    {
      //fprintf(stderr, "ThreadSanitizerLoggerHelper::filterByOrigin(%d, %s) -- OMP CRITICAL\n", level, origin);
      try {
        ok = baselogger.filterByOrigin(level, origin);
      } catch (...) {
        got_exception = true;
        exception_str = std::string("Caught exception in filterByOrigni: ")
          + boost::current_exception_diagnostic_information();
      }
    }
    if (got_exception) {
      throw std::runtime_error(exception_str);
    }
    return ok;
  }
};

//
// specialize the helper for when logging to a thread-safe base logger. No critical
// section needed because the logger is already thread-safe.
//
template<typename BaseLogger>
struct ThreadSanitizerLoggerHelper<BaseLogger, true>
 {
  static inline void emitLog(BaseLogger & baselogger, int level, const char * origin, const std::string & msg)
  {
    //fprintf(stderr, "ThreadSanitizerLoggerHelper::emitLog(%d, %s, %s) -- NORMAL\n", level, origin, msg.c_str());
    baselogger.emitLog(level, origin, msg);
  }
  static inline bool filterByOrigin(BaseLogger & baselogger, int level, const char * origin)
  {
    //fprintf(stderr, "ThreadSanitizerLoggerHelper::filterByOrigin(%d, %s) -- NORMAL\n", level, origin);
    return baselogger.filterByOrigin(level, origin);
  }
};

} // namespace tomo_internal


/** \brief Wrapper logger to call non-thread-safe loggers from a multithreaded environment.
 *
 * Wraps calls to emit log messages into a OpenMP
 * \code
 * #pragma omp critical
 * \endcode
 * sections, which ensure thread-safety of the logging. Of course don't log too often,
 * as this will drastically slow down the execution of your program!!
 *
 * \note If the base logger is already thread-safe (as defined by \ref
 *       Logger::DefaultLoggerTraits::IsThreadSafe "LoggerTraits::IsThreadSafe"), then the
 *       call to emit the log is not wrapped in a critical section, but directly called.
 *
 * \todo Buffer log entries here to optimize performance and to limit the number of
 *       <code>\#pragma omp critical</code> blocks.
 *       <b>---NO DON'T. It would make it complex to debug afterwards; if there is a crash,
 *       some messages may not be displayed making debugging difficult.</b>
 *
 * \warning The runtime level of this logger is fixed to the level of the base logger at
 * the moment of instanciation. Any changes to the level of the base logger afterwards
 * will not be reflected here. This is for thread-safety/consistency reasons.
 *
 * \warning If your base logger has a \a filterByOrigin() mechanism and is not
 * thread-safe, this might be very slow because a OMP critical section is opened on each
 * log message which needs to be tested for its origin.
 *
 * Example usage:
 * \code
 *   SomeLogger logger;
 *
 * #pragma omp parallel ...
 *   {
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
 *     FidelityHistogramStatsCollector<MatrQ, double, ThreadSanitizerLogger<SomeLogger> >
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
  BaseLogger & _baselogger;
public:

  /** \brief Constructor
   *
   * This constructor accepts arbitrary more arguments and ignores them.  The reason is
   * because the OMP task dispatcher does not know for sure which type the task-logger
   * is (you can specify your custom type), and will always invoke the constructor with
   * additional parameters such as a pointer to the \a TaskCData.  Here we don't need
   * those so we can just ignore any additional args.
   */
  template<typename... MoreArgs>
  ThreadSanitizerLogger(BaseLogger & logger, MoreArgs&&...)
    // NOTE: pass the baselogger's level on here. The ThreadSanitizerLogger's level is
    // this one, and is fixed and cannot be changed while running.
    : Logger::LoggerBase<ThreadSanitizerLogger<BaseLogger> >(logger.level()),
    _baselogger(logger)
  {
    // when you have to debug the debug log mechanism... lol
    //printf("ThreadSanitizerLogger(): object created\n");
    //_baselogger.debug("ThreadSanitizerLogger()", "log from constructor.");
    //emitLog(Logger::DEBUG, "ThreadSanitizerLogger!", "emitLog from constructor");
    //LoggerBase<ThreadSanitizerLogger<BaseLogger> >::debug("ThreadSanitizerLogger", "debug from constructor");
  }
    
  ~ThreadSanitizerLogger()
  {
  }

    
  //! Implementation of Logger::LoggerBase::emitLog()
  inline void emitLog(int level, const char * origin, const std::string& msg)
  {
    //printf("ThreadSanitizerLogger::emitLog(%d, %s, %s)\n", level, origin, msg.c_str());
    tomo_internal::ThreadSanitizerLoggerHelper<BaseLogger,
                                               Logger::LoggerTraits<BaseLogger>::IsThreadSafe>
      ::emitLog(
          _baselogger, level, origin, msg
          );
  }

  //! Implementation of Logger::LoggerBase::filterByOrigin()
  template<bool dummy = true>
  inline typename std::enable_if<dummy && Logger::LoggerTraits<BaseLogger>::HasFilterByOrigin, bool>::type
    filterByOrigin(int level, const char * origin) const
  {
    return tomo_internal::ThreadSanitizerLoggerHelper<BaseLogger,
                                                      Logger::LoggerTraits<BaseLogger>::IsThreadSafe>
      ::filterByOrigin(
          _baselogger, level, origin
          );
  }
};

} // namespace OMP
} // namespace MultiProc

namespace Logger {
/** \brief Specialized Traits for \ref
 *         Tomographer::MultiProc::OMP::ThreadSanitizerLogger<typename BaseLogger> --
 *         see \ref Tomographer::Logger::LoggerTraits<typename LoggerType>
 *
 * Logger traits for \ref MultiProc::OMP::ThreadSanitizerLogger.
 */
template<typename BaseLogger>
struct LoggerTraits<MultiProc::OMP::ThreadSanitizerLogger<BaseLogger> > : public LoggerTraits<BaseLogger>
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
namespace OMP {
    

/** \brief Dispatches tasks to parallel threads using OpenMP
 *
 * Uses <a href="http://openmp.org/">OpenMP</a> to parallelize the repetition of a same
 * task with different inputs.
 *
 * Check out <a href="https://computing.llnl.gov/tutorials/openMP/">this good tutorial on
 * OpenMP</a>.
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
 *      without wrapping them into OMP critical sections.)
 *
 *      For each task, a new \a TaskLoggerType will be created. The constructor is expected
 *      to accept the following arguments:
 *      \code
 *         TaskLoggerType(LoggerType & baselogger, const TaskCData * pcdata, CountIntType k)
 *      \endcode
 *      where \a baselogger is the logger given to the \a TaskDispatcher constructor,
 *      \a pcdata is the constant shared data pointer also given to the constructor, and
 *      \a k is the task number (which may range from 0 to the total number of tasks -
 *      1). The task logger is NOT constructed in a thread-safe code region, so use \c
 *      "\#pragma omp critical" if necessary. You may use \a omp_get_thread_num() and \a
 *      omp_get_num_threads() to get the current thread number and the total number of
 *      threads, respectively.
 *
 * <li> \a CountIntType should be a type to use to count the number of tasks. Usually
 *      there's no reason not to use an \c int.
 *
 * </ul>
 *
 */
template<typename TaskType_, typename TaskCData_, typename ResultsCollector_,
         typename LoggerType_, typename CountIntType_ = int,
         typename TaskLoggerType_ = ThreadSanitizerLogger<LoggerType_> >
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
  typedef TaskLoggerType_ TaskLoggerType;
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
                       CountIntType num_total_runs_, CountIntType n_chunk_)
      : pcdata(pcdata_),
        results(results_),
        logger(logger_),
        time_start(),
        status_report_underway(false),
        status_report_initialized(false),
        status_report_ready(false),
        status_report_counter(0),
        status_report_periodic_interval(-1),
        status_report_numreportsrecieved(0),
        status_report_full(),
        status_report_user_fn(),
        interrupt_requested(0),
        num_total_runs(num_total_runs_), n_chunk(n_chunk_), num_completed(0),
        num_active_working_threads(0)
    { }

    const TaskCData * pcdata;
    ResultsCollector * results;
    LoggerType & logger;

    StdClockType::time_point time_start;

    bool status_report_underway;
    bool status_report_initialized;
    bool status_report_ready;
    volatile std::sig_atomic_t status_report_counter;
    int status_report_periodic_interval;
    CountIntType status_report_numreportsrecieved;

    FullStatusReportType status_report_full;
    FullStatusReportCallbackType status_report_user_fn;

    volatile std::sig_atomic_t interrupt_requested;

    CountIntType num_total_runs;
    CountIntType n_chunk;
    CountIntType num_completed;

    CountIntType num_active_working_threads;
  };
  //! thread-local variables and stuff &mdash; also serves as TaskManagerIface
  struct thread_private_data
  {
    thread_shared_data * shared_data;

    TaskLoggerType * logger;

    CountIntType kiter;
    CountIntType local_status_report_counter;

    inline bool statusReportRequested() const
    {
      if (shared_data->interrupt_requested) {
        throw TaskInterruptedInnerException();
      }

      //
      // if we're the master thread, we have some admin to do.
      //      
      // NOTE: #pragma omp master prevents us from throwing an exception! (at least on clang++3.8)
      if (omp_get_thread_num() == 0) {
        // Update the status_report_counter according to whether
        // we should provoke a periodic status report
        if (shared_data->status_report_periodic_interval > 0) {
          _master_thread_update_status_report_periodic_interval_counter();
        }

        // if we're the master thread, then also check if there is a status report ready
        // to be sent.
        if (shared_data->status_report_ready) {
          bool got_exception = false;
          std::string exception_str;
#pragma omp critical
          {
            try {
              // call user-defined status report handler
              shared_data->status_report_user_fn(shared_data->status_report_full);
              // all reports recieved: done --> reset our status_report_* flags
              shared_data->status_report_numreportsrecieved = 0;
              shared_data->status_report_underway = false;
              shared_data->status_report_initialized = false;
              shared_data->status_report_ready = false;
              shared_data->status_report_full.workers_running.clear();
              shared_data->status_report_full.workers_reports.clear();
            } catch (...) {
              got_exception = true;
              exception_str = std::string("Caught exception in status reporting callback: ")
                + boost::current_exception_diagnostic_information();
            }
          }
          if (got_exception) {
            throw TaskInnerException(exception_str);
          }
        }
      } // omp master

      return (int)local_status_report_counter != (int)shared_data->status_report_counter;
    }

    // internal use only:
    inline void _master_thread_update_status_report_periodic_interval_counter() const
    {
      shared_data->status_report_counter = (
          (std::chrono::duration_cast<std::chrono::milliseconds>(
              StdClockType::now().time_since_epoch()
              ).count()  /  shared_data->status_report_periodic_interval) & 0x00FFFFFF
          ) << 6;
      // the (x << 6) (equivalent to (x * 64)) allows individual increments from
      // unrelated additional requestStatusReport() to be taken into account (allows 64
      // such additional requests per periodic status report)
    }

    inline void submitStatusReport(const TaskStatusReportType &statreport)
    {
      if ((int)local_status_report_counter == (int)shared_data->status_report_counter) {
        // error: task submitted unsollicited report
        logger->warning("OMP TaskDispatcher/taskmanageriface", "Task submitted unsollicited status report");
        return;
      }


      bool got_exception = false;
      std::string exception_str;
#pragma omp critical
      {
        try {
          bool ok = true; // whether to proceed or not

          // we've reacted to the given "signal"
          local_status_report_counter = shared_data->status_report_counter;
          
          // add our status report to being-prepared status report in the  shared data
          int threadnum = omp_get_thread_num();

          // Important: access the original logger, not the thread-safe-wrapped logger!!
          // Otherwise this could lead to deadlocks because of nested critical blocks.
          shared_data->logger.longdebug("OMP TaskDispatcher/taskmanageriface", [&](std::ostream & stream) {
              stream << "status report received for thread #" << threadnum << ", treating it ...";
            });

          //
          // If we're the first reporting thread, we need to initiate the status reporing
          // procedure and initialize the general data
          //
          if (!shared_data->status_report_initialized) {

            //
            // Check that we indeed have to submit a status report.
            //
            if (shared_data->status_report_underway) {
              // status report already underway!
              // Important: access the original logger, not the thread-safe-wrapped logger!!
              // Otherwise this could lead to deadlocks because of nested critical blocks.
              shared_data->logger.warning("OMP TaskDispatcher/taskmanageriface",
                                          "status report already underway!");
              ok = false;
            }
            if (!shared_data->status_report_user_fn) {
              // no user handler set
              // Important: access the original logger, not the thread-safe-wrapped logger!!
              // Otherwise this could lead to deadlocks because of nested critical blocks.
              shared_data->logger.warning("OMP TaskDispatcher/taskmanageriface",
                                          "no user status report handler set!"
                                          " call setStatusReportHandler() first.");
              ok = false;
            }

            // since we can't return out of a critical section, we use an if block.
            if (ok) {

              shared_data->status_report_underway = true;
              shared_data->status_report_initialized = true;
              shared_data->status_report_ready = false;
              
              // initialize status report object & overall data
              shared_data->status_report_full = FullStatusReportType();
              shared_data->status_report_full.num_completed = shared_data->num_completed;
              shared_data->status_report_full.num_total_runs = shared_data->num_total_runs;
              shared_data->status_report_full.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                  StdClockType::now() - shared_data->time_start
                  ).count() * 1e-3;
              // shared_data->status_report_full.num_active_working_threads = shared_data->num_active_working_threads;
              int num_threads = omp_get_num_threads();
              // shared_data->status_report_full.num_threads = num_threads;
              
              // initialize task-specific reports
              // fill our lists with default-constructed values & set all running to false.
              shared_data->status_report_full.workers_running.clear();
              shared_data->status_report_full.workers_reports.clear();
              shared_data->status_report_full.workers_running.resize(num_threads, false);
              shared_data->status_report_full.workers_reports.resize(num_threads);

              shared_data->status_report_numreportsrecieved = 0;

              // Important: access the original logger, not the thread-safe-wrapped logger!!
              // Otherwise this could lead to deadlocks because of nested critical blocks.
              shared_data->logger.debug("OMP TaskDispatcher/taskmanageriface", [&](std::ostream & stream) {
                  stream << "vectors resized to workers_running.size()="
                         << shared_data->status_report_full.workers_running.size()
                         << " and workers_reports.size()="
                         << shared_data->status_report_full.workers_reports.size()
                         << ".";
                });
            }

          } // status_report_initialized

          // if we're the first reporting thread, then maybe ok was set to false above, so
          // check again.
          if (ok) {

            //
            // Report the data corresponding to this thread.
            //
            // Important: access the original logger, not the thread-safe-wrapped logger!!
            // Otherwise this could lead to deadlocks because of nested critical blocks.
            shared_data->logger.debug("OMP TaskDispatcher/taskmanageriface", "threadnum=%ld, workers_reports.size()=%ld",
                                      (long)threadnum, (long)shared_data->status_report_full.workers_reports.size());

            tomographer_assert(0 <= threadnum &&
                               (std::size_t)threadnum < shared_data->status_report_full.workers_reports.size());

            shared_data->status_report_full.workers_running[threadnum] = true;
            shared_data->status_report_full.workers_reports[threadnum] = statreport;

            ++ shared_data->status_report_numreportsrecieved;

            if (shared_data->status_report_numreportsrecieved == shared_data->num_active_working_threads) {
              //
              // the report is ready to be transmitted to the user: go!
              //
              // Don't send it quite yet, queue it for the master thread to send.  We add
              // this guarantee so that the status report handler can do things which only
              // the master thread can do (e.g. in Python, call PyErr_CheckSignals()).
              shared_data->status_report_ready = true;
            }
          } // if ok
        } catch (...) {
          //      std::string msg(boost::current_exception_diagnostic_information());
          //      fprintf(stderr, "CAUGHT AN EXCEPTION: %s\n", msg.c_str());

          // if an exception occurred propagate it out to end the task and cause an
          // interruption. The user may not know what caused the interrupt.  Don't
          // terminate or cause a huge fuss, as this might be actually controlled
          // (e.g. boost::python::error_already_set) Also, the logger itself may have
          // caused the exception, so don't use the logger here!!:
          //shared_data->logger.debug("OMP TaskDispatcher/taskmanageriface", [&](std::ostream & stream) {
          //        stream << "Error while processing status report, exception caught: "
          //        << boost::current_exception_diagnostic_information();
          //      });
          got_exception = true;
          exception_str = std::string("Caught exception while processing status report: ")
            + boost::current_exception_diagnostic_information();
        }
      } // omp critical
      if (got_exception) {
        throw TaskInnerException(exception_str);
      }
    }
  };

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
   *
   * \param n_chunk_ How many tasks to chunk together into one thread.  This corresponds
   *                 to OpenMP's chunk argument in the instruction
   *                 <em>schedule(dynamic,chunk)</em> in a <em>\#pragma omp for</em>
   *                 instruction (see <a
   *                 href="https://computing.llnl.gov/tutorials/openMP/\#DO"
   *                 target="_blank">this page</a>).
   */
  TaskDispatcher(TaskCData * pcdata_, ResultsCollector * results_, LoggerType & logger_,
                 CountIntType num_total_runs_, CountIntType n_chunk_ = 1)
    : shared_data(pcdata_, results_, logger_, num_total_runs_, n_chunk_)
  {
  }

  /** \brief Run the specified tasks
   *
   * Do everything, run tasks, collect results etc.
   */
  void run()
  {
    shared_data.logger.debug("MultiProc::OMP::TaskDispatcher::run()", "Let's go!");
    shared_data.time_start = StdClockType::now();

    shared_data.results->init(shared_data.num_total_runs, shared_data.n_chunk, shared_data.pcdata);
    
    shared_data.logger.debug("MultiProc::OMP::TaskDispatcher::run()", "preparing for parallel runs");

#ifndef _OPENMP
    shared_data.logger.warning("MultiProc::OMP::TaskDispatcher::run()", "OpenMP is disabled; tasks will run serially.");
#endif

    // declaring these as "const" causes a weird compiler error
    // "`n_chunk' is predetermined `shared' for `shared'"
    CountIntType num_total_runs = shared_data.num_total_runs;
    CountIntType n_chunk = shared_data.n_chunk;
    (void)n_chunk; // silence "unused variable" warning when compiling without OMP support

    CountIntType k = 0;

    thread_shared_data *shdat = &shared_data;
    thread_private_data privdat;

    shared_data.logger.debug("MultiProc::OMP::TaskDispatcher::run()", "About to start parallel section");

    int num_active_parallel = 0;

    std::string inner_exception;

#pragma omp parallel default(none) private(k, privdat) shared(shdat, num_total_runs, n_chunk, num_active_parallel, inner_exception)
    {
      privdat.shared_data = shdat;
      privdat.kiter = 0;

#pragma omp atomic
      ++num_active_parallel;

      //
      // The main, parallel FOR loop:
      //
#pragma omp for schedule(dynamic,n_chunk) nowait
      for (k = 0; k < num_total_runs; ++k) {

        try {

          // make separate function call, so that we can tell GCC to realign the stack on
          // platforms which don't do that automatically (yes, MinGW, it's you I'm looking
          // at)
          _run_task(privdat, shdat, k);

        } catch (...) {
#pragma omp critical
          {
            shdat->interrupt_requested = 1;
            inner_exception += std::string("Exception caught inside task: ")
              + boost::current_exception_diagnostic_information() + "\n";
          }
        }
          
      } // omp for

#pragma omp atomic
      --num_active_parallel;

#pragma omp master
      {
        if (shdat->status_report_periodic_interval > 0) {
          // master thread should continue providing regular status updates
          while (num_active_parallel > 0) {
            TOMOGRAPHER_SLEEP_FOR_MS(shdat->status_report_periodic_interval);
            privdat._master_thread_update_status_report_periodic_interval_counter();
          }
        }
      }
      
    } // omp parallel

    if (inner_exception.size()) {
      // interrupt was requested because of an inner exception, not an explicit interrupt request
      throw std::runtime_error(inner_exception);
    }

    if (shared_data.interrupt_requested) {
      throw TasksInterruptedException();
    }
    
    shared_data.results->runsFinished(num_total_runs, shared_data.pcdata);
  }

private:
  void _run_task(thread_private_data & privdat, thread_shared_data * shdat, CountIntType k)
    TOMOGRAPHER_CXX_STACK_FORCE_REALIGN
  {

    // do not execute task if an interrupt was requested.
    if (shdat->interrupt_requested) {
      return;
    }

#pragma omp critical
    {
      ++ shdat->num_active_working_threads;
      privdat.local_status_report_counter = shdat->status_report_counter;
    }

    // construct a thread-safe logger we can use
    TaskLoggerType threadsafelogger(shdat->logger, shdat->pcdata, k);
      
    // not sure an std::ostream would be safe here threadwise...?
    threadsafelogger.longdebug("Tomographer::MultiProc::OMP::TaskDispatcher::_run_task()",
                               "Run #%lu: thread-safe logger set up", (unsigned long)k);
      
    // set up our thread-private data
    privdat.kiter = k;
    privdat.logger = &threadsafelogger;
      
    // not sure an std::ostream would be safe here threadwise...?
    threadsafelogger.longdebug("Tomographer::MultiProc::OMP::TaskDispatcher::_run_task()",
                               "Run #%lu: querying CData for task input", (unsigned long)k);

    auto input = shdat->pcdata->getTaskInput(k);

    // not sure an std::ostream would be safe here threadwise...?
    threadsafelogger.debug("Tomographer::MultiProc::OMP::TaskDispatcher::_run_task()",
                           "Running task #%lu ...", (unsigned long)k);
      
    // construct a new task instance
    TaskType t(input, shdat->pcdata, threadsafelogger);
      
    // not sure an std::ostream would be safe here threadwise...?
    threadsafelogger.longdebug("Tomographer::MultiProc::OMP::TaskDispatcher::_run_task()",
                               "Task #%lu set up.", (unsigned long)k);
      
    // and run it
    try {
      t.run(shdat->pcdata, threadsafelogger, &privdat);
    } catch (const TaskInterruptedInnerException & ) {
      return;
    }
      
    bool got_exception = false;
    std::string exception_str;
#pragma omp critical
    {
      try {
        shdat->results->collectResult(k, t.getResult(), shdat->pcdata);
      } catch (...) {
          got_exception = true;
          exception_str = std::string("Caught exception while processing status report: ")
            + boost::current_exception_diagnostic_information();
      }
        
      if ((int)privdat.local_status_report_counter != (int)shdat->status_report_counter) {
        // status report request missed by task... do as if we had provided a
        // report, but don't provide report.
        ++ shdat->status_report_numreportsrecieved;
      }
        
      ++ shdat->num_completed;
      -- shdat->num_active_working_threads;
    } // omp critical
    if (got_exception) {
      throw std::runtime_error(exception_str);
    }

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
   * called from within the main thread, that is, the one with <code>omp_get_thread_num()
   * == 0</code>.
   */
  inline void setStatusReportHandler(FullStatusReportCallbackType fnstatus)
  {
#pragma omp critical
    {
      shared_data.status_report_user_fn = fnstatus;
    }
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
    // including inside a malloc() or gomp lock. So can't call any function which needs
    // malloc or a #pragma omp critical.
    //
    // So just increment an atomic int.
    //

    shared_data.status_report_counter = (shared_data.status_report_counter + 1) & 0x7f;

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
#pragma omp critical
    {
      shared_data.status_report_periodic_interval = milliseconds;
    }
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
    shared_data.interrupt_requested = 1;
  }

    
}; // class TaskDispatcher


/** \brief Create an OMP task dispatcher. Useful if you want C++'s template argument
 *         deduction mechanism
 */
template<typename TaskType_, typename TaskCData_, typename ResultsCollector_,
         typename LoggerType_, typename CountIntType_ = int>
inline TaskDispatcher<TaskType_, TaskCData_, ResultsCollector_,
                      LoggerType_, CountIntType_>
makeTaskDispatcher(TaskCData_ * pcdata_, ResultsCollector_ * results_, LoggerType_ & logger_,
                   CountIntType_ num_total_runs_, CountIntType_ n_chunk_ = 1)
{
  // RVO should be rather obvious to the compiler
  return TaskDispatcher<TaskType_, TaskCData_, ResultsCollector_,
                        LoggerType_, CountIntType_>(
                            pcdata_, results_, logger_, num_total_runs_, n_chunk_
                            );
}



} // namespace OMP
} // namespace MultiProc

} // namespace Tomographer





#endif
