/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
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


#ifndef TOMOGRAPHER_TOOLS_SIGNAL_STATUS_REPORT
#define TOMOGRAPHER_TOOLS_SIGNAL_STATUS_REPORT


/** \file signal_status_report.h
 *
 * \brief Basic common code for intercepting a signal to print status information of a
 *        task manager.
 *
 */

#include <tomographer2/tools/signal_handler.h>
#include <tomographer2/multiproc.h>


namespace Tomographer
{
namespace Tools
{

/** \brief A generic handler which requests a status report from an OMPTaskDispatcher
 *
 * Note: The time displayed in the report is counted from the moment this signal handler
 * is instantiated.
 */
template<typename TaskDispatcher, typename Logger,
         typename TimerClock = std::chrono::system_clock>
struct SigHandlerTaskDispatcherStatusReporter
  : public SignalHandler
{
  typedef Tomographer::MultiProc::FullStatusReport<typename TaskDispatcher::TaskType::StatusReportType>
    FullStatusReportType;

  SigHandlerTaskDispatcherStatusReporter(TaskDispatcher * tasks_, Logger & logger_)
    : tasks(tasks_), logger(logger_), time_start()
  {
    tasks->setStatusReportHandler(
        [this](const FullStatusReportType& report) {
          logger.debug("SigHandlerStatusReporter/lambda", "intermediate progress report lambda called");
          this->intermediateProgressReport(report);
        });
  }
  
  TaskDispatcher * tasks;
  Logger & logger;

  typename TimerClock::time_point time_start;

  virtual void handleSignal(int /*sig*/)
  {
    tasks->requestStatusReport();
  }

  /** \brief Format a nice intermediate progress report.
   *
   */
  void intermediateProgressReport(const FullStatusReportType& report)
  {
    std::string elapsed = fmtDuration(TimerClock::now() - time_start);
    fprintf(stderr,
            "\n"
            "=========================== Intermediate Progress Report ============================\n"
            "                                             (hit Ctrl+C within %2d sec. to interrupt)\n"
            "  Total Completed Runs: %d/%d: %5.2f%%\n"
            "  %s total elapsed\n",
            TOMOGRAPHER_SIG_HANDLER_REPEAT_EXIT_DELAY,
            report.num_completed, report.num_total_runs,
            (double)report.num_completed/report.num_total_runs*100.0,
            elapsed.c_str());
    if (report.workers_running.size() == 1) {
      if (report.workers_running[0]) {
        fprintf(stderr, "--> %s\n", report.workers_reports[0].msg.c_str());
      }
    } else if (report.workers_running.size() > 1) {
      fprintf(stderr,
              "Current Run(s) information (workers working/spawned %d/%d):\n",
              (int)std::count(report.workers_running.begin(), report.workers_running.end(), true),
              (int)report.workers_running.size()
          );
      for (unsigned int k = 0; k < report.workers_running.size(); ++k) {
        std::string msg = report.workers_running[k] ? report.workers_reports[k].msg : std::string("<idle>");
        fprintf(stderr, "=== #%2u: %s\n", k, msg.c_str());
      }
    } else {
      // no info. (workers_running.size() == 0)
    }
    fprintf(stderr,
            "=====================================================================================\n\n");
  };

};

template<typename TaskDispatcher, typename LoggerT>
SigHandlerTaskDispatcherStatusReporter<TaskDispatcher, LoggerT>
makeSigHandlerTaskDispatcherStatusReporter(TaskDispatcher * tasks, LoggerT & logger)
{
  return SigHandlerTaskDispatcherStatusReporter<TaskDispatcher, LoggerT>(tasks, logger);
}


} // namespace Tools
} // namespace Tomographer


#endif
