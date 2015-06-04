
#ifndef LOGGERS_H
#define LOGGERS_H

#include <cassert>
#include <cstdio>
#include <cstdarg>

#include <string>
#include <sstream> // stringstream
#include <iostream>
#include <functional>
#include <type_traits>

#include <tomographer/tools/fmt.h>

#define ENABLE_IF_Fn_CALLABLE_OSTREAM                                      \
  typename std::enable_if<std::is_convertible<Fn,std::function<void(std::ostream&)> >::value, void>::type


namespace Tomographer
{

/** \brief General definitions for Tomographer's \ref pageLoggers "Logging Framework"
 *
 */
namespace Logger
{
  /** \brief Possible logging levels.
   *
   * Don't trust the numeric values here, they may change at any time. Just the order is
   * important.
   */
  enum {
    /** \brief Error logging level
     *
     * A log message with this level signifies that a critical error has occurred which
     * prevents further processing. The program should terminate.
     */
    ERROR = 0,

    /** \brief Warning logging level
     *
     * A log message with this level signifies a warning for the user. The computation may
     * continue, but most likely the user made provided erroneous input, or there are
     * features which will not be available.
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
     * The \c DEBUG level is meant to still be readable on a terminal output, even with
     * multiple threads. Don't generate too many messages with this level. Use the
     * \c LONGDEBUG level to generate as many messages as you want.
     */
    DEBUG,

    /** \brief Long Debug logging level
     *
     * This level is meant to signify very specific messages, such as individual
     * iterations. The log produced with this level may be very long an no longer readable
     * on a terminal.
     *
     * Use this level for messages that are generated very often.
     */
    LONGDEBUG
  };

  /** \brief helper to compare severity levels
   *
   * Returns \c true if the given \a level is at least as severe (equal severity or higher
   * severity) as \a baselevel (for example, if <code>level == Logger::ERROR</code> and
   * <code>baselevel == Logger::INFO</code>).
   *
   * Just to make sure we don't confuse the numerical ordering of the levels, we should
   * always use this helper function to see whether a level is at least of a given
   * severity.
   */
  inline bool is_at_least_of_severity(int level, int baselevel)
  {
    return (level <= baselevel);
  }

  /** \brief helper for statically determining if Level is at least as severe as BaseLevel.
   *
   * The member \a value is statically defined to \c 1 if \a Level is at least as severe
   * as \a BaseLevel, otherwise it is defined to \c 0.
   *
   * See also is_at_least_of_severity().
   */
  template<int Level, int BaseLevel>
  struct static_is_at_least_of_severity {
    enum {
      value = (Level <= BaseLevel)
    };
  };
} // namespace Logger


/** \brief Traits for Logger classes.
 *
 * If you write your own logger class, you can provide also information about this class
 * such as whether it is thread-safe by specializing this traits class, e.g.:
 * \code
 *   template<>
 *   struct LoggerTraits<MyCustomLogger> : public Tomographer::DefaultLoggerTraits
 *   {
 *     enum {
 *       IsThreadSafe = 1,
 *       StaticMinimumImportanceLevel = -1
 *     };
 *   };
 * \endcode
 *
 * It's a good idea to inherit from \ref DefaultLoggerTraits, so that if additional traits
 * are added in the future, then your code will still compile.
 */
template<typename Logger>
struct LoggerTraits
{
  enum {
    /** \brief Whether a same logger instance may be called from different threads
     * simultaneously
     *
     * By default, loggers are not thread-safe.
     */
    IsThreadSafe = 0,

    /** \brief A statically-determined minimum importance logging level
     *
     * If set to \c -1, then all messages will be decided at runtime whether to be emitted
     * or not, depending on the level the logger instance was set to at run-time.
     *
     * If a logger defines a minimum importance level different than \c -1, then any
     * message logged with strictly lesser importance level will automatically be
     * discarded, regardless of the level the logger is given at run-time.
     */
    StaticMinimumImportanceLevel = -1,

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
     * This flag may be set to either \c 0 (no filtering) or \c 1 (filtering).
     *
     * If filtering is activated, then before formatting and emitting messages the logging
     * engine will test the logger object's <code>filter_by_origin(int level, const char *
     * origin)</code>. The message will only be emitted if this function returns \c
     * true. Note that messages may have already been discarded if their severity was less
     * important that the runtime level of the logger, and those messages don't even reach
     * this filtering function call.
     */
    HasFilterByOrigin = 0
  };
};

/** \brief Default logger traits.
 *
 * When defining your own \ref LoggerTraits, inherit from this type to get the default
 * traits. This way, if we add traits in the future, your code still compiles without any
 * changes.
 */
typedef LoggerTraits<int>  DefaultLoggerTraits;



namespace tomo_internal {
/** A small helper that stores a level if it's supposed to store a level, or that does
 * nothing if the derived logger object anyway provides its own level() method.
 */
template<bool hasOwnGetLevel>
class LoggerHelperRuntimeLevel {
public:
  LoggerHelperRuntimeLevel(int level)
    : _level(level)
  {
  }

  //  template<typename Derived>
  inline int getLevel(/* const Derived * d */) const
  {
    return _level;
  }
  
  inline void setLevel(int level)
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
class LoggerHelperRuntimeLevel<true> {
public:
  LoggerHelperRuntimeLevel(int /*level*/)
  {
  }
  //  template<typename Derived>
  //  inline int getLevel(const Derived * d) const
  //  {
  //    return d->level();
  //  }
  inline void setLevel(int) {
    assert(0 && "Call to setLevel() for Logger which defines HasOwnGetLevel=1!");
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
 * See also \ref SimpleFoutLogger.
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
 * example, \ref SimpleFoutLogger).
 */
template<typename Derived>
class LoggerBase
{
public:

  /** \brief Construct the base logger object
   *
   * The logging level is set to \a level, by default \ref Logger::INFO. Any messages with
   * lesser importance will be automatically discarded.
   *
   * Note that if the derived logger class provides its own \c logger() method, and
   * declares it with \a LoggerTraits<Derived>::HasOwnGetLevel, then the level provided
   * here is discarded and ignored.
   */
  LoggerBase(int level = Logger::INFO)
    : _level(level)
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

  /** \brief Check whether the logger is statically disabled for some levels
   *
   * \tparam Level the log level to test for.
   *
   * \return \c false if the logger was explicitly disabled for the level \a Level via
   * LoggerTraits::StaticMinimumImportanceLevel, otherwise returns \c true.
   *
   * If \c true was returned, this does not yet mean that a log message at the given level
   * will necessarily be produced; in this case, rather, the message is not expliclty
   * discarded and the logger's level set at run-time will be tested (see \ref
   * enabled_for()).
   */
  static inline bool statically_enabled_for(int level)
  {
    return ( (LoggerTraits<Derived>::StaticMinimumImportanceLevel == -1) ||
             (Logger::is_at_least_of_severity(level, LoggerTraits<Derived>::StaticMinimumImportanceLevel)) );
  }

  /** \brief Static version of statically_enabled_for()
   *
   */
  template<int Level>
  static inline bool statically_enabled_for()
  {
    return ( (LoggerTraits<Derived>::StaticMinimumImportanceLevel == -1) ||
             (Logger::static_is_at_least_of_severity<
	          Level,
	          LoggerTraits<Derived>::StaticMinimumImportanceLevel
	          >::value) );
  }

  /** \brief Check whether messages at the given log level are enabled
   *
   * This function returns \c true if a message at the given level will be emitted, i.e. 
   * transmitted to the underlying logger implementation.
   *
   * The condition for a log message to be emitted is that the message's log level not be
   * explicitly disabled by LoggerTraits::StaticMinimumImportanceLevel, and that the
   * message's log level be at least as important as the current level set for this logger
   * (see \ref level()).
   */
  inline bool enabled_for(int level) const
  {
    return statically_enabled_for(level) && Logger::is_at_least_of_severity(level, getLevel());
  };

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
   *
   * Just ignore the \a dummy template parameter, which is just there so that we can make
   * C++ template SFINAE kick in...
   */
  template<bool dummy = true>
  inline typename std::enable_if< dummy && ! LoggerTraits<Derived>::HasOwnGetLevel , int >::type
    level() const
  {
    return getLevel();
  }
  

protected:

  // version in case we store our own level
  template<bool dummy = true>
  inline typename std::enable_if< dummy && ! LoggerTraits<Derived>::HasOwnGetLevel , int >::type
    getLevel() const
  {
    return _level.getLevel();
  }
  // version in case we delegate to derived class' level()
  template<bool dummy = true>
  inline typename std::enable_if< dummy && LoggerTraits<Derived>::HasOwnGetLevel , int >::type
    getLevel() const
  {
    return static_cast<const Derived*>(this)->level();
  }

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
   * false. (You'll get an assert(false) otherwise.)
   */
  inline void setLogLevel(int level)
  {
    assert(! LoggerTraits<Derived>::HasOwnGetLevel);
    _level.setLevel(level);
  }

private:
  typedef tomo_internal::LoggerHelperRuntimeLevel<LoggerTraits<Derived>::HasOwnGetLevel>
    RuntimeLevel;
  
  /** \brief The stored run-time-given log level. See \ref level().
   *
   * The base class does not provide a <code>setLevel()</code> function, in case your
   * logger does not support setting any level, or in case you need your logger to be
   * completely thread-safe, or for any other reason.
   *
   * Subclasses may, however, change the log level by explicily assigning to \c _level.
   */
  RuntimeLevel _level;
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
      return static_cast<const Derived*>(loggerbase)->filter_by_origin(level, origin);
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
                                     const std::string & msg) throw()
    {
      try {
        //printf("Calling emit_log(%d,\"%s\",\"%s\") on object %p\n", level, origin, msg.c_str(), loggerbase);
        static_cast<Derived*>(loggerbase)->emit_log(level, origin, msg);
      } catch (const std::exception & e) {
        std::fprintf(stderr,
		     "Warning in LoggerBaseHelperDynamic::call_emit_log(%d, \"%s\", msg):"
		     " Exception caught: %s\n",
		     level, origin, e.what());
      }
    }
    static inline bool test_should_emit(LoggerBase<Derived> * loggerbase, int level, const char * origin)
    {
      if ( ! static_cast<const Derived*>(loggerbase)->enabled_for(level) ) {
	return false;
      }
      if ( ! LoggerBaseHelperFilterByOrigin<Derived, LoggerTraits<Derived>::HasFilterByOrigin>
	   ::test_logger_filter(loggerbase, level, origin) ) {
	return false;
      }
      return true;
    }

    static inline void test_and_call_emit_log(LoggerBase<Derived> * loggerbase, int level, const char * origin,
                                              const std::string & msg) throw()
    {
      if ( ! test_should_emit(loggerbase, level, origin) ) {
        return;
      }

      call_emit_log(loggerbase, level, origin, msg);
    }
    static inline void test_and_call_emit_log(LoggerBase<Derived> * loggerbase, int level, const char * origin,
                                              const char * fmt, va_list ap) throw()
    {
      if ( ! test_should_emit(loggerbase, level, origin) ) {
        return;
      }

      try {
        std::string msg = Tools::vfmts(fmt, ap);
        test_and_call_emit_log(loggerbase, level, origin, msg);
      } catch (const std::exception & e) {
        std::fprintf(stderr,
		     "Warning in LoggerBase::test_and_call_emit_log(%d, \"%s\", \"%s\", ...):"
		     " Exception caught for vfmts(): %s\n",
		     level, origin, fmt, e.what());
      }
    }
    template<typename Fn>
    static inline
    ENABLE_IF_Fn_CALLABLE_OSTREAM
    test_and_call_emit_log(LoggerBase<Derived> * loggerbase, int level, const char * origin, Fn f) throw()
    {
      if ( ! test_should_emit(loggerbase, level, origin) ) {
        return;
      }

      try {
        std::ostringstream sstr;
        f(sstr);
        test_and_call_emit_log(loggerbase, level, origin, sstr.str());
      } catch (const std::exception & e) {
        std::fprintf(stderr, "Warning in LoggerBase::test_and_call_emit_log(%d, \"%s\", f(ostream)):"
		     " Exception caught: %s\n",
		     level, origin, e.what());
      }
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
                                              Args... args) throw()
    {
      LoggerBaseHelperDynamic<Derived>::test_and_call_emit_log(loggerbase, Level, origin, args...);
    }
  };
  // specialization for calls where the message should be statically discarded
  template<typename Derived, int Level>
  struct LoggerBaseHelperStatic2<Derived, Level, true> {
    template<typename... Args>
    static inline void test_and_call_emit_log(LoggerBase<Derived> *, const char *, Args...) throw()
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
                                              Args... args) throw()
    {
      // call the default or specialized version of our second helper, which will either
      // relay the call the dynamic version or discard the message.
      //std::fprintf(stderr,
      //        "LoggerBaseHelperStatic<Derived,Level=%d>::test_and_call_emit_log(...). StaticMinimumImportanceLevel=%d\n",
      //        (int)Level, (int)(LoggerTraits<Derived>::StaticMinimumImportanceLevel));
      LoggerBaseHelperStatic2<
          Derived, Level,
          ((LoggerTraits<Derived>::StaticMinimumImportanceLevel != -1) &&
           (LoggerTraits<Derived>::StaticMinimumImportanceLevel < Level))
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
  tomo_internal::LoggerBaseHelperStatic<Derived,Logger::ERROR>::test_and_call_emit_log(this, origin, fmt, ap);
  va_end(ap);
}

template<typename Derived>
inline void LoggerBase<Derived>::error(const char * origin, const std::string & msg)
{
  tomo_internal::LoggerBaseHelperStatic<Derived,Logger::ERROR>::test_and_call_emit_log(this, origin, msg);
}

template<typename Derived>
template<typename Fn>
inline ENABLE_IF_Fn_CALLABLE_OSTREAM
LoggerBase<Derived>::error(const char * origin, Fn f)
{
  tomo_internal::LoggerBaseHelperStatic<Derived,Logger::ERROR>::test_and_call_emit_log(this, origin, f);
}


template<typename Derived>
inline void LoggerBase<Derived>::warning(const char * origin, const char * fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  tomo_internal::LoggerBaseHelperStatic<Derived,Logger::WARNING>::test_and_call_emit_log(this, origin, fmt, ap);
  va_end(ap);
}

template<typename Derived>
inline void LoggerBase<Derived>::warning(const char * origin, const std::string & msg)
{
  tomo_internal::LoggerBaseHelperStatic<Derived,Logger::WARNING>::test_and_call_emit_log(this, origin, msg);
}

template<typename Derived>
template<typename Fn>
inline ENABLE_IF_Fn_CALLABLE_OSTREAM
LoggerBase<Derived>::warning(const char * origin, Fn f)
{
  tomo_internal::LoggerBaseHelperStatic<Derived,Logger::WARNING>::test_and_call_emit_log(this, origin, f);
}

template<typename Derived>
inline void LoggerBase<Derived>::info(const char * origin, const char * fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  tomo_internal::LoggerBaseHelperStatic<Derived,Logger::INFO>::test_and_call_emit_log(this, origin, fmt, ap);
  va_end(ap);
}

template<typename Derived>
inline void LoggerBase<Derived>::info(const char * origin, const std::string & msg)
{
  tomo_internal::LoggerBaseHelperStatic<Derived,Logger::INFO>::test_and_call_emit_log(this, origin, msg);
}

template<typename Derived>
template<typename Fn>
inline ENABLE_IF_Fn_CALLABLE_OSTREAM
LoggerBase<Derived>::info(const char * origin, Fn f)
{
  tomo_internal::LoggerBaseHelperStatic<Derived,Logger::INFO>::test_and_call_emit_log(this, origin, f);
}

template<typename Derived>
inline void LoggerBase<Derived>::debug(const char * origin, const char * fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  tomo_internal::LoggerBaseHelperStatic<Derived,Logger::DEBUG>::test_and_call_emit_log(this, origin, fmt, ap);
  va_end(ap);
}

template<typename Derived>
inline void LoggerBase<Derived>::debug(const char * origin, const std::string & msg)
{
  tomo_internal::LoggerBaseHelperStatic<Derived,Logger::DEBUG>::test_and_call_emit_log(this, origin, msg);
}

template<typename Derived>
template<typename Fn>
inline ENABLE_IF_Fn_CALLABLE_OSTREAM
LoggerBase<Derived>::debug(const char * origin, Fn f)
{
  tomo_internal::LoggerBaseHelperStatic<Derived,Logger::DEBUG>::test_and_call_emit_log(this, origin, f);
}

template<typename Derived>
inline void LoggerBase<Derived>::longdebug(const char * origin, const char * fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  tomo_internal::LoggerBaseHelperStatic<Derived,Logger::LONGDEBUG>::test_and_call_emit_log(this, origin, fmt, ap);
  va_end(ap);
}

template<typename Derived>
inline void LoggerBase<Derived>::longdebug(const char * origin, const std::string & msg)
{
  tomo_internal::LoggerBaseHelperStatic<Derived,Logger::LONGDEBUG>::test_and_call_emit_log(this, origin, msg);
}

template<typename Derived>
template<typename Fn>
inline ENABLE_IF_Fn_CALLABLE_OSTREAM
LoggerBase<Derived>::longdebug(const char * origin, Fn f)
{
  tomo_internal::LoggerBaseHelperStatic<Derived,Logger::LONGDEBUG>::test_and_call_emit_log(this, origin, f);
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




//
// Traits for SimpleFoutLogger -- fprintf is actually thread-safe, that's good :)
//
class SimpleFoutLogger;
template<>
struct LoggerTraits<SimpleFoutLogger> : public DefaultLoggerTraits
{
  enum {
    IsThreadSafe = 1,
    StaticMinimumImportanceLevel = -1
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
class SimpleFoutLogger : public LoggerBase<SimpleFoutLogger>
{
public:
  SimpleFoutLogger(FILE * fp_, int level = Logger::INFO, bool display_origin_ = true)
    : LoggerBase<SimpleFoutLogger>(level), fp(fp_), display_origin(display_origin_)
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

  inline void emit_log(int level, const char * origin, const std::string & msg)
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

    // force output also on stderr for warnings and errors if we are being redirected to a file
    if (fp != stdout && fp != stderr && level <= Logger::WARNING) {
      std::fprintf(stderr, "%s\n", finalmsg.c_str());
    }
  }

private:
  std::FILE * fp;
  bool display_origin;
};





//
// Traits for VacuumLogger -- is a NOOP thread-safe? yeah probably.
//
class VacuumLogger;
template<>
struct LoggerTraits<VacuumLogger> : public DefaultLoggerTraits
{
  enum {
    IsThreadSafe = 1,
    StaticMinimumImportanceLevel = Logger::ERROR // discard all messages
  };
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

/** \brief A VacuumLogger instance which can be used as an LVALUE
 *
 * This instance may be used as an LVALUE, for example in default function arguments. A
 * VacuumLogger doesn't do anything, anyway.
 */
static VacuumLogger vacuum_logger;




//
// Traits for BufferLogger -- not thread-safe
//
class BufferLogger;
template<>
struct LoggerTraits<BufferLogger> : public DefaultLoggerTraits
{
  enum {
    IsThreadSafe = 0
  };
};

/** \brief Log messages into an internal memory buffer
 *
 * Logs messages into an internal string buffer. The contents of the buffer may be
 * retrieved with \ref get_contents().
 */
class BufferLogger : public LoggerBase<BufferLogger>
{
  std::ostringstream buffer;
public:
  BufferLogger(int level)
    : LoggerBase<BufferLogger>(level)
  {
  }

  inline void emit_log(int /*level*/, const char * origin, const std::string& msg)
  {
    buffer << (origin&&origin[0] ? "["+std::string(origin)+"] " : std::string())
           << msg.c_str() << "\n";
  }

  /** \brief Clears the internal memory buffer.
   *
   * Clears all messages logged so far. A future call to \ref get_contents() will only
   * return the messages logged after calling this function.
   */
  inline void clear()
  {
    buffer.clear();
  }

  /** \brief get the contents of the internal buffer
   *
   * This returns a string containing all the messages that have been logged so far.
   */
  inline std::string get_contents() const
  {
    return buffer.str();
  }
};





//
// Traits for MinimumImportanceLogger<BaseLogger,Level>
//
template<typename, int> class MinimumImportanceLogger;
template<typename BaseLogger, int Level>
struct LoggerTraits<MinimumImportanceLogger<BaseLogger,Level> > : public LoggerTraits<BaseLogger>
{
  enum {
    // discard all messages less important than the given level
    StaticMinimumImportanceLevel = Level,
    // delegate calls for current level() to base logger
    HasOwnGetLevel = 1
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
class MinimumImportanceLogger : public LoggerBase<MinimumImportanceLogger<BaseLogger,Level> >
{
  BaseLogger & baselogger;
public:
  MinimumImportanceLogger(BaseLogger & baselogger_)
    : LoggerBase<MinimumImportanceLogger<BaseLogger,Level> >(), baselogger(baselogger_)
  {
  }

  inline void emit_log(int level, const char * origin, const std::string& msg)
  {
    baselogger.emit_log(level, origin, msg);
  }

  inline int level() const
  {
    return baselogger.level();
  }

  template<bool dummy = true>
  inline std::enable_if<dummy && LoggerTraits<BaseLogger>::HasFilterByOrigin, bool>
  filter_by_origin(int level, const char * origin)
  {
    return baselogger.filter_by_origin(level, origin);
  }
};



} // namespace Tomographer



#endif
