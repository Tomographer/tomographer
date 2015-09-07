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


#ifndef SIGNAL_STATUS_REPORT
#define SIGNAL_STATUS_REPORT

/** \file signal_status_report.h
 *
 * \brief Basic common code for intercepting a signal to print status information of a
 *        task manager.
 *
 */



#include <signal.h>

#include <cstdio>
#include <ctime>

#include <chrono>


namespace Tomographer
{
namespace Tools
{

/** \brief An abstract signal handler, in class form
 */
struct SignalHandler
{
  SignalHandler() { }
  virtual ~SignalHandler() { }
  
  virtual void handle_signal(int) = 0;
};
  

/** \brief A generic handler which requests a status report from an OMPTaskDispatcher
 */
template<typename TaskDispatcher, typename Logger, typename TimerClock = std::chrono::system_clock>
struct SigHandlerTaskDispatcherStatusReporter : public SignalHandler
{
  SigHandlerTaskDispatcherStatusReporter(TaskDispatcher * tasks_, Logger & logger_)
    : tasks(tasks_), logger(logger_), time_start()
  {
    tasks->set_status_report_handler(
        [this](const typename TaskDispatcher::FullStatusReportType& report) {
          logger.debug("SigHandlerStatusReporter/lambda", "intermediate progress report lambda called");
          this->intermediate_progress_report(report);
        });
  }
  
  TaskDispatcher * tasks;
  Logger & logger;

  typename TimerClock::time_point time_start;

  virtual void handle_signal(int /*sig*/)
  {
    tasks->request_status_report();
  }

  /** \brief Format a nice intermediate progress report.
   *
   */
  void intermediate_progress_report(const typename TaskDispatcher::FullStatusReportType& report)
  {
    std::string elapsed = fmt_duration(TimerClock::now() - time_start);
    fprintf(stderr,
            "\n"
            "=========================== Intermediate Progress Report ============================\n"
            "                                              (hit Ctrl+C quickly again to interrupt)\n"
            "  Total Completed Runs: %d/%d: %5.2f%%\n"
            "  %s total elapsed\n"
            "Current Run(s) information (threads working/spawned %d/%d):\n",
            report.num_completed, report.num_total_runs,
            (double)report.num_completed/report.num_total_runs*100.0,
            elapsed.c_str(), report.num_active_working_threads,
            report.num_threads
            );
    // calculate padding needed to align results
    //    typedef typename OMPTaskDispatcher::TaskStatusReportType TaskStatusReportType;
    //    auto elem = std::max_element(report.tasks_reports.begin(), report.tasks_reports.end(),
    //                                 [](const TaskStatusReportType& a, const TaskStatusReportType& b) -> bool {
    //                                   return a.msg.size() < b.msg.size();
    //                                 });
    //    std::size_t maxmsgwid = (*elem).msg.size();
    for (int k = 0; k < report.num_threads; ++k) {
      std::string msg = report.tasks_running[k] ? report.tasks_reports[k].msg : std::string("<idle>");
      //      if (msg.size() < maxmsgwid) {
      //        msg.insert(0, maxmsgwid - msg.size(), ' ');
      //      }
      fprintf(stderr, "=== Thread #%2d: %s\n", k, msg.c_str());
    }
    fprintf(stderr,
            "=====================================================================================\n\n");
  };

};

template<typename TaskDispatcher, typename LoggerT>
SigHandlerTaskDispatcherStatusReporter<TaskDispatcher, LoggerT>
makeSigHandlerTaskDispatcherStatusReporter(TaskDispatcher * tasks, LoggerT & logger)
{
  // yes, RVO better kick in
  return SigHandlerTaskDispatcherStatusReporter<TaskDispatcher, LoggerT>(tasks, logger);
}




#ifndef SIG_STATUS_REPORT_REPEAT_EXIT_DELAY
#define SIG_STATUS_REPORT_REPEAT_EXIT_DELAY 2
#endif

namespace tomo_internal {
  static std::time_t last_sig_hit_time = 0;
  static SignalHandler * signal_handler = NULL;

  void signal_dispatch_fn(int signal)
  {
    std::fprintf(stderr, "\n*** interrupt\n");
    std::time_t now;
    time(&now);
    if ( (now - tomo_internal::last_sig_hit_time) < SIG_STATUS_REPORT_REPEAT_EXIT_DELAY ) {
      // two interrupts within two seconds --> exit
      std::fprintf(stderr, "\n*** Exit\n");
      ::exit(1);
      return;
    }
    tomo_internal::last_sig_hit_time = now;
    
    if (signal_handler != NULL) {
      signal_handler->handle_signal(signal);
    } else {
      fprintf(stderr, "Warning: sig_handle: no signal handler set (got signal %d)\n", signal);
    }
  }
}


/** \brief Installs the given signal handler to catch the signal \a signum
 *
 * \warning This will replace any previous signal handlers set, including for other
 * signals!
 *
 * \bug Don't replace the signal handlers for other signals...
 *
 */
template<typename SigHandler>
void installSignalStatusReportHandler(int signum, SigHandler * sobj)
{
  tomo_internal::signal_handler = sobj;
  signal(signum, tomo_internal::signal_dispatch_fn);
}


} // namespace Tools
} // namespace Tomographer


#endif
