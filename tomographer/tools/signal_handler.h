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
#include <stdio.h>

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
struct TOMOGRAPHER_EXPORT SignalHandler
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

  //
  // helper for formatting a string inside a signal handler, using only
  // signal-safe POSIX C functions
  //
  static inline void format_string_with_signal_num(char * buffer, const std::size_t buffer_max_len,
                                                   int signum, const char * msgstart, const char * msgend)
  {
    const std::size_t msgstartlen = strlen(msgstart);
    const std::size_t msgendlen = strlen(msgend);

    // silence "unused" warnings if building without asserts
    (void)buffer_max_len;
    (void)msgendlen;

    assert(signum < 100) ;
    assert(buffer_max_len >= msgstartlen + 2 + msgendlen + 1) ;

    // strcpy() and strcat() are signal-safe according to POSIX requirements

    strcpy(buffer, msgstart);

    buffer[msgstartlen] = (48 + (signum/10)%10); // 48 == '0'
    buffer[msgstartlen+1] = 48 + (signum%10);
    buffer[msgstartlen+2] = '\0';

    strcat(buffer, msgend);
  }

  static inline void write_string_stderr_ignore_result(const char * msg) // zero-terminated
  {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"

    write(STDERR_FILENO, msg, strlen(msg));

#pragma GCC diagnostic pop
  }

  static std::time_t last_sig_hit_time[NSIG] = { 0 };
  static SignalHandler * signal_handler[NSIG] = { NULL };

  //
  // hmmm.... actually our implementation is not safe strictly speaking, because
  // we call functions like std::fprintf which are not signal-reentrant -- see
  // http://en.cppreference.com/w/cpp/utility/program/signal#Signal_handler
  //
  static void signal_dispatch_fn(int signum)
  {
    //
    // POSIX defines a set of functions which we can call from a signal handler --
    // http://pubs.opengroup.org/onlinepubs/9699919799/functions/V2_chap02.html#tag_15_04_03_03
    //

    using namespace std;

    // abort() is OK in a signal handler
    assert(signum >= 0 && signum < NSIG && "signum out of range 0...NSIG !") ;
    
#if defined(__MINGW32__) || defined(__MINGW64__)
    // re-attach handler---seems like it's needed on Windows...
    // signal() is OK in a signal handler
    signal(signum, tomo_internal::signal_dispatch_fn);
#endif
    
    // write() is OK in a signal handler, fprintf() is not.  So do the formatting ourselves.
    //fprintf(stderr, "\n*** interrupt (%d)\n", signum);
    // strcpy() and strlen() are OK
    { const char * msgint1 = "\n*** interrupt (";
      const char * msgint2 = ")\n";
      char msgint[32];
      format_string_with_signal_num(msgint, sizeof(msgint), signum, msgint1, msgint2);
      write_string_stderr_ignore_result(msgint);
    }

    time_t now;

    // time() is OK in a signal handler
    time(&now);

    if ( (now - tomo_internal::last_sig_hit_time[signum]) < TOMOGRAPHER_SIG_HANDLER_REPEAT_EXIT_DELAY ) {
      // two interrupts within two seconds --> exit
      // write() is OK in a signal handler, fprintf() is not
      //fprintf(stderr, "\n*** Exit\n");
      const char * bufferexit = "\n*** Exit\n";
      write_string_stderr_ignore_result(bufferexit);
      ::exit(1);
      return;
    }

    tomo_internal::last_sig_hit_time[signum] = now;
    
    if (signal_handler[signum] != NULL) {
      signal_handler[signum]->handleSignal(signum);
    } else {
      // write() is OK in a signal handler, fprintf() is not.  So do the formatting ourselves.
      const char * msghandle1 = "Warning: sig_handle: no signal handler set (got signal ";
      const char * msghandle2 = ")\n";
      char msghandle[1024];
      format_string_with_signal_num(msghandle, sizeof(msghandle), signum, msghandle1, msghandle2) ;
      write_string_stderr_ignore_result(msghandle);
    }

  }
}


/** \brief Installs the given signal handler to catch the signal \a signum
 *
 * If other handlers are alreay set for a different signal number, they are not
 * overridden.
 */
inline void installSignalHandler(int signum, SignalHandler * sobj)
{
  tomo_internal::signal_handler[signum] = sobj;
  signal(signum, tomo_internal::signal_dispatch_fn);
}


} // namespace Tools
} // namespace Tomographer


#endif
