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
#include <tomographer/multiprocthreadcommon.h>


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
class TOMOGRAPHER_EXPORT ThreadSanitizerLogger
  : public Logger::LoggerBase<ThreadSanitizerLogger<BaseLogger> >
{
public:
  static constexpr bool IsBaseLoggerThreadSafe = Logger::LoggerTraits<BaseLogger>::IsThreadSafe;
private:
  BaseLogger & _baselogger;

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
    ThreadSanitizerLogger(BaseLogger & logger, MoreArgs && ...)
    // NOTE: pass the baselogger's level on here. The ThreadSanitizerLogger's level is
    // this one, and is fixed and cannot be changed while running.
    : Logger::LoggerBase<ThreadSanitizerLogger<BaseLogger> >(logger.level()),
    _baselogger(logger)
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
#pragma omp critical
    {
      _baselogger.emitLog(level, origin, msg);
    }
  }

  //! Implementation of Logger::LoggerBase::filterByOrigin() for a base logger which is not thread-safe
  TOMOGRAPHER_ENABLED_IF(Logger::LoggerTraits<BaseLogger>::HasFilterByOrigin &&
                         !IsBaseLoggerThreadSafe)
  bool filterByOrigin(int level, const char * origin) const
  {
    bool ok;
#pragma omp critical
    {
      ok = _baselogger.filterByOrigin(level, origin);
    }
    
    return ok;
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
struct TOMOGRAPHER_EXPORT LoggerTraits<MultiProc::OMP::ThreadSanitizerLogger<BaseLogger> >
  : public LoggerTraits<BaseLogger>
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
 * \since Changed in %Tomographer 5.0: removed results collector, introduced
 *        collectedTaskResults() and friends
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
 * <li> \a TaskCountIntType should be a type to use to count the number of tasks. Usually
 *      there's no reason not to use an \c int.
 *
 * </ul>
 *
 */
template<typename TaskType_, typename TaskCData_, typename LoggerType_,
         typename TaskCountIntType_ = int,
         typename TaskLoggerType_ = ThreadSanitizerLogger<LoggerType_> >
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
  typedef TaskLoggerType_ TaskLoggerType;

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
  TaskCountIntType n_chunk;

  struct CriticalSectionManager {

    template<typename Fn>
    inline void critical_status_report(Fn && fn) {
#pragma omp critical
      {
        fn();
      }
    }
    template<typename Fn>
    inline void critical_status_report_and_user_fn(Fn && fn) {
#pragma omp critical
      {
        fn();
      }
    }
    template<typename Fn>
    inline void critical_status_report_and_schedule(Fn && fn) {
#pragma omp critical
      {
        fn();
      }
    }
    template<typename Fn>
    inline void critical_schedule(Fn && fn) {
#pragma omp critical
      {
        fn();
      }
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
   * \param pcdata_  The constant shared data, which will be accessible by all
   *                 tasks
   *
   * \param logger_  The logger instance to use to log messages.  This logger
   *                 does not need to be thread safe.
   *
   * \param num_total_runs_ The number of tasks to run in total.  Recall that
   *                 the inputs to the different task instances are provided by
   *                 the TaskCData's getTaskInput() method (see \ref
   *                 pageInterfaceTaskCData).
   *
   * \param n_chunk_ How many tasks to chunk together into one thread.  This
   *                 corresponds to OpenMP's chunk argument in the instruction
   *                 <em>schedule(dynamic,chunk)</em> in a <em>\#pragma omp
   *                 for</em> instruction (see <a
   *                 href="https://computing.llnl.gov/tutorials/openMP/\#DO"
   *                 target="_blank">this page</a>).
   */
  TaskDispatcher(TaskCData * pcdata_, LoggerType & logger_,
                 TaskCountIntType num_total_runs_,
                 TaskCountIntType n_chunk_ = 1)
    : shared_data(pcdata_, logger_, num_total_runs_, 0), n_chunk(n_chunk_)
  {
  }

  TaskDispatcher(TaskDispatcher && x)
    : shared_data(std::move(x.shared_data)),
      n_chunk(x.n_chunk)
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

#ifndef _OPENMP
    logger.warning("OpenMP is disabled; tasks will run serially.");
#endif

    // declaring these as "const" causes a weird compiler error
    // "`n_chunk' is predetermined `shared' for `shared'"
    TaskCountIntType num_total_runs = shared_data.schedule.num_total_runs;
    TaskCountIntType n_chunk_ = n_chunk;
    (void)n_chunk_; // silence "unused variable" warning when compiling without OMP support

    TaskCountIntType k = 0;

    ThreadSharedDataType * shdat = & shared_data;

    const std::string logger_prefix = logger.originPrefix()+logger.glue()+"worker";
    const std::string * logger_prefix_ptr = &logger_prefix;

    logger.debug("About to start parallel section");

#pragma omp parallel default(none) private(k) shared(shdat, logger_prefix_ptr, num_total_runs, n_chunk_)
    {
      // construct a thread-safe logger we can use
      TaskLoggerType threadsafelogger(shared_data.logger, shdat->pcdata, k);

      Tomographer::Logger::LocalLogger<TaskLoggerType> locallogger(*logger_prefix_ptr, threadsafelogger);

      ThreadPrivateDataType private_data(omp_get_thread_num(), & shared_data, locallogger, critical);

      private_data.shared_data = shdat;
      private_data.task_id = -1;

      // master thread sets shared_data.schedule.num_threads ...
#pragma omp master
      {
        shdat->schedule.num_threads = omp_get_num_threads();
      }

      // ... while other threads wait for master to be done
#pragma omp barrier
#pragma omp flush


      //
      // Register new parallel worker
      //
      this->run_worker_enter(private_data, *shdat);

      //
      // The main, parallel FOR loop:
      //
#pragma omp for schedule(dynamic,n_chunk) nowait
      for (k = 0; k < num_total_runs; ++k) {

        private_data.task_id = k;

        this->run_task(private_data, shared_data);
          
      } // omp for

      //
      // De-register parallel worker
      //
      this->run_worker_exit(private_data, *shdat);

#pragma omp master
      {
        this->master_continue_monitoring_status(private_data, *shdat) ;
      }
      
    } // omp parallel

    logger.debug("OpenMP parallel section finished");
    
    this->run_epilog(shared_data, logger) ;

    logger.debug("Done.");
  }

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
    return *shared_data.results[k];
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
   * guaranteed to be called from within the main thread, that is, the one with
   * <code>omp_get_thread_num() == 0</code>.
   */
  inline void setStatusReportHandler(FullStatusReportCallbackType fnstatus)
  {
#pragma omp critical
    {
      shared_data.status_report.user_fn = fnstatus;
    }
  }

  /** \brief Request a status report
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
    // anything, including inside a malloc() or gomp lock. So can't call any
    // function which needs malloc or a #pragma omp critical.
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
#pragma omp critical
    {
      shared_data.status_report.periodic_interval = milliseconds;
    }
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
    shared_data.schedule.interrupt_requested = 1;
  }

    
}; // class TaskDispatcher


/** \brief Create an OMP task dispatcher. Useful if you want C++'s template argument
 *         deduction mechanism
 */
template<typename TaskType_, typename TaskCData_,
         typename LoggerType_, typename TaskCountIntType_ = int>
inline TaskDispatcher<TaskType_, TaskCData_,
                      LoggerType_, TaskCountIntType_>
makeTaskDispatcher(TaskCData_ * pcdata_, LoggerType_ & logger_,
                   TaskCountIntType_ num_total_runs_, TaskCountIntType_ n_chunk_ = 1)
{
  // RVO should be rather obvious to the compiler
  return TaskDispatcher<TaskType_, TaskCData_,
                        LoggerType_, TaskCountIntType_>(
                            pcdata_, logger_, num_total_runs_, n_chunk_
                            );
}



} // namespace OMP
} // namespace MultiProc

} // namespace Tomographer





#endif
