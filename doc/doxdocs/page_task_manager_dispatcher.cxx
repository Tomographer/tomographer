
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
 */



/** \page pageInterfaceResultsCollector ResultsCollector Interface
 *
 * Responsible for collecting results from many repetitions of a task.
 *
 * ......................
 */


/** \page pageInterfaceTaskManagerIface TaskManagerIface Interface
 *
 * Provides an interface for the task to query the task manager about, e.g., if the user
 * has requested a status report.
 *
 * .......................
 */
