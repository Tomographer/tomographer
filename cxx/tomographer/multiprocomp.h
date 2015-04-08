
#ifndef MULTIPROCOMP_H
#define MULTIPROCOMP_H


#include <omp.h>


namespace Tomographer {



namespace MultiProc
{

  /** \brief Internal. Helper for \ref OMPThreadSanitizerLogger
   *
   * Emits the log to \c baselogger. This template is specialized for
   * <code>baseLoggerIsThreadSafe=true</code>, in which case the call to baselogger's
   * emit_log is not wrapped into a critical section.
   *
   * This helper is meant to be called as
   * \code
   *      OMPThreadSanitizerLoggerHelper<BaseLogger, LoggerTraits<BaseLogger>::IsThreadSafe>
   *        ::emit_log(
   *            baselogger, level, origin, msg
   *          );
   * \endcode
   */
  template<typename BaseLogger, bool baseLoggerIsThreadSafe>
  struct OMPThreadSanitizerLoggerHelper
  {
    static inline void emit_log(BaseLogger & baselogger, int level, const char * origin, const std::string & msg)
    {
#pragma omp critical
      {
        //printf("OMPThreadSanitizerLoggerHelper::emit_log(%d, %s, %s) -- CRITICAL\n", level, origin, msg.c_str());
        baselogger.emit_log(level, origin, msg);
      }
    }
  };

  //
  // specialize the helper for when logging to a thread-safe base logger. No critical
  // section needed because the logger is already thread-safe.
  //
  template<typename BaseLogger>
  struct OMPThreadSanitizerLoggerHelper<BaseLogger, true>
  {
    static inline void emit_log(BaseLogger & baselogger, int level, const char * origin, const std::string & msg)
    {
      //printf("OMPThreadSanitizerLoggerHelper::emit_log(%d, %s, %s) -- NORMAL\n", level, origin, msg.c_str());
      baselogger.emit_log(level, origin, msg);
    }
  };


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
   * \bug WE SHOULD BUFFER LOG ENTRIES, and emit them only every N seconds. Currently we
   *      emit the logs as they come.
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
   *     // thread-safe. So create a OMPThreadSanitizerLogger to which we can
   *     // safely log and pass to sub-routines that want a logger.
   *     OMPThreadSanitizerLogger<SomeLogger> threadsafelogger;
   *
   *     threadsafelogger.longdebug( ... ); // safe
   *
   *     // the logger may be passed to subtasks
   *     FidelityHistogramStatsCollector<MatrQ, double, OMPThreadSanitizerLogger<SomeLogger> >
   *       fidelityhistogramcollector(..., threadsafelogger);
   *
   *     ... // more parallel code
   *
   *   }
   * \endcode
   * 
   */
  template<typename BaseLogger>
  class OMPThreadSanitizerLogger : public LoggerBase<OMPThreadSanitizerLogger<BaseLogger> >
  {
    BaseLogger & _baselogger;
  public:

    OMPThreadSanitizerLogger(BaseLogger & logger)
      : LoggerBase<OMPThreadSanitizerLogger<BaseLogger> >(logger.level()),
        _baselogger(logger)
    {
      // when you have to debug the log mechanism.... lol
      //printf("OMPThreadSanitizerLogger(): object created\n");
      //_baselogger.debug("OMPThreadSanitizerLogger()", "log from constructor.");
      //emit_log(Logger::DEBUG, "OMPThreadSanitizerLogger!", "emit_log from constructor");
      //LoggerBase<OMPThreadSanitizerLogger<BaseLogger> >::debug("OMPThreadSanitizerLogger", "debug from constructor");
    }
    
    ~OMPThreadSanitizerLogger()
    {
    }
    
    inline void emit_log(int level, const char * origin, const std::string& msg)
    {
      //printf("OMPThreadSanitizerLogger::emit_log(%d, %s, %s)\n", level, origin, msg.c_str());
      OMPThreadSanitizerLoggerHelper<BaseLogger, LoggerTraits<BaseLogger>::IsThreadSafe>
        ::emit_log(
            _baselogger, level, origin, msg
          );
    }
  };

  /** \brief Thread-safe logger for OMP tasks
   *
   * This is the type of the logger that tasks will be guaranteed to be given within
   * \ref run_omp_tasks().
   */
  template<typename BaseLogger>
  struct OMPTaskLogger : public OMPThreadSanitizerLogger<BaseLogger> {
    OMPTaskLogger(BaseLogger & baselogger) : OMPThreadSanitizerLogger<BaseLogger>(baselogger) { }
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
   * <li> \c ConstantDataType may be any struct which contains all the information
   *     which needs to be accessed by the task. It should be read-only, i.e. the task
   *     should not need to write to this information. (This typically encodes the data
   *     of the problem, ie. experimental measurement results.)
   *
   *     There is no particular structure imposed on \c ConstantDataType.
   *
   * <li> \c Task should represent an instance of the task to complete (e.g. a
   *     Metropolis-Hastings random walk). It should provide the following methods, and
   *     keep in mind that each of these methods will be called from a local thread.
   *
   *     <ul>
   *     <li> <code>static Task::get_input(unsigned int k, const ConstantDataType * pcdata)</code>
   *          should provide input to a new task. \c k is the task iteration number
   *          and \c pcdata is a pointer to the shared const data.
   *
   *          The return value may be any type.
   *               
   *     <li> <code>Task::Task( <input> , const ConstantDataType * pcdata)</code> --
   *          construct a Task instance which will solve the task for the given input.
   *          The <tt>&lt;input&gt;</tt> parameter is whatever \c Task::get_input()
   *          returned.
   *
   *     <li> <code>template<typename T>
   *                void Task::run(const ConstantDataType * pcdata,
   *                               OMPTaskLogger<Logger> & logger,
   *                               T * p)</code>
   *          actually runs the task. It can log to the given \c logger (see
   *          \ref LoggerBase). Note that the `logger` is NOT directly the one initially
   *          given, but an internal thread-safe wrapper to it. You can of course take a
   *          \c Logger template parameter to avoid spelling out the full type.
   *
   *          The code in \c run() should poll <code>p->status_report_requested()</code>
   *          and provide a status report if requested to do so via
   *          <code>p->status_report(const TaskStatusReportType &)</code>. See documentation
   *          for \ref request_status_report().
   *     </ul>
   *
   * <li> \c ResultsCollector takes care of collecting the results from each task run. It
   *     should provide the following methods:
   *
   *     <ul>
   *     <li> <code>void ResultsCollector::init(unsigned int num_runs, unsigned int n_chunk,
   *                                            const ConstantDataType * pcdata)</code>
   *         will be called before the tasks are run, and before starting the parallel
   *         section.
   *
   *     <li> <code>void ResultsCollector::collect_results(const Task& task)</code> is
   *         called each time a task has finished. It is called <b>from a \c critical
   *         OMP section</b>, meaning that it may safely access and write shared data.
   *     </ul>
   *
   * <li> \c Logger is a logger type derived from \ref LoggerBase, for example
   *      \ref SimpleFoutLogger.
   *
   *      The logger which will be provided to tasks inside the parallel section will be
   *      a thread-local instance of \c OMPTaskLogger<Logger>, which ensures that the
   *      logging is thread-safe. See \c OMPTaskLogger<Logger>, which is nothing else than
   *      a synonym for an appropriate \ref OMPThreadSanitizerLogger.
   *
   *      Note that if the originally given \c logger is thread-safe (see
   *      \ref LoggerTraits), then \ref OMPThreadSanitizerLogger directly relays calls
   *      without wrapping them into OMP critical sections.
   *
   * </ul>
   *
   *
   */
  template<typename Task_, typename ConstantDataType_, typename ResultsCollector_,
           typename Logger_, typename CountIntType_>
  class OMPTaskDispatcher
  {
  public:
    typedef Task_ Task;
    typedef typename Task::StatusReportType TaskStatusReportType;
    typedef ConstantDataType_ ConstantDataType;
    typedef ResultsCollector_ ResultsCollector;
    typedef Logger_ Logger;
    typedef CountIntType_ CountIntType;

    typedef std::function<void(int, const TaskStatusReportType&)> ThreadStatReportFnType;

  private:
    ///! thread-shared variables
    struct thread_shared_data {
      thread_shared_data(const ConstantDataType *pcdata_, ResultsCollector * results_, Logger & logger_,
                         CountIntType num_runs_, CountIntType n_chunk_)
        : pcdata(pcdata_), results(results_), logger(logger_),
          status_report_fn(), status_report_counter(0), status_report_numreportsrecieved(0),
          num_runs(num_runs_), n_chunk(n_chunk_), num_completed(0),
          num_active_working_threads(0)
      { }

      const ConstantDataType * pcdata;
      ResultsCollector * results;
      Logger & logger;

      ThreadStatReportFnType status_report_fn;
      CountIntType status_report_counter;
      CountIntType status_report_numreportsrecieved;

      CountIntType num_runs;
      CountIntType n_chunk;
      CountIntType num_completed;

      CountIntType num_active_working_threads;
    };
    ///! thread-local variables and stuff
    struct thread_private_data {
      thread_private_data(thread_shared_data * shared_data_ = NULL)
        : shared_data(shared_data_), kiter(0),
          local_status_report_counter(0)
      {
        if (shared_data != NULL) {
          local_status_report_counter = shared_data->status_report_counter;
        }
      }

      thread_shared_data * shared_data;

      CountIntType kiter;
      CountIntType local_status_report_counter;

      inline bool status_report_requested()
      {
        //fprintf(stderr, "status_report_requested(), shared_data=%p\n", shared_data);
        return local_status_report_counter != shared_data->status_report_counter;
      }

      inline void submit_status_report(const TaskStatusReportType &statreport)
      {
        local_status_report_counter = shared_data->status_report_counter;
        shared_data->status_report_fn(omp_get_thread_num(), statreport);
      }
    };

    thread_shared_data shared_data;

  public:
    OMPTaskDispatcher(ConstantDataType * pcdata_, ResultsCollector * results_, Logger & logger_,
                      CountIntType num_runs_, CountIntType n_chunk_)
      : shared_data(pcdata_, results_, logger_, num_runs_, n_chunk_)
    {
    }

    inline void run()
    {
      shared_data.results->init(shared_data.num_runs, shared_data.n_chunk, shared_data.pcdata);
      
      shared_data.logger.debug("run_omp_tasks()", "About to start parallel section.");

      // declaring these as "const" causes a weird compiler error
      // "`n_chunk' is predetermined `shared' for `shared'"
      CountIntType num_runs = shared_data.num_runs;
      CountIntType n_chunk = shared_data.n_chunk;

      CountIntType k = 0;

      thread_shared_data *shdat = &shared_data;
      thread_private_data privdat;

#pragma omp parallel default(none) private(k, privdat) shared(shdat, num_runs, n_chunk)
      {
        privdat.shared_data = shdat;
        privdat.kiter = 0;
        privdat.local_status_report_counter = shdat->status_report_counter;

#pragma omp for schedule(dynamic,n_chunk) nowait
        for (k = 0; k < num_runs; ++k) {

#pragma omp critical
          {
            ++ shdat->num_active_working_threads;
          }

          privdat.kiter = k;

          // construct a thread-safe logger we can use
          OMPTaskLogger<Logger> threadsafelogger(shdat->logger);

          threadsafelogger.debug("run_omp_tasks()", "Running task #%lu ...", (unsigned long)k);

          // construct a new task instance
          Task t(Task::get_input(k, shdat->pcdata), shdat->pcdata, threadsafelogger);

          // and run it
          t.run(shdat->pcdata, threadsafelogger, &privdat);

#pragma omp critical
          {
            shdat->results->collect_results(t);

            if (privdat.local_status_report_counter != shdat->status_report_counter) {
              // status report request missed by task... do as if we had provided a
              // report, but don't provide report.
              ++ shdat->status_report_numreportsrecieved;
            }

            ++ shdat->num_completed;
            -- shdat->num_active_working_threads;
          }
        }
      }
    
      shared_data.results->run_finished();
    }


    /** \brief Request a status report
     *
     * (This can be typically called, e.g. from a signal handler.)
     *
     * \par Status Reporting Mechanism.
     * This function, in turn:
     *
     *  - calls <code>void fnoverall(int num_completed, int num_runs,
     *                               int num_active_working_threads,
     *                               int num_threads)</code>
     *    passing as parameters the number of completed tasks, the total number of tasks
     *    to perform, the number of actively working threads, and the number of spawned
     *    threads.
     *
     *  - signals each working thread's task to provide a status report, and correspondingly
     *    calls <code>void fntask(int threadnum, const TaskStatusReportType& srep)</code> for each
     *    working thread, in a thread-safe manner.
     *
     *  - calls <code>void fndone()</code> after all threads have finished reporting.
     *
     * \par
     * All calls to the callbacks <code>fnoverall()</code>, <code>fntask()</code>, and
     * <code>fndone()</code> are done within an OMP \c critical section, making them thread-safe.
     *
     * \par
     * These callbacks are not expected to return anything.
     *
     * \par How Tasks should handle status reports.
     * Task's must regularly check whether a status report has been requested as they run. This
     * is done by regularly calling the function <code>p->status_report_requested()</code> on the
     * \c p object provided to <code>Task::run()</code>. This function call does not require a
     * \c critical section and is fast, so this check can be done often. The function
     * <code>p->status_report_requested()</code> returns a \c bool indicating whether such a report
     * was requested or not. If such a report was requested, then the thread should prepare its
     * status report object (of type \c TaskStatusReportType), and call
     * <code>p->submit_status_report(const TaskStatusReportType & obj)</code>.
     *
     * \par
     * Note that the task should provide a member type named \c StatusReportType, which can be
     * for example a simple typedef to \ref MultiProc::StatusReport, which specifies the
     * type of its status reports.
     *
     */
    template<typename Fn1, typename Fn2, typename Fn3>
    inline void request_status_report(
        Fn1 fnoverall,
        Fn2 fntask,
        Fn3 fndone
        )
    {
      if (shared_data.status_report_fn) {
        // status report already underway!
        shared_data.logger.debug("OMPTaskDispatcher::request_status_report()",
                                 "status report already underway!");
        return;
      }

      shared_data.logger.longdebug("OMPTaskDispatcher::request_status_report()",
                               "will request a status report.");

      // prepare function to pass on to threads, in a thread-safe wrapper
      shared_data.status_report_fn = [&fntask,&shared_data,&fndone](int threadnum, const TaskStatusReportType& srep) {
#pragma omp critical
        {
          //          shared_data.logger.debug("OMPTaskDispatcher::request_status_report()",
          //                                   "got report from thread #%d.", threadnum);
          fntask(threadnum, srep);
          ++ shared_data.status_report_numreportsrecieved;
          if ((int)shared_data.status_report_numreportsrecieved == shared_data.num_active_working_threads) {
            // all reports recieved: done
            shared_data.status_report_numreportsrecieved = 0;
            shared_data.status_report_fn = ThreadStatReportFnType(); // empty function
            fndone();
            shared_data.logger.debug("OMPTaskDispatcher::request_status_report()",
                                     "finished reporting.");
          }
        }
      };

#pragma omp critical
      {
        // overall progress report
        fnoverall(shared_data.num_completed, shared_data.num_runs,
                  shared_data.num_active_working_threads, omp_get_num_threads());
        // initialize
        shared_data.status_report_numreportsrecieved = 0;
        // now, pass on signal to other threads to invite them to report progress
        ++ shared_data.status_report_counter;
      }

    }

  };

  template<typename Task_, typename ConstantDataType_, typename ResultsCollector_,
           typename Logger_, typename CountIntType_ = unsigned int>
  OMPTaskDispatcher<Task_, ConstantDataType_, ResultsCollector_,
                    Logger_, CountIntType_>
  makeOMPTaskDispatcher(ConstantDataType_ * pcdata_, ResultsCollector_ * results_, Logger_ & logger_,
                        CountIntType_ num_runs_, CountIntType_ n_chunk_)
  {
    return OMPTaskDispatcher<Task_, ConstantDataType_, ResultsCollector_,
                             Logger_, CountIntType_>(
        pcdata_, results_, logger_, num_runs_, n_chunk_
        );
  }
    
} // namespace MultiProc

} // namespace Tomographer





#endif
