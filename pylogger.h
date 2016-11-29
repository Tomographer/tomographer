#ifndef PYLOGGER_H

#include <tomographer2/tools/loggers.h>


class PyLogger; // forward declaration
namespace Tomographer { namespace Logger {
  // Traits for PyLogger
  template<>
  struct LoggerTraits<PyLogger> : DefaultLoggerTraits
  {
    enum {
      //      // We enforce thread-safety with "#pragma omp critical" sections
      //      IsThreadSafe = 1, // does not work, causes deadlocks when already in a critical section
      // set this to a particular level to unconditinally discard any
      // message logged with strictly lower importance level.
      StaticMinimumImportanceLevel = -1
      //      // reflect the level from Python's logger -- NO, TOO SLOW.
      //      HasOwnGetLevel = 1
    };
  };
} } // namespaces
class PyLogger : public Tomographer::Logger::LoggerBase<PyLogger>
{
private:
  boost::python::object py_logging; // the Python "logging" module
  boost::python::object py_logger; // the Python "logging.Logger" instance for our use
  bool _bypasspython;
public:
  PyLogger()
    : Tomographer::Logger::LoggerBase<PyLogger>(),
    py_logging(),
    py_logger(),
    _bypasspython(false)
  {
  }

  inline void initPythonLogger()
  {
    py_logging = boost::python::import("logging");
    py_logger = py_logging.attr("getLogger")("tomographer");

    setLevel( fromPythonLevel(py_logger.attr("level")) );

    this->debug("PyLogger::initPythonLogger", "Initialized python-compatible logging.");
  }

  inline void setLevel(int level)
  {
    setLogLevel(level);
  }

//   inline int level() const
//   {
//     bool boost_error = false;
//     int level = -1;
// #pragma omp critical
//     {
//       try {
//         if (_bypasspython) {
//           level = Tomographer::Logger::INFO;
//         } else if (py_logger.is_none()) {
//           fprintf(stderr, "INTERNAL ERROR: PYTHON LOGGER NOT SET.\nIn attempt to call level()\n");
//           level = -1;
//         } else {
//           level = fromPythonLevel(py_logger.attr("getEffectiveLevel")());
//         }
//       } catch (boost::python::error_already_set & e) {
//         boost_error = true;
//       }
//     } // omp critical
//     if (boost_error) {
//       throw boost::python::error_already_set();
//     }
//     return level;
//   }

  inline void emitLog(int level, const char * origin, const std::string & msg)
  {
    if (_bypasspython) {
      fprintf(stderr, "%s:%s:%s (bypassed python logger)\n",
              Tomographer::Logger::LogLevel(level).levelName().c_str(),
              origin,
              msg.c_str());
      return;
    }
    // need custom critical:
    if (py_logger.is_none()) {
      fprintf(stderr, "INTERNAL ERROR: PYTHON LOGGER NOT SET.\n");
      fprintf(stderr, "Message was (%d): %s: %s\n\n", level, origin, msg.c_str());
    } else {
      //fprintf(stderr, "Emitting log ... (%s)\n", msg.c_str());

      boost::python::object pylevel = toPythonLevel(level);

      boost::python::dict extra;
      extra["origin"] = origin;
      boost::python::dict kwargs;
      kwargs["extra"] = extra;
      auto logfn = py_logger.attr("log");
      try {
        logfn(*boost::python::make_tuple(pylevel, msg), **kwargs);
      } catch (boost::python::error_already_set & e) {
        PyErr_Print();
        //fprintf(stderr, "Ignored error in call to python logging function.\n");
        throw(e);
      }
    }
  }

  inline boost::python::object toPythonLevel(int level)
  {
    if (py_logging.is_none()) {
      fprintf(stderr, "INTERNAL ERROR: PYTHON LOGGING MODULE NOT SET.\nIn attempt to call toPythonLevel().");
      return boost::python::object();
    }
    switch (level) {
    case Tomographer::Logger::ERROR:
      return py_logging.attr("ERROR");
    case Tomographer::Logger::WARNING:
      return py_logging.attr("WARNING");
    case Tomographer::Logger::INFO:
      return py_logging.attr("INFO");
    case Tomographer::Logger::DEBUG:
      return py_logging.attr("DEBUG");
    case Tomographer::Logger::LONGDEBUG:
    default:
      return py_logging.attr("NOTSET");
    }
  }

  inline int fromPythonLevel(boost::python::object pylvl) const
  {
    if (py_logging.is_none()) {
      fprintf(stderr, "INTERNAL ERROR: PYTHON LOGGING MODULE NOT SET.\nIn attempt to call fromPythonLevel().");
      return -1;
    }

    if (pylvl < py_logging.attr("DEBUG")) {
      return Tomographer::Logger::LONGDEBUG;
    } else if (pylvl < py_logging.attr("INFO")) {
      return Tomographer::Logger::DEBUG;
    } else if (pylvl < py_logging.attr("WARNING")) {
      return Tomographer::Logger::INFO;
    } else if (pylvl < py_logging.attr("ERROR")) {
      return Tomographer::Logger::WARNING;
    } else {
      return Tomographer::Logger::ERROR;
    }
  }

  inline void bypassPython() {
    _bypasspython = true;
  }
  inline void endBypassPython() {
    _bypasspython = false;
  }

  struct _BypassPython {
    PyLogger *pylogger;
    _BypassPython(PyLogger * l) : pylogger(l) {
      pylogger->bypassPython();
    }
    _BypassPython(const _BypassPython & copy) = delete;
    _BypassPython(_BypassPython && other) : pylogger(other.pylogger) { other.pylogger = NULL; }
    ~_BypassPython() {
      if (pylogger != NULL) {
        pylogger->endBypassPython();
      }
    }
  };
  inline _BypassPython pushBypassPython() {
    return _BypassPython(this);
  }

  
};






// to allow for debugging
//Tomographer::Logger::FileLogger tpy_logger(std::stderr, Tomographer::Logger::INFO, true /* display_origin */);
// to be fast
//Tomographer::Logger::VacuumLogger tpy_logger;
// slow, but really good integration to Python logging
extern PyLogger tpy_logger;



typedef decltype(tpy_logger) TPyLoggerType;



#endif

