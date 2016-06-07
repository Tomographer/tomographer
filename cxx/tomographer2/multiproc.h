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

#ifndef MULTIPROC_H
#define MULTIPROC_H

#include <string>


/** \file multiproc.h
 *
 * \brief Some common definitions for multiprocessing interfaces.
 *
 * See also \ref pageTaskManagerDispatcher as well as \ref Tomographer::MultiProc::OMP.
 */


namespace Tomographer {


/** \brief Definitions for multi-processing classes and helpers
 *
 * This namespace encompasses classes and functions allowing to dispatch and process
 * several tasks simultaneously.
 *
 * Currently, only an OpenMP implementation is provided, see \ref MultiProc::OMP and \ref
 * MultiProc::OMP::TaskDispatcher.
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
