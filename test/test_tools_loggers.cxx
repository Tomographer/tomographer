/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
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

#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <tuple>
#include <string>

#include <boost/math/constants/constants.hpp>

// definitions for Tomographer test framework -- this must be included before any
// <Eigen/...> or <tomographer/...> header
#include "test_tomographer.h"

#include <Eigen/Core>

#include <tomographer/tools/loggers.h>




// #############################################################################
//
// Fixtures, helpers, dummy classes etc. for testing.
//



// needed because if there is a single comma in the string passed to BOOST_CHECK, then the
// preprocessor thinks it's two macro arguments (!!)
#define MY_BOOST_CHECK(...) { auto check = [&]() -> bool { return __VA_ARGS__; }; BOOST_CHECK(check()); }


//
// dummy logger class which records which methods were called into a std::ostringstream.
// (Yes, in effect it is a logger which logs calls to the various logger methods.)
//
template<typename Derived>
struct DummyLoggerImplementation : public Tomographer::Logger::LoggerBase<Derived>
{
  std::string * target_record_calls;

  DummyLoggerImplementation(int level, std::string * target_record_calls_)
    : Tomographer::Logger::LoggerBase<Derived>(level), target_record_calls(target_record_calls_)
  {
  }

  inline void emitLog(int level, const char * origin, const std::string & msg)
  {
    std::ostringstream record_calls;
    record_calls << "emitLog(level=" << level << ", origin=\"" << origin << "\", msg=\"" << msg << "\")\n";
    *target_record_calls += record_calls.str();
  }

  // We define the level() method in all cases, including if the traits do not specify
  // that we have HasOwnGetLevel set. This is to test that the logger engine in that case
  // does NOT call this level() but the underlying stored level instead.
  inline int level() const
  {
    std::ostringstream record_calls;
    record_calls << "level()\n";
    *target_record_calls += record_calls.str();
    return Tomographer::Logger::INFO;
  }

  inline bool filterByOrigin(int level, const char * origin) const
  {
    std::ostringstream record_calls;
    record_calls << "filterByOrigin(level=" << level << ", origin=\"" << origin << "\")\n";
    *target_record_calls += record_calls.str();
    return (origin == std::string("origin_which_passes_filter()"));
  }

};


#define DEFINE_DUMMY_LOGGER_WITH_TRAITS(LoggerName, ...)                \
  struct LoggerName;                                                    \
  namespace Tomographer { namespace Logger {                            \
      template<> struct LoggerTraits<LoggerName> : public DefaultLoggerTraits { \
        __VA_ARGS__;                                                    \
      };                                                                \
  } } /* namespace Tomographer::Logger */                               \
  struct LoggerName                                                     \
    : public DummyLoggerImplementation<LoggerName>                      \
  {                                                                     \
    LoggerName(int mylevel, std::string * target)                       \
      : DummyLoggerImplementation<LoggerName>(mylevel, target)          \
        { }                                                             \
  };




DEFINE_DUMMY_LOGGER_WITH_TRAITS(DummyLoggerMinSeverity, enum {
    IsThreadSafe = 0,
    StaticMinimumSeverityLevel = Tomographer::Logger::WARNING,
    HasOwnGetLevel = 0,
    HasFilterByOrigin = 0
  });
DEFINE_DUMMY_LOGGER_WITH_TRAITS(DummyLoggerOwnGetLevel, enum {
    // rest set to inherited defaults
    IsThreadSafe = 0,
    HasOwnGetLevel = 1,
    HasFilterByOrigin = 0
  });
DEFINE_DUMMY_LOGGER_WITH_TRAITS(DummyLoggerOriginFilter, enum {
    // rest set to inherited defaults
    HasOwnGetLevel = 0,
    HasFilterByOrigin = 1
  });


// -----------------------------------------------------------------------------


struct fixture_bufferlogger {
  fixture_bufferlogger() : logger(Tomographer::Logger::DEBUG) { }
  ~fixture_bufferlogger() { }

  Tomographer::Logger::BufferLogger logger;
};



// -----------------------------------------------------------------------------


struct fixture_originfilteredlogger {
  Tomographer::Logger::BufferLogger buflog;
  Tomographer::Logger::OriginFilteredLogger<Tomographer::Logger::BufferLogger> logger;

  fixture_originfilteredlogger() : buflog(Tomographer::Logger::INFO), logger(buflog) {
    logger.setDomainLevel("my_origin_class", Tomographer::Logger::DEBUG);
    logger.setDomainLevel("my_origin_class::mymethod()", Tomographer::Logger::LONGDEBUG);
    logger.setDomainLevel("my_origin_class::mymethod2()", Tomographer::Logger::WARNING);
    logger.setDomainLevel("my_other_origin_class::nested_class", Tomographer::Logger::ERROR);
  }
  ~fixture_originfilteredlogger() { }

  void produce_logs_with_origin(const char * origin) {
    logger.longdebug(origin, "longdebug level");
    logger.debug(origin, "debug level");
    logger.info(origin, "info level");
    logger.warning(origin, "warning level");
    logger.error(origin, "error level");
  }
};


// -----------------------------------------------------------------------------


template<typename LoggerType>
void a_method_which_accepts_a_dumb_logger(LoggerType & logger)
{
  logger.info("a_method_which_accepts_a_dumb_logger", "Here is an info message.");
}



class test_local_logger
{
  Tomographer::Logger::LocalLogger<Tomographer::Logger::BufferLogger> _logger;

public:

  test_local_logger(Tomographer::Logger::BufferLogger & logger)
    : _logger(TOMO_ORIGIN, logger)
  {
    _logger.longdebug("constructor!");
    _logger.debug("constructor!");
    _logger.info("constructor!");
    _logger.warning("constructor!");
    _logger.error("constructor!");
  }

  ~test_local_logger()
  {
    _logger.debug("destructor.");
    auto l = _logger.subLogger("[destructor]", "-");
    l.info("destructor.");
    auto l2 = l.subLogger("yo!");
    l2.info("depth two!");
  }

  void some_method()
  {
    Tomographer::Logger::LocalLogger<decltype(_logger)> logger(TOMO_ORIGIN, _logger);
    
    logger.debug("Hi there!");
    for (int k = 0; k < 10; ++k) {
      logger.longdebug("Number = %d", k);
    }
  }

  template<int I = 1342, char c = 'Z', typename T = std::string>
  void tmpl(const T & value = T("fdsk"))
  {
    auto l = _logger.subLogger(TOMO_ORIGIN);
    l.info("info message. Value = %s", value.c_str());

    auto l2 = l.subLogger("inner logger");
    l2.debug("I = %d, c=%c", I, c);
  }

  void test_parent_logger()
  {
    // _logger is the first-level LocalLogger, so base == parent
    BOOST_CHECK( & _logger.baseLogger() == & _logger.parentLogger() );
    a_method_which_accepts_a_dumb_logger(_logger.baseLogger());
    a_method_which_accepts_a_dumb_logger(_logger.parentLogger());

    auto logger = _logger.subLogger(TOMO_ORIGIN);
    // logger is NOT a first-level LocalLogger, so base != parent , even types are different
    BOOST_CHECK( (void*) & logger.baseLogger() != (void *) & logger.parentLogger() );
    BOOST_CHECK( std::string(typeid(logger.baseLogger()).name()) !=
                 std::string(typeid(logger.parentLogger()).name()) );
    a_method_which_accepts_a_dumb_logger(logger.parentLogger());
  }
};


template<typename BaseLoggerType>
void test_locallogger_function(int value, BaseLoggerType & b)
{
  auto logger = makeLocalLogger(TOMO_ORIGIN, b);
  logger.debug("value is %d", value);
  
  auto some_callback = [&](std::string some_other_value) {
    auto innerlogger = logger.subLogger("some_callback[lambda]");
    innerlogger.debug([&](std::ostream& str) {
	str << "Inside callback: " << some_other_value;
      });
  };
  some_callback("42");
}




// #############################################################################


BOOST_AUTO_TEST_SUITE(test_tools_loggers)


BOOST_FIXTURE_TEST_SUITE(bufferlogger, fixture_bufferlogger)

BOOST_AUTO_TEST_CASE(basiclogging)
{
  logger.longdebug("origin1", "long debug message");
  logger.debug("origin2", "debug message");
  logger.info("origin3", "info message");
  logger.warning("origin4", "warning message");
  logger.error("origin5", "error message");

  std::string contents = logger.getContents();
  BOOST_CHECK_EQUAL(contents, std::string("[origin2] debug message\n"
                                          "[origin3] info message\n"
                                          "[origin4] warning message\n"
                                          "[origin5] error message\n"));
}

BOOST_AUTO_TEST_CASE(formats)
{
  const char * pstr1 = "test string";
  std::string str2 = "another test string";
  logger.debug("origin", "int: %d, uint: %u, double: %5.2f, strings: \"%s\", \"%s\"",
               1, 2, 3.141592653589, pstr1, str2.c_str());
  std::string contents1 = logger.getContents();
  BOOST_CHECK_EQUAL(contents1, std::string(
                        "[origin] int: 1, uint: 2, double:  3.14, "
                        "strings: \"test string\", \"another test string\"\n"
                        ));

  // ---------------
  logger.clear();

  std::string preformatted_str = "->\tget the contents of the internal buffer. More...";
  logger.debug("origin", preformatted_str);
  std::string contents2 = logger.getContents();
  BOOST_CHECK_EQUAL(contents2, std::string(
                        "[origin] "+preformatted_str+"\n"
                        ));


  // ---------------
  logger.clear();

  int value = 42;
  Eigen::Matrix2d mat = Eigen::Matrix2d::Identity();
  logger.debug("origin", [&](std::ostream & str) {
      str << "C++ stream output: value = " << value << ". The 2x2 identity matrix is =\n" << mat;
    });
  std::string contents3 = logger.getContents();
  BOOST_CHECK_EQUAL(contents3, std::string(
                        "[origin] C++ stream output: value = 42. The 2x2 identity matrix is =\n"
                        "1 0\n0 1\n"
                        ));
}

BOOST_AUTO_TEST_CASE(levelfunc)
{
  BOOST_CHECK_EQUAL(logger.level(), Tomographer::Logger::DEBUG);
  BOOST_CHECK(!logger.enabledFor(Tomographer::Logger::LONGDEBUG));
  BOOST_CHECK(logger.enabledFor(Tomographer::Logger::DEBUG));
  BOOST_CHECK(logger.enabledFor(Tomographer::Logger::INFO));
  BOOST_CHECK(logger.enabledFor(Tomographer::Logger::WARNING));
  BOOST_CHECK(logger.enabledFor(Tomographer::Logger::ERROR));

  Tomographer::Logger::BufferLogger logger2(Tomographer::Logger::WARNING);
  BOOST_CHECK_EQUAL(logger2.level(), Tomographer::Logger::WARNING);
  BOOST_CHECK(!logger2.enabledFor(Tomographer::Logger::LONGDEBUG));
  BOOST_CHECK(!logger2.enabledFor(Tomographer::Logger::DEBUG));
  BOOST_CHECK(!logger2.enabledFor(Tomographer::Logger::INFO));
  BOOST_CHECK(logger2.enabledFor(Tomographer::Logger::WARNING));
  BOOST_CHECK(logger2.enabledFor(Tomographer::Logger::ERROR));
}

BOOST_AUTO_TEST_CASE(optimized_formatting)
{
  // formatting should only occur if the message is going to be emitted.
  Tomographer::Logger::BufferLogger logger2(Tomographer::Logger::ERROR);

  // GCC is smart enough to detect that we're attempting to do something fishy, so silence
  // the compiler warning:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"

  // '%n' accepts a pointer to an int variable, where printf() will store the amount of
  // chars printed. Use this with NULL parameter in order to assert that printf() is not
  // called.
  logger2.info("origin()", "This will SEGFAULT if attempted to format: %n", (int*)NULL);

#pragma GCC diagnostic pop

  bool lambda_called = false;
  logger2.warning("origin()", [&lambda_called](std::ostream & str) {
      lambda_called = true;
      str << "log message here";
    });

  BOOST_CHECK_EQUAL(lambda_called, false);
  BOOST_CHECK_EQUAL(logger2.getContents(), std::string(""));
}



BOOST_AUTO_TEST_SUITE_END() // bufferlogger



// -----------------------------------------------------------------------------

  
// can't call DEFINE_DUMMY_LOGGER_WITH_TRAITS here, because BOOST test suites are in fact
// class or namespaces declarations, and here we're inside a class/namespace declaration.

BOOST_AUTO_TEST_SUITE(loggertraits)

BOOST_AUTO_TEST_CASE(helpers)
{
  using namespace Tomographer::Logger;
  
  MY_BOOST_CHECK(isAtLeastOfSeverity(ERROR, ERROR));
  MY_BOOST_CHECK(isAtLeastOfSeverity(ERROR, WARNING));
  MY_BOOST_CHECK(isAtLeastOfSeverity(ERROR, INFO));
  MY_BOOST_CHECK(isAtLeastOfSeverity(ERROR, DEBUG));
  MY_BOOST_CHECK(isAtLeastOfSeverity(ERROR, LONGDEBUG));

  MY_BOOST_CHECK(! isAtLeastOfSeverity(WARNING, ERROR));
  MY_BOOST_CHECK(isAtLeastOfSeverity(WARNING, WARNING));
  MY_BOOST_CHECK(isAtLeastOfSeverity(WARNING, INFO));
  MY_BOOST_CHECK(isAtLeastOfSeverity(WARNING, DEBUG));
  MY_BOOST_CHECK(isAtLeastOfSeverity(WARNING, LONGDEBUG));

  MY_BOOST_CHECK(! isAtLeastOfSeverity(INFO, ERROR));
  MY_BOOST_CHECK(! isAtLeastOfSeverity(INFO, WARNING));
  MY_BOOST_CHECK(isAtLeastOfSeverity(INFO, INFO));
  MY_BOOST_CHECK(isAtLeastOfSeverity(INFO, DEBUG));
  MY_BOOST_CHECK(isAtLeastOfSeverity(INFO, LONGDEBUG));

  MY_BOOST_CHECK(! isAtLeastOfSeverity(DEBUG, ERROR));
  MY_BOOST_CHECK(! isAtLeastOfSeverity(DEBUG, WARNING));
  MY_BOOST_CHECK(! isAtLeastOfSeverity(DEBUG, INFO));
  MY_BOOST_CHECK(isAtLeastOfSeverity(DEBUG, DEBUG));
  MY_BOOST_CHECK(isAtLeastOfSeverity(DEBUG, LONGDEBUG));
  
  MY_BOOST_CHECK(! isAtLeastOfSeverity(LONGDEBUG, ERROR));
  MY_BOOST_CHECK(! isAtLeastOfSeverity(LONGDEBUG, WARNING));
  MY_BOOST_CHECK(! isAtLeastOfSeverity(LONGDEBUG, INFO));
  MY_BOOST_CHECK(! isAtLeastOfSeverity(LONGDEBUG, DEBUG));
  MY_BOOST_CHECK(isAtLeastOfSeverity(LONGDEBUG, LONGDEBUG));

  MY_BOOST_CHECK(! isAtLeastOfSeverity(LOWEST_SEVERITY_LEVEL, ERROR));
  MY_BOOST_CHECK(! isAtLeastOfSeverity(LOWEST_SEVERITY_LEVEL, WARNING));
  MY_BOOST_CHECK(! isAtLeastOfSeverity(LOWEST_SEVERITY_LEVEL, INFO));
  MY_BOOST_CHECK(! isAtLeastOfSeverity(LOWEST_SEVERITY_LEVEL, DEBUG));
  MY_BOOST_CHECK(! isAtLeastOfSeverity(LOWEST_SEVERITY_LEVEL, LONGDEBUG));
  
  MY_BOOST_CHECK(StaticIsAtLeastOfSeverity<ERROR, ERROR>::value);
  MY_BOOST_CHECK(StaticIsAtLeastOfSeverity<ERROR, WARNING>::value);
  MY_BOOST_CHECK(StaticIsAtLeastOfSeverity<ERROR, INFO>::value);
  MY_BOOST_CHECK(StaticIsAtLeastOfSeverity<ERROR, DEBUG>::value);
  MY_BOOST_CHECK(StaticIsAtLeastOfSeverity<ERROR, LONGDEBUG>::value);

  MY_BOOST_CHECK(! StaticIsAtLeastOfSeverity<WARNING, ERROR>::value);
  MY_BOOST_CHECK(StaticIsAtLeastOfSeverity<WARNING, WARNING>::value);
  MY_BOOST_CHECK(StaticIsAtLeastOfSeverity<WARNING, INFO>::value);
  MY_BOOST_CHECK(StaticIsAtLeastOfSeverity<WARNING, DEBUG>::value);
  MY_BOOST_CHECK(StaticIsAtLeastOfSeverity<WARNING, LONGDEBUG>::value);

  MY_BOOST_CHECK(! StaticIsAtLeastOfSeverity<INFO, ERROR>::value);
  MY_BOOST_CHECK(! StaticIsAtLeastOfSeverity<INFO, WARNING>::value);
  MY_BOOST_CHECK(StaticIsAtLeastOfSeverity<INFO, INFO>::value);
  MY_BOOST_CHECK(StaticIsAtLeastOfSeverity<INFO, DEBUG>::value);
  MY_BOOST_CHECK(StaticIsAtLeastOfSeverity<INFO, LONGDEBUG>::value);

  MY_BOOST_CHECK(! StaticIsAtLeastOfSeverity<DEBUG, ERROR>::value);
  MY_BOOST_CHECK(! StaticIsAtLeastOfSeverity<DEBUG, WARNING>::value);
  MY_BOOST_CHECK(! StaticIsAtLeastOfSeverity<DEBUG, INFO>::value);
  MY_BOOST_CHECK(StaticIsAtLeastOfSeverity<DEBUG, DEBUG>::value);
  MY_BOOST_CHECK(StaticIsAtLeastOfSeverity<DEBUG, LONGDEBUG>::value);
  
  MY_BOOST_CHECK(! StaticIsAtLeastOfSeverity<LONGDEBUG, ERROR>::value);
  MY_BOOST_CHECK(! StaticIsAtLeastOfSeverity<LONGDEBUG, WARNING>::value);
  MY_BOOST_CHECK(! StaticIsAtLeastOfSeverity<LONGDEBUG, INFO>::value);
  MY_BOOST_CHECK(! StaticIsAtLeastOfSeverity<LONGDEBUG, DEBUG>::value);
  MY_BOOST_CHECK(StaticIsAtLeastOfSeverity<LONGDEBUG, LONGDEBUG>::value);

  MY_BOOST_CHECK(! StaticIsAtLeastOfSeverity<LOWEST_SEVERITY_LEVEL, ERROR>::value);
  MY_BOOST_CHECK(! StaticIsAtLeastOfSeverity<LOWEST_SEVERITY_LEVEL, WARNING>::value);
  MY_BOOST_CHECK(! StaticIsAtLeastOfSeverity<LOWEST_SEVERITY_LEVEL, INFO>::value);
  MY_BOOST_CHECK(! StaticIsAtLeastOfSeverity<LOWEST_SEVERITY_LEVEL, DEBUG>::value);
  MY_BOOST_CHECK(! StaticIsAtLeastOfSeverity<LOWEST_SEVERITY_LEVEL, LONGDEBUG>::value);
  
}

BOOST_AUTO_TEST_CASE(minseverity)
{
  std::string recorded;
  DummyLoggerMinSeverity logger(Tomographer::Logger::DEBUG, &recorded);
  
  BOOST_CHECK_EQUAL(Tomographer::Logger::LoggerTraits<DummyLoggerMinSeverity>::StaticMinimumSeverityLevel,
                    Tomographer::Logger::WARNING); // what we declared above

  BOOST_CHECK(DummyLoggerMinSeverity::staticallyEnabledFor<Tomographer::Logger::ERROR>());
  BOOST_CHECK(DummyLoggerMinSeverity::staticallyEnabledFor<Tomographer::Logger::WARNING>());
  BOOST_CHECK(!DummyLoggerMinSeverity::staticallyEnabledFor<Tomographer::Logger::INFO>());
  BOOST_CHECK(!DummyLoggerMinSeverity::staticallyEnabledFor<Tomographer::Logger::DEBUG>());
  BOOST_CHECK(!DummyLoggerMinSeverity::staticallyEnabledFor<Tomographer::Logger::LONGDEBUG>());
  BOOST_CHECK(DummyLoggerMinSeverity::staticallyEnabledFor(Tomographer::Logger::ERROR));
  BOOST_CHECK(DummyLoggerMinSeverity::staticallyEnabledFor(Tomographer::Logger::WARNING));
  BOOST_CHECK(!DummyLoggerMinSeverity::staticallyEnabledFor(Tomographer::Logger::INFO));
  BOOST_CHECK(!DummyLoggerMinSeverity::staticallyEnabledFor(Tomographer::Logger::DEBUG));
  BOOST_CHECK(!DummyLoggerMinSeverity::staticallyEnabledFor(Tomographer::Logger::LONGDEBUG));

  BOOST_CHECK(logger.enabledFor(Tomographer::Logger::ERROR));
  BOOST_CHECK(logger.enabledFor(Tomographer::Logger::WARNING));
  BOOST_CHECK(!logger.enabledFor(Tomographer::Logger::INFO));
  BOOST_CHECK(!logger.enabledFor(Tomographer::Logger::DEBUG));
  BOOST_CHECK(!logger.enabledFor(Tomographer::Logger::LONGDEBUG));

  logger.longdebug("origin", "message1");
  logger.debug("origin", "message2");
  logger.info("origin", "message3");
  logger.warning("origin", "message4");
  logger.error("origin", "message5");

  BOOST_CHECK_EQUAL(recorded,
                    Tomographer::Tools::fmts("emitLog(level=%d, origin=\"%s\", msg=\"%s\")\n",
                                             Tomographer::Logger::WARNING, "origin", "message4") + 
                    Tomographer::Tools::fmts("emitLog(level=%d, origin=\"%s\", msg=\"%s\")\n",
                                             Tomographer::Logger::ERROR, "origin", "message5")
      );

  // also check that a non-statically limited level is statically enabled for all
  BOOST_CHECK(Tomographer::Logger::FileLogger::staticallyEnabledFor(Tomographer::Logger::ERROR));
  BOOST_CHECK(Tomographer::Logger::FileLogger::staticallyEnabledFor(Tomographer::Logger::WARNING));
  BOOST_CHECK(Tomographer::Logger::FileLogger::staticallyEnabledFor(Tomographer::Logger::INFO));
  BOOST_CHECK(Tomographer::Logger::FileLogger::staticallyEnabledFor(Tomographer::Logger::DEBUG));
  BOOST_CHECK(Tomographer::Logger::FileLogger::staticallyEnabledFor(Tomographer::Logger::LONGDEBUG));
  BOOST_CHECK(Tomographer::Logger::FileLogger::staticallyEnabledFor(Tomographer::Logger::LOWEST_SEVERITY_LEVEL));
}

BOOST_AUTO_TEST_CASE(ownlevel)
{
  {
    std::string recorded;
    DummyLoggerOwnGetLevel logger(Tomographer::Logger::DEBUG, &recorded);
    
    // This is what we declared above in DummyLoggerImplementation:
    BOOST_CHECK_EQUAL(logger.level(), Tomographer::Logger::INFO) ;

    BOOST_CHECK(logger.enabledFor(Tomographer::Logger::ERROR));
    BOOST_CHECK(logger.enabledFor(Tomographer::Logger::WARNING));
    BOOST_CHECK(logger.enabledFor(Tomographer::Logger::INFO));
    BOOST_CHECK(!logger.enabledFor(Tomographer::Logger::DEBUG));
    BOOST_CHECK(!logger.enabledFor(Tomographer::Logger::LONGDEBUG));
  }
  {
    std::string recorded;
    DummyLoggerOwnGetLevel logger(Tomographer::Logger::DEBUG, &recorded);
    
    logger.longdebug("origin", "message1");
    logger.debug("origin", "message2");
    logger.info("origin", "message3");
    logger.warning("origin", "message4");
    logger.error("origin", "message5");

    BOOST_CHECK_EQUAL(recorded, std::string() +
                      "level()\n" +
                      "level()\n" +
                      "level()\n" +
                      Tomographer::Tools::fmts("emitLog(level=%d, origin=\"%s\", msg=\"%s\")\n",
                                               Tomographer::Logger::INFO, "origin", "message3") + 
                      "level()\n" +
                      Tomographer::Tools::fmts("emitLog(level=%d, origin=\"%s\", msg=\"%s\")\n",
                                               Tomographer::Logger::WARNING, "origin", "message4") + 
                      "level()\n" +
                      Tomographer::Tools::fmts("emitLog(level=%d, origin=\"%s\", msg=\"%s\")\n",
                                               Tomographer::Logger::ERROR, "origin", "message5")
        );
  }
}


BOOST_AUTO_TEST_CASE(originfilter)
{
  std::string recorded;
  DummyLoggerOriginFilter logger(Tomographer::Logger::INFO, &recorded);

  logger.longdebug("some::origin()", "message1");
  logger.debug("some::origin()", "message2");
  logger.info("some::origin()", "message3");
  logger.warning("some::origin()", "message4");
  logger.error("some::origin()", "message5");

  logger.longdebug("origin_which_passes_filter()", "message1");
  logger.debug("origin_which_passes_filter()", "message2");
  logger.info("origin_which_passes_filter()", "message3");
  logger.warning("origin_which::DoesNot::pass_filter()", "message4");
  logger.error("origin_which_passes_filter()", "message5");

  BOOST_CHECK_EQUAL(recorded, std::string() +
                    // checks not reached, because message already discarded by log level:
                    // Tomographer::Tools::fmts("filterByOrigin(level=%d, origin=\"%s\")\n",
                    //                          Tomographer::Logger::LONGDEBUG, "some::origin()") +
                    // Tomographer::Tools::fmts("filterByOrigin(level=%d, origin=\"%s\")\n",
                    //                          Tomographer::Logger::DEBUG, "some::origin()") +
                    Tomographer::Tools::fmts("filterByOrigin(level=%d, origin=\"%s\")\n",
                                             Tomographer::Logger::INFO, "some::origin()") +
                    Tomographer::Tools::fmts("filterByOrigin(level=%d, origin=\"%s\")\n",
                                             Tomographer::Logger::WARNING, "some::origin()") +
                    Tomographer::Tools::fmts("filterByOrigin(level=%d, origin=\"%s\")\n",
                                             Tomographer::Logger::ERROR, "some::origin()") +

                    // Tomographer::Tools::fmts("filterByOrigin(level=%d, origin=\"%s\")\n",
                    //                          Tomographer::Logger::LONGDEBUG, "origin_which_passes_filter()") +
                    // message not emitted because of logger level
                    // Tomographer::Tools::fmts("filterByOrigin(level=%d, origin=\"%s\")\n",
                    //                          Tomographer::Logger::DEBUG, "origin_which_passes_filter()") +
                    // message not emitted because of logger level
                    Tomographer::Tools::fmts("filterByOrigin(level=%d, origin=\"%s\")\n",
                                             Tomographer::Logger::INFO, "origin_which_passes_filter()") +
                    Tomographer::Tools::fmts("emitLog(level=%d, origin=\"%s\", msg=\"%s\")\n",
                                             Tomographer::Logger::INFO, "origin_which_passes_filter()",
                                             "message3") + 
                    Tomographer::Tools::fmts("filterByOrigin(level=%d, origin=\"%s\")\n",
                                             Tomographer::Logger::WARNING, "origin_which::DoesNot::pass_filter()") +
                    // not emitted because of bad origin
                    Tomographer::Tools::fmts("filterByOrigin(level=%d, origin=\"%s\")\n",
                                             Tomographer::Logger::ERROR, "origin_which_passes_filter()") +
                    Tomographer::Tools::fmts("emitLog(level=%d, origin=\"%s\", msg=\"%s\")\n",
                                             Tomographer::Logger::ERROR, "origin_which_passes_filter()",
                                             "message5")
        );
}

BOOST_AUTO_TEST_SUITE_END() // loggertraits




// -----------------------------------------------------------------------------



BOOST_FIXTURE_TEST_SUITE(originfilteredlogger, fixture_originfilteredlogger)

BOOST_AUTO_TEST_CASE(origin1)
{
  produce_logs_with_origin("my_origin_class");
  BOOST_CHECK_EQUAL(
      buflog.getContents(),
      "[my_origin_class] debug level\n"
      "[my_origin_class] info level\n"
      "[my_origin_class] warning level\n"
      "[my_origin_class] error level\n"
      );
}

BOOST_AUTO_TEST_CASE(origin2)
{
  produce_logs_with_origin("my_origin_class::mymethod()");
  BOOST_CHECK_EQUAL(
      buflog.getContents(),
      "[my_origin_class::mymethod()] longdebug level\n"
      "[my_origin_class::mymethod()] debug level\n"
      "[my_origin_class::mymethod()] info level\n"
      "[my_origin_class::mymethod()] warning level\n"
      "[my_origin_class::mymethod()] error level\n"
      );
}

BOOST_AUTO_TEST_CASE(origin3)
{
  produce_logs_with_origin("my_origin_class::mymethod2()");
  BOOST_CHECK_EQUAL(
      buflog.getContents(),
      "[my_origin_class::mymethod2()] warning level\n"
      "[my_origin_class::mymethod2()] error level\n"
      );
}

BOOST_AUTO_TEST_CASE(origin4)
{
  produce_logs_with_origin("my_other_origin_class::nested_class");
  BOOST_CHECK_EQUAL(
      buflog.getContents(),
      "[my_other_origin_class::nested_class] error level\n"
      );
}

BOOST_AUTO_TEST_CASE(origin_norule)
{
  produce_logs_with_origin("origin::with::no::rule::set()");
  BOOST_CHECK_EQUAL(
      buflog.getContents(),
      "[origin::with::no::rule::set()] info level\n"
      "[origin::with::no::rule::set()] warning level\n"
      "[origin::with::no::rule::set()] error level\n"
      );
}


BOOST_AUTO_TEST_SUITE_END() // originfilteredlogger



// -----------------------------------------------------------------------------


BOOST_AUTO_TEST_SUITE(locallogger)

BOOST_AUTO_TEST_CASE(basic)
{
  Tomographer::Logger::BufferLogger b(Tomographer::Logger::LONGDEBUG);

  {
    test_local_logger tst(b);
    tst.some_method();
    tst.tmpl();
    tst.test_parent_logger();
  }
  {
    test_locallogger_function(10, b);
  }

  std::string contents = b.getContents();

  BOOST_MESSAGE(contents);
  BOOST_CHECK_EQUAL(contents,
                    "[test_local_logger] constructor!\n"
                    "[test_local_logger] constructor!\n"
                    "[test_local_logger] constructor!\n"
                    "[test_local_logger] constructor!\n"
                    "[test_local_logger] constructor!\n"
                    "[test_local_logger::some_method()] Hi there!\n"
                    "[test_local_logger::some_method()] Number = 0\n"
                    "[test_local_logger::some_method()] Number = 1\n"
                    "[test_local_logger::some_method()] Number = 2\n"
                    "[test_local_logger::some_method()] Number = 3\n"
                    "[test_local_logger::some_method()] Number = 4\n"
                    "[test_local_logger::some_method()] Number = 5\n"
                    "[test_local_logger::some_method()] Number = 6\n"
                    "[test_local_logger::some_method()] Number = 7\n"
                    "[test_local_logger::some_method()] Number = 8\n"
                    "[test_local_logger::some_method()] Number = 9\n"
                    "[test_local_logger::tmpl()] info message. Value = fdsk\n"
                    "[test_local_logger::tmpl()/inner logger] I = 1342, c=Z\n"
                    "[a_method_which_accepts_a_dumb_logger] Here is an info message.\n"
                    "[a_method_which_accepts_a_dumb_logger] Here is an info message.\n"
                    "[a_method_which_accepts_a_dumb_logger] Here is an info message.\n"
                    "[test_local_logger] destructor.\n"
                    "[test_local_logger::[destructor]] destructor.\n"
                    "[test_local_logger::[destructor]-yo!] depth two!\n"
		    "[test_locallogger_function()] value is 10\n"
		    "[test_locallogger_function()/some_callback[lambda]] Inside callback: 42\n"
      );

  // ............
}



BOOST_AUTO_TEST_SUITE_END() // locallogger



// #############################################################################

BOOST_AUTO_TEST_SUITE_END() // test_tools_loggers


