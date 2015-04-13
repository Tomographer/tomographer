
#ifndef MULTIPROC_H
#define MULTIPROC_H

#include <string>


namespace Tomographer {


/** \brief Definitions for multi-processing classes and helpers
 *
 * This namespace encompasses classes and functions allowing to dispatch and process
 * several tasks simultaneously.
 *
 * Currently, only an OpenMP implementation is provided, see \ref OMPTaskDispatcher.
 */
namespace MultiProc {

  /** \brief Basic status report class
   *
   * This may serve as base class for a more detailed and specific status report.
   *
   * See also:
   *  - \ref pageTaskManagerDispatcher;
   *  - \ref OMPTaskDispatcher's status report mechanism;
   *  - \ref DMIntegratorTasks::MHRandomWalkTask::StatusReport for an example usage.
   *
   */
  struct StatusReport
  {
    StatusReport()
      : fraction_done(0), msg("<unknown>")
    { }
    StatusReport(double fraction_done_, const std::string& msg_)
      : fraction_done(fraction_done_), msg(msg_)
    { }

    double fraction_done;
    std::string msg;
  };



}; // namespace MultiProc

}; // namespace Tomographer

#endif
