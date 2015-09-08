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
 * <em>The following describes &lsquo;type interfaces.&#rsquo; See \ref pageTypeInterfaces
 * for more info on what that is.</em>
 *
 * The following are type interfaces that allow the creation of several instances of a
 * task, possibly reporting intermediate status, and collecting results in the end.
 *
 * Such interfaces are required, for example, to run parallel tasks with the OMP task
 * dispatcher, \ref Tomographer::MultiProc::OMP::TaskDispatcher.
 *
 * In the future, I hope we can also write an MPI implementation using the same
 * interfaces. (Hopefully everything works fine if \a ResultType and \a ConstantDataType
 * is serializable)
 *
 * A task is implemented by a \ref pageInterfaceTask. It may refer to some global,
 * constant data (the parameters of the problem) stored in a struct (referred to as \a
 * ConstantDataType in the following docs). Each \a Task generates a result, which is sent
 * to and collected by a type responsible for aggregating the data in a usable form (e.g.,
 * calculating an average and std. deviation), which compiles with the \ref
 * pageInterfaceResultsCollector. The task manager exposes an API to interact with the \a
 * Task objects: it is the \ref pageInterfaceTaskManagerIface. This interface may be used
 * for example in order to submit intermediate status reports.
 *
 * Documented type interfaces:
 *
 *  - \subpage pageInterfaceTask
 *  - \subpage pageInterfaceResultsCollector
 *  - \subpage pageInterfaceTaskManagerIface
 */





/** \page pageInterfaceTask Task Interface
 *
 * A task which may be repeated in parallel with different inputs.
 *
 * \c Task should represent an instance of the task to complete (e.g. a
 * Metropolis-Hastings random walk). It should provide the following methods.
 *
 * In the following, \a ConstantDataType is some type that was specified to the task
 * dispatcher.
 *
 * \par typedef .. ResultType
 *          An alias for the type of, e.g. a structure, which contains the result of
 *          the given task. See <code>Task::getResult()</code>.
 *
 * \par static InputType get_input(unsigned int k, const ConstantDataType * pcdata)
 *          Provide input to a new task. \c k is the task iteration number
 *          and \c pcdata is a pointer to the shared const data.
 *
 * \par
 *          The return value may be any type. It will be passed directly to the first
 *          argument of the constructor.
 *
 * \par Task(InputType input, const ConstantDataType * pcdata)
 *          Task constructor: construct a Task instance which will solve the task for the
 *          given input. The \a input parameter is whatever \c
 *          Task::get_input() returned.
 *
 * \par void run(const ConstantDataType * pcdata, LoggerType & logger, TaskManagerIface * tmgriface)
 *          Actually runs the task. It can log to the given \c logger (see \ref
 *          Tomographer::Logger::LoggerBase). Note that the \a logger need NOT be the
 *          logger that may have been specified, e.g., to the task dispatcher: it could
 *          be, for example, an internal thread-safe wrapper to your original logger. To
 *          be sure, you should make this a template method with parameters \a LoggerType
 *          and TaskManagerIface.
 *
 * \par
 *          The code in \c run() should poll <code>tmgriface->status_report_requested()</code>
 *          and provide a status report if requested to do so via
 *          <code>tmgriface->status_report(const TaskStatusReportType &)</code>. See documentation
 *          for \ref pageInterfaceTaskManagerIface.
 *
 * \par ResultType getResult()
 *          Return a custom type which holds the result for the given task. This will be
 *          given to the result collector (see \ref pageInterfaceResultsCollector).
 */



/** \page pageInterfaceResultsCollector ResultsCollector Interface
 *
 * \a ResultsCollector takes care of collecting the results from each task run.
 *
 * In the following, \a ConstantDataType is some type that was specified to the task
 * dispatcher. The type \a CountType designates the type used to count tasks (usually an
 * integral type of course). Also the type \a ResultType is the type declared for a result
 * by the task (\ref pageInterfaceTask). You may make the methods here take template
 * parameters so you don't have to worry about the exact types of these parameters.
 *
 * \par void init(CountType num_total_runs, CountType n_chunk, const ConstantDataType * pcdata)
 *         \a init() will be called before the tasks are run (e.g. before starting the
 *         parallel section), and may be used to initialize data.
 *
 * \par collect_result(CountType task_no, const ResultType& taskresult, const ConstantDataType * pcdata)
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
 * \par void runs_finished(CountType num_total_runs, const ConstantDataType * pcdata)
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
 * Tasks should poll \a status_report_requested() often (e.g. at each iteration), and
 * submit a status report if that function returns \c true.
 *
 * \par bool status_report_requested()
 *          Return \c true if a status report was recently requested. If so, you should
 *          submit your report using the \a status_report() method.  This function should
 *          be implemented such that it can be called often without performance problems
 *          (e.g. check shared variable value).
 *
 * \par void status_report(const TaskStatusReportType &)
 *          Submit the status report if \a status_report_requested() returned \c
 *          true. Call this function ONCE only per task, and only if a status report was
 *          requested. The behavior of this function otherwise is undefined.
 *
 */
