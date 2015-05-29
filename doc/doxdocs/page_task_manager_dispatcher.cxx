
/** \page pageTaskManagerDispatcher Task Manager/Dispatcher Interfaces
 *
 * \todo WRITE DOCUMENTATION ...........................
 *
 * These are the interfaces required by \ref Tomographer::MultiProc::OMPTaskDispatcher,
 * which handles \c Task objects, has access to some fixed constant data \c CData, and
 * collects results in a \c ResultsCollector.
 *
 *
 * Documented type interfaces:
 *
 *  - \subpage pageInterfaceTask
 *  - \subpage pageInterfaceResultsCollector
 *  - \subpage pageInterfaceTaskManagerIface
 */





/** \page pageInterfaceTask Task Interface
 *
 * A task which may be repeated with different inputs.
 *
 * ......................
 *
 *
 * \c Task should represent an instance of the task to complete (e.g. a
 * Metropolis-Hastings random walk). It should provide the following methods, and keep in
 * mind that each of these methods will be called from a local thread.
 *
 *     <ul>
 *     <li> <code>typedef &lt;Custom-Type> ResultType;</code>
 *          an alias for the type of, e.g. a structure, which contains the result of
 *          the given task. See <code>Task::getResult()</code>.
 *
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
 *     <li> <code>template<typename TaskManagerIface>
 *                void Task::run(const ConstantDataType * pcdata,
 *                               OMPTaskLogger<Logger> & logger,
 *                               TaskManagerIface * tmgriface)</code>
 *          actually runs the task. It can log to the given \c logger (see
 *          \ref LoggerBase). Note that the `logger` is NOT directly the one initially
 *          given, but an internal thread-safe wrapper to it. You can of course take a
 *          \c Logger template parameter to avoid spelling out the full type.
 *
 *          The code in \c run() should poll <code>tmgriface->status_report_requested()</code>
 *          and provide a status report if requested to do so via
 *          <code>tmgriface->status_report(const TaskStatusReportType &)</code>. See documentation
 *          for \ref request_status_report().
 *
 *     <li> <code>Task::ResultType Task::getResult()</code>
 *          to return a custom type which holds the result for the given task. This will be
 *          given to the result collector.
 *     </ul>
 */



/** \page pageInterfaceResultsCollector ResultsCollector Interface
 *
 * Responsible for collecting results from many repetitions of a task.
 *
 * ......................
 *
 * \c ResultsCollector takes care of collecting the results from each task run. It
 * should provide the following methods:
 *
 *     <ul>
 *     <li><code>void ResultsCollector::init(unsigned int num_total_runs, unsigned int n_chunk,
 *                                           const ConstantDataType * pcdata)</code>
 *         will be called before the tasks are run, and before starting the parallel
 *         section.
 *
 *     <li><code>void ResultsCollector::collect_result(unsigned int task_no,
 *                                                     const Task::ResultType& taskresult,
 *                                                     const ConstantDataType * pcdata)</code>
 *         is called each time a task has finished. It is called <b>from a \c critical
 *         OMP section</b>, meaning that it may safely access and write shared data.
 *
 *     <li><code>void ResultsCollector::runs_finished(CountIntType num_total_runs,
 *                                                     const ConstantDataType * pcdata)</code> .............
 *     </ul>
 *
 */


/** \page pageInterfaceTaskManagerIface TaskManagerIface Interface
 *
 * Provides an interface for the task to query the task manager about, e.g., if the user
 * has requested a status report.
 *
 * .......................
 */
