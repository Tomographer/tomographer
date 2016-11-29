#ifndef PYLOGGER_H
#define PYLOGGER_H

#include <boost/python.hpp>

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
  PyLogger();

  void initPythonLogger();

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
  inline _BypassPython pushBypassPython() {
    return _BypassPython(this);
  }
};




#endif

