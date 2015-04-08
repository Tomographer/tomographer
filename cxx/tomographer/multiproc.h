
#ifndef MULTIPROC_H
#define MULTIPROC_H

#include <string>


namespace Tomographer {

namespace MultiProc {

  /** \brief Simple status report class
   *
   * This may serve as base class for a more detailed and specific status report.
   *
   * See OMPTaskDispatcher's status report mechanism.
   */
  struct StatusReport
  {
    StatusReport(double fraction_done_, const std::string& msg_)
      : fraction_done(fraction_done_), msg(msg_)
    { }

    double fraction_done;
    std::string msg;
  };



}; // namespace MultiProc

}; // namespace Tomographer

#endif
