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

#ifndef MULTIPROCOMP_H
#define MULTIPROCOMP_H

#include <csignal>

#ifdef _OPENMP
#include <omp.h>
#else
inline constexpr int omp_get_thread_num() { return 0; }
inline constexpr int omp_get_num_threads() { return 1; }
#endif


#include <tomographer2/tools/loggers.h>


/** \file multiprocomp.h
 *
 * \brief Multiprocessing with OpenMP parallel multithreading.
 *
 * See \ref Tomographer::MultiProc::OMP.
 *
 */


namespace Tomographer {



namespace MultiProc
{
//! Definitions for multitasking using OpenMP multithreading.
namespace OMP
{

  namespace tomo_internal {

  /** \brief Internal. Helper for \ref ThreadSanitizerLogger
   *
   * Emits the log to \c baselogger. This template is specialized for
   * <code>baseLoggerIsThreadSafe=true</code>, in which case the call to baselogger's
   * emit_log is not wrapped into a critical section.
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
    static inline void emit_log(BaseLogger & baselogger, int level, const char * origin, const std::string & msg)
    {
#pragma omp critical
      {
        //printf("ThreadSanitizerLoggerHelper::emit_log(%d, %s, %s) -- OMP CRITICAL\n", level, origin, msg.c_str());
        baselogger.emit_log(level, origin, msg);
      }
    }
    static inline bool filter_by_origin(BaseLogger & baselogger, int level, const char * origin)
    {
      bool ok = true;
#pragma omp critical
      {
        //printf("ThreadSanitizerLoggerHelper::filter_by_origin(%d, %s) -- OMP CRITICAL\n", level, origin);
        ok = baselogger.filter_by_origin(level, origin);
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
    static inline void emit_log(BaseLogger & baselogger, int level, const char * origin, const std::string & msg)
    {
      //printf("ThreadSanitizerLoggerHelper::emit_log(%d, %s, %s) -- NORMAL\n", level, origin, msg.c_str());
      baselogger.emit_log(level, origin, msg);
    }
    static inline bool filter_by_origin(BaseLogger & baselogger, int level, const char * origin)
    {
      //printf("ThreadSanitizerLoggerHelper::filter_by_origin(%d, %s) -- NORMAL\n", level, origin);
      return baselogger.filter_by_origin(level, origin);
    }
  };

  } // namespace tomo_internal
  

  /** \brief Wrapper logger to call non-thread-safe loggers from a multithreaded environment.
   *
   * Wraps calls to emit log messages into a OpenMP \code #pragma omp critical \endcode
   * sections, which ensure thread-safety of the logging. Of course don't log too often,
   * as this will drastically slow down the execution of your program!!
   *
   * \note If the base logger is already thread-safe (as defined by
   *       \ref LoggerTraits::IsThreadSafe), then the call to emit the log is not wrapped
   *       in a critical section, but directly called.
   *
   * \todo Buffer log entries here to optimize performance and to limit the number of
   *       <code>#pragma omp critical</code> blocks.
   *
   * \warning The runtime level of this logger is fixed to the level of the base logger at
   * the moment of instanciation. Any changes to the level of the base logger afterwards
   * will not be reflected here. This is for thread-safety/consistency reasons.
   *
   * \warning If your base logger has a \a filter_by_origin() mechanism and is not
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
  class ThreadSanitizerLogger : public Logger::LoggerBase<ThreadSanitizerLogger<BaseLogger> >
  {
    BaseLogger & _baselogger;
  public:

    template<typename... MoreArgs>
    ThreadSanitizerLogger(BaseLogger & logger, MoreArgs...)
      // NOTE: pass the baselogger's level on here. The ThreadSanitizerLogger's level is
      // this one, and is fixed and cannot be changed while running.
      : Logger::LoggerBase<ThreadSanitizerLogger<BaseLogger> >(logger.level()),
        _baselogger(logger)
    {
      // when you have to debug the log mechanism.... lol
      //printf("ThreadSanitizerLogger(): object created\n");
      //_baselogger.debug("ThreadSanitizerLogger()", "log from constructor.");
      //emit_log(Logger::DEBUG, "ThreadSanitizerLogger!", "emit_log from constructor");
      //LoggerBase<ThreadSanitizerLogger<BaseLogger> >::debug("ThreadSanitizerLogger", "debug from constructor");
    }
    
    ~ThreadSanitizerLogger()
    {
    }
    
    inline void emit_log(int level, const char * origin, const std::string& msg)
    {
      //printf("ThreadSanitizerLogger::emit_log(%d, %s, %s)\n", level, origin, msg.c_str());
      tomo_internal::ThreadSanitizerLoggerHelper<BaseLogger,
                                                    Logger::LoggerTraits<BaseLogger>::IsThreadSafe>
        ::emit_log(
            _baselogger, level, origin, msg
	    );
    }

    template<bool dummy = true>
    inline typename std::enable_if<dummy && Logger::LoggerTraits<BaseLogger>::HasFilterByOrigin, bool>::type
      filter_by_origin(int level, const char * origin) const
    {
      return tomo_internal::ThreadSanitizerLoggerHelper<BaseLogger,
                                                           Logger::LoggerTraits<BaseLogger>::IsThreadSafe>
        ::filter_by_origin(
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
    enum {
      // explicitly require our logger instance to store its level. The level cannot be
      // changed.
      HasOwnGetLevel = 0,
      IsThreadSafe = 1
    };
  };
} // namespace Logger


namespace MultiProc {
namespace OMP {
    
  /** \brief A complete status report of currently running threads.
   */
  template<typename TaskStatusReportType>
  struct FullStatusReport
  {
    FullStatusReport() : tasks_running(), tasks_reports() { }

    //! Number of completed tasks
    int num_completed;
    //! Total number of tasks to perform
    int num_total_runs;
    //! Number of currently active threads (which are actively solving a task)
    int num_active_working_threads;
    //! Number of spawned threads (some may be idle)
    int num_threads;
    
    /** \brief List of length \a num_threads, specifying for each spawned thread whether
     * it is active or not
     *
     * If <em>tasks_running[k]</em> is \c true, then thread number \a k is currently
     * running a task. Otherwise, it is waiting (because of chunking of tasks, for
     * example, or because there are are no new tasks to start)
     */
    std::vector<bool> tasks_running;
    /** \brief List of length \a num_threads with the raw report submitted from each
     * individual thread.
     *
     * If thread number \a k is active and currently running a task, then
     * <em>tasks_reports[k]</em> is the raw status report that was submitted by that
     * thread. If the thread is not running a task, <em>tasks_reports[k]</em> is an
     * invalid, or default-constructed value.
     */
    std::vector<TaskStatusReportType> tasks_reports;
  };



  /** \brief Dispatches tasks to parallel threads using OpenMP
   *
   * Uses <a href="http://openmp.org/">OpenMP</a> to parallelize the repetition of a same
   * task with different inputs.
   *
   * Check out <a href="https://computing.llnl.gov/tutorials/openMP/">this good tutorial
   * for OpenMP</a>.
   *
   * <ul>
   * <li> \a Task must be a \ref pageInterfaceTask compliant type 
   *
   * <li> \a ConstantDataType may be any struct which contains all the information
   *     which needs to be accessed by the task. It should be read-only, i.e. the task
   *     should not need to write to this information. (This typically encodes the data
   *     of the problem, ie. experimental measurement results.)
   *
   *     There is no particular structure imposed on \c ConstantDataType.
   *
   * <li> \a ResultsCollector must be a \ref pageInterfaceResultsCollector compliant type 
   *
   * <li> \a %Logger is a logger type derived from \ref LoggerBase, for example \ref
   *      FileLogger. This is the type of a logger defined in the caller's scope (and
   *      given as constructor argument here) to which messages should be logged to.
   *
   * <li> \a TaskLogger is the type of the logger which will be provided to tasks inside
   *      the parallel section. Such logger should ensure that the logging is
   *      thread-safe. By default \a TaskLogger is nothing else than an appropriate \ref
   *      ThreadSanitizerLogger.
   *
   *      (Note that if the originally given \c logger is thread-safe (see
   *      \ref LoggerTraits), then \ref ThreadSanitizerLogger directly relays calls
   *      without wrapping them into OMP critical sections.)
   *
   *      For each task, a new \a TaskLogger will be created. The constructor is expected
   *      to accept the following arguments:
   *      \code
   *         TaskLogger(Logger & baselogger, const ConstantDataType * pcdata, CountIntType k)
   *      \endcode
   *      where \a baselogger is the logger given to the \a TaskDispatcher constructor,
   *      \a pcdata is the constant shared data pointer also given to the constructor, and
   *      \a k is the task number (which may range from 0 to the total number of tasks -
   *      1). The task logger is NOT constructed in a thread-safe code region, so use \c
   *      "#pragma omp critical" if necessary. You may use \a omp_get_thread_num() and \a
   *      omp_get_num_threads() to get the current thread number and the total number of
   *      threads, respectively.
   *
   * <li> \a CountIntType should be a type to use to count the number of tasks. Usually
   *      there's no reason not to use an \c int.
   *
   * </ul>
   *
   */
  template<typename Task_, typename ConstantDataType_, typename ResultsCollector_,
           typename Logger_, typename CountIntType_ = int,
           typename TaskLogger_ = ThreadSanitizerLogger<Logger_> >
  class TaskDispatcher
  {
  public:
    typedef Task_ Task;
    typedef typename Task::StatusReportType TaskStatusReportType;
    typedef ConstantDataType_ ConstantDataType;
    typedef ResultsCollector_ ResultsCollector;
    typedef Logger_ Logger;
    typedef CountIntType_ CountIntType;
    typedef TaskLogger_ TaskLogger;
    typedef FullStatusReport<TaskStatusReportType> FullStatusReportType;

    typedef std::function<void(const FullStatusReportType&)> FullStatusReportCallbackType;

  private:

    //! thread-shared variables
    struct thread_shared_data {
      thread_shared_data(const ConstantDataType *pcdata_, ResultsCollector * results_, Logger & logger_,
                         CountIntType num_total_runs_, CountIntType n_chunk_)
        : pcdata(pcdata_),
          results(results_),
          logger(logger_),
          status_report_underway(false),
	  status_report_initialized(false),
          status_report_counter(0),
          status_report_numreportsrecieved(0),
          status_report_full(),
          status_report_user_fn(),
          num_total_runs(num_total_runs_), n_chunk(n_chunk_), num_completed(0),
          num_active_working_threads(0)
      { }

      const ConstantDataType * pcdata;
      ResultsCollector * results;
      Logger & logger;

      bool status_report_underway;
      bool status_report_initialized;
      volatile std::sig_atomic_t status_report_counter;
      CountIntType status_report_numreportsrecieved;

      FullStatusReport<TaskStatusReportType> status_report_full;
      FullStatusReportCallbackType status_report_user_fn;

      CountIntType num_total_runs;
      CountIntType n_chunk;
      CountIntType num_completed;

      CountIntType num_active_working_threads;
    };
    //! thread-local variables and stuff &mdash; also serves as TaskManagerIface
    struct thread_private_data
    {
      thread_shared_data * shared_data;

      TaskLogger * logger;

      CountIntType kiter;
      CountIntType local_status_report_counter;

      inline bool status_report_requested()
      {
        //fprintf(stderr, "status_report_requested(), shared_data=%p\n", shared_data);
        return (int)local_status_report_counter != (int)shared_data->status_report_counter;
      }

      inline void submit_status_report(const TaskStatusReportType &statreport)
      {
        if ((int)local_status_report_counter == (int)shared_data->status_report_counter) {
          // error: task submitted unsollicited report
          logger->warning("OMP TaskDispatcher/taskmanageriface", "Task submitted unsollicited status report");
          return;
        }

#pragma omp critical
        {
          bool ok = true; // whether to proceed or not

          // we've reacted to the given "signal"
          local_status_report_counter = shared_data->status_report_counter;
          
          // add our status report to being-prepared status report in the  shared data
          int threadnum = omp_get_thread_num();

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
              logger->warning("OMP TaskDispatcher/taskmanageriface",
                              "status report already underway!");
              ok = false;
            }
            if (!shared_data->status_report_user_fn) {
              // no user handler set
              logger->warning("OMP TaskDispatcher/taskmanageriface",
                              "no user status report handler set!"
                              " call set_status_report_handler() first.");
              ok = false;
            }

            // since we can't return out of a critical section(?), we use an if block.
            if (ok) {

              shared_data->status_report_underway = true;
              shared_data->status_report_initialized = true;
              
              // initialize status report object & overall data
              shared_data->status_report_full = FullStatusReport<TaskStatusReportType>();
              shared_data->status_report_full.num_completed = shared_data->num_completed;
              shared_data->status_report_full.num_total_runs = shared_data->num_total_runs;
              shared_data->status_report_full.num_active_working_threads = shared_data->num_active_working_threads;
              int num_threads = omp_get_num_threads();
              shared_data->status_report_full.num_threads = num_threads;
              
              // initialize task-specific reports
              // fill our lists with default-constructed values & set all running to false.
              shared_data->status_report_full.tasks_running.clear();
              shared_data->status_report_full.tasks_reports.clear();
              shared_data->status_report_full.tasks_running.resize(num_threads, false);
              shared_data->status_report_full.tasks_reports.resize(num_threads);
              logger->debug("OMP TaskDispatcher/taskmanageriface", "vectors resized to %lu & %lu, resp.",
                           shared_data->status_report_full.tasks_running.size(),
                           shared_data->status_report_full.tasks_reports.size());
              shared_data->status_report_numreportsrecieved = 0;
            }

          } // status_report_initialized

          // if we're the first reporting thread, then maybe ok was set to false above, so
          // check again.
          if (ok) {

            //
            // Report the data corresponding to this thread.
            //
	    logger->debug("OMP TaskDispatcher/taskmanageriface", "threadnum=%ld, tasks_reports.size()=%ld",
			  (long)threadnum, (long)shared_data->status_report_full.tasks_reports.size());

            assert(0 <= threadnum && (std::size_t)threadnum < shared_data->status_report_full.tasks_reports.size());

            shared_data->status_report_full.tasks_running[threadnum] = true;
            shared_data->status_report_full.tasks_reports[threadnum] = statreport;

            ++ shared_data->status_report_numreportsrecieved;

            if (shared_data->status_report_numreportsrecieved == shared_data->num_active_working_threads) {
              // the report is ready to be transmitted to the user: go!
              shared_data->status_report_user_fn(shared_data->status_report_full);
              // all reports recieved: done --> reset our status_report_* flags
              shared_data->status_report_numreportsrecieved = 0;
              shared_data->status_report_underway = false;
              shared_data->status_report_initialized = false;
              shared_data->status_report_full.tasks_running.clear();
              shared_data->status_report_full.tasks_reports.clear();
            }
          } // if ok
        } // omp critical

      }
    };

    thread_shared_data shared_data;

  public:
    TaskDispatcher(ConstantDataType * pcdata_, ResultsCollector * results_, Logger & logger_,
                      CountIntType num_total_runs_, CountIntType n_chunk_)
      : shared_data(pcdata_, results_, logger_, num_total_runs_, n_chunk_)
    {
    }

    inline void run()
    {
      shared_data.results->init(shared_data.num_total_runs, shared_data.n_chunk, shared_data.pcdata);
      
      shared_data.logger.debug("run_omp_tasks()", "About to start parallel section.");

      // declaring these as "const" causes a weird compiler error
      // "`n_chunk' is predetermined `shared' for `shared'"
      CountIntType num_total_runs = shared_data.num_total_runs;
      CountIntType n_chunk = shared_data.n_chunk;
      (void)n_chunk; // silence "unused variable" warning when compiling without OMP support

      CountIntType k = 0;

      thread_shared_data *shdat = &shared_data;
      thread_private_data privdat;

#pragma omp parallel default(none) private(k, privdat) shared(shdat, num_total_runs, n_chunk)
      {
        privdat.shared_data = shdat;
        privdat.kiter = 0;

#pragma omp for schedule(dynamic,n_chunk) nowait
        for (k = 0; k < num_total_runs; ++k) {

#pragma omp critical
          {
            ++ shdat->num_active_working_threads;
            privdat.local_status_report_counter = shdat->status_report_counter;
          }

          // construct a thread-safe logger we can use
          TaskLogger threadsafelogger(shdat->logger, shdat->pcdata, k);

          // set up our thread-private data
          privdat.kiter = k;
          privdat.logger = &threadsafelogger;

          threadsafelogger.debug("run_omp_tasks()", "Running task #%lu ...", (unsigned long)k);

          // construct a new task instance
          Task t(Task::get_input(k, shdat->pcdata), shdat->pcdata, threadsafelogger);

          // and run it
          t.run(shdat->pcdata, threadsafelogger, &privdat);

#pragma omp critical
          {
            shdat->results->collect_result(k, t.getResult(), shdat->pcdata);

            if ((int)privdat.local_status_report_counter != (int)shdat->status_report_counter) {
              // status report request missed by task... do as if we had provided a
              // report, but don't provide report.
              ++ shdat->status_report_numreportsrecieved;
            }

            ++ shdat->num_completed;
            -- shdat->num_active_working_threads;
          }
        }
      }
    
      shared_data.results->runs_finished(num_total_runs, shared_data.pcdata);
    }


    /** \brief assign a callable to be called whenever a status report is requested
     *
     * This function remembers the given \a fnstatus callable, so that each time that \ref
     * request_status_report() is called at any later point, then this callback will be
     * invoked.
     *
     * The callback, when invoked, will be called with a single parameter of type \ref
     * FullStatusReport<TaskStatusReportType>.
     *
     * \par How Tasks should handle status reports.
     * Task's must regularly check whether a status report has been requested as they run. 
     * This is done by regularly calling the function
     * <code>tmgriface->status_report_requested()</code> on the \c tmgriface object
     * provided to <code>Task::run()</code>. This function call does not require a \c
     * critical section and is fast, so this check can be done often. The function
     * <code>tmgriface->status_report_requested()</code> returns a \c bool indicating
     * whether such a report was requested or not. If such a report was requested, then
     * the thread should prepare its status report object (of type \c
     * TaskStatusReportType), and call <code>tmgriface->submit_status_report(const
     * TaskStatusReportType & obj)</code>.
     *
     * \par
     * Note that the task should provide a member type named \c StatusReportType, which can be
     * for example a simple typedef to \ref MultiProc::StatusReport, which specifies the
     * type of its status reports.
     *
     */
    template<typename Fn>
    inline void set_status_report_handler(Fn fnstatus)
    {
#pragma omp critical
      {
        shared_data.status_report_user_fn = fnstatus;
      }
    }

    inline void request_status_report()
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

  };

  /** \brief Create an OMP task dispatcher. Useful if you want C++'s template parameter
   *         deduction mechanism.
   *
   */
  template<typename Task_, typename ConstantDataType_, typename ResultsCollector_,
           typename Logger_, typename CountIntType_ = unsigned int>
  inline TaskDispatcher<Task_, ConstantDataType_, ResultsCollector_,
                        Logger_, CountIntType_>
  makeTaskDispatcher(ConstantDataType_ * pcdata_, ResultsCollector_ * results_, Logger_ & logger_,
                        CountIntType_ num_total_runs_, CountIntType_ n_chunk_)
  {
    // RVO should be rather obvious to the compiler
    return TaskDispatcher<Task_, ConstantDataType_, ResultsCollector_,
                             Logger_, CountIntType_>(
        pcdata_, results_, logger_, num_total_runs_, n_chunk_
        );
  }

} // namespace OMP
} // namespace MultiProc

} // namespace Tomographer





#endif
