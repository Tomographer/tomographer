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


#ifndef TOMOGRAPHER_PY_PYLOGGER_H
#define TOMOGRAPHER_PY_PYLOGGER_H

// this is normally not required (circular), because common.h includes this file; yet keep
// this in case a user includes pylogger.h without including common.h first.
#include <tomographerpy/common.h>

#if PY_MAJOR_VERSION < 3
// If using Python 2, we need the eval() function internally below -- see toPythonLevel()
#include <pybind11/eval.h>
#endif

#include <tomographer/tools/loggers.h>


namespace tpy {
  class PyLogger; // forward declaration
}
namespace Tomographer { namespace Logger {
  // Traits for PyLogger
  template<>
  struct TOMOGRAPHER_EXPORT LoggerTraits<tpy::PyLogger> : DefaultLoggerTraits
  {
    enum {
      // Python calls are not thread safe.
      IsThreadSafe = 0
    };
  };
} } // namespaces

namespace tpy {

/** \brief Logger providing transparent integration with Python's \a logging module
 *
 * This logger can be used whenever you need to specify a logger to any of Tomographer's
 * classes, and it will log messages directly into Python's \a logging mechanism.
 *
 * The log level is set upon initialization of the PyLogger, by replicating the effective
 * level of the Python logging mechanism.
 *
 * \note Changes from the Python side to the \a logging's level will not be noticed on the
 *       C++ side.  If you need to be able to dynamically change the log level, then you
 *       should expose your instance to Python (\a PyLogger is exposed as a class), and
 *       set the new level BOTH via Python's \c logging module AND by setting the \a
 *       logging attribute of the PyLogger instance to the same level.
 *
 * Log levels correspond as follows:
 *
 *  - Tomographer::Logger::ERROR - \a logging.ERROR
 *  - Tomographer::Logger::WARNING - \a logging.WARNING
 *  - Tomographer::Logger::INFO - \a logging.INFO
 *  - Tomographer::Logger::DEBUG - \a logging.DEBUG
 *  - Tomographer::Logger::LONGDEBUG - \a 1 (one)
 *
 * (Note that there is no corresponding level name in the \c logging module for \c
 * LONGDEBUG.)
 *
 * This logger is not thread-safe. You shouldn't even use it with any
 * Tomographer::MultiProc task dispatcher, even if these latter provide locks, because
 * once the GIL (Global Interpreter Lock) is released, you need to acquire it before
 * calling any Python API function. In this case, you should use a \ref
 * GilProtectedPyLogger.
 */
class TOMOGRAPHER_EXPORT PyLogger
  : public Tomographer::Logger::LoggerBase<PyLogger>
{
private:
  pybind11::object py_logging; // the Python "logging" module
  pybind11::object py_logger; // the Python "logging.Logger" instance for our use
  bool _bypasspython;
public:
  //! Constructor
  inline PyLogger();

  //! Initialize the logger and attach it to a logger in the \c logging module named \a logger_name
  inline void initPythonLogger(std::string logger_name = "tomographer");

  /** \brief Change the level of the current logger. Note that this will NOT automatically
   *         change the effective level of the underlying Python logger!
   */
  inline void setLevel(int level);

  //! Convert a Tomographer::Logger level to a Python \c logging level (Python integer)
  inline py::object toPythonLevel(int level) const;

  //! Convert a Tomographer::Logger level to a Python \c logging level name (Python string)
  inline py::object toPythonLevelName(int level) const;

  //! Convert a \c logging level to a Tomographer::Logger level constant
  inline int fromPythonLevel(py::object pylvl) const;


  //! Callback for actually emitting log messages, don't call this manually
  inline void emitLog(int level, const char * origin, const std::string & msg);


  // // Tools to bypass a Python call to logger and just write to stderr -- not used so far.
  // // To be used in a critical situation where it's not a good idea to call Python code
  // inline void bypassPython() {
  //   _bypasspython = true;
  // }
  // inline void endBypassPython() {
  //   _bypasspython = false;
  // }
  // struct _BypassPython {
  //   PyLogger *pylogger;
  //   _BypassPython(PyLogger * l) : pylogger(l) {
  //     pylogger->bypassPython();
  //   }
  //   _BypassPython(const _BypassPython & copy) = delete;
  //   _BypassPython(_BypassPython && other) : pylogger(other.pylogger) { other.pylogger = NULL; }
  //   ~_BypassPython() {
  //     if (pylogger != NULL) {
  //       pylogger->endBypassPython();
  //     }
  //   }
  // };
  // /**
  //  * \code
  //  *   {
  //  *     auto dummy = pushBypassPython();
  //  *
  //  *     ...
  //  *   } // bypass released after "dummy" goes out of scope
  //  * \endcode
  //  */
  // inline _BypassPython pushBypassPython() {
  //   return _BypassPython(this);
  // }

};


inline
PyLogger::PyLogger()
  : Tomographer::Logger::LoggerBase<PyLogger>(),
  py_logging(),
  py_logger(),
  _bypasspython(false)
{
}


inline
void PyLogger::initPythonLogger(std::string logger_name)
{
  py_logging = py::module::import("logging");
  py_logger = py::getattr(py_logging, "getLogger")(logger_name);

  // set the level at which messages will be actually seen -- any changes to the Python
  // logger's level configuration must be manually reported to us as well in order to have
  // the correct logging level.
  int lvl = fromPythonLevel(py::getattr(py_logger, "getEffectiveLevel")());
  setLevel( lvl );

  this->debug("PyLogger::initPythonLogger", [lvl](std::ostream & stream) {
      stream << "Initialized python-compatible logging. level = " << Tomographer::Logger::LogLevel(lvl);
    });
}


inline
void PyLogger::setLevel(int level)
{
  setLogLevel(level);
  // produce a warning if the level is set to LONGDEBUG but the messages won't display --
  // this really slows down the computation time and a user could be wondering why
  if (level == Tomographer::Logger::LONGDEBUG) {
    if (!py_logger.is_none()) {
      // but only perform this check if py_logger is not None
      int effective_level = fromPythonLevel(py::getattr(py_logger, "getEffectiveLevel")());
      if (effective_level != Tomographer::Logger::LONGDEBUG) {
        this->warning("PyLogger::setLevel", [&](std::ostream & stream) {
            stream << "Log level LONGDEBUG set on C++ logger but Python logger only displays messages of "
                   << "severity at least " << Tomographer::Logger::LogLevel(effective_level) << ". This will "
                   << "considerably and uselessly slow down the computation as tons of messages on the "
                   << "C++ side will be emitted to the Python logger (where they will be ignored) instead of "
                   << "being filtered out immediately.";
          });
      }
    }
  }
}

inline
void PyLogger::emitLog(int level, const char * origin, const std::string & msg)
{
  //fprintf(stderr, "HERE (X)\n");
  if (_bypasspython) {
    fprintf(stderr, "%s:%s:%s (bypassed python logger)\n",
            Tomographer::Logger::LogLevel(level).levelName().c_str(),
            origin,
            msg.c_str());
    return;
  }
  if (py_logger.is_none()) {
    fprintf(stderr,
            "tomographer:PyLogger: INTERNAL ERROR: PYTHON LOGGING MODULE NOT SET.\n"
            "In attempt to call emitLog().");
    fprintf(stderr,
            "Message was (%d): %s: %s\n\n", level, origin, msg.c_str());
  } else {
    //fprintf(stderr, "Emitting log ... (%s)\n", msg.c_str());

    py::object pylevel = toPythonLevel(level);

    //DEBUG::: fprintf(stderr, "DEBUG: pylevel = %s\n", py::repr(pylevel).cast<std::string>().c_str());

    std::string full_msg = std::string("<")+origin+"> "+msg;

    py::dict extra;
    extra["origin"] = origin;
    extra["raw_msg"] = msg;
    py::dict kwargs;
    kwargs["extra"] = extra;
    auto logfn = py::getattr(py_logger, "log");
    //      try {
    // if (PyErr_Occurred() != NULL || PyErr_CheckSignals() == -1) {
    //   throw py::error_already_set();
    // }
    logfn(*py::make_tuple(pylevel, full_msg), **kwargs);
    // if (PyErr_Occurred() != NULL || PyErr_CheckSignals() == -1) {
    //   throw py::error_already_set();
    // }
  }
}


inline
py::object PyLogger::toPythonLevel(int level) const
{
  if (py_logging.is_none()) {
    fprintf(stderr,
            "tomographer:PyLogger: INTERNAL ERROR: PYTHON LOGGING MODULE NOT SET.\n"
            "In attempt to call toPythonLevel().");
    return py::none();
  }
  switch (level) {
  case Tomographer::Logger::ERROR:
    return py::getattr(py_logging, "ERROR");
  case Tomographer::Logger::WARNING:
    return py::getattr(py_logging, "WARNING");
  case Tomographer::Logger::INFO:
    return py::getattr(py_logging, "INFO");
  case Tomographer::Logger::DEBUG:
    return py::getattr(py_logging, "DEBUG");
  case Tomographer::Logger::LONGDEBUG:
  default:
#if PY_MAJOR_VERSION >= 3
    return py::cast(1); //py::getattr(py_logging, "NOTSET");
#else
    // Returning py::cast(1) above causes an error in Python2 "TypeError: level must be an
    // integer" ... because it casts the 1 into "1L", a 'long', and not an 'int'!!  So
    // resort to an ugly hack:
    return py::eval("1");
#endif
  }
}

inline
py::object PyLogger::toPythonLevelName(int level) const
{
  if (py_logging.is_none()) {
    fprintf(stderr,
            "tomographer:PyLogger: INTERNAL ERROR: PYTHON LOGGING MODULE NOT SET.\n"
            "In attempt to call toPythonLevelName().");
    return py::none();
  }
  return py_logging.attr("getLevelName")(toPythonLevel(level));
}

inline
int PyLogger::fromPythonLevel(py::object pylvl) const
{
  if (py_logging.is_none()) {
    fprintf(stderr,
            "tomographer:PyLogger: INTERNAL ERROR: PYTHON LOGGING MODULE NOT SET.\n"
            "In attempt to call fromPythonLevel().");
    return -1;
  }

  if (pylvl.cast<int>() < py::getattr(py_logging, "DEBUG").cast<int>()) {
    return Tomographer::Logger::LONGDEBUG;
  } else if (pylvl.cast<int>() < py::getattr(py_logging, "INFO").cast<int>()) {
    return Tomographer::Logger::DEBUG;
  } else if (pylvl.cast<int>() < py::getattr(py_logging, "WARNING").cast<int>()) {
    return Tomographer::Logger::INFO;
  } else if (pylvl.cast<int>() < py::getattr(py_logging, "ERROR").cast<int>()) {
    return Tomographer::Logger::WARNING;
  } else {
    return Tomographer::Logger::ERROR;
  }
}




} // namespace tpy




#endif

