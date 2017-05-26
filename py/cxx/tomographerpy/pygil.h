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


#ifndef TOMOGRAPHER_PY_PYGIL_H
#define TOMOGRAPHER_PY_PYGIL_H

#include <vector>
#include <algorithm>

#include <tomographerpy/common.h>

#include <tomographer/tools/cxxutil.h>
#include <tomographer/tools/loggers.h>

#include <tomographerpy/pylogger.h>


#define TPY_EXPR_WITH_GIL( x )                      \
  [&]() { py::gil_scoped_acquire _tpy_gil; return (x); } ()




namespace tpy {
  class GilProtectedPyLogger; // forward declaration
} // namespace tpy
namespace Tomographer { namespace Logger {
  // Traits for PyLogger
  template<>
  struct TOMOGRAPHER_EXPORT LoggerTraits<tpy::GilProtectedPyLogger> : public DefaultLoggerTraits
  {
    enum {
      // This logger is thread-safe because it makes sure that it acquires the GIL as
      // needed.
      IsThreadSafe = 1
    };
  };
} } // namespaces Tomographer::Logger


namespace tpy {

/** \brief Logger type that relays to (wraps around) a PyLogger while protecting
 *         the call with GIL acquisition.
 *
 * You should use this logger type whenever you need to provide a logger to
 * Tomographer C++ classes which might be emitting messages while the GIL is not
 * held.
 *
 * This class behaves as a usual logger, i.e. you can call \a debug(), \a
 * info(), \a warning(), etc.
 *
 * Any calls to emitLog() will relay to the specified PyLogger instance after
 * ensuring the GIL is held.  GIL acquisition may be disabled by calling \a
 * requireGilAcquisition(false) (or by passing \a false as second argument to
 * the constructor), which may be useful in case you need to create the instance
 * outside of a GIL-release block:
 *
 * \code
 *   tpy::GilProtectedPyLogger gil_logger(logger, false);
 *
 *   // Messages emitted here should not attempt to acquire GIL as we haven't
 *   // released it yet
 *   Tomographer::MultiProc::CxxThreads::TaskDispatcher<....,GilProtectedPyLogger>
 *     tasks(..., gil_logger, ...);
 *
 *   {
 *     py::gil_scoped_release gilrelease;
 *     gil_logger->requireGilAcquisition(true);
 *
 *     tasks.run(); // emitted messages will acquire GIL
 *   }
 *   // Here we hold the GIL again
 *   gil_logger->requireGilAcquisition(false);
 *
 *   // now, extract results from the tasks
 *   py::dict results = ....  extract from `tasks` ...
 * \endcode
 */
class TOMOGRAPHER_EXPORT GilProtectedPyLogger
  : public Tomographer::Logger::LoggerBase<GilProtectedPyLogger>
{
public:
  /** \brief Constructor
   *
   * The argument \a logger_ is the PyLogger instance to wrap.
   *
   * If \a require_gil_acquisition_ is False
   */
  inline GilProtectedPyLogger(PyLogger & logger_, bool require_gil_acquisition_ = true)
    : LoggerBase<GilProtectedPyLogger>(logger_.level()), // freeze to current level
      logger(logger_),
      require_gil_acquisition(require_gil_acquisition_)
  {
  }

  //! The PyLogger which we relay messages to
  inline PyLogger & getLogger() const { return logger; }

  //! Whether or not we are set to acquire the GIL for emitting messages at this point
  inline bool getRequireGilAcquisition() const { return require_gil_acquisition; }

  //! Instruct to acquire (true) or not (false) the GIL when emitting messages
  inline void requireGilAcquisition(bool value) {
    tomographer_assert(require_gil_acquisition != value);
    require_gil_acquisition = value;
  }

  //! The callback function for the logger, you shouldn't call this directly
  inline void emitLog(int level, const char * origin, const std::string & msg)
  {
    if (require_gil_acquisition) {
      py::gil_scoped_acquire gil_acquire;
      logger.emitLog(level, origin, msg);
    } else {
      logger.emitLog(level, origin, msg);
    }
  }

private:
  PyLogger & logger;
  bool require_gil_acquisition;
};

} // namespace tpy


#endif
