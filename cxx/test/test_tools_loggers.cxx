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

// we want `eigen_assert()` to raise an `eigen_assert_exception` here
#include <tomographer2/tools/eigen_assert_exception.h>

#include <Eigen/Core>

#include <tomographer2/tools/loggers.h>

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>


// tolerance, in *PERCENT*
const double tol_percent = 1e-12;
const double tol = tol_percent * 0.01;









// #############################################################################

// Helpers, dummy classes etc. for testing.


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

  inline void emit_log(int level, const char * origin, const std::string & msg)
  {
    std::ostringstream record_calls;
    record_calls << "emit_log(level=" << level << ", origin=\"" << origin << "\", msg=\"" << msg << "\")\n";
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

  inline bool filter_by_origin(int level, const char * origin) const
  {
    std::ostringstream record_calls;
    record_calls << "filter_by_origin(level=" << level << ", origin=\"" << origin << "\")\n";
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




// #############################################################################


BOOST_AUTO_TEST_SUITE(test_loggers);


// -----------------------------------------------------------------------------


struct fixture_bufferlogger {
  fixture_bufferlogger() : logger(Tomographer::Logger::DEBUG) { }
  ~fixture_bufferlogger() { }

  Tomographer::Logger::BufferLogger logger;
};

BOOST_FIXTURE_TEST_SUITE(bufferlogger, fixture_bufferlogger);

BOOST_AUTO_TEST_CASE(basiclogging)
{
  logger.longdebug("origin1", "long debug message");
  logger.debug("origin2", "debug message");
  logger.info("origin3", "info message");
  logger.warning("origin4", "warning message");
  logger.error("origin5", "error message");

  std::string contents = logger.get_contents();
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
  std::string contents1 = logger.get_contents();
  BOOST_CHECK_EQUAL(contents1, std::string(
                        "[origin] int: 1, uint: 2, double:  3.14, "
                        "strings: \"test string\", \"another test string\"\n"
                        ));

  // ---------------
  logger.clear();

  std::string preformatted_str = "->\tget the contents of the internal buffer. More...";
  logger.debug("origin", preformatted_str);
  std::string contents2 = logger.get_contents();
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
  std::string contents3 = logger.get_contents();
  BOOST_CHECK_EQUAL(contents3, std::string(
                        "[origin] C++ stream output: value = 42. The 2x2 identity matrix is =\n"
                        "1 0\n0 1\n"
                        ));
}

BOOST_AUTO_TEST_CASE(levelfunc)
{
  BOOST_CHECK_EQUAL(logger.level(), Tomographer::Logger::DEBUG);
  BOOST_CHECK(!logger.enabled_for(Tomographer::Logger::LONGDEBUG));
  BOOST_CHECK(logger.enabled_for(Tomographer::Logger::DEBUG));
  BOOST_CHECK(logger.enabled_for(Tomographer::Logger::INFO));
  BOOST_CHECK(logger.enabled_for(Tomographer::Logger::WARNING));
  BOOST_CHECK(logger.enabled_for(Tomographer::Logger::ERROR));

  Tomographer::Logger::BufferLogger logger2(Tomographer::Logger::WARNING);
  BOOST_CHECK_EQUAL(logger2.level(), Tomographer::Logger::WARNING);
  BOOST_CHECK(!logger2.enabled_for(Tomographer::Logger::LONGDEBUG));
  BOOST_CHECK(!logger2.enabled_for(Tomographer::Logger::DEBUG));
  BOOST_CHECK(!logger2.enabled_for(Tomographer::Logger::INFO));
  BOOST_CHECK(logger2.enabled_for(Tomographer::Logger::WARNING));
  BOOST_CHECK(logger2.enabled_for(Tomographer::Logger::ERROR));
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
  BOOST_CHECK_EQUAL(logger2.get_contents(), std::string(""));
}



BOOST_AUTO_TEST_SUITE_END(); // test_bufferlogger




// -----------------------------------------------------------------------------

  
// can't call DEFINE_DUMMY_LOGGER_WITH_TRAITS here, because BOOST test suites are in fact
// class or namespaces declarations, and here we're inside a class/namespace declaration.


BOOST_AUTO_TEST_SUITE(loggertraits);

// needed because if there is a single comma in the string passed to BOOST_CHECK, then the
// preprocessor thinks it's two macro arguments (!!)
#define MY_BOOST_CHECK(...) { auto check = [&]() -> bool { return __VA_ARGS__; }; BOOST_CHECK(check()); }

BOOST_AUTO_TEST_CASE(helpers)
{
  using namespace Tomographer::Logger;
  
  MY_BOOST_CHECK(is_at_least_of_severity(ERROR, ERROR));
  MY_BOOST_CHECK(is_at_least_of_severity(ERROR, WARNING));
  MY_BOOST_CHECK(is_at_least_of_severity(ERROR, INFO));
  MY_BOOST_CHECK(is_at_least_of_severity(ERROR, DEBUG));
  MY_BOOST_CHECK(is_at_least_of_severity(ERROR, LONGDEBUG));

  MY_BOOST_CHECK(! is_at_least_of_severity(WARNING, ERROR));
  MY_BOOST_CHECK(is_at_least_of_severity(WARNING, WARNING));
  MY_BOOST_CHECK(is_at_least_of_severity(WARNING, INFO));
  MY_BOOST_CHECK(is_at_least_of_severity(WARNING, DEBUG));
  MY_BOOST_CHECK(is_at_least_of_severity(WARNING, LONGDEBUG));

  MY_BOOST_CHECK(! is_at_least_of_severity(INFO, ERROR));
  MY_BOOST_CHECK(! is_at_least_of_severity(INFO, WARNING));
  MY_BOOST_CHECK(is_at_least_of_severity(INFO, INFO));
  MY_BOOST_CHECK(is_at_least_of_severity(INFO, DEBUG));
  MY_BOOST_CHECK(is_at_least_of_severity(INFO, LONGDEBUG));

  MY_BOOST_CHECK(! is_at_least_of_severity(DEBUG, ERROR));
  MY_BOOST_CHECK(! is_at_least_of_severity(DEBUG, WARNING));
  MY_BOOST_CHECK(! is_at_least_of_severity(DEBUG, INFO));
  MY_BOOST_CHECK(is_at_least_of_severity(DEBUG, DEBUG));
  MY_BOOST_CHECK(is_at_least_of_severity(DEBUG, LONGDEBUG));
  
  MY_BOOST_CHECK(! is_at_least_of_severity(LONGDEBUG, ERROR));
  MY_BOOST_CHECK(! is_at_least_of_severity(LONGDEBUG, WARNING));
  MY_BOOST_CHECK(! is_at_least_of_severity(LONGDEBUG, INFO));
  MY_BOOST_CHECK(! is_at_least_of_severity(LONGDEBUG, DEBUG));
  MY_BOOST_CHECK(is_at_least_of_severity(LONGDEBUG, LONGDEBUG));

  MY_BOOST_CHECK(! is_at_least_of_severity(LOWEST_SEVERITY_LEVEL, ERROR));
  MY_BOOST_CHECK(! is_at_least_of_severity(LOWEST_SEVERITY_LEVEL, WARNING));
  MY_BOOST_CHECK(! is_at_least_of_severity(LOWEST_SEVERITY_LEVEL, INFO));
  MY_BOOST_CHECK(! is_at_least_of_severity(LOWEST_SEVERITY_LEVEL, DEBUG));
  MY_BOOST_CHECK(! is_at_least_of_severity(LOWEST_SEVERITY_LEVEL, LONGDEBUG));
  
  MY_BOOST_CHECK(static_is_at_least_of_severity<ERROR, ERROR>::value);
  MY_BOOST_CHECK(static_is_at_least_of_severity<ERROR, WARNING>::value);
  MY_BOOST_CHECK(static_is_at_least_of_severity<ERROR, INFO>::value);
  MY_BOOST_CHECK(static_is_at_least_of_severity<ERROR, DEBUG>::value);
  MY_BOOST_CHECK(static_is_at_least_of_severity<ERROR, LONGDEBUG>::value);

  MY_BOOST_CHECK(! static_is_at_least_of_severity<WARNING, ERROR>::value);
  MY_BOOST_CHECK(static_is_at_least_of_severity<WARNING, WARNING>::value);
  MY_BOOST_CHECK(static_is_at_least_of_severity<WARNING, INFO>::value);
  MY_BOOST_CHECK(static_is_at_least_of_severity<WARNING, DEBUG>::value);
  MY_BOOST_CHECK(static_is_at_least_of_severity<WARNING, LONGDEBUG>::value);

  MY_BOOST_CHECK(! static_is_at_least_of_severity<INFO, ERROR>::value);
  MY_BOOST_CHECK(! static_is_at_least_of_severity<INFO, WARNING>::value);
  MY_BOOST_CHECK(static_is_at_least_of_severity<INFO, INFO>::value);
  MY_BOOST_CHECK(static_is_at_least_of_severity<INFO, DEBUG>::value);
  MY_BOOST_CHECK(static_is_at_least_of_severity<INFO, LONGDEBUG>::value);

  MY_BOOST_CHECK(! static_is_at_least_of_severity<DEBUG, ERROR>::value);
  MY_BOOST_CHECK(! static_is_at_least_of_severity<DEBUG, WARNING>::value);
  MY_BOOST_CHECK(! static_is_at_least_of_severity<DEBUG, INFO>::value);
  MY_BOOST_CHECK(static_is_at_least_of_severity<DEBUG, DEBUG>::value);
  MY_BOOST_CHECK(static_is_at_least_of_severity<DEBUG, LONGDEBUG>::value);
  
  MY_BOOST_CHECK(! static_is_at_least_of_severity<LONGDEBUG, ERROR>::value);
  MY_BOOST_CHECK(! static_is_at_least_of_severity<LONGDEBUG, WARNING>::value);
  MY_BOOST_CHECK(! static_is_at_least_of_severity<LONGDEBUG, INFO>::value);
  MY_BOOST_CHECK(! static_is_at_least_of_severity<LONGDEBUG, DEBUG>::value);
  MY_BOOST_CHECK(static_is_at_least_of_severity<LONGDEBUG, LONGDEBUG>::value);

  MY_BOOST_CHECK(! static_is_at_least_of_severity<LOWEST_SEVERITY_LEVEL, ERROR>::value);
  MY_BOOST_CHECK(! static_is_at_least_of_severity<LOWEST_SEVERITY_LEVEL, WARNING>::value);
  MY_BOOST_CHECK(! static_is_at_least_of_severity<LOWEST_SEVERITY_LEVEL, INFO>::value);
  MY_BOOST_CHECK(! static_is_at_least_of_severity<LOWEST_SEVERITY_LEVEL, DEBUG>::value);
  MY_BOOST_CHECK(! static_is_at_least_of_severity<LOWEST_SEVERITY_LEVEL, LONGDEBUG>::value);
  
}

BOOST_AUTO_TEST_CASE(minseverity)
{
  std::string recorded;
  DummyLoggerMinSeverity logger(Tomographer::Logger::DEBUG, &recorded);
  
  BOOST_CHECK_EQUAL(Tomographer::Logger::LoggerTraits<DummyLoggerMinSeverity>::StaticMinimumSeverityLevel,
                    Tomographer::Logger::WARNING); // what we declared above

  BOOST_CHECK(DummyLoggerMinSeverity::statically_enabled_for<Tomographer::Logger::ERROR>());
  BOOST_CHECK(DummyLoggerMinSeverity::statically_enabled_for<Tomographer::Logger::WARNING>());
  BOOST_CHECK(!DummyLoggerMinSeverity::statically_enabled_for<Tomographer::Logger::INFO>());
  BOOST_CHECK(!DummyLoggerMinSeverity::statically_enabled_for<Tomographer::Logger::DEBUG>());
  BOOST_CHECK(!DummyLoggerMinSeverity::statically_enabled_for<Tomographer::Logger::LONGDEBUG>());
  BOOST_CHECK(DummyLoggerMinSeverity::statically_enabled_for(Tomographer::Logger::ERROR));
  BOOST_CHECK(DummyLoggerMinSeverity::statically_enabled_for(Tomographer::Logger::WARNING));
  BOOST_CHECK(!DummyLoggerMinSeverity::statically_enabled_for(Tomographer::Logger::INFO));
  BOOST_CHECK(!DummyLoggerMinSeverity::statically_enabled_for(Tomographer::Logger::DEBUG));
  BOOST_CHECK(!DummyLoggerMinSeverity::statically_enabled_for(Tomographer::Logger::LONGDEBUG));

  BOOST_CHECK(logger.enabled_for(Tomographer::Logger::ERROR));
  BOOST_CHECK(logger.enabled_for(Tomographer::Logger::WARNING));
  BOOST_CHECK(!logger.enabled_for(Tomographer::Logger::INFO));
  BOOST_CHECK(!logger.enabled_for(Tomographer::Logger::DEBUG));
  BOOST_CHECK(!logger.enabled_for(Tomographer::Logger::LONGDEBUG));

  logger.longdebug("origin", "message1");
  logger.debug("origin", "message2");
  logger.info("origin", "message3");
  logger.warning("origin", "message4");
  logger.error("origin", "message5");

  BOOST_CHECK_EQUAL(recorded,
                    Tomographer::Tools::fmts("emit_log(level=%d, origin=\"%s\", msg=\"%s\")\n",
                                             Tomographer::Logger::WARNING, "origin", "message4") + 
                    Tomographer::Tools::fmts("emit_log(level=%d, origin=\"%s\", msg=\"%s\")\n",
                                             Tomographer::Logger::ERROR, "origin", "message5")
      );

  // also check that a non-statically limited level is statically enabled for all
  BOOST_CHECK(Tomographer::Logger::FileLogger::statically_enabled_for(Tomographer::Logger::ERROR));
  BOOST_CHECK(Tomographer::Logger::FileLogger::statically_enabled_for(Tomographer::Logger::WARNING));
  BOOST_CHECK(Tomographer::Logger::FileLogger::statically_enabled_for(Tomographer::Logger::INFO));
  BOOST_CHECK(Tomographer::Logger::FileLogger::statically_enabled_for(Tomographer::Logger::DEBUG));
  BOOST_CHECK(Tomographer::Logger::FileLogger::statically_enabled_for(Tomographer::Logger::LONGDEBUG));
  BOOST_CHECK(Tomographer::Logger::FileLogger::statically_enabled_for(Tomographer::Logger::LOWEST_SEVERITY_LEVEL));
}

BOOST_AUTO_TEST_CASE(ownlevel)
{
  {
    std::string recorded;
    DummyLoggerOwnGetLevel logger(Tomographer::Logger::DEBUG, &recorded);
    
    // This is what we declared above in DummyLoggerImplementation:
    BOOST_CHECK_EQUAL(logger.level(), Tomographer::Logger::INFO) ;

    BOOST_CHECK(logger.enabled_for(Tomographer::Logger::ERROR));
    BOOST_CHECK(logger.enabled_for(Tomographer::Logger::WARNING));
    BOOST_CHECK(logger.enabled_for(Tomographer::Logger::INFO));
    BOOST_CHECK(!logger.enabled_for(Tomographer::Logger::DEBUG));
    BOOST_CHECK(!logger.enabled_for(Tomographer::Logger::LONGDEBUG));
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
                      Tomographer::Tools::fmts("emit_log(level=%d, origin=\"%s\", msg=\"%s\")\n",
                                               Tomographer::Logger::INFO, "origin", "message3") + 
                      "level()\n" +
                      Tomographer::Tools::fmts("emit_log(level=%d, origin=\"%s\", msg=\"%s\")\n",
                                               Tomographer::Logger::WARNING, "origin", "message4") + 
                      "level()\n" +
                      Tomographer::Tools::fmts("emit_log(level=%d, origin=\"%s\", msg=\"%s\")\n",
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
                    // Tomographer::Tools::fmts("filter_by_origin(level=%d, origin=\"%s\")\n",
                    //                          Tomographer::Logger::LONGDEBUG, "some::origin()") +
                    // Tomographer::Tools::fmts("filter_by_origin(level=%d, origin=\"%s\")\n",
                    //                          Tomographer::Logger::DEBUG, "some::origin()") +
                    Tomographer::Tools::fmts("filter_by_origin(level=%d, origin=\"%s\")\n",
                                             Tomographer::Logger::INFO, "some::origin()") +
                    Tomographer::Tools::fmts("filter_by_origin(level=%d, origin=\"%s\")\n",
                                             Tomographer::Logger::WARNING, "some::origin()") +
                    Tomographer::Tools::fmts("filter_by_origin(level=%d, origin=\"%s\")\n",
                                             Tomographer::Logger::ERROR, "some::origin()") +

                    // Tomographer::Tools::fmts("filter_by_origin(level=%d, origin=\"%s\")\n",
                    //                          Tomographer::Logger::LONGDEBUG, "origin_which_passes_filter()") +
                    // message not emitted because of logger level
                    // Tomographer::Tools::fmts("filter_by_origin(level=%d, origin=\"%s\")\n",
                    //                          Tomographer::Logger::DEBUG, "origin_which_passes_filter()") +
                    // message not emitted because of logger level
                    Tomographer::Tools::fmts("filter_by_origin(level=%d, origin=\"%s\")\n",
                                             Tomographer::Logger::INFO, "origin_which_passes_filter()") +
                    Tomographer::Tools::fmts("emit_log(level=%d, origin=\"%s\", msg=\"%s\")\n",
                                             Tomographer::Logger::INFO, "origin_which_passes_filter()",
                                             "message3") + 
                    Tomographer::Tools::fmts("filter_by_origin(level=%d, origin=\"%s\")\n",
                                             Tomographer::Logger::WARNING, "origin_which::DoesNot::pass_filter()") +
                    // not emitted because of bad origin
                    Tomographer::Tools::fmts("filter_by_origin(level=%d, origin=\"%s\")\n",
                                             Tomographer::Logger::ERROR, "origin_which_passes_filter()") +
                    Tomographer::Tools::fmts("emit_log(level=%d, origin=\"%s\", msg=\"%s\")\n",
                                             Tomographer::Logger::ERROR, "origin_which_passes_filter()",
                                             "message5")
        );
}

BOOST_AUTO_TEST_SUITE_END();




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

BOOST_FIXTURE_TEST_SUITE(originfilteredlogger, fixture_originfilteredlogger);

BOOST_AUTO_TEST_CASE(origin1)
{
  produce_logs_with_origin("my_origin_class");
  BOOST_CHECK_EQUAL(
      buflog.get_contents(),
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
      buflog.get_contents(),
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
      buflog.get_contents(),
      "[my_origin_class::mymethod2()] warning level\n"
      "[my_origin_class::mymethod2()] error level\n"
      );
}

BOOST_AUTO_TEST_CASE(origin4)
{
  produce_logs_with_origin("my_other_origin_class::nested_class");
  BOOST_CHECK_EQUAL(
      buflog.get_contents(),
      "[my_other_origin_class::nested_class] error level\n"
      );
}

BOOST_AUTO_TEST_CASE(origin_norule)
{
  produce_logs_with_origin("origin::with::no::rule::set()");
  BOOST_CHECK_EQUAL(
      buflog.get_contents(),
      "[origin::with::no::rule::set()] info level\n"
      "[origin::with::no::rule::set()] warning level\n"
      "[origin::with::no::rule::set()] error level\n"
      );
}


BOOST_AUTO_TEST_SUITE_END();



// -----------------------------------------------------------------------------




class test_origin_logger
{
  Tomographer::Logger::LocalLogger<Tomographer::Logger::BufferLogger> _logger;

public:

  test_origin_logger(Tomographer::Logger::BufferLogger & logger)
    : _logger(TOMO_ORIGIN, logger)
  {
    _logger.longdebug("constructor!");
    _logger.debug("constructor!");
    _logger.info("constructor!");
    _logger.warning("constructor!");
    _logger.error("constructor!");
  }

  ~test_origin_logger()
  {
    _logger.debug("destructor.");
    auto l = _logger.sublogger("[destructor]", "-");
    l.info("destructor.");
    auto l2 = l.sublogger("yo!");
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
    auto l = _logger.sublogger(TOMO_ORIGIN);
    l.info("info message. Value = %s", value.c_str());

    auto l2 = l.sublogger("inner logger");
    l2.debug("I = %d, c=%c", I, c);
  }
};


BOOST_AUTO_TEST_CASE(local_logger)
{
  Tomographer::Logger::BufferLogger b(Tomographer::Logger::LONGDEBUG);

  {
    test_origin_logger tst(b);
    tst.some_method();
    tst.tmpl();
  }
  
  BOOST_CHECK_EQUAL(b.get_contents(),
                    "[test_origin_logger] constructor!\n"
                    "[test_origin_logger] constructor!\n"
                    "[test_origin_logger] constructor!\n"
                    "[test_origin_logger] constructor!\n"
                    "[test_origin_logger] constructor!\n"
                    "[test_origin_logger::some_method()] Hi there!\n"
                    "[test_origin_logger::some_method()] Number = 0\n"
                    "[test_origin_logger::some_method()] Number = 1\n"
                    "[test_origin_logger::some_method()] Number = 2\n"
                    "[test_origin_logger::some_method()] Number = 3\n"
                    "[test_origin_logger::some_method()] Number = 4\n"
                    "[test_origin_logger::some_method()] Number = 5\n"
                    "[test_origin_logger::some_method()] Number = 6\n"
                    "[test_origin_logger::some_method()] Number = 7\n"
                    "[test_origin_logger::some_method()] Number = 8\n"
                    "[test_origin_logger::some_method()] Number = 9\n"
                    "[test_origin_logger::tmpl()] info message. Value = fdsk\n"
                    "[test_origin_logger::tmpl()/inner logger] I = 1342, c=Z\n"
                    "[test_origin_logger] destructor.\n"
                    "[test_origin_logger::[destructor]] destructor.\n"
                    "[test_origin_logger::[destructor]-yo!] depth two!\n"
      );

  // ............
}





// #############################################################################

BOOST_AUTO_TEST_SUITE_END() // test_loggers



