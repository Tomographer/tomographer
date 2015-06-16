
#include <iostream>
#include <iomanip>
#include <stdexcept>

// we want `eigen_assert()` to raise an `eigen_assert_exception` here
#include <tomographer/tools/eigen_assert_exception.h>

#include <tomographer/tools/loggers.h>

#include <Eigen/Core>

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




DEFINE_DUMMY_LOGGER_WITH_TRAITS(DummyLoggerMinImportance, enum {
    IsThreadSafe = 0,
    StaticMinimumImportanceLevel = Tomographer::Logger::WARNING,
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




// =============================================================================

BOOST_AUTO_TEST_SUITE(test_loggers_basic);

// -----------------------------------------------------------------------------

struct fixture_bufferlogger {
  fixture_bufferlogger() : logger(Tomographer::Logger::DEBUG) { }
  ~fixture_bufferlogger() { }

  Tomographer::Logger::BufferLogger logger;
};

BOOST_FIXTURE_TEST_SUITE(test_bufferlogger, fixture_bufferlogger);

BOOST_AUTO_TEST_CASE(test_bufferlogger_logging)
{
  logger.longdebug("origin1", "long debug message");
  logger.debug("origin2", "debug message");
  logger.debug("origin3", "info message");
  logger.debug("origin4", "warning message");
  logger.debug("origin5", "error message");

  std::string contents = logger.get_contents();
  BOOST_CHECK_EQUAL(contents, std::string("[origin2] debug message\n"
                                          "[origin3] info message\n"
                                          "[origin4] warning message\n"
                                          "[origin5] error message\n"));
}

BOOST_AUTO_TEST_CASE(test_bufferlogger_formats)
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

BOOST_AUTO_TEST_CASE(test_bufferlogger_levelfunc)
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

BOOST_AUTO_TEST_CASE(test_bufferlogger_optimized_formatting)
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

// -----------------------------------------------------------------------------


BOOST_AUTO_TEST_SUITE_END(); // test_bufferlogger

  
// =============================================================================

// can't call DEFINE_DUMMY_LOGGER_WITH_TRAITS here, because BOOST test suites are in fact
// class or namespaces declarations, and here we're inside a class/namespace declaration.

BOOST_AUTO_TEST_SUITE(test_loggertraits);

// needed because if there is a single comma in the string passed to BOOST_CHECK, then the
// preprocessor thinks it's two macro arguments (!!)
#define MY_BOOST_CHECK(...) { auto check = [&]() -> bool { return __VA_ARGS__; }; BOOST_CHECK(check()); }

BOOST_AUTO_TEST_CASE(test_loggertraits_helpers)
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

  MY_BOOST_CHECK(! is_at_least_of_severity(DefaultLoggerTraits::NoStaticMinimumImportance, ERROR));
  MY_BOOST_CHECK(! is_at_least_of_severity(DefaultLoggerTraits::NoStaticMinimumImportance, WARNING));
  MY_BOOST_CHECK(! is_at_least_of_severity(DefaultLoggerTraits::NoStaticMinimumImportance, INFO));
  MY_BOOST_CHECK(! is_at_least_of_severity(DefaultLoggerTraits::NoStaticMinimumImportance, DEBUG));
  MY_BOOST_CHECK(! is_at_least_of_severity(DefaultLoggerTraits::NoStaticMinimumImportance, LONGDEBUG));
  
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

  MY_BOOST_CHECK(! static_is_at_least_of_severity<DefaultLoggerTraits::NoStaticMinimumImportance, ERROR>::value);
  MY_BOOST_CHECK(! static_is_at_least_of_severity<DefaultLoggerTraits::NoStaticMinimumImportance, WARNING>::value);
  MY_BOOST_CHECK(! static_is_at_least_of_severity<DefaultLoggerTraits::NoStaticMinimumImportance, INFO>::value);
  MY_BOOST_CHECK(! static_is_at_least_of_severity<DefaultLoggerTraits::NoStaticMinimumImportance, DEBUG>::value);
  MY_BOOST_CHECK(! static_is_at_least_of_severity<DefaultLoggerTraits::NoStaticMinimumImportance, LONGDEBUG>::value);
  
}

BOOST_AUTO_TEST_CASE(test_loggertraits_minimportance)
{
  std::string recorded;
  DummyLoggerMinImportance logger(Tomographer::Logger::DEBUG, &recorded);
  
  BOOST_CHECK_EQUAL(Tomographer::Logger::LoggerTraits<DummyLoggerMinImportance>::StaticMinimumImportanceLevel,
                    Tomographer::Logger::WARNING); // what we declared above

  BOOST_CHECK(DummyLoggerMinImportance::statically_enabled_for<Tomographer::Logger::ERROR>());
  BOOST_CHECK(DummyLoggerMinImportance::statically_enabled_for<Tomographer::Logger::WARNING>());
  BOOST_CHECK(!DummyLoggerMinImportance::statically_enabled_for<Tomographer::Logger::INFO>());
  BOOST_CHECK(!DummyLoggerMinImportance::statically_enabled_for<Tomographer::Logger::DEBUG>());
  BOOST_CHECK(!DummyLoggerMinImportance::statically_enabled_for<Tomographer::Logger::LONGDEBUG>());
  BOOST_CHECK(DummyLoggerMinImportance::statically_enabled_for(Tomographer::Logger::ERROR));
  BOOST_CHECK(DummyLoggerMinImportance::statically_enabled_for(Tomographer::Logger::WARNING));
  BOOST_CHECK(!DummyLoggerMinImportance::statically_enabled_for(Tomographer::Logger::INFO));
  BOOST_CHECK(!DummyLoggerMinImportance::statically_enabled_for(Tomographer::Logger::DEBUG));
  BOOST_CHECK(!DummyLoggerMinImportance::statically_enabled_for(Tomographer::Logger::LONGDEBUG));

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
  BOOST_CHECK(Tomographer::Logger::FileLogger::statically_enabled_for(
                  Tomographer::Logger::DefaultLoggerTraits::NoStaticMinimumImportance
                  ));
}

BOOST_AUTO_TEST_CASE(test_loggertraits_ownlevel)
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


BOOST_AUTO_TEST_CASE(test_loggertraits_originfilter)
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



// =============================================================================

BOOST_AUTO_TEST_SUITE_END() // test_loggers_basic