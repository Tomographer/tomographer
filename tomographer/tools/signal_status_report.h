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


#ifndef TOMOGRAPHER_TOOLS_SIGNAL_STATUS_REPORT
#define TOMOGRAPHER_TOOLS_SIGNAL_STATUS_REPORT


/** \file signal_status_report.h
 *
 * \brief Basic common code for intercepting a signal to print status information of a
 *        task manager.
 *
 */

#include <tomographer/tools/signal_handler.h>
#include <tomographer/multiproc.h>


namespace Tomographer
{
namespace Tools
{

/** \brief A generic handler which requests a status report from an OMPTaskDispatcher
 *
 */
template<typename TaskDispatcher, typename Logger>
struct TOMOGRAPHER_EXPORT SigHandlerTaskDispatcherStatusReporter
  : public SignalHandler
{
  typedef typename TaskDispatcher::FullStatusReportType FullStatusReportType;

  SigHandlerTaskDispatcherStatusReporter(TaskDispatcher * tasks_, Logger & logger_)
    : tasks(tasks_), logger(logger_)
  {
    logger.debug("Tomographer::Tools::SigHandlerTaskDispatcherStatusReporter",
                 "Setting up signal handler, with default callback") ;
    tasks->setStatusReportHandler(intermediateProgressReport) ;
  }

  template<typename CallbackFn>
  SigHandlerTaskDispatcherStatusReporter(TaskDispatcher * tasks_, Logger & logger_, CallbackFn && fn)
    : tasks(tasks_), logger(logger_)
  {
    logger.debug("Tomographer::Tools::SigHandlerTaskDispatcherStatusReporter",
                 "Setting up signal handler, with custom callback") ;
    tasks->setStatusReportHandler(std::forward<CallbackFn>(fn)) ;
  }
  
  TaskDispatcher * tasks;
  Logger & logger;

  virtual void handleSignal(int /*sig*/)
  {
    tasks->requestStatusReport();
  }

  /** \brief Format a nice intermediate progress report.
   *
   */
  static void intermediateProgressReport(const FullStatusReportType& report)
  {
    std::cerr << "\n" << report.getHumanReport() << "\n";
  };

};

template<typename TaskDispatcher, typename LoggerT>
inline SigHandlerTaskDispatcherStatusReporter<TaskDispatcher, LoggerT>
makeSigHandlerTaskDispatcherStatusReporter(TaskDispatcher * tasks, LoggerT & logger)
{
  return SigHandlerTaskDispatcherStatusReporter<TaskDispatcher, LoggerT>(tasks, logger);
}


template<typename TaskDispatcher, typename LoggerT, typename CallbackFn>
inline SigHandlerTaskDispatcherStatusReporter<TaskDispatcher, LoggerT>
makeSigHandlerTaskDispatcherStatusReporter(TaskDispatcher * tasks, LoggerT & logger, CallbackFn && fn)
{
  return SigHandlerTaskDispatcherStatusReporter<TaskDispatcher, LoggerT>(
      tasks,
      logger,
      std::forward<CallbackFn>(fn)
      );
}



} // namespace Tools
} // namespace Tomographer


#endif
