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



/** \page pageTaskManagerDispatcher Task Manager/Dispatcher Interfaces
 *
 * <em>The following describes &lsquo;type interfaces.&rsquo; See \ref pageTypeInterfaces
 * for more info on what that is.</em>
 *
 * The following are type interfaces that allow the creation of several instances of a
 * task, possibly reporting intermediate status, and collecting results in the end.
 *
 * Such interfaces are required, for example, to run parallel tasks with the OMP task
 * dispatcher, \ref Tomographer::MultiProc::OMP::TaskDispatcher.
 *
 * In the future, I hope we can also write an MPI implementation using the same
 * interfaces. (Hopefully everything works fine if \a ResultType and \a TaskCData
 * is serializable)
 *
 * A task is implemented by a \ref pageInterfaceTask. It may refer to some global,
 * constant data (the parameters of the problem) stored in a struct (referred to as \a
 * TaskCData in the following docs). Each \a Task generates a result, which is sent
 * to and collected by a type responsible for aggregating the data in a usable form (e.g.,
 * calculating an average and std. deviation), which compiles with the \ref
 * pageInterfaceResultsCollector. The task manager exposes an API to interact with the \a
 * Task objects: it is the \ref pageInterfaceTaskManagerIface. This interface may be used
 * for example in order to submit intermediate status reports.
 *
 * Type interfaces which are used by the task dispatcher:
 *
 *  - \subpage pageInterfaceTaskCData
 *  - \subpage pageInterfaceTask
 *  - \subpage pageInterfaceResultsCollector
 *  - \subpage pageInterfaceTaskManagerIface
 *
 * The type interface which the task dispatcher itself obeys:
 *
 *  - \subpage pageInterfaceTaskDispatcher
 *
 */


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
 * \par InputType getTaskInput(unsigned int k) const
 *          Provide input to a new task. \c k is the task iteration number
 *          and \c pcdata is a pointer to the shared const data.
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
 * \c Task should represent an instance of the task to complete (e.g. a
 * Metropolis-Hastings random walk). It should provide the following methods.
 *
 * \par typedef .. ResultType
 *          An alias for the type of, e.g. a structure, which contains the result of
 *          the given task. See <code>Task::getResult()</code>.
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
 *          be sure, you should make this a template method with parameters \a LoggerType
 *          and \a TaskManagerIface.
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
 *
 * \par ResultType getResult()
 *          Return a custom type which holds the result for the given task. This will be
 *          given to the result collector (see \ref pageInterfaceResultsCollector).
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



/** \page pageInterfaceResultsCollector ResultsCollector Interface
 *
 * \a ResultsCollector takes care of collecting the results from each task run.
 *
 * In the following, \a TaskCData is some type that was specified to the task
 * dispatcher. The type \a CountType designates the type used to count tasks (usually an
 * integral type of course). Also the type \a ResultType is the type declared for a result
 * by the task (\ref pageInterfaceTask). You may make the methods here take template
 * parameters so you don't have to worry about the exact types of these parameters.
 *
 * \par void init(CountType num_total_runs, CountType n_chunk, const TaskCData * pcdata)
 *         \a init() will be called before the tasks are run (e.g. before starting the
 *         parallel section), and may be used to initialize data.
 *
 * \par void collectResult(CountType task_no, const ResultType& taskresult, const TaskCData * pcdata)
 *         Called each time a task has finished, with the corresponding task result, along
 *         with information about which task it was and as always the shared constant
 *         data. This method does not need to worry about concurrence, for example writing
 *         to shared data (it is either called from the main thread in a critical section,
 *         or the data was serialized and passed to the main process, etc.)
 *
 * \par
 *         in the OMP task dispatcher (\ref Tomographer::MultiProc::OMP::TaskDispatcher),
 *         this is called within a \c critical OMP section, so it may safely access and
 *         write shared data.
 *
 * \par void runsFinished(CountType num_total_runs, const TaskCData * pcdata)
 *         Called after all the tasks have finished. This is the good time to, e.g.,
 *         finalize collected values, such as multiplying by parameter space volume,
 *         dividing by the number of samples to get the average, etc.
 *
 */


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
 * The task manager/dispatcher takes care of running tasks.  It should handle tasks
 * provided by a \ref pageInterfaceTask compliant template parameter, should allow these
 * tasks to share data via a \ref pageInterfaceTaskCData compliant type also specified as
 * template parameter, and should allow the results to be collected by a \ref
 * pageInterfaceResultsCollector.
 *
 * The exact APIs of how these tasks are specified, managed and run is not clearly
 * specified, but it is strongly advised to follow a model such as the simplistic \ref
 * Tomographer::MultiProc::Sequential::TaskDispatcher or the OpenMP-based \ref
 * Tomographer::MultiProc::OMP::TaskDispatcher.
 *
 * The task manager/dispatcher must, however, provide the following methods:
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
 * The task manager/dispatcher must also provide the following typedef:
 *
 * \par typedef ... TaskType;
 *   The \ref pageInterfaceTask-compliant type used to describe a task.
 *
 *
 * \todo DESCRIBE WHAT THE TASK MANAGER/DISPATCHER MUST DO; WHICH FUNCTIONS IT MUST CALL !!
 *
 * Grosso modo (see \ref MultiProc::Sequential::TaskDispatcher for a simple example):
 *
 * - initialize the results collector ;
 * 
 * - instantiate tasks and run them, by getting input for each one of them and providing
 *   them a pointer to the shared data structure CData ;
 *
 * - provide a task-manager-interface object (\ref pageInterfaceTaskManagerIface) to allow
 *   querying for status reports ;
 *
 * - after the task has finished, collect its result and call the results collector's
 *   collectResult() method ;
 *
 * - finalize the resultscollector .
 *
 */
