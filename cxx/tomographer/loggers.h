
#ifndef LOGGERS_H
#define LOGGERS_H

#include <string>
#include <iostream>
#include <cstdio>
#include <cstdarg>


#define PRINTF1_ARGS_SAFE  __attribute__ ((format (printf, 1, 2)))
#define PRINTF2_ARGS_SAFE  __attribute__ ((format (printf, 2, 3)))
#define PRINTF3_ARGS_SAFE  __attribute__ ((format (printf, 3, 4)))
#define PRINTF4_ARGS_SAFE  __attribute__ ((format (printf, 4, 5)))



/** \brief Exception for bad \c printf format
 *
 * This exception is raised when \ref fmts() or \ref vfmts() are called with a bad format
 * argument, causing \c vsnprintf() to return a negative number.
 */
class bad_fmts_format : public std::exception
{
  std::string msg;
public:
  bad_fmts_format(const std::string& msg_) : msg(msg_) { }
  ~bad_fmts_format() throw() { }

  const char * what() const throw() {
    return msg.c_str();
  }
};



/** \brief \c printf- formatting to a \c std::string, with \c va_list pointer
 *
 * Does safe allocation, etc.
 */
inline std::string vfmts(const char* fmt, va_list vl)
{
  // check out printf() formatting for std::string:
  //    http://stackoverflow.com/a/10150393/1694896
  //    http://stackoverflow.com/a/26197300/1694896
  
  int size = 10;
  char * buffer = new char[size];
  va_list ap1;
  va_copy(ap1, vl);
  int nsize = vsnprintf(buffer, size, fmt, ap1);
  if (nsize < 0) {
    // failure: bad format probably
    throw bad_fmts_format("vsnprintf("+std::string(fmt)+") failure: code="+std::to_string(nsize));
  }
  if(size <= nsize) {
    // buffer too small: delete buffer and try again
    delete[] buffer;
    size = nsize+1; // +1 for "\0"
    buffer = new char[size];
    nsize = vsnprintf(buffer, size, fmt, vl);
  }
  std::string ret(buffer);
  delete[] buffer;
  va_end(ap1);
  return ret;
}


/** \brief \c printf- format to a \c std::string
 *
 * Does safe allocation, etc.
 */
inline std::string fmts(const char * fmt, ...)  PRINTF1_ARGS_SAFE;

// definition. It seems definitions cannot have function attributes (here
// PRINTF1_ARGS_SAFE), so that's why it's separate
inline std::string fmts(const char * fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  std::string result = vfmts(fmt, ap);
  va_end(ap);
  return result;
}


/** \brief Utility macro to format stream tokens to a \c std::string
 *
 * Usage example:
 * \code
 *   Eigen::VectorXd x = ...;
 *   std::string s = streamstr("x is = " << x.transpose());
 * \endcode
 *
 * This is typically useful if you want to feed in C++ objects to a printf()-like output
 * environment as used in the logger module (e.g. \ref SimpleFoutLogger).
 */
#define streamstr(tokens)                       \
  dynamic_cast<std::ostringstream&>(            \
      std::ostringstream().seekp(0) << tokens   \
      ).str()                                   \

/** \brief Utility macro to format stream tokens to a C string
 *
 * Shorthand for:
 * \code
 *    streamstr( tokens ).c_str()
 * \endcode
 */
#define streamcstr(tokens)  streamstr(tokens).c_str()




namespace Logger
{
  /** \brief Possible logging levels.
   *
   * Don't trust the numeric values here, they may change at any time. Just the order is
   * important.
   */
  enum {
    ERROR = 0,
    WARNING,
    INFO,
    DEBUG,
    LONGDEBUG
  };
};


/** \brief Base logger class.
 *
 * This class serves as base class for logger implementations. It provides storing a
 * current given level, emitting the log only if the level is reached, etc. Don't
 * instantiate this class directly.
 *
 * See also \ref SimpleFoutLogger.
 */
template<typename Derived>
class LoggerBase
{
public:

  LoggerBase(int level = Logger::INFO)
    : _level(level)
  {
  }

  PRINTF3_ARGS_SAFE
  inline void longdebug(const char * origin, const char * fmt, ...);
  inline void longdebug(const char * origin, const std::string & msg);


  PRINTF4_ARGS_SAFE
  inline void log(int level, const char * origin, const char * fmt, ...);
  inline void log(int level, const char * origin, const std::string & msg);

  inline bool enabled_for(int level) const
  {
    return level <= _level;
  };

private:
  int _level;

  inline void call_emit_log(int level, const char * origin, const std::string & msg);
};

/** \internal */
#define LOGGERS_H_MAKE_MSG_FROM_ARGPTR_FMT      \
  va_list ap;                                   \
  va_start(ap, fmt);                            \
  std::string msg = vfmts(fmt, ap);             \
  va_end(ap);


template<typename Derived>
inline void LoggerBase<Derived>::longdebug(const char * origin, const char * fmt, ...)
{
  if (!enabled_for(Logger::LONGDEBUG)) {
    return;
  }

  LOGGERS_H_MAKE_MSG_FROM_ARGPTR_FMT;

  call_emit_log(Logger::LONGDEBUG, origin, msg);
}

template<typename Derived>
inline void LoggerBase<Derived>::longdebug(const char * origin, const std::string & msg)
{
  if (!enabled_for(Logger::LONGDEBUG)) {
    return;
  }
  call_emit_log(Logger::LONGDEBUG, origin, msg);
}


template<typename Derived>
inline void LoggerBase<Derived>::log(int level, const char * origin, const char * fmt, ...)
{
  if (!enabled_for(level)) {
    return;
  }

  LOGGERS_H_MAKE_MSG_FROM_ARGPTR_FMT;

  call_emit_log(level, origin, msg);
}

template<typename Derived>
inline void LoggerBase<Derived>::log(int level, const char * origin, const std::string & msg)
{
  if (!enabled_for(level)) {
    return;
  }
  call_emit_log(level, origin, msg);
}

template<typename Derived>
inline void LoggerBase<Derived>::call_emit_log(int level, const char * origin, const std::string & msg)
{
  try {
    static_cast<Derived*>(this)->emit_log(level, origin, msg);
  } catch (const std::exception & e) {
    fprintf(stderr, "Warning in LoggerBase::call_emit_log(%d, \"%s\", msg): Exception caught: %s\n",
            level, origin, e.what());
  }
}



#undef LOGGERS_H_MAKE_MSG_FROM_ARGPTR_FMT




/** \brief Simple logger class which logs everything into a given FILE pointer
 *
 * The \c FILE may be any C \c FILE* pointer, in particular \c stdin or \c stderr. If it
 * is a file, it should be open and writeable.
 */
class SimpleFoutLogger : public LoggerBase<SimpleFoutLogger>
{
public:
  SimpleFoutLogger(FILE * fp_, int level = Logger::INFO)
    : LoggerBase<SimpleFoutLogger>(level), fp(fp_)
  {
  }

  inline void emit_log(int level, const char * origin, const std::string & msg)
  {
    fprintf(fp, "%s\n", ((origin&&origin[0] ? "["+std::string(origin)+"] " : std::string()) + msg).c_str());
  }

private:
  std::FILE * fp;
};




/** \brief Logger that discards all messages.
 *
 * Use this logger if you don't want to log messages.
 */
class VacuumLogger : public LoggerBase<VacuumLogger>
{
public:
  inline void emit_log(int /*level*/, const char * /*origin*/, const std::string & /*msg*/)
  {
  }
};




#endif
