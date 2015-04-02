
#ifndef MULTIPROCOMP_H
#define MULTIPROCOMP_H



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
   *     <li> <code>static Task::get_input(std::size_t k, const ConstantDataType * pcdata)</code>
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
   *     <li> <code>void Task::run(const ConstantDataType * pcdata,
   *                               OMPTaskLogger<Logger> & logger)</code>
   *          actually runs the task. It can log to the given \c logger (see
   *          \ref LoggerBase). Note that the `logger` is NOT directly the one initially
   *          given, but an internal thread-safe wrapper to it. You can of course take a
   *          \c Logger template parameter to avoid spelling out the full type.
   *     </ul>
   *
   * <li> \c ResultsCollector takes care of collecting the results from each task run. It
   *     should provide the following methods:
   *
   *     <ul>
   *     <li> <code>void ResultsCollector::init(std::size_t num_runs, std::size_t n_chunk,
   *                                            const ConstantDataType * pcdata)</code>
   *         will be called before the tasks are run, and before starting the parallel
   *         section.
   *
   *     <li> <code>void ResultsCollector::collect_results(const Task& task)</code> is
   *         called each time a task has finished. It is called <b>from a \c critical
   *         OMP section</b>, meaning that it may safely access and write shared data.
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
  template<typename Task, typename ConstantDataType, typename ResultsCollector, typename Logger>
  inline void run_omp_tasks(const ConstantDataType * pcdata, ResultsCollector * results,
                            std::size_t num_runs, std::size_t n_chunk, Logger & logger)
  {
    std::size_t k;
    
    results->init(num_runs, n_chunk, pcdata);
    
    logger.debug("run_omp_tasks()", "About to start parallel section.");

    // note: shared(pcdata, results) doesn't really do anything to the sharedness of the
    // ResultsCollector and/or ConstantDataType, because `results` is a *pointer*
#pragma omp parallel default(none) private(k) shared(pcdata, results, num_runs, n_chunk, logger)
    {
#pragma omp for schedule(dynamic,n_chunk) nowait
      for (k = 0; k < num_runs; ++k) {

        // construct a thread-safe logger we can use
        OMPTaskLogger<Logger> threadsafelogger(logger);

        threadsafelogger.debug("run_omp_tasks()", "Running task #%lu ...", (unsigned long)k);

        // construct a new task instance
        Task t(Task::get_input(k, pcdata), pcdata, threadsafelogger);

        // and run it
        t.run(pcdata, threadsafelogger);

#pragma omp critical
        {
          results->collect_results(t);
        }
      }
    }
    
    results->run_finished();
  }
    
};






#endif
