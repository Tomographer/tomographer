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
  return SigHandlerTaskDispatcherStatusReporter<TaskDispatcher, LoggerT>(tasks, logger);
}


} // namespace Tools
} // namespace Tomographer


#endif
