

#include "common.h"

#include "pylogger.h"



PyLogger::PyLogger()
  : Tomographer::Logger::LoggerBase<PyLogger>(),
  py_logging(),
  py_logger(),
  _bypasspython(false)
{
}


void PyLogger::initPythonLogger()
{
  py_logging = boost::python::import("logging");
  py_logger = py_logging.attr("getLogger")("tomographer");

  //setLevel( fromPythonLevel(py_logger.attr("level")) );
  setLevel( fromPythonLevel(py_logger.attr("getEffectiveLevel")()) ); // level at which messages will be seen

  this->debug("PyLogger::initPythonLogger", "Initialized python-compatible logging.");
}


void PyLogger::setLevel(int level)
{
  setLogLevel(level);
}

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
    //      try {
    if (PyErr_Occurred() != NULL || PyErr_CheckSignals() == -1) {
      throw boost::python::error_already_set();
    }
    logfn(*boost::python::make_tuple(pylevel, msg), **kwargs);
    if (PyErr_Occurred() != NULL || PyErr_CheckSignals() == -1) {
      throw boost::python::error_already_set();
    }
    // } catch (boost::python::error_already_set & e) {
    //   //PyErr_Print();
    //   //fprintf(stderr, "Propagating exception in call to python logging function.\n");
    //   throw e;
    // }
  }
  //fprintf(stderr, "HERE (Y)\n");
}



boost::python::object PyLogger::toPythonLevel(int level) const
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

int PyLogger::fromPythonLevel(boost::python::object pylvl) const
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
