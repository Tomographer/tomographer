
#ifndef MULTIPROCOMP_H
#define MULTIPROCOMP_H



namespace MultiProc
{

  /** \brief Dispatches tasks to parallel threads using OpenMP
   *
   * Uses <a href="http://openmp.org/">OpenMP</a> pragma directives to parallelize the
   * repetition of a same task with a range of different inputs.
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
   *     <li> <code>static Task::get_input(size_t k, const ConstantDataType * pcdata)</code>
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
   *     <li> <code>void Task::run(const ConstantDataType * pcdata)</code>
   *          actually runs the task. 
   *     </ul>
   *
   * <li> \c ResultsCollector takes care of collecting the results from each task run. It
   *     should provide the following methods:
   *
   *     <ul>
   *     <li> <code>void ResultsCollector::init()</code> will be called before the tasks
   *         are run, and before starting the parallel section.
   *
   *     <li> <code>void ResultsCollector::collect_results(const Task& task)</code> is
   *         called each time a task has finished. It is called <b>from a critical
   *         OMP section</b>, meaning that it may access shared data.
   *
   * </ul>
   *
   *
   */
  template<typename Task, typename ConstantDataType, typename ResultsCollector>
  inline void run_omp_tasks(const ConstantDataType * pcdata, ResultsCollector * results,
                            size_t num_runs, size_t n_chunk)
  {
    size_t k;
    
    results->init();
    
    // note: shared(pcdata, results) doesn't really do anything to the sharedness of the
    // ResultsCollector and/or ConstantDataType, because `results` is a *pointer*
#pragma omp parallel default(none) private(k) shared(pcdata, results, num_runs, n_chunk)
    {
#pragma omp for schedule(dynamic,n_chunk) nowait
      for (k = 0; k < num_runs; ++k) {

        // construct a new task instance
        Task t(Task::get_input(k, pcdata), pcdata);

        // and run it
        t.run(pcdata);

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
