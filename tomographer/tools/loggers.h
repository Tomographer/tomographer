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

#ifndef TOMOGRAPHER_TOOLS_LOGGERS_H
#define TOMOGRAPHER_TOOLS_LOGGERS_H

#include <cstdio>
#include <cstdarg>

#include <string>
#include <sstream> // std::stringstream
#include <iostream>
#include <functional> // std::function
#include <type_traits> // std::enable_if
#include <map>

#include <boost/algorithm/string.hpp> // to_upper()

#include <tomographer/tools/fmt.h>
#include <tomographer/tools/conststr.h>


/** \file loggers.h
 *
 * \brief Utilities for logging messages.
 *
 */



#ifndef TOMOGRAPHER_PARSED_BY_DOXYGEN

#define ENABLE_IF_Fn_CALLABLE_OSTREAM                                      \
  typename std::enable_if<std::is_convertible<Fn,std::function<void(std::ostream&)> >::value, void>::type

#endif


namespace Tomographer
{

/** \brief %Tomographer's \ref pageLoggers "Logging Framework"
 *
 */
namespace Logger
{

/** \brief Possible logging levels.
 *
 * Don't trust the numeric values here, they may change at any time. Just the order is
 * important.
 */
enum LogLevelCode
{
  /** \brief Error logging level
   *
   * A log message with this level signifies that a critical error has occurred which
   * prevents further processing.  The program is about to terminate.
   */
  ERROR = 0,

  /** \brief Warning logging level
   *
   * A log message with this level signifies a warning for the user. The computation may
   * continue, but most likely the user provided erroneous input, a routine may have
   * failed to reach the required precision, or there are features which will not be
   * available.
   */
  WARNING,

  /** \brief Information logging level
   *
   * Log messages with the \c INFO level are general messages which inform the user
   * about the global steps being taken. They may indicate, for example, what will be
   * computed.
   */
  INFO,

  /** \brief Debug logging level
   *
   * More verbose information, meant to debug either the input data or the
   * \c Tomographer program itself. These messages should make it easier to locate a
   * specific error, and generally give a more specific idea of what the program does.
   *
   * The \c DEBUG level is meant to still be readable on a terminal output (with decent
   * scrollback), even with multiple threads. Don't generate too many messages with this
   * level. Use the \c LONGDEBUG level to generate as many messages as you want.
   */
  DEBUG,

  /** \brief Long Debug logging level
   *
   * This level is meant to signify very specific messages, such as infomation about
   * individual iterations. The log produced with this level may be very long an no longer
   * readable on a terminal.
   *
   * Use this level for messages that are generated very often.
   */
  LONGDEBUG,


  /** \brief Highest severity possible.
   *
   * Don't use as a level. Use, rather, as value e.g. for \ref
   * DefaultLoggerTraits::StaticMinimumSeverityLevel
   * "LoggerTraits::StaticMinimumSeverityLevel".
   */
  HIGHEST_SEVERITY_LEVEL = 0,

  /** \brief Lowest severity possible.
   *
   * Don't use as a level. Use, rather, as value e.g. for \ref
   * DefaultLoggerTraits::StaticMinimumSeverityLevel
   * "LoggerTraits::StaticMinimumSeverityLevel".
   */
  LOWEST_SEVERITY_LEVEL = 0x7fffffff
};



/** \brief Object which stores a log level and can initialize from a string.
 *
 * The only valid levels are LONGDEBUG, DEBUG, INFO, WARNING and ERROR. Any attempt to
 * store any other integer value, or to assign any other string will result in an \ref
 * std::invalid_argument exception.
 *
 * The acceptable strings are "LONGDEBUG", "DEBUG", "INFO", "WARNING" and "ERROR". They
 * are parsed case-insensitive.
 *
 * This may be used as type for one of Boost's Programs Options type, for example.
 *
 * This class has also C++ i/ostream operators defined, which parse and write the level
 * name as a \ref std::string.
 */
TOMOGRAPHER_EXPORT class LogLevel
{
  int _level;
public:
  //! Construct a level using an integer level code. See \ref LogLevelCode.
  LogLevel(int level_ = INFO) { setLevel(level_); }
  //! Construct a level using a string level name
  LogLevel(const char * s) { setLevel(s); }
  //! Construct a level using a string level name
  LogLevel(const std::string& s) { setLevel(s); }

  //! Get the stored level code. See \ref LogLevelCode.
  inline int level() const { return _level; }

  //! This class is implicitly convertible to an int
  inline operator int() const { return _level; }

  //! Get the stored level name.
  inline std::string levelName() const
  {
    switch (_level) {
    case LONGDEBUG: return std::string("LONGDEBUG");
    case DEBUG:     return std::string("DEBUG");
    case INFO:      return std::string("INFO");
    case WARNING:   return std::string("WARNING");
    case ERROR:     return std::string("ERROR");
    default: return std::string("<INVALID LEVEL>");
    };
  }

  //! Set the level to the given level code. See class doc and \ref LogLevelCode.
  inline void setLevel(int level)
  {
    if (level != LONGDEBUG &&
        level != DEBUG &&
        level != INFO &&
        level != WARNING &&
        level != ERROR) {
      throw std::invalid_argument("Invalid level code: "+std::to_string(level));
    }
    _level = level;
  }

  //! Set the level to the given level name. See class doc. 
  inline void setLevel(std::string s)
  {
    // NOT const string `s`! we need our copy for to_upper():
    boost::to_upper(s);
    if (s == "LONGDEBUG") {
      _level = LONGDEBUG;
    } else if (s == "DEBUG") {
      _level = DEBUG;
    } else if (s == "INFO") {
      _level = INFO;
    } else if (s == "WARNING") {
      _level = WARNING;
    } else if (s == "ERROR") {
      _level = ERROR;
    } else {
      throw std::invalid_argument("Invalid log level: '"+s+"'");
    }
  }
};

//! C++ input stream operator for \ref LogLevel
inline std::istream & operator>>(std::istream & str, LogLevel &l)
{
  std::string s;
  str >> s;
  l.setLevel(s);
  return str;
}
//! C++ output stream operator for \ref LogLevel
inline std::ostream & operator<<(std::ostream & str, const LogLevel &l)
{
  return str << l.levelName();
}



/** \brief Helper to compare severity levels.
 *
 * Returns \c true if the given \a level is at least as severe (equal severity or higher
 * severity) as \a baselevel (for example, if <code>level == Logger::ERROR</code> and
 * <code>baselevel == Logger::INFO</code>).
 *
 * Just to make sure we don't confuse the numerical ordering of the levels, we should
 * always use this helper function to see whether a level is at least of a given
 * severity.
 */
inline bool isAtLeastOfSeverity(int level, int baselevel)
{
  return (level <= baselevel);
}

/** \brief Helper for statically determining if Level is at least as severe as BaseLevel.
 *
 * The member \a value is statically defined to \c 1 if \a Level is at least as severe
 * as \a BaseLevel, otherwise it is defined to \c 0.
 *
 * See also \ref isAtLeastOfSeverity().
 */
template<int Level, int BaseLevel>
struct StaticIsAtLeastOfSeverity {
  enum {
    value = (Level <= BaseLevel)
  };
};


/** \brief Default traits for Logger implementations.
 *
 * If you write your own logger class, you must also provide information about this class
 * such as whether it is thread-safe by specializing the traits class, e.g.:
 * \code
 *   namespace Tomographer { namespace Logger {
 *   template<>
 *   struct LoggerTraits<MyCustomLogger> : public DefaultLoggerTraits
 *   {
 *     enum {
 *       IsThreadSafe = 1,
 *       StaticMinimumSeverityLevel = LOWEST_SEVERITY_LEVEL
 *     };
 *   };
 *   } } // namespaces
 * \endcode
 *
 * It's a good idea to inherit from \ref DefaultLoggerTraits, so that if additional traits
 * are added in the future, then your code will still compile.
 */
TOMOGRAPHER_EXPORT struct DefaultLoggerTraits
{
  enum {
    /** \brief Whether a same logger instance may be called from different threads
     * simultaneously
     *
     * By default, loggers are not thread-safe.
     */
    IsThreadSafe = 0,

    /** \brief A statically-determined minimum severity logging level
     *
     * The logger will automatically discard at compile-time any message with severity
     * level strictly lesser than this level (regardless of the runtime level). Of course,
     * the runtime level of the logger will be observed, but the runtime level cannot be
     * set to a lower severity level than \a StaticMinimumSeverityLevel.
     *
     * If set to \ref LOWEST_SEVERITY_LEVEL, no messages are discarded at compile-time,
     * and all messages are processed at runtime.
     *
     * If set to \a -1, all messages are discarded (cf. \ref VacuumLogger).
     */
    StaticMinimumSeverityLevel = LOWEST_SEVERITY_LEVEL,

    /** \brief Whether the logger class can provide its own run-time level
     *
     * If set to 0 (the default), then the base logger stores the level given in the
     * constructor and uses that as the logger's level. It also provides a level() method
     * which returns the current level. Subclasses may set the level by calling the
     * protected method \ref LoggerBase::setLogLevel().
     *
     * If set to 1, then any time the base logging engine needs to know the current
     * logger's level, it will call the method \a level() on the derived logger object to
     * obtain the current level.
     */
    HasOwnGetLevel = 0,

    /** \brief Whether the logger has a filter based on the given 'origin'
     *
     * This flag may be set to either \c 0 (no filtering, the default) or \c 1
     * (filtering).
     *
     * If filtering is activated, then before formatting and emitting messages the logging
     * engine will test the logger object's <code>filterByOrigin(int level, const char *
     * origin)</code>. The message will only be emitted if this function returns \c
     * true. Note that messages may have already been discarded if their severity was less
     * important than the runtime level of the logger: those messages don't even reach
     * this filtering function call.
     */
    HasFilterByOrigin = 0
  };
};

/** \brief Traits template struct to be specialized for specific Logger implementations.
 *
 * It is mandatory to specialize the logger traits for your class. This is simply to ease
 * debugging: this way you know your traits are being seen (e.g. declared in the right
 * namespace, etc.).
 *
 * Your specialized traits should inherit from \ref DefaultLoggerTraits. That way, if
 * future traits are added you get the defaults for free and your code still compiles.
 */
template<typename LoggerType>
struct LoggerTraits
{
  // by default, contains nothing and will produce errors if used unspecialized.
};



namespace tomo_internal {
/** A small helper that stores a level if it's supposed to store a level, or that does
 * nothing if the derived logger object anyway provides its own level() method.
 */
template<bool hasOwnGetLevel>
class LoggerRuntimeLevel {
public:
  LoggerRuntimeLevel(int level)
    : _level(level)
  {
  }

  inline int level() const
  {
    return _level;
  }

protected:
  inline void setLogLevel(int level)
  {
    _level = level;
  }
  
private:
  int _level;
};
//
// Specialization for those classes which provide their own level() method. Do nothing.
//
template<>
class LoggerRuntimeLevel<true> {
public:
  LoggerRuntimeLevel(int /*level*/)
  {
  }

  inline void setLogLevel(int) {
    tomographer_assert(0 && "Call to LoggerRuntimeLevel::setLogLevel() for Logger which defines HasOwnGetLevel=1!");
  }
};
}


/** \brief Base logger class.
 *
 * Please read \ref pageLoggers to understand Tomographer's logging framework.
 *
 * This class serves as base class for logger implementations. It provides storing a
 * current given level, emitting the log only if the level is reached, etc. Don't
 * instantiate this class directly.
 *
 * See also \ref FileLogger.
 *
 * A logger always has a level set at run-time, which can always be retrieved by calling
 * \c level(). By default, this class stores the level given to the constructor. If you
 * want your derived class to store its own level, or simply to relay log messages to a
 * further logger and inherit its level, then you should declare \a HasOwnGetLevel=1 in
 * your logger traits (see \ref LoggerTraits), and provide your own \a level() method,
 * which should be \c const and return an \c int.
 *
 * Also, by default there is no public \c setLevel() method, in case your logger's level
 * is statically fixed or otherwise can't be changed, or if you need a thread-safe logger.
 * Derived classes may set the logger's level (again, only if \a HasOwnGetLevel=0) by
 * calling the protected \ref setLogLevel(int). You may of course then also expose a
 * public function such as \c setLevel() which calls setLogLevel(), if you want (see, for
 * example, \ref FileLogger).
 *
 */
template<typename Derived>
TOMOGRAPHER_EXPORT class LoggerBase
#ifndef TOMOGRAPHER_PARSED_BY_DOXYGEN
  : public tomo_internal::LoggerRuntimeLevel<LoggerTraits<Derived>::HasOwnGetLevel>
#endif
{
public:
  /** \brief Shortcuts to properties defined in the traits class
   */
  enum {
    //! See LoggerTraits<Derived> and DefaultLoggerTraits
    IsThreadSafe = LoggerTraits<Derived>::IsThreadSafe,
    //! See LoggerTraits<Derived> and DefaultLoggerTraits
    StaticMinimumSeverityLevel = LoggerTraits<Derived>::StaticMinimumSeverityLevel,
    //! See LoggerTraits<Derived> and DefaultLoggerTraits
    HasOwnGetLevel = LoggerTraits<Derived>::HasOwnGetLevel,
    //! See LoggerTraits<Derived> and DefaultLoggerTraits
    HasFilterByOrigin = LoggerTraits<Derived>::HasFilterByOrigin
  };

  /** \brief Construct the base logger object
   *
   * The logging level is set to \a level, by default \ref Logger::INFO. Any messages with
   * lesser severity will be automatically discarded.
   *
   * Note that if the derived logger class provides its own \c logger() method, and
   * declares it with \a LoggerTraits<Derived>::HasOwnGetLevel, then the level provided
   * here is discarded and ignored.
   */
  LoggerBase(int level_ = INFO)
    : tomo_internal::LoggerRuntimeLevel<HasOwnGetLevel>(level_)
  {
  }

  /** \brief emit an error message
   *
   * Call this method to report an error.
   *
   * See \ref debug(const char *, const char *, ...) for information about the function
   * arguments.
   */
  PRINTF3_ARGS_SAFE
  inline void error(const char * origin, const char * fmt, ...);
  /** \brief emit an error message
   *
   * Call this method to report an error.
   *
   * See \ref debug(const char *, const std::string&) for information about the function
   * arguments.
   */
  inline void error(const char * origin, const std::string & msg);
  /** \brief emit an error message
   *
   * Call this method to report an error.
   *
   * See \ref debug(const char *, Fn) for information about the function arguments.
   */
  template<typename Fn>
  inline ENABLE_IF_Fn_CALLABLE_OSTREAM
  error(const char * origin, Fn f);

  /** \brief emit a warning message
   *
   * Call this method to report a warning.
   *
   * See \ref debug(const char *, const char *, ...) for information about the function
   * arguments.
   */
  PRINTF3_ARGS_SAFE
  inline void warning(const char * origin, const char * fmt, ...);
  /** \brief emit a warning message
   *
   * Call this method to report a warning.
   *
   * See \ref debug(const char*, const std::string&) for information about the function
   * arguments.
   */
  inline void warning(const char * origin, const std::string & msg);
  /** \brief emit a warning message
   *
   * Call this method to report a warning.
   *
   * See \ref debug(const char *, Fn) for information about the function arguments.
   */
  template<typename Fn>
  inline ENABLE_IF_Fn_CALLABLE_OSTREAM
  warning(const char * origin, Fn f);

  /** \brief emit an information/notice message
   *
   * Call this method to report an information/notice message.
   *
   * See \ref debug(const char *, const char *, ...) for information about the function
   * arguments.
   */
  PRINTF3_ARGS_SAFE
  inline void info(const char * origin, const char * fmt, ...);
  /** \brief emit an information/notice message
   *
   * Call this method to report an information/notice message.
   *
   * See \ref debug(const char*, const std::string&) for information about the function
   * arguments.
   */
  inline void info(const char * origin, const std::string & msg);
  /** \brief emit an information/notice message
   *
   * Call this method to report an information/notice message.
   *
   * See \ref debug(const char *, Fn) for information about the function arguments.
   */
  template<typename Fn>
  inline ENABLE_IF_Fn_CALLABLE_OSTREAM
  info(const char * origin, Fn f);

  /** \brief emit an debug message
   *
   * Call this method to report a debugging message with this logger. This will relay the
   * the call to the underlying logger implementation (subclass) if logging at the level
   * \c Logger::DEBUG is enabled.
   *
   * This method overload allows you to format messages in a \c printf -like fashion. The
   * final message will be formatted using an actual function of the \c sprintf() family.
   *
   * See \ref pageLoggers for more information about Tomographer's logging framework.
   *
   * \param origin A string which identifies the place in code from where the debugging
   * message is originating. This is useful to trace back error messages. Conventionally,
   * specify the name of the function with its class (e.g. \c "MyClass::my_method()").
   * \param fmt A \c printf format string
   * \param ... additional arguments for the \c printf formatting string.
   */
  PRINTF3_ARGS_SAFE
  inline void debug(const char * origin, const char * fmt, ...);

  /** \brief emit an debug message
   *
   * Call this method to report a debugging message with this logger. This will relay the
   * the call to the underlying logger implementation (subclass) if logging at the level
   * \c Logger::DEBUG is enabled.
   *
   * This method overload assumes that you have already suitably formatted your message,
   * and will directly provide the given \c msg to the underlying logger implementation.
   *
   * See \ref pageLoggers for more information about Tomographer's logging framework.
   *
   * \param origin A string which identifies the place in code from where the debugging
   * message is originating. This is useful to trace back error messages. Conventionally,
   * specify the name of the function with its class (e.g. \c "MyClass::my_method()").
   * \param msg the log message to produce.
   */
  inline void debug(const char * origin, const std::string & msg);
  /** \brief emit an debug message
   *
   * Call this method to report a debugging message with this logger. This will relay the
   * the call to the underlying logger implementation (subclass) if logging at the level
   * \c Logger::DEBUG is enabled.
   *
   * See \ref pageLoggers for more information about Tomographer's logging framework.
   *
   * This method overload allows you to write to the logger using the C++ stream syntax. 
   * This function takes as argument a callable \a f, which will be called with a
   * <code>std::ostream&</code> parameter to which it can write to. This is especially
   * handy combined with C++11 lambdas, for example:
   * \code
   *   logger.debug("origin_fn()", [value](std::ostream& stream) {
   *                  stream << "The value is: " << value;
   *                });
   * \endcode
   *
   * \param origin A string which identifies the place in code from where the debugging
   * message is originating. This is useful to trace back error messages. Conventionally,
   * specify the name of the function with its class (e.g. \c "MyClass::my_method()"). 
   * \param f a callable which accepts a single <code>std::ostream&</code> argument, to
   * which it should write its log message.
   *
   * Internally, the log message is first written to a <code>std::ostringstream</code>
   * before transmitting it to the underlying logger implementation. (But don't rely on
   * this!)
   */
  template<typename Fn>
  inline ENABLE_IF_Fn_CALLABLE_OSTREAM
  debug(const char * origin, Fn f);

  /** \brief emit a very verbose debugging message
   *
   * Call this method to report debugging messages which are emitted very often.
   *
   * See \ref debug(const char *, const char *, ...) for information about the function
   * arguments.
   */
  PRINTF3_ARGS_SAFE
  inline void longdebug(const char * origin, const char * fmt, ...);
  /** \brief emit a very verbose debugging message
   *
   * Call this method to report debugging messages which are emitted very often.
   *
   * See \ref debug(const char*, const std::string&) for information about the function
   * arguments.
   */
  inline void longdebug(const char * origin, const std::string & msg);
  /** \brief emit a very verbose debugging message
   *
   * Call this method to report debugging messages which are emitted very often.
   *
   * See \ref debug(const char *, Fn) for information about the function arguments.
   */
  template<typename Fn>
  inline ENABLE_IF_Fn_CALLABLE_OSTREAM
  longdebug(const char * origin, Fn f);


  /** \brief emit a log message at the given log level.
   *
   * You should never need to call this method, and I'm not sure it's a good design
   * choice. (Mind that messages logged here may not be statically removed if a static
   * minimum level is set.)
   *
   * Otherwise, see \ref debug(const char *, const char *, ...) for information about the
   * function arguments.
   */
  PRINTF4_ARGS_SAFE
  inline void log(int level, const char * origin, const char * fmt, ...);
  /** \brief emit a log message at the given log level.
   *
   * You should never need to call this method, and I'm not sure it's a good design
   * choice. (Mind that messages logged here may not be statically removed if a static
   * minimum level is set.)
   *
   * Otherwise, see \ref debug(const char*, const std::string&) for information about the
   * function arguments.
   */
  inline void log(int level, const char * origin, const std::string & msg);
  /** \brief emit a log message at the given log level.
   *
   * You should never need to call this method, and I'm not sure it's a good design
   * choice. (Mind that messages logged here may not be statically removed if a static
   * minimum level is set.)
   *
   * Otherwise, see \ref debug(const char *, Fn) for information about the function
   * arguments.
   */
  template<typename Fn>
  inline ENABLE_IF_Fn_CALLABLE_OSTREAM
  log(int level, const char * origin, Fn f);



  /** \brief emit a log message at the given log level.
   *
   * The log level is given statically. You shouldn't need to call this method directly,
   * it's probably more readable to use \ref debug(), \ref warning(), etc.
   *
   * See \ref debug(const char *, const char *, ...) for information about the
   * function arguments.
   */
  template<int Level>
  PRINTF3_ARGS_SAFE
  inline void log(const char * origin, const char * fmt, ...);
  /** \brief emit a log message at the given log level.
   *
   * The log level is given statically. You shouldn't need to call this method directly,
   * it's probably more readable to use \ref debug(), \ref warning(), etc.
   *
   * This function is a convenience method which accepts an argument list pointer instead
   * of \c ... . The format string and argument list are expected to be the same as for
   * \ref log(const char * origin, const char * fmt, ...) .
   */
  template<int Level>
  inline void log(const char * origin, const char * fmt, va_list ap);
  /** \brief emit a log message at the given log level.
   *
   * The log level is given statically. You shouldn't need to call this method directly,
   * it's probably more readable to use \ref debug(), \ref warning(), etc.
   *
   * Otherwise, see \ref debug(const char*, const std::string&) for information about the
   * function arguments.
   */
  template<int Level>
  inline void log(const char * origin, const std::string & msg);
  /** \brief emit a log message at the given log level.
   *
   * The log level is given statically. You shouldn't need to call this method directly,
   * it's probably more readable to use \ref debug(), \ref warning(), etc.
   *
   * Otherwise, see \ref debug(const char *, Fn) for information about the function
   * arguments.
   */
  template<int Level, typename Fn>
  inline ENABLE_IF_Fn_CALLABLE_OSTREAM
  log(const char * origin, Fn f);


  /** \brief Check whether the logger is statically disabled for some levels
   *
   * \tparam Level the log level to test for.
   *
   * \return \c false if the logger was explicitly disabled for the level \a Level via
   * LoggerTraits<Derived>::StaticMinimumSeverityLevel, otherwise returns \c true.
   *
   * If \c true was returned, this does not yet mean that a log message at the given level
   * will necessarily be produced; in this case, rather, the message is not expliclty
   * discarded and the logger's level set at run-time will be tested (see \ref
   * enabledFor()).
   */
  static inline bool staticallyEnabledFor(int level)
  {
    return ( isAtLeastOfSeverity(level, StaticMinimumSeverityLevel) );
  }

  /** \brief Static version of staticallyEnabledFor()
   *
   */
  template<int Level>
  static inline bool staticallyEnabledFor()
  {
    return ( StaticIsAtLeastOfSeverity<
	          Level,
	          StaticMinimumSeverityLevel
	          >::value ) ;
  }

  /** \brief Check whether messages at the given log level are enabled
   *
   * This function returns \c true if a message at the given level will be emitted, i.e. 
   * transmitted to the underlying logger implementation.
   *
   * The condition for a log message to be emitted is that the message's log level not be
   * explicitly disabled by LoggerTraits::StaticMinimumSeverityLevel, and that the
   * message's log level be at least as important as the current level set for this logger
   * (see \ref level()).
   */
  inline bool enabledFor(int level_) const
  {
    return derived()->staticallyEnabledFor(level_) &&
      isAtLeastOfSeverity(level_, getLevel());
  };

#ifdef TOMOGRAPHER_PARSED_BY_DOXYGEN
  // this method is actually implemented by the base class LoggerRuntimeLevel, and is only
  // exposed in the case where the logger doesn't define its own method. This is important
  // to avoid "ambiguous calls to `level()`".

  /** \brief Get the log level set for this logger
   *
   * This is the log level given to the constructor at run-time, or set with the protected
   * \c setLogLevel() method.
   *
   * This method is provided for if your own derived class doesn't provide it already,
   * i.e. if the logger traits of your derived class declares \a HasOwnGetLevel to \c 0 or
   * doesn't declare it. If you call this function with \a HasOwnGetLevel set to \c 1,
   * then the derived class' method will be called. So if you override this method in the
   * derived class with \c HasOwnGetLevel=1, then don't call the base implementation!
   */
  inline int level() const
  {
  }
#endif
  

protected:

  // version in case we store our own level
  TOMOGRAPHER_ENABLED_IF(!HasOwnGetLevel)
  inline int getLevel() const
  {
    // use base class' implementation
    return tomo_internal::LoggerRuntimeLevel<HasOwnGetLevel>::level();
  }
  // version in case we delegate to derived class' level()
  TOMOGRAPHER_ENABLED_IF(HasOwnGetLevel)
  inline int getLevel() const
  {
    return derived()->level();
  }


  inline Derived * derived()
  {
    return static_cast<Derived*>(this);
  }
  inline const Derived * derived() const
  {
    return static_cast<const Derived*>(this);
  }

#ifdef TOMOGRAPHER_PARSED_BY_DOXYGEN
  // this method is actually implemented by the base class LoggerRuntimeLevel, and is only
  // exposed in the case where the logger can actually set a new level.

  /** \brief Store a new run-time log level
   *
   * See also \ref level().
   *
   * The base class does not provide a public <code>setLevel()</code> function, in case
   * your logger does not support setting any level, or in case you need your logger to be
   * completely thread-safe, or for any other reason.
   *
   * Subclasses may, however, change the log level by explicily calling this method. This
   * method however should ONLY be used if your derived class doesn't define its own \c
   * level() method and thus only if the logger traits \a HasOwnGetLogger is \c
   * false. (The method is not exposed otherwise.)
   */
  inline void setLogLevel(int level)
  {
  }
#endif

private:
};


namespace tomo_internal {
  /** Helper to invoke the logger's filter_by_origin() method, if applicable.
   */
  template<typename Derived, bool derivedHasFilterByOrigin>
  struct LoggerBaseHelperFilterByOrigin {
    static inline bool test_logger_filter(LoggerBase<Derived> * /*loggerbase*/, int /*level*/,
					  const char * /*origin*/)
    {
      // do nothing by default
      return true;
    }
  };
  //
  // specialization for loggers with filtering by origin
  //
  template<typename Derived>
  struct LoggerBaseHelperFilterByOrigin<Derived, true> {
    static inline bool test_logger_filter(LoggerBase<Derived> * loggerbase, int level,
					  const char * origin)
    {
      return static_cast<const Derived*>(loggerbase)->filterByOrigin(level, origin);
    }
  };
  /**
   * Helper to decide whether to emit log entry or not for a given log level. This is
   * separate from \c LoggerBase really only because otherwise it would have needed to be
   * a public member to be accessible from \c LoggerBaseHelperStatic.
   */
  template<typename Derived>
  struct LoggerBaseHelperDynamic {
    static inline void call_emit_log(LoggerBase<Derived> * loggerbase, int level, const char * origin,
                                     const std::string & msg)
    {
      //      try {
        //printf("Calling emit_log(%d,\"%s\",\"%s\") on object %p\n", level, origin, msg.c_str(), loggerbase);
      static_cast<Derived*>(loggerbase)->emitLog(level, origin, msg);
      // } catch (const std::exception & e) {
      //   std::fprintf(stderr,
      //   	     "Warning in LoggerBaseHelperDynamic::call_emit_log(%d, \"%s\", \"%s\"):"
      //   	     " Exception caught: %s\n",
      //   	     level, origin, msg.c_str(), e.what());
      // }
    }
    static inline bool test_should_emit(LoggerBase<Derived> * loggerbase, int level, const char * origin)
    {
      if ( ! static_cast<const Derived*>(loggerbase)->enabledFor(level) ) {
	return false;
      }
      if ( ! LoggerBaseHelperFilterByOrigin<Derived, LoggerTraits<Derived>::HasFilterByOrigin>
	   ::test_logger_filter(loggerbase, level, origin) ) {
	return false;
      }
      return true;
    }

    static inline void test_and_call_emit_log(LoggerBase<Derived> * loggerbase, int level, const char * origin,
                                              const std::string & msg)
    {
      if ( ! test_should_emit(loggerbase, level, origin) ) {
        return;
      }

      call_emit_log(loggerbase, level, origin, msg);
    }
    static inline void test_and_call_emit_log(LoggerBase<Derived> * loggerbase, int level, const char * origin,
                                              const char * fmt, va_list ap)
    {
      if ( ! test_should_emit(loggerbase, level, origin) ) {
        return;
      }

      //      try {
      const std::string msg = Tools::vfmts(fmt, ap);
      call_emit_log(loggerbase, level, origin, msg);
      // } catch (const std::exception & e) {
      //   std::fprintf(stderr,
      //   	     "Warning in LoggerBase::test_and_call_emit_log(%d, \"%s\", \"%s\", ...):"
      //   	     " Exception caught: %s\n",
      //   	     level, origin, fmt, e.what());
      // }
    }
    template<typename Fn>
    static inline
    ENABLE_IF_Fn_CALLABLE_OSTREAM
    test_and_call_emit_log(LoggerBase<Derived> * loggerbase, int level, const char * origin, Fn f)
    {
      if ( ! test_should_emit(loggerbase, level, origin) ) {
        return;
      }

      //      try {
      std::ostringstream sstr;
      f(sstr);
      call_emit_log(loggerbase, level, origin, sstr.str());
      // } catch (const std::exception & e) {
      //   std::fprintf(stderr, "Warning in LoggerBase::test_and_call_emit_log(%d, \"%s\", f(ostream)):"
      //   	     " Exception caught: %s\n",
      //   	     level, origin, e.what());
      // }
    }
  };

  /** \brief helper which delegates the call to the dynamic version \ref
   * LoggerBaseHelperDynamic.
   *
   * If the log message should be statically discarded (as given by the template parameter
   * \a isStaticallyDiscarded), then nothing is done.
   */
  template<typename Derived, int Level, bool isStaticallyDiscarded = false>
  struct LoggerBaseHelperStatic2 {
    template<typename... Args>
    static inline void test_and_call_emit_log(LoggerBase<Derived> * loggerbase, const char * origin,
                                              Args... args)
    {
      LoggerBaseHelperDynamic<Derived>::test_and_call_emit_log(loggerbase, Level, origin, args...);
    }
  };
  // specialization for calls where the message should be statically discarded
  template<typename Derived, int Level>
  struct LoggerBaseHelperStatic2<Derived, Level, true> {
    template<typename... Args>
    static inline void test_and_call_emit_log(LoggerBase<Derived> *, const char *, Args...)
    {
      // discard logging message.
    }
  };

  /** \brief Helper to statically decide whether to emit log entry or not for a given log
   * level.
   */
  template<typename Derived, int Level>
  struct LoggerBaseHelperStatic {
    template<typename... Args>
    static inline void test_and_call_emit_log(LoggerBase<Derived> * loggerbase, const char * origin,
                                              Args... args)
    {
      // call the default or specialized version of our second helper, which will either
      // relay the call the dynamic version or discard the message.
      //std::fprintf(stderr,
      //        "LoggerBaseHelperStatic<Derived,Level=%d>::test_and_call_emit_log(...). StaticMinimumSeverityLevel=%d\n",
      //        (int)Level, (int)(LoggerTraits<Derived>::StaticMinimumSeverityLevel));
      LoggerBaseHelperStatic2<
	Derived, Level,
	! StaticIsAtLeastOfSeverity<Level, LoggerTraits<Derived>::StaticMinimumSeverityLevel>::value
	>::test_and_call_emit_log(
	    loggerbase, origin, args...
	    );
    }
  };
  

} // namespace tomo_internal


template<typename Derived>
inline void LoggerBase<Derived>::error(const char * origin, const char * fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  derived()->template log<ERROR>(origin, fmt, ap);
  va_end(ap);
}

template<typename Derived>
inline void LoggerBase<Derived>::error(const char * origin, const std::string & msg)
{
  derived()->template log<ERROR>(origin, msg);
}

template<typename Derived>
template<typename Fn>
inline ENABLE_IF_Fn_CALLABLE_OSTREAM
LoggerBase<Derived>::error(const char * origin, Fn f)
{
  derived()->template log<ERROR>(origin, f);
}


template<typename Derived>
inline void LoggerBase<Derived>::warning(const char * origin, const char * fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  derived()->template log<WARNING>(origin, fmt, ap);
  va_end(ap);
}

template<typename Derived>
inline void LoggerBase<Derived>::warning(const char * origin, const std::string & msg)
{
  derived()->template log<WARNING>(origin, msg);
}

template<typename Derived>
template<typename Fn>
inline ENABLE_IF_Fn_CALLABLE_OSTREAM
LoggerBase<Derived>::warning(const char * origin, Fn f)
{
  derived()->template log<WARNING>(origin, f);
}

template<typename Derived>
inline void LoggerBase<Derived>::info(const char * origin, const char * fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  derived()->template log<INFO>(origin, fmt, ap);
  va_end(ap);
}

template<typename Derived>
inline void LoggerBase<Derived>::info(const char * origin, const std::string & msg)
{
  derived()->template log<INFO>(origin, msg);
}

template<typename Derived>
template<typename Fn>
inline ENABLE_IF_Fn_CALLABLE_OSTREAM
LoggerBase<Derived>::info(const char * origin, Fn f)
{
  derived()->template log<INFO>(origin, f);
}

template<typename Derived>
inline void LoggerBase<Derived>::debug(const char * origin, const char * fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  derived()->template log<DEBUG>(origin, fmt, ap);
  va_end(ap);
}

template<typename Derived>
inline void LoggerBase<Derived>::debug(const char * origin, const std::string & msg)
{
  derived()->template log<DEBUG>(origin, msg);
}

template<typename Derived>
template<typename Fn>
inline ENABLE_IF_Fn_CALLABLE_OSTREAM
LoggerBase<Derived>::debug(const char * origin, Fn f)
{
  derived()->template log<DEBUG>(origin, f);
}

template<typename Derived>
inline void LoggerBase<Derived>::longdebug(const char * origin, const char * fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  derived()->template log<LONGDEBUG>(origin, fmt, ap);
  va_end(ap);
}

template<typename Derived>
inline void LoggerBase<Derived>::longdebug(const char * origin, const std::string & msg)
{
  derived()->template log<LONGDEBUG>(origin, msg);
}

template<typename Derived>
template<typename Fn>
inline ENABLE_IF_Fn_CALLABLE_OSTREAM
LoggerBase<Derived>::longdebug(const char * origin, Fn f)
{
  derived()->template log<LONGDEBUG>(origin, f);
}


template<typename Derived>
inline void LoggerBase<Derived>::log(int level, const char * origin, const char * fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  tomo_internal::LoggerBaseHelperDynamic<Derived>::test_and_call_emit_log(this, level, origin, fmt, ap);
  va_end(ap);
}

template<typename Derived>
inline void LoggerBase<Derived>::log(int level, const char * origin, const std::string & msg)
{
  tomo_internal::LoggerBaseHelperDynamic<Derived>::test_and_call_emit_log(this, level, origin, msg);
}

template<typename Derived>
template<typename Fn>
inline ENABLE_IF_Fn_CALLABLE_OSTREAM
LoggerBase<Derived>::log(int level, const char * origin, Fn f)
{
  tomo_internal::LoggerBaseHelperDynamic<Derived>::test_and_call_emit_log(this, level, origin, f);
}


template<typename Derived>
template<int Level>
inline void LoggerBase<Derived>::log(const char * origin, const char * fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  tomo_internal::LoggerBaseHelperStatic<Derived, Level>::test_and_call_emit_log(this, origin, fmt, ap);
  va_end(ap);
}

template<typename Derived>
template<int Level>
inline void LoggerBase<Derived>::log(const char * origin, const char * fmt, va_list ap)
{
  tomo_internal::LoggerBaseHelperStatic<Derived, Level>::test_and_call_emit_log(this, origin, fmt, ap);
}

template<typename Derived>
template<int Level>
inline void LoggerBase<Derived>::log(const char * origin, const std::string & msg)
{
  tomo_internal::LoggerBaseHelperStatic<Derived, Level>::test_and_call_emit_log(this, origin, msg);
}

template<typename Derived>
template<int Level, typename Fn>
inline ENABLE_IF_Fn_CALLABLE_OSTREAM
LoggerBase<Derived>::log(const char * origin, Fn f)
{
  tomo_internal::LoggerBaseHelperStatic<Derived, Level>::test_and_call_emit_log(this, origin, f);
}




class FileLogger;
/** \brief Specialized Traits for \ref FileLogger -- see \ref LoggerTraits
 *
 * fprintf is actually thread-safe, that's good :)
 */
template<>
struct LoggerTraits<FileLogger> : public DefaultLoggerTraits
{
  //! Overridden values. see \ref DefaultLoggerTraits.
  enum {
    IsThreadSafe = 1 //!< This logger is indeed thread-safe.
  };
};

/** \brief Simple logger class which logs everything into a given FILE pointer
 *
 * The \c FILE may be any C \c FILE* pointer, in particular \c stdin or \c stderr. If it
 * is a file, it should be open and writeable.
 *
 * You may set the given log level with \ref setLevel().
 *
 * \note This class is thread-safe AS LONG AS you DO NOT CHANGE the target \c fp file
 *       pointer AND YOU DO NOT CHANGE the log level with \ref setLevel()
 */
TOMOGRAPHER_EXPORT class FileLogger : public LoggerBase<FileLogger>
{
public:
  FileLogger(FILE * fp_, int level = INFO, bool display_origin_ = true)
    : LoggerBase<FileLogger>(level), fp(fp_), display_origin(display_origin_)
  {
  }

  /**
   * \warning This method is not thread-safe!
   */
  inline void setFp(FILE * fp_)
  {
    fp = fp_;
  }

  /**
   * \warning This method is not thread-safe!
   */
  inline void setLevel(int level)
  {
    setLogLevel(level);
  }

  /**
   * \warning This method is not thread-safe!
   */
  inline void setDisplayOrigin(bool display_origin_)
  {
    display_origin = display_origin_;
  }

  inline void emitLog(int level, const char * origin, const std::string & msg)
  {
    static const std::string level_prefixes[] = {
      std::string("\n\n*** ERROR -- "),
      std::string("\n*** Warning: ")
    };

    std::string finalmsg = (
        ( (level >= 0 && level < (int)std::extent<decltype(level_prefixes)>::value)
          ? level_prefixes[level] : std::string() )
        + ((display_origin && origin && origin[0]) ? "["+std::string(origin)+"] " : std::string())
        + msg
        );

    // display the log message
    std::fprintf(fp, "%s\n", finalmsg.c_str());

    if (isAtLeastOfSeverity(level, WARNING)) {
      // force output also on stderr for warnings and errors if we are being redirected to a
      // file, or at least flush the buffer.
      std::fflush(fp);
      if (fp != stdout && fp != stderr) {
        std::fprintf(stderr, "%s\n", finalmsg.c_str());
      }
    }
  }

private:
  std::FILE * fp;
  bool display_origin;
};





class VacuumLogger;
/** \brief Specialied Traits for \ref VacuumLogger. See \ref LoggerTraits<BaseLogger>.
 *
 * We'll assume that doing nothing is actually thread safe.
 */
template<>
struct LoggerTraits<VacuumLogger> : public DefaultLoggerTraits
{
  //! Overridden values. see \ref DefaultLoggerTraits.
  enum {
    IsThreadSafe = 1, //!< Logger is thread-safe
    StaticMinimumSeverityLevel = -1 //!< Logger discards all messages regardless of severity
  };
};

/** \brief Logger that discards all messages.
 *
 * Use this logger if you don't want to log messages.
 */
TOMOGRAPHER_EXPORT class VacuumLogger : public LoggerBase<VacuumLogger>
{
public:
  inline void emitLog(int /*level*/, const char * /*origin*/, const std::string & /*msg*/)
  {
  }

};

/** \brief A VacuumLogger instance which can be used as an LVALUE
 *
 * This instance may be used as an LVALUE, for example in default function arguments. A
 * VacuumLogger doesn't do anything, anyway.
 */
static VacuumLogger vacuum_logger;




class BufferLogger;
/** \brief Specialized Traits for BufferLogger -- see \ref LoggerTraits
 *
 * BufferLogger is not thread-safe.
 */
template<>
struct LoggerTraits<BufferLogger> : public DefaultLoggerTraits
{
  //! Overridden values. see \ref DefaultLoggerTraits.
  enum {
    IsThreadSafe = 0 //!< Logger is not thread-safe
  };
};

/** \brief Log messages into an internal memory buffer
 *
 * Logs messages into an internal string buffer. The contents of the buffer may be
 * retrieved with \ref getContents().
 */
TOMOGRAPHER_EXPORT class BufferLogger : public LoggerBase<BufferLogger>
{
  std::ostringstream buffer;
public:
  BufferLogger(int level)
    : LoggerBase<BufferLogger>(level)
  {
  }

  inline void emitLog(int /*level*/, const char * origin, const std::string& msg)
  {
    buffer << (origin&&origin[0] ? "["+std::string(origin)+"] " : std::string())
           << msg.c_str() << "\n";
  }

  /** \brief Changes the runtime log level to a new value.
   *
   * New messages will be emitted according to the new log level \a level.
   */
  inline void setLevel(int level)
  {
    setLogLevel(level);
  }

  /** \brief Clears the internal memory buffer.
   *
   * Clears all messages logged so far. A future call to \ref getContents() will only
   * return the messages logged after calling this function.
   */
  inline void clear()
  {
    buffer.clear();
    buffer.str(std::string());
  }

  /** \brief get the contents of the internal buffer
   *
   * This returns a string containing all the messages that have been logged so far.
   */
  inline std::string getContents() const
  {
    return buffer.str();
  }
};




template<typename, int> class MinimumSeverityLogger;
/** \brief Specialized Traits for \ref MinimumSeverityLogger -- see \ref LoggerTraits
 */
template<typename BaseLogger, int Level>
struct LoggerTraits<MinimumSeverityLogger<BaseLogger,Level> > : public LoggerTraits<BaseLogger>
{
  //! Overridden values. see \ref DefaultLoggerTraits.
  enum {

    //! Logger will discard all messages less important than the given level
    StaticMinimumSeverityLevel = Level,

    //! Logger will delegate calls for current level() to base logger
    HasOwnGetLevel = 1

    // Note: filter by origin flag is inherited from base logger.
  };
};

/** \brief Logger which statically discards any messages less important than a fixed
 * severity
 *
 * This logger interfaces another logger of type \a BaseLogger, but will only log messages
 * that are at least of severity \a Level. Other messages are discarded at compile-time,
 * hopefully speeding up your code, also.
 *
 * If \a BaseLogger is thread-safe, then this logger is also thread-safe.
 */
template<typename BaseLogger, int Level>
TOMOGRAPHER_EXPORT class MinimumSeverityLogger : public LoggerBase<MinimumSeverityLogger<BaseLogger,Level> >
{
  //! Keep reference to the base logger, to which we relay calls
  BaseLogger & baselogger;
public:
  /** \brief Constructor from a base logger
   *
   * You must provide a reference to an instance of a logger, to which we will relay
   * relevant calls.
   */
  MinimumSeverityLogger(BaseLogger & baselogger_)
    : LoggerBase<MinimumSeverityLogger<BaseLogger,Level> >(), baselogger(baselogger_)
  {
  }

  //! Emit a log by relaying to the base logger
  inline void emitLog(int level, const char * origin, const std::string& msg)
  {
    baselogger.emitLog(level, origin, msg);
  }

  //! Get the base logger's set level.
  inline int level() const
  {
    return baselogger.level();
  }

  //! If relevant for the base logger, filter the messages by origin from the base logger.
  template<bool dummy = true>
  inline std::enable_if<dummy && LoggerTraits<BaseLogger>::HasFilterByOrigin, bool>
  filterByOrigin(int level, const char * origin)
  {
    return baselogger.filterByOrigin(level, origin);
  }
};


namespace tomo_internal {

/** \brief Determine length of overlap of two strings.
 *
 * Returns the largest integer \a k such that the substrings \c s[0:k] and \c pattern[0:k]
 * are equal.
 */
inline std::size_t matched_length(const std::string & s, const std::string & pattern)
{
  std::size_t k = 0;
  while (s[k] == pattern[k]) {
    ++k;
  }
  return k;
}

} // namespace tomo_internal

template<typename BaseLogger> class OriginFilteredLogger;
/** \brief Specialized Logger Traits for \ref OriginFilteredLogger -- see \ref
 *         LoggerTraits
 */
template<typename BaseLogger>
struct LoggerTraits<OriginFilteredLogger<BaseLogger> > : public LoggerTraits<BaseLogger>
{
  //! Overridden values. see \ref DefaultLoggerTraits.
  enum {
    HasOwnGetLevel = 1, //!< This logger delegates level() to base logger
    HasFilterByOrigin = 1 //!< We have customized origin filtering
  };
};

/** \brief A logger which filters entries according to where they originated from.
 *
 * This logger implementation logs messages at specific severity levels depending on the
 * \a origin of the messages.
 *
 * Use \ref setDomainLevel() to set specific log levels to particular origin
 * patterns. Each pattern given to \ref setDomainLevel() is a string: if the origin of a
 * particular messages <em>starts with this string</em>, then the log level is applied. If
 * no rules apply, the level of the base logger (the one we act as proxy to) is used.
 *
 * This logger acts as a proxy for another logger implementation. For example, you can do:
 * \code
 *    Logger::FileLogger flog1(stdout, Logger::INFO);
 *    Logger::OriginFilteredLogger<Logger::FileLogger> logger(flog1);
 *    // set a specific log level rule
 *    logger.setDomainLevel("my_origin_class", Logger::DEBUG);
 *    // this message will be displayed to stdout:
 *    logger.debug("my_origin_class::mymethod()", "this is a message. Hello World!");
 * \endcode
 */
template<typename BaseLogger>
TOMOGRAPHER_EXPORT class OriginFilteredLogger
  : public Tomographer::Logger::LoggerBase<OriginFilteredLogger<BaseLogger> >
{
  //! Reference to the base logger
  BaseLogger & baselogger;

  //! List of patterns set. See class documentation.
  std::map<std::string,int> levels_set;
public:

  /** \brief Constructor based on a base logger reference.
   *
   * Pass a reference to the base logger here. Calls to \ref emitLog(), etc., will be
   * relayed to this base logger.
   */
  OriginFilteredLogger(BaseLogger & baselogger_)
    : Tomographer::Logger::LoggerBase<OriginFilteredLogger<BaseLogger> >(),
      baselogger(baselogger_),
      levels_set()
  {
  }

  /** \brief Set a rule to log messages based on their origin.
   *
   * Messages whose \a origin start with the substring \a origin_pattern will be emitted
   * only if they are at least of severity \a level. (The rule corresponding to the
   * longest pattern, i.e. the most specific pattern, is always the one which is applied.)
   *
   * Calling this function several times with the same pattern replaces the previous
   * setting.
   */
  inline void setDomainLevel(const std::string& origin_pattern, int level)
  {
    levels_set[origin_pattern] = level;
  }

  /** \brief Remove a rule set by \ref setDomainLevel()
   *
   * \param s the origin pattern corresponding to the rule to erase. This must be a
   *     pattern previously passed to \ref setDomainLevel().
   */
  inline void removeDomainSetting(const std::string& s)
  {
    auto it = levels_set.find(s);
    if (it == levels_set.end()) {
      this->warning("OriginFilteredLogger<BaseLogger>::removeDomainSetting", "domain not set: `%s'", s.c_str());
      return;
    }
    levels_set.erase(it);
  }

  /** \brief Unconditionally return \ref LOWEST_SEVERITY_LEVEL for the underlying logging
   * engine.
   *
   * This level() is used to pre-filter all the log messages, before our \a
   * filterByOrigin() gets called. So we return \ref LOWEST_SEVERITY_LEVEL here to
   * "trick" the logging engine, and use the base logger's level() only in \ref
   * filterByOrigin().
   */
  inline int level() const
  {
    return LOWEST_SEVERITY_LEVEL;
  }

  /** \brief Emit a log message (relay to base logger).
   *
   * Simply relay the call to the base logger instance's \a emitLog().
   */
  inline void emitLog(int level, const char * origin, const std::string& msg)
  {
    baselogger.emitLog(level, origin, msg);
  }

  /** \brief Message filtering by origin implementation.
   *
   * This function returns \c true if the message should be emitted, given the set of
   * rules set by previous calls to \ref setDomainLevel().
   *
   * See also \ref DefaultLoggerTraits::HasFilterByOrigin "LoggerTraits::HasFilterByOrigin".
   */
  inline bool filterByOrigin(int level, const char * origin) const
  {
    typedef std::map<std::string, int>::const_iterator ConstIterator;

    std::string s(origin);

    int loglevel = -1;
    std::size_t last_matched_length = 0;
    for (ConstIterator it = levels_set.begin(); it != levels_set.end(); ++it) {
      const std::size_t mlen = tomo_internal::matched_length((*it).first, s);
      if (mlen > last_matched_length) {
	loglevel = (*it).second;
	last_matched_length = mlen;
      }
    }
    if (loglevel == -1) {
      // default logger level
      loglevel = baselogger.level();
    }
    return isAtLeastOfSeverity(level, loglevel);
  }

};





// --------------------------------------------------





//! Tool to specify arguments to \ref LocalLogger
struct LocalLoggerOriginSpec {
  //! Origin prefix for the local logger
  const Tools::conststr origin_prefix;
  //! optionally some string to append to origin_prefix
  const Tools::conststr origin_prefix_add;
  //! the glue to use in the local logger
  const Tools::conststr glue;

  //! constructor. origin_prefix_add is left blank.
  constexpr LocalLoggerOriginSpec(const Tools::conststr& s, const Tools::conststr& gl)
    : origin_prefix(s), origin_prefix_add(""), glue(gl) { }
  //! complete constructor.
  constexpr LocalLoggerOriginSpec(const Tools::conststr& s, const Tools::conststr& s2, const Tools::conststr& gl)
    : origin_prefix(s), origin_prefix_add(s2), glue(gl) { }
};

namespace tomo_internal {
/** \internal */
struct extractTomoOrigin_helper {
  static inline constexpr LocalLoggerOriginSpec step2(const Tools::conststr fn, std::size_t last_doublecolons,
							 std::size_t after_prelast_doublecolons)
  {
    return ( fn.substr_e(after_prelast_doublecolons, last_doublecolons) == fn.substr(last_doublecolons+2)
	     // fn is a constructor, so keep class name and use "::" as glue
	     ? LocalLoggerOriginSpec(fn.substr(last_doublecolons+2), "::")
	     // looks like a method name. Strip off the class name. Also use an internal
	     // glue to indicate a logical level.
	     : LocalLoggerOriginSpec(fn.substr(last_doublecolons+2), "()", "/")
	);
  }
  static inline constexpr std::size_t afterprelast_doublecolons(std::size_t prelast_doublecolons_found)
  {
    return (prelast_doublecolons_found == std::string::npos) ? 0 : prelast_doublecolons_found+2;
  }
  static inline constexpr LocalLoggerOriginSpec step1(const Tools::conststr fn, std::size_t last_doublecolons)
  {
    return last_doublecolons == std::string::npos || last_doublecolons == 0
      ? LocalLoggerOriginSpec(fn, "()", "/") // looks like simple function name with no parent scope
      : step2(fn, last_doublecolons, afterprelast_doublecolons(fn.rfind("::", last_doublecolons-1)));
  }

  static inline constexpr LocalLoggerOriginSpec extract_from_func_name(const Tools::conststr fn)
  {
    return step1(fn, fn.rfind("::"));
  }
};

/** \internal */
constexpr const LocalLoggerOriginSpec extractTomoOrigin(const Tools::conststr fn)
{
  return extractTomoOrigin_helper::extract_from_func_name(Tomographer::Tools::extractFuncName(fn));
}

} // namespace tomo_internal


/** \brief Use this as argument for a \ref Tomographer::Logger::LocalLogger constructor . 
 *
 * \warning TOMO_ORIGIN does not work inside C++11 lambda's.
 */
#define TOMO_ORIGIN Tomographer::Logger::tomo_internal::extractTomoOrigin(TOMO_FUNCTION)




template<typename BaseLoggerType_>  class LocalLogger;


namespace tomo_internal {
//! Utility to check if a logger is in fact a LocalLogger
template<typename BaseLoggerType>
struct is_local_logger {
  enum { value = 0 };
};
template<typename BaseLoggerType>
struct is_local_logger<LocalLogger<BaseLoggerType> > {
  enum { value = 1 };
};
//! Utility to get parent logger type
template<typename BaseLoggerType>
struct local_logger_parent {
  typedef BaseLoggerType ParentType;
};
template<typename BaseLoggerType>
struct local_logger_parent<LocalLogger<BaseLoggerType> > {
  typedef typename local_logger_parent<BaseLoggerType>::ParentType ParentType;
};
} // namespace tomo_internal

//! Specialized Traits for \ref LocalLogger. See \ref LoggerTraits<BaseLoggerType_>
template<typename BaseLoggerType_>
struct LoggerTraits<LocalLogger<BaseLoggerType_> > : public LoggerTraits<BaseLoggerType_>
{
  enum {
    //! Logger will delegate calls for current level() to base logger
    HasOwnGetLevel = 1
  };
};


/** \brief Local logger: avoid having to repeat origin at each emitted message
 *
 * This type of logger accepts origin information in its constructor. Then, you may call
 * the \ref longdebug(), \ref debug(), \ref info(), \ref warning() and \ref error()
 * methods without having to specify the \a origin information, which is known from the
 * constructor.
 *
 * This logger is to be used with another logger instance, which is actually responsible
 * for producing the log messages.  This logger then relays log messages that base logger
 * instance, of type \a BaseLoggerType (for example, \ref FileLogger, \ref BufferLogger
 * etc.).
 *
 * You may also nest these loggers, yielding an automatic nested origin string following
 * C++ names.  See method \ref subLogger(const std::string&) for more info.  In case of
 * nesting, what happens is that the \a BaseLoggerType is a LocalLogger itself.
 *
 * A very handy shorthand is \ref makeLocalLogger(), which allows to spare some typing
 * (see documentation for \ref makeLocalLogger()).
 *
 * The macro \ref TOMO_ORIGIN is particularly useful for this class, as you can specify it
 * in the \a origin parameter to automatically yield the name of the current method.
 *
 * Example usage:
 * \code
 *   template<typename BaseLoggerType>
 *   class XYZ
 *   {
 *     LocalLogger<BaseLoggerType> _logger;
 *   public:
 *     XYZ(BaseLoggerType & baselogger)
 *       : _logger(TOMO_ORIGIN, baselogger)
 *     {
 *       // this message's origin will be "XYZ"
 *       _logger.debug("constructor was called.");
 *     }
 *
 *     void method(int k)
 *     {
 *       // this message's origin will be "XYZ"
 *       _logger.debug("method() was called. k=%d", k);
 *
 *       // if you need to pass a logger to any external procedure, pass on the
 *       // parentLogger() as the other routine expects to be able to specify its
 *       // own origin string:
 *       some_external_routine(k, .., _logger.parentLogger());
 *     }
 *
 *     void longmethod(int N)
 *     {
 *       auto logger = _logger.subLogger(TOMO_ORIGIN);
 *       for (int k = 0; k < N; ++k) {
 *         // this message's origin will be "XYZ::longmethod()"
 *         logger.debug("inner loop: k=%d out of %d", k, N);
 *       }
 *     }
 *   };
 * \endcode
 */
template<typename BaseLoggerType_>
TOMOGRAPHER_EXPORT class LocalLogger
  : public LoggerBase<LocalLogger<BaseLoggerType_> >
{
public:
  //! The base logger type (see class documentation)
  typedef BaseLoggerType_ BaseLoggerType;

private:
  typedef LoggerBase<LocalLogger> Base_;

  /** See \ref originPrefix() */
  const std::string _origin_prefix;
  /** See \ref glue() */
  const std::string _glue;

  BaseLoggerType & _baselogger;

public:
  /** \brief Construct a local logger
   *
   * \param origin_fn_name is the origin you want to associate with log messages generated
   *        by this logger.  You may also use the macro \ref TOMO_ORIGIN for this argument
   *        to automatically set the origin to the current function or method name.
   *
   * \param logger_ is a logger instance of type \ref BaseLoggerType, which is responsible
   *        for actually emitting the messages in some useful way (e.g. it may be a \ref
   *        FileLogger, \ref BufferLogger, etc.)
   */
  LocalLogger(const std::string & origin_fn_name, BaseLoggerType & logger_)
    : Base_(), _origin_prefix(origin_fn_name), _glue("::"), _baselogger(logger_)
  {
    this->longdebug("[begin]");
  }
  /** \brief Construct a local logger
   *
   * See \ref LocalLogger(const std::string&, BaseLoggerType&).  This overload allows you
   * to also specify the glue to use when concatenating origins for sub-loggers, see \ref
   * glue() and \ref subLogger().
   */
  LocalLogger(const std::string & origin_prefix, const std::string & glue, BaseLoggerType & logger_)
    : Base_(), _origin_prefix(origin_prefix), _glue(glue), _baselogger(logger_)
  {
    this->longdebug("[begin]");
  }
  /** \brief Construct a local logger
   *
   * See \ref LocalLogger(const std::string&, BaseLoggerType&).  This overload is the one
   * which is called if you use the macro \ref TOMO_ORIGIN for the first argument.
   */
  LocalLogger(const LocalLoggerOriginSpec & spec, BaseLoggerType & logger_)
    : Base_(), _origin_prefix(spec.origin_prefix.to_string()+spec.origin_prefix_add.to_string()),
      _glue(spec.glue.to_string()), _baselogger(logger_)
  {
    this->longdebug("[begin]");
  }

  /**
   * We need this for \ref makeLocalLogger().
   */
  LocalLogger(LocalLogger && movecopy)
    : Base_(), _origin_prefix(std::move(movecopy._origin_prefix)), _glue(std::move(movecopy._glue)),
      _baselogger(movecopy._baselogger)
  {
    this->longdebug("[logger moved]");
  }
  //! Make the local-logger copyable -- there's nothing wrong with that
  LocalLogger(const LocalLogger & other)
    : Base_(), _origin_prefix(other._origin_prefix), _glue(other._glue),
      _baselogger(other._baselogger)
  {
    this->longdebug("[logger copied]");
  }

  ~LocalLogger()
  {
    this->longdebug("[done]");
  }

  /** \brief The fixed origin specified at the constructor
   *
   * This will be used as origin if you call one of the logging functions \ref
   * longdebug(), \ref debug(), \ref info(), \ref warning(), \ref error() or \ref log()
   * "log<LEVEL>()".
   *
   * This string will also be used as a "prefix" for sub-loggers.
   */
  inline std::string originPrefix() const { return _origin_prefix; }

  /** \brief The "glue" string to use to concatenate origins from sub-loggers
   */
  inline std::string glue() const { return _glue; }

  /** \brief The base logger type specified to the constructor
   *
   * If we are a sub-logger of another local logger, then \a BaseLoggerType is itself also
   * a LocalLogger.
   */
  inline BaseLoggerType & baseLogger() { return _baselogger; }

#ifdef TOMOGRAPHER_PARSED_BY_DOXYGEN
  /** \brief Type of the parent logger type, ultimately responsible for actually emitting
   *         the messages in some useful way
   *
   * See \ref parentLogger().
   */
  typedef _PARENT_LOGGER_TYPE  ParentLoggerType;

  /** \brief The parent logger responsible for actually emitting the messages in some
   *         useful way
   *
   * This function returns \ref baseLogger() if the \a BaseLoggerType is not a
   * LocalLogger.  Otherwise, it follows the base loggers of the LocalLoggers until a
   * non-LocalLogger parent is found; the latter is returned.
   */
  ParentLoggerType & parentLogger() { }
#else

  typedef typename tomo_internal::local_logger_parent<BaseLoggerType>::ParentType  ParentLoggerType;

  TOMOGRAPHER_ENABLED_IF(!tomo_internal::is_local_logger<BaseLoggerType>::value)
  inline ParentLoggerType & parentLogger()
  {
    return baseLogger();
  }
  TOMOGRAPHER_ENABLED_IF(tomo_internal::is_local_logger<BaseLoggerType>::value)
  inline ParentLoggerType & parentLogger() {
    return baseLogger().parentLogger();
  }
#endif

  /** \brief Create a sub-logger
   *
   * The origin of the messages generated by this new sub-logger will be the concatenation
   * of the parent (this) logger's origin, with the \a new_prefix, using the parent (this)
   * object's \ref glue() string.
   *
   * You may use the macro \ref TOMO_ORIGIN as argument here to automatically set the
   * origin to the current function or method name.
   */
  inline LocalLogger<LocalLogger<BaseLoggerType> > subLogger(const std::string & new_prefix)
  {
    return LocalLogger<LocalLogger<BaseLoggerType> >(new_prefix, *this);
  }
  /** \brief Create a sub-logger
   *
   * See \ref subLogger(const std::string &).  This overload allows to specify the new
   * object's glue string, which will be used in case the sub-logger itself becomes parent
   * to a (sub-)sub-logger.
   */
  inline LocalLogger<LocalLogger<BaseLoggerType> > subLogger(const std::string & new_prefix,
                                                             const std::string & new_glue)
  {
    return LocalLogger<LocalLogger<BaseLoggerType> >(new_prefix, new_glue, *this);
  }
  /** \brief Create a sub-logger
   *
   * See \ref subLogger(const std::string &).  This overload is called when the macro \ref
   * TOMO_ORIGIN is specified.
   */
  inline LocalLogger<LocalLogger<BaseLoggerType> > subLogger(const LocalLoggerOriginSpec & spec)
  {
    return LocalLogger<LocalLogger<BaseLoggerType> >(spec, *this);
  }

  /** \brief Generate a log message with level \ref Logger::LONGDEBUG (printf-like syntax)
   *
   * The origin parameter is automatically set, and is not specified here.
   */
  PRINTF2_ARGS_SAFE inline void longdebug(const char * fmt, ...)
  { va_list ap; va_start(ap, fmt);  log<LONGDEBUG>(fmt, ap); va_end(ap); }
  /** \brief Generate a log message with level \ref Logger::DEBUG (printf-like syntax)
   *
   * The origin parameter is automatically set, and is not specified here.
   */
  PRINTF2_ARGS_SAFE inline void debug(const char * fmt, ...)
  { va_list ap; va_start(ap, fmt);  log<DEBUG>(fmt, ap); va_end(ap); }
  /** \brief Generate a log message with level \ref Logger::INFO (printf-like syntax)
   *
   * The origin parameter is automatically set, and is not specified here.
   */
  PRINTF2_ARGS_SAFE inline void info(const char * fmt, ...)
  { va_list ap; va_start(ap, fmt);  log<INFO>(fmt, ap); va_end(ap); }
  /** \brief Generate a log message with level \ref Logger::WARNING (printf-like syntax)
   *
   * The origin parameter is automatically set, and is not specified here.
   */
  PRINTF2_ARGS_SAFE inline void warning(const char * fmt, ...)
  { va_list ap; va_start(ap, fmt);  log<WARNING>(fmt, ap); va_end(ap); }
  /** \brief Generate a log message with level \ref Logger::ERROR (printf-like syntax)
   *
   * The origin parameter is automatically set, and is not specified here.
   */
  PRINTF2_ARGS_SAFE inline void error(const char * fmt, ...)
  { va_list ap; va_start(ap, fmt);  log<ERROR>(fmt, ap); va_end(ap); }

  /** \brief Generate a log message with level \ref Logger::LONGDEBUG
   *
   * The origin parameter is automatically set, and is not specified here.
   */
  template<typename... Args>
  inline void longdebug(Args &&... a) { log<Tomographer::Logger::LONGDEBUG>(std::forward<Args>(a)...); }
  /** \brief Generate a log message with level \ref Logger::DEBUG
   *
   * The origin parameter is automatically set, and is not specified here.
   */
  template<typename... Args>
  inline void debug(Args &&... a) { log<Tomographer::Logger::DEBUG>(std::forward<Args>(a)...); }
  /** \brief Generate a log message with level \ref Logger::INFO
   *
   * The origin parameter is automatically set, and is not specified here.
   */
  template<typename... Args>
  inline void info(Args &&... a) { log<Tomographer::Logger::INFO>(std::forward<Args>(a)...); }
  /** \brief Generate a log message with level \ref Logger::WARNING
   *
   * The origin parameter is automatically set, and is not specified here.
   */
  template<typename... Args>
  inline void warning(Args &&... a) { log<Tomographer::Logger::WARNING>(std::forward<Args>(a)...); }
  /** \brief Generate a log message with level \ref Logger::ERROR
   *
   * The origin parameter is automatically set, and is not specified here.
   */
  template<typename... Args>
  inline void error(Args &&... a) { log<Tomographer::Logger::ERROR>(std::forward<Args>(a)...); }

  /** \brief Generate a log message with level \a Level
   *
   * The origin parameter is automatically set, and is not specified here.
   */
  template<int Level, typename... Args>
  inline void log(Args... args)
  {
    Base_::template log<Level>("", args...);
  }


  // relay calls to base logger

  /** \brief The full origin string to use for a sub-logger
   *
   * This simply returns the concatenation of \ref originPrefix() with \a origin using the
   * \ref glue().
   *
   * It is the full origin parameter which is used when creating sub-loggers of this
   * logger (see \ref subLogger()).
   */
  inline std::string getSubOrigin(const char * origin) const
  {
    return ( origin == NULL || origin[0] == 0
	     ? _origin_prefix
	     : _origin_prefix + _glue + origin );
  }

  //! Emit a log by relaying to the base logger
  inline void emitLog(int level, const char * origin, const std::string& msg)
  {
    // this might also be called if we have a sublogger. In that case, if we have a
    // sublogger, then use their prefix.
    _baselogger.emitLog(level, getSubOrigin(origin).c_str(), msg);
  }

  //! Get the base logger's set level.
  inline int level() const
  {
    return _baselogger.level();
  }

  //! If relevant for the base logger, filter the messages by origin from the base logger.
  TOMOGRAPHER_ENABLED_IF(Tomographer::Logger::LoggerTraits<BaseLoggerType>::HasFilterByOrigin)
  inline bool filterByOrigin(int level, const char * origin)
  {
    return _baselogger.filterByOrigin(level, getSubOrigin(origin).c_str());
  }
};



/** \brief Create a local logger
 *
 * This function is useful to avoid having to repeat the base logger's type, by using
 * C++'s \c auto keyword as well as automatic template argument deduction:
 * \code
 *   auto logger = makeLocalLogger(TOMO_ORIGIN, baselogger);
 * \endcode
 *
 * The above code is equivalent to
 * \code
 *   LocalLogger<decltype(baselogger)> logger(TOMO_ORIGIN, baselogger);
 * \endcode
 *
 * Each overload of \a makeLocalLogger() calls the matching \ref LocalLogger constructor.
 * As in the example above, you may use \ref TOMO_ORIGIN as the origin argument.
 */
template<typename BaseLoggerType>
inline LocalLogger<BaseLoggerType> makeLocalLogger(const std::string & origin_fn_name, BaseLoggerType & baselogger)
{
  return LocalLogger<BaseLoggerType>(origin_fn_name, baselogger);
}

/** \brief Create a local logger
 *
 * This function overload provides the same functionality as \ref makeLocalLogger(const
 * std::string&,BaseLoggerType&), but simply calls the other corresponding constructor.
 */
template<typename BaseLoggerType>
inline LocalLogger<BaseLoggerType> makeLocalLogger(const std::string & origin_prefix, const std::string & glue,
					    BaseLoggerType & baselogger)
{
  return LocalLogger<BaseLoggerType>(origin_prefix, glue, baselogger);
}

/** \brief Create a local logger
 *
 * This function overload provides the same functionality as \ref makeLocalLogger(const
 * std::string&,BaseLoggerType&), but simply calls the other corresponding constructor.
 */
template<typename BaseLoggerType>
inline LocalLogger<BaseLoggerType> makeLocalLogger(const LocalLoggerOriginSpec & spec, BaseLoggerType & baselogger)
{
  return LocalLogger<BaseLoggerType>(spec, baselogger);
}





} // namespace Logger
} // namespace Tomographer



#endif
