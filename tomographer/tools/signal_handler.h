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


#ifndef TOMOGRAPHER_TOOLS_SIGNAL_HANDLER
#define TOMOGRAPHER_TOOLS_SIGNAL_HANDLER


/** \file signal_handler.h
 *
 * \brief Basic common code for intercepting a signal and basic interfacing to C++
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

/** \brief An abstract signal handler (C++ interface)
 *
 * Just override \ref handleSignal() to define the handler.  A \ref SignalHandler
 * instance can then be provided to \ref installSignalHandler().
 */
TOMOGRAPHER_EXPORT struct SignalHandler
{
  SignalHandler() { }
  virtual ~SignalHandler() { }

  /** \brief Perform some action in reaction to a signal
   *
   * Override this method to specify what to do when the signal \a sig_num is caught.
   */
  virtual void handleSignal(int sig_num) = 0;
};
  


#ifndef TOMOGRAPHER_SIG_HANDLER_REPEAT_EXIT_DELAY
#define TOMOGRAPHER_SIG_HANDLER_REPEAT_EXIT_DELAY 2
#endif

namespace tomo_internal {
  static std::time_t last_sig_hit_time[NSIG] = { 0 };
  static SignalHandler * signal_handler[NSIG] = { NULL };

  static void signal_dispatch_fn(int signum)
  {
    assert(signum >= 0 && signum < NSIG && "signum out of range 0...NSIG !") ;
    
#if defined(__MINGW32__) || defined(__MINGW64__)
    // re-attach handler---seems like it's needed on Windows---why??
    signal(signum, tomo_internal::signal_dispatch_fn);
#endif
    
    std::fprintf(stderr, "\n*** interrupt (%d)\n", signum);
    std::time_t now;

    time(&now);

    if ( (now - tomo_internal::last_sig_hit_time[signum]) < TOMOGRAPHER_SIG_HANDLER_REPEAT_EXIT_DELAY ) {
      // two interrupts within two seconds --> exit
      std::fprintf(stderr, "\n*** Exit\n");
      ::exit(1);
      return;
    }

    tomo_internal::last_sig_hit_time[signum] = now;
    
    if (signal_handler[signum] != NULL) {
      signal_handler[signum]->handleSignal(signum);
    } else {
      std::fprintf(stderr, "Warning: sig_handle: no signal handler set (got signal %d)\n", signum);
    }

  }
}


/** \brief Installs the given signal handler to catch the signal \a signum
 *
 * If other handlers are alreay set for a different signal number, they are not
 * overridden.
 */
TOMOGRAPHER_EXPORT
void installSignalHandler(int signum, SignalHandler * sobj)
{
  tomo_internal::signal_handler[signum] = sobj;
  signal(signum, tomo_internal::signal_dispatch_fn);
}


} // namespace Tools
} // namespace Tomographer


#endif
