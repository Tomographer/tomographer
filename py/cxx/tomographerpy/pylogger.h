

#ifndef TOMOGRAPHER_PY_PYLOGGER_H
#define TOMOGRAPHER_PY_PYLOGGER_H

// this is normally not required (circular), because common.h includes this file; yet keep
// this in case a user includes pylogger.h without including common.h first.
#include <tomographerpy/common.h>

#include <tomographer/tools/loggers.h>




class PyLogger; // forward declaration
namespace Tomographer { namespace Logger {
  // Traits for PyLogger
  template<>
  struct LoggerTraits<PyLogger> : DefaultLoggerTraits
  {
    enum {
      // Python calls are not thread-safe. Enforcing thread-safety manually with "#pragma
      // omp critical" sections is not a good idea, as it will cause deadlocks if called
      // from within code which itself is in a critical section.
      IsThreadSafe = 0
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
  PyLogger();

  void initPythonLogger(std::string logger_name = "tomographer");

  void setLevel(int level);

  void emitLog(int level, const char * origin, const std::string & msg);

  boost::python::object toPythonLevel(int level) const;

  int fromPythonLevel(boost::python::object pylvl) const;


  // Tools to bypass a Python call to logger and just write to stderr -- not used so far.
  // To be used in a critical situation where it's not a good idea to call Python code
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
  /**
   * \code
   *   {
   *     auto dummy = pushBypassPython();
   *
   *     ...
   *   } // bypass released after "dummy" goes out of scope
   * \endcode
   */
  inline _BypassPython pushBypassPython() {
    return _BypassPython(this);
  }
};




#endif

