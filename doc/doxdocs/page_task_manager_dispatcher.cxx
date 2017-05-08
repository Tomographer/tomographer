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



/** \page pageTaskManagerDispatcher Multiprocessing Task Interfaces
 *
 * <em>The following describes &lsquo;type interfaces.&rsquo; See \ref pageTypeInterfaces
 * for more info on what that is.</em>
 *
 * The following are type interfaces that allow the creation of several instances of a
 * task, possibly reporting intermediate status, and collecting results in the end.
 *
 * Such interfaces are required, for example, to run parallel tasks with the OMP task
 * dispatcher, \ref Tomographer::MultiProc::OMP::TaskDispatcher, which is itself a \ref
 * pageInterfaceTaskDispatcher compliant type.
 *
 * In the future, I hope we can also write an MPI implementation using the same
 * interfaces. (Hopefully everything works fine if \a ResultType and \a TaskCData
 * is serializable)
 *
 * A task is implemented by a \ref pageInterfaceTask. It may refer to some global,
 * constant data (the parameters of the problem) stored in a struct (referred to as \a
 * TaskCData in the following docs). Each \a Task must also conform to the \ref
 * pageInterfaceResultable and generates a result, which is then collected and returned by
 * the task dispatcher.  The task manager exposes an API to interact with the \a Task
 * objects: it is the \ref pageInterfaceTaskManagerIface. This interface may be used for
 * example in order to submit intermediate status reports.
 *
 * \since Changed in %Tomographer 5.0: removed the \a ResultsCollector type interface; the
 *        tasks must now themselves conform to the \ref pageInterfaceResultable.
 *
 * Type interfaces which are used by the task dispatcher:
 *
 *  - \subpage pageInterfaceTaskCData
 *  - \subpage pageInterfaceTask
 *  - \subpage pageInterfaceTaskManagerIface
 *
 * The type interface which the task dispatcher itself obeys:
 *
 *  - \subpage pageInterfaceTaskDispatcher
 *
 */

// no longer:  *  - \subpage pageInterfaceResultsCollector




/** \page pageInterfaceTaskCData TaskCData Interface
 *
 * <em>This is a &lsquo;type interface.&rsquo; See \ref pageTypeInterfaces
 * for more info on what that is.</em>
 *
 * This type is meant to store all constant, shared data which may be accessed by the
 * tasks while they run.  It also provides the inputs to the tasks.
 *
 * Note that the \a TaskCData type interface is only used if you use the
 * &lsquo;low-level&rsquo; task interface and implement the tasks yourself.  If you use
 * the higher-level Tomographer::MHRWTasks or Tomographer::MHRWTasks::ValueHistogramTasks, your
 * CData-type class should inherit the respective CData classes for those.
 *
 * \par InputType getTaskInput(unsigned int task_id) const
 *          Provide input to a new task. \a task_id is the task iteration number (task
 *          id).
 *
 * \par
 *          The return value may be any type. It will be passed directly to the first
 *          argument of the constructor.
 *
 *
 */


/** \page pageInterfaceTask Task Interface
 *
 * A task which may be repeated in parallel with different inputs.
 *
 * A \a Task should represent an instance of the task to complete (e.g. a
 * Metropolis-Hastings random walk).
 *
 * A \a Task interface compliant type should provide the following methods.
 *
 * \par typedef .. StatusReportType
 *          The type storing information for a status report (task progress, message,
 *          additional info such as acceptance ratio etc.).  This class must inherit from
 *          Tomographer::MultiProc::TaskStatusReport, such that at least \a fraction_done
 *          and \a msg are provided.  (So that generic status reporter helpers such as
 *          \ref Tomographer::Tools::SigHandlerTaskDispatcherStatusReporter can at least
 *          rely on this information.)
 *
 * \par Task(InputType input, const TaskCData * pcdata, LoggerType & logger)
 *          Task constructor: construct a Task instance which will solve the task for the
 *          given input. The \a input parameter is whatever \c
 *          TaskCData::getTaskInput() returned.
 * \par 
 *          This method can log to the given \c logger (see \ref
 *          Tomographer::Logger::LoggerBase). Note that the \a logger need NOT be the
 *          logger that may have been specified, e.g., to the task dispatcher: it could
 *          be, for example, an internal thread-safe wrapper to your original logger. To
 *          be sure, you should make this a template method with a parameter \a LoggerType.
 *
 * \par void run(const TaskCData * pcdata, LoggerType & logger, TaskManagerIface * tmgriface)
 *          This method actually runs the task.
 *
 * \par 
 *          This method can log to the given \c logger (see \ref
 *          Tomographer::Logger::LoggerBase). Note that the \a logger need NOT be the
 *          logger that may have been specified, e.g., to the task dispatcher: it could
 *          be, for example, an internal thread-safe wrapper to your original logger. To
 *          be sure, you should make this a template method with parameters \a LoggerType
 *          and \a TaskManagerIface.
 *
 * \par
 *          The code in \c run() should poll
 *          <code>tmgriface->statusReportRequested()</code> and provide a status report if
 *          requested to do so via <code>tmgriface->statusReport(const
 *          TaskStatusReportType &)</code>. \a tmgriface is an object which complies to
 *          the \ref pageInterfaceTaskManagerIface.
 *
 * A \a Task interface compliant type must always also be compliant with the \ref
 * pageInterfaceResultable, meaning that it must provide the following methods:
 *
 * \par typedef .. ResultType
 *          An alias for the type, e.g. a structure, which contains the result of
 *          the given task. See <code>Task::getResult()</code>.
 *
 * \par ResultType getResult()
 *          Return a custom type which holds the result for the given task.
 *
 * \par
 *          NOTE: Tasks are explicitly allowed to assume that their getResult() will only
 *          be called once.  This allows them e.g. to std::move their internal result
 *          object into the return value of getResult().
 *
 *
 * \par Note on Status Reports: 
 *   Tasks must regularly check whether a status report has been requested as they run.
 *   This is done by regularly calling the function
 *   <code>tmgriface->statusReportRequested()</code> on the \c tmgriface object provided
 *   to <code>TaskType::run()</code>. This function call is meant to be very efficient
 *   (for example, it does not require a \c critical section in the OpenMP
 *   implementation), so this check can be done often. The function
 *   <code>tmgriface->statusReportRequested()</code> returns a \c bool indicating whether
 *   such a report was requested or not. If such a report was requested, then the thread
 *   should prepare its status report object (of type \c TaskStatusReportType), and call
 *   <code>tmgriface->submitStatusReport(const TaskStatusReportType & obj)</code>.
 *
 * \par
 *   The task should provide a member type named \c StatusReportType, which
 *   can be for example a simple typedef to \ref Tomographer::MultiProc::TaskStatusReport,
 *   which specifies the type of its status reports.
 */



// /* * \page pageInterfaceResultsCollector ResultsCollector Interface
//  *
//  * \a ResultsCollector takes care of collecting the results from each task run.
//  *
//  * In the following, \a TaskCData is some type that was specified to the task
//  * dispatcher. The type \a CountType designates the type used to count tasks (usually an
//  * integral type of course). Also the type \a ResultType is the type declared for a result
//  * by the task (\ref pageInterfaceTask). You may make the methods here take template
//  * parameters so you don't have to worry about the exact types of these parameters.
//  *
//  * \par void init(CountType num_total_runs, CountType n_chunk, const TaskCData * pcdata)
//  *         \a init() will be called before the tasks are run (e.g. before starting the
//  *         parallel section), and may be used to initialize data.
//  *
//  * \par void collectResult(CountType task_no, const ResultType& taskresult, const TaskCData * pcdata)
//  *         Called each time a task has finished, with the corresponding task result, along
//  *         with information about which task it was and as always the shared constant
//  *         data. This method does not need to worry about concurrence, for example writing
//  *         to shared data (it is either called from the main thread in a critical section,
//  *         or the data was serialized and passed to the main process, etc.)
//  *
//  * \par
//  *         in the OMP task dispatcher (\ref Tomographer::MultiProc::OMP::TaskDispatcher),
//  *         this is called within a \c critical OMP section, so it may safely access and
//  *         write shared data.
//  *
//  * \par void runsFinished(CountType num_total_runs, const TaskCData * pcdata)
//  *         Called after all the tasks have finished. This is the good time to, e.g.,
//  *         finalize collected values, such as multiplying by parameter space volume,
//  *         dividing by the number of samples to get the average, etc.
//  *
//  */


/** \page pageInterfaceTaskManagerIface TaskManagerIface Interface
 *
 * Provides an interface for the task to interact with the task
 * manager/dispatcher. Currently, the only use is to check whether the user has requested
 * a status report, and submit a report if this was the case.
 *
 * Tasks should poll \a statusReportRequested() often (e.g. at each iteration), and
 * submit a status report if that function returns \c true.
 *
 * \par bool statusReportRequested()
 *          Return \c true if a status report was recently requested. If so, you should
 *          submit your report using the \a statusReport() method.  This function should
 *          be implemented such that it can be called often without performance problems
 *          (e.g. check shared variable value).
 *
 * \par void submitStatusReport(const TaskStatusReportType &)
 *          Submit the status report if \a statusReportRequested() returned \c
 *          true. Call this function ONCE only per task, and only if a status report was
 *          requested. The behavior of this function otherwise is undefined.
 *
 * See also the documentation for the \ref pageInterfaceTask.
 *
 */





// ---------- the dispatcher itself ----------

/** \page pageInterfaceTaskDispatcher TaskDispatcher Interface
 *
 * The task dispatcher takes care of running tasks.  It should handle tasks provided by a
 * \ref pageInterfaceTask compliant template parameter, should allow these tasks to share
 * data via a \ref pageInterfaceTaskCData compliant type also specified as template
 * parameter.
 *
 * \since Changed in %Tomographer 5.0: Removed the results collector entirely. Now the
 *        tasks must be \ref pageInterfaceResultable "Resultable" 's which the task
 *        dispatcher simply collects and make available to the caller with \a
 *        collectedTaskResults().
 *
 * <h3>What the task dispatcher should do</h3>
 *
 * The task dispatcher is responsible for scheduling and running the tasks.  See \ref
 * Tomographer::MultiProc::Sequential::TaskDispatcher for a simple example.
 *
 * It should be provided, or should otherwise have knowledge of:
 *
 *   - A task type (\a TaskType), which obeys the \ref pageInterfaceTask ;
 *
 *   - A constant shared data structure (which we later refer to as \a TaskCData), which
 *     obeys the \ref pageInterfaceTaskCData and which provides the necessary input data
 *     to carry out the tasks ;
 *
 *
 * Upon execution, say within a \a run() method exposed by the task dispatcher, the task
 * dispatcher is expected to do the following steps, in this order:
 *
 *   - Schedule the tasks however they are meant to be run (in different threads, in
 *     different processes, serially one at a time, etc.), and run each task following
 *     these steps:
 *
 *     * Get the input to the new task from the \a TaskCData, by invoking its \a
 *       getTaskInput() method;
 *
 *     * Instantiate a new \a TaskType instance, providing it the input, as well as a
 *       suitable logger instance so that the task can log messages;
 *
 *     * Run the task, by calling its \a run() function. You must provide a pointer to an
 *       object obeying the \ref pageInterfaceTaskManagerIface, which allows the task to
 *       query whether status updates were requested.  This object may be for instance a
 *       relevant private class or struct, it does not need to be public type.
 *       Theoretically, it could also be the task dispatcher itself (if it exposes the
 *       relevant methods), but this would mean having to publicly expose functions which
 *       should really only be called by the tasks.  See \ref
 *       Tomographer::MultiProc::Sequential::TaskDispatcher for an example.
 *
 *     * Recover the task's result using its \a getResult() method.
 *
 *   - All the time during the above task execution, if the a status report is requested
 *     explicitly from an external caller (e.g. a signal handler) with a call to this
 *     class' \a requestStatusReport(), then the \a TaskManagerIface provided to the
 *     task's \a run() function should inform the task to provide its status report (see
 *     \ref pageInterfaceTask).
 *
 *     When a status report is requested, the tasks will submit their reports to the \a
 *     TaskManagerIface (see \ref pageInterfaceTask). Once all the reports are received,
 *     they should be combined into a full status report (\ref
 *     Tomographer::MultiProc::FullStatusReport), and the status report handler (set by \a
 *     setStatusReportHandler()) should be called.
 *
 *     The same holds if a status report is periodically due because a periodic status
 *     report was enabled (e.g. with \a requestPeriodicStatusReport() called before tasks
 *     were started).  In this case it is the TaskManagerIface which should tell the task
 *     to submit a status report every so many milliseconds using the same interface.
 *
 *   - After all tasks have completed, call the results collectors' \a runsFinished()
 *     method to finalize the results.
 *
 * \note The tasks, the results collector and the constant data structure do NOT know in
 *       which kind of multiprocessing environment they will be run (they just express
 *       some abstract task to be carried out).  Any data protection, data race
 *       conditions, approprate thread locking, communication of results across processes,
 *       or whatever other housekeeping is required is the task dispatcher's
 *       responsibility; not that of the tasks.  See \ref
 *       Tomographer::MultiProc::OMP::TaskDispatcher for an example; there, sections where
 *       data could be accessed simultaneously by different threads are protected by
 *       <code>\#pragma omp critical</code> blocks.
 *
 *
 * <h3>The API exposed by the task dispatcher</h3>
 *
 * The exact APIs of how these tasks are specified, managed and run is not clearly
 * specified, but it is strongly advised to follow a model such as the simplistic \ref
 * Tomographer::MultiProc::Sequential::TaskDispatcher or the OpenMP-based \ref
 * Tomographer::MultiProc::OMP::TaskDispatcher.
 *
 * The \a TaskDispatcher must however provide the following methods:
 *
 * \par void setStatusReportHandler(Fn fn)
 *   The argument should be a callable (e.g. lambda function) which accepts a single
 *   paramter of const reference to a <code>FullStatusReportType</code>
 *
 * \par void requestStatusReport()
 *   Initiate a status report query.  The call will typically return immediately, and
 *   sometime later the callback set by \a setStatusReportHandler() will be called with
 *   the relevant status report information.
 *
 * \par void requestPeriodicStatusReport(int milliseconds)
 *   Request that the task manager periodically send a status report to the handler set by
 *   \a setStatusReportHandler().  The interval time is specified in milliseconds.  Pass
 *   the value \c -1 to disable periodic status reports.
 *
 * \par void requestInterrupt()
 *   Interrupt all tasks as soon as possible.
 *
 * The \a TaskDispatcher must also provide the following typedef:
 *
 * \par typedef ... TaskType;
 *   The \ref pageInterfaceTask -compliant type used to describe a task.
 *
 */
