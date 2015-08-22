
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <tuple>

// we want `eigen_assert()` to raise an `eigen_assert_exception` here
#include <tomographer/tools/eigen_assert_exception.h>

#include <Eigen/Core>

#include <tomographer/tools/loggers.h>

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>


// tolerance, in *PERCENT*
const double tol_percent = 1e-12;
const double tol = tol_percent * 0.01;


// ----------------------- NEW EXPERIMENT --------------------------


// test new type of logger, fixed origin

#define TOMO_FUNCTION __PRETTY_FUNCTION__

#define TOMO_ORIGIN extractTomoOrigin(TOMO_FUNCTION)



template<char... Chars>
struct from_chars_helper
{
  char array[sizeof...(Chars)];
  constexpr from_chars_helper() : array{Chars...} { }
};

class conststr
{
public:
  const char* _p;
  std::size_t _sz;
public:
  template<std::size_t N>
  constexpr conststr(const char(&a)[N]) : _p(a), _sz(N - 1) {}
  constexpr conststr(const char *a, std::size_t n) : _p(a), _sz(n) {}

private:
public:
  template<char... Chars>
  static inline constexpr conststr from_chars()
  {
    return conststr(from_chars_helper<Chars...>().array, sizeof...(Chars));
  }
 
  inline constexpr char operator[](std::size_t n) const
  {
    return n < _sz ? _p[n] : throw std::out_of_range("");
  }
  inline constexpr std::size_t size() const { return _sz; }

  inline constexpr bool is_in_range(std::size_t n) const
  {
    return n < _sz ? true : false;
  }
  inline constexpr bool check_range(std::size_t n, bool answer = true) const
  {
    return is_in_range(n) ? answer : throw std::out_of_range("");
  }

  inline constexpr std::size_t clamp_to_range(const std::size_t pos)
  {
    return pos >= _sz ? _sz-1 : pos;
  }

  /*
  template<char... Chars>
  inline constexpr auto append_array() -> std::array<char, sizeof...(Chars)+size()>
  {
    return to_array<0, Chars...>();
  }

  template<std::size_t N = 0, char... Chars>
  inline constexpr auto to_array() -> std::array<char, sizeof...(Chars)+size()-N>
  {
    return (N < size()
	    ? to_array<N + 1, Chars..., _p[N]>()
	    : std::array<char, sizeof...(Chars)>{Chars...});
  }
  */

  // returns the string composed of, in this order (for N<size()):
  //   - the given Chars...
  //   - the substring of *this [N,end]
  //   - the string s
  // if N>size() but N<size()+s.size(), then returns:
  //   - the given Chars...
  //   - the substring s[N-size(),end]
  // if N>=size()+s.size(), returns the given Chars... as a conststr.
  template<std::size_t N = 0, char... Chars>
  inline constexpr conststr add_string(const conststr& s) const
  {
    return (N < size()
	    ? add_string<N+1, Chars..., _p[N]>(s)
	    : (N < size() + s.size()
	       ? add_string<N+1, Chars..., s[N-size()]>(s)
	       : from_chars<Chars...>()));
  }

  inline constexpr conststr operator+(const conststr & s) const
  {
    return add_string<0>(s);
  }

  inline constexpr bool startswith(const conststr& s, std::size_t StartOffset = 0, std::size_t S_I = 0) const {
    return ((S_I >= s.size())
	    ? true
	    : (StartOffset+S_I < size() && s[S_I] == operator[](StartOffset+S_I)
	       ? startswith(s, StartOffset, S_I+1)
	       : false)
	);
  }

  inline constexpr bool operator==(const conststr& other) {
    return startswith(other) && other.size() == size();
  }

  inline constexpr conststr substr(std::size_t pos, std::size_t count = std::string::npos) const {
    return conststr(_p+pos, (pos > size() || count > size() || pos+count>size()) ? (size()-pos) : count);
  }
  inline constexpr conststr substr_e(std::size_t pos, std::size_t end = std::string::npos) const {
    return conststr(_p+pos, (end>size()) ? (size()-pos) : end-pos);
  }

  inline constexpr std::size_t find(const conststr& s, std::size_t pos = 0,
				    std::size_t not_found = std::string::npos) const
  {
    return (!is_in_range(pos)
	    ? ( not_found )
	    : ( startswith(s, pos)
		? pos
		: (pos <= size()-s.size()
		   ? find(s, pos+1, not_found)
		   : not_found) ));
  }

  inline constexpr std::size_t rfind(const conststr& s, std::size_t pos = std::string::npos,
				     std::size_t not_found = std::string::npos) const
  {
    return ((s.size() > size())
	    ? ( not_found )
	    : ((pos > size()-s.size())
	       ? rfind(s, size()-s.size(), not_found)
	       : ( startswith(s, pos)
		   ? pos
		   : ((pos > s.size())
		      ? rfind(s, pos-1, not_found)
		      : not_found) )));
  }

  inline std::string to_string() const { return std::string(_p, _sz); }
};


// test conststr
#define TOMO_STATIC_ASSERT_EXPR(...)				\
  static_assert(__VA_ARGS__, #__VA_ARGS__)

TOMO_STATIC_ASSERT_EXPR(conststr("abcdef") == conststr("abcdef"));
TOMO_STATIC_ASSERT_EXPR(!(conststr("ksfldnfa") == conststr("abcdef")));
TOMO_STATIC_ASSERT_EXPR(!(conststr("abcdef") == conststr("abcde")));
TOMO_STATIC_ASSERT_EXPR(!(conststr("abcde") == conststr("abcdef")));
TOMO_STATIC_ASSERT_EXPR(conststr("fdknslf")[0] == 'f');
TOMO_STATIC_ASSERT_EXPR(conststr("fdknslf")[1] == 'd');
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789")[8] == '8');
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789")[9] == '9');
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789").is_in_range(0u));
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789").is_in_range(1u));
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789").is_in_range(9u));
TOMO_STATIC_ASSERT_EXPR(!conststr("0123456789").is_in_range(10u));
TOMO_STATIC_ASSERT_EXPR(!conststr("0123456789").is_in_range(std::string::npos));
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789").clamp_to_range(0) == 0);
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789").clamp_to_range(1) == 1);
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789").clamp_to_range(13) == 9);
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789").startswith(conststr("01234")));
TOMO_STATIC_ASSERT_EXPR(!conststr("0123456789").startswith(conststr("abcdef")));
TOMO_STATIC_ASSERT_EXPR(!conststr("012").startswith(conststr("0123456789")));
TOMO_STATIC_ASSERT_EXPR(conststr("xyz0123456789").startswith(conststr("01234"), 3));
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789").startswith(conststr("9"),9));
TOMO_STATIC_ASSERT_EXPR(conststr("xyz0123456789").startswith(conststr("X1234"), 3, 1));
// substr(start, count) or substr_e(start, end)
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789").substr(0,3) == conststr("012"));
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789").substr(2,3) == conststr("234"));
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789").substr_e(2,5) == conststr("234"));
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789").substr(2) == conststr("23456789"));
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789").substr(2, 8) == conststr("23456789"));
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789").substr(2, 10) == conststr("23456789"));
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789").substr(2, std::string::npos) == conststr("23456789"));
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789").substr_e(2, 10) == conststr("23456789"));
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789").substr_e(2, std::string::npos) == conststr("23456789"));
// find(s,pos,not_found)
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789").find("234") == 2);
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789").find("ab") == std::string::npos);
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789").find("ab",2,999u) == 999u);
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789").find("0123xyz",0) == std::string::npos);
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789").find("9",3) == 9);
// rfind(s,pos,not_found)
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789").rfind("9") == 9);
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789").rfind("4") == 4);
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789").rfind("4",4) == 4);
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789").rfind("4",std::string::npos) == 4);
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789").rfind("4",3) == std::string::npos);
TOMO_STATIC_ASSERT_EXPR(conststr("0123456789").rfind("4",3,999u) == 999u);

// BOOST_AUTO_TEST_CASE(test_conststr)
// {
//   conststr s = conststr("0123456789");
//   BOOST_MESSAGE(std::to_string(conststr("0123456789").rfind(conststr("9"),10)));
//   BOOST_MESSAGE(s._p + std::string(" len=") + std::to_string(s._sz));
//   BOOST_MESSAGE(s.to_string());
// }

// -----------------------------------------------


// logic taken from KLatexFormula/klftools: klfdefs.cpp / klfShortFuncSignature()
struct extractFuncName_helper {
  struct extracted {
    const std::size_t decl_pos;
    const conststr extr;
    constexpr extracted(std::size_t dp, const conststr& s) : decl_pos(dp), extr(s) { }
  };
  static constexpr conststr alltofirstparen(const conststr& s)
  {
    return s.substr(0, s.find(conststr("("), 0, s.size()));
  }
  static constexpr std::size_t declpos_from_found_spc(std::size_t found_pos)
  {
    return found_pos == std::string::npos ? 0 : found_pos + 1;
  }
  static constexpr std::size_t pos_decl(const conststr& s)
  {
    return ((s.size() > 2)
	    ? declpos_from_found_spc(s.rfind(conststr(" "), std::string::npos))
	    : 0);
  }
  static constexpr extracted allfromfirstspace(const conststr& s)
  {
    return extracted(pos_decl(s),
		     s.substr_e(pos_decl(s),
				s.size()));
  }
  static constexpr extracted do_extract(const conststr& funcname)
  {
    return allfromfirstspace(alltofirstparen(funcname));
  }
  static constexpr conststr extract_choose(const extracted& do_extracted,
					   const conststr& funcname)
  {
    return (do_extracted.extr.substr(0,8) == conststr("operator")
	    ? funcname.substr(do_extracted.decl_pos)
	    : do_extracted.extr);
  }
  static constexpr conststr extract(const conststr& funcname)
  {
    return extract_choose(do_extract(funcname), funcname);
  }
};
constexpr inline conststr extractFuncName(const conststr & funcname)
{
  return extractFuncName_helper::extract(funcname);
}


TOMO_STATIC_ASSERT_EXPR(extractFuncName("void class::subclass::subclass(int)") == "class::subclass::subclass");
TOMO_STATIC_ASSERT_EXPR(extractFuncName("conststr ns::subclass::method()") == "ns::subclass::method");
TOMO_STATIC_ASSERT_EXPR(extractFuncName("int ns::subclass::method(const int&, void, conststr *)") == "ns::subclass::method");
TOMO_STATIC_ASSERT_EXPR(extractFuncName("int ns::subclass::operator==(int)") == "ns::subclass::operator==");
TOMO_STATIC_ASSERT_EXPR(extractFuncName("int operator==(const ns::subclass&, char)") == "operator==(const ns::subclass&, char)");

// BOOST_AUTO_TEST_CASE(a)
// {
//   BOOST_MESSAGE( extractFuncName("int ns::subclass::operator==(char)")...........);
// }



struct OriginedFilterOriginSpec {
  const conststr origin_prefix;
  const conststr glue;

  constexpr OriginedFilterOriginSpec(const conststr& s, const conststr& gl)
    : origin_prefix(s), glue(gl) { }
};

struct extractTomoOrigin_helper {
  static inline constexpr OriginedFilterOriginSpec step2(const conststr fn, std::size_t last_doublecolons,
							 std::size_t after_prelast_doublecolons)
  {
    return ( fn.substr_e(after_prelast_doublecolons, last_doublecolons) == fn.substr(last_doublecolons+2)
	     // fn is a constructor, so keep class name and use "::" as glue
	     ? OriginedFilterOriginSpec(fn.substr(last_doublecolons+2), "::")
	     // looks like a method name. Strip off the class name. Also use an internal
	     // glue to indicate a logical level.
	     : OriginedFilterOriginSpec(fn.substr(last_doublecolons+2) + "()", "/")
	);
  }
  static inline constexpr std::size_t afterprelast_doublecolons(std::size_t prelast_doublecolons_found)
  {
    return (prelast_doublecolons_found == std::string::npos) ? 0 : prelast_doublecolons_found+2;
  }
  static inline constexpr OriginedFilterOriginSpec step1(const conststr fn, std::size_t last_doublecolons)
  {
    return last_doublecolons == std::string::npos || last_doublecolons == 0
      ? OriginedFilterOriginSpec(fn, "/") // looks like simple function name with no parent scope
      : step2(fn, last_doublecolons, afterprelast_doublecolons(fn.rfind("::", last_doublecolons-1)));
  }

  static inline constexpr OriginedFilterOriginSpec extract_from_func_name(const conststr fn)
  {
    return step1(fn, fn.rfind("::"));
  }
};  

constexpr const OriginedFilterOriginSpec extractTomoOrigin(const conststr fn)
{
  return extractTomoOrigin_helper::extract_from_func_name(extractFuncName(fn));
}

/*
constexpr OriginedFilterOriginSpec extractTomoOrigin(const char * funcname)
{
  std::string prefix = extractFuncName(origin_fn_name);
  // determine if it is a constructor.
  std::size_t last_doublecolons = prefix.rfind("::");
  if (last_doublecolons == std::string::npos || last_doublecolons == 0) {
    // looks like a simple function name with no parent scope.
    return std::pair<std::string,std::string>(prefix, "/");
  }

  std::size_t afterprelast_doublecolons = prefix.rfind("::", last_doublecolons-1);
  if (afterprelast_doublecolons == std::string::npos) {
    afterprelast_doublecolons = 0;
  } else {
    afterprelast_doublecolons += 2; // point on the class name
  }
  if (prefix.substr(afterprelast_doublecolons, last_doublecolons - afterprelast_doublecolons)
      == prefix.substr(last_doublecolons+2)) {
    // prefix is a constructor, so keep class name and "::" glue.
    return std::pair<std::string,std::string>(prefix.substr(0, last_doublecolons), "::");
  }

  // a function name. Strip off the class name if we're nested. Also use an internal
  // glue to indicate a logical level.
  std::string baseprefix =
    tomo_internal::OriginedLoggerGetPrefix<BaseLoggerType>::prefix(_baselogger);
  std::string newprefix =
    prefix.substr(Tomographer::Logger::tomo_internal::matched_length(baseprefix, prefix));
  if (newprefix.substr(0,2) == "::") {
    newprefix = newprefix.substr(2);
  }
  return std::pair<std::string,std::string>(newprefix+"()", "/");
}
*/

template<typename BaseLoggerType_>  class OriginedLogger;

namespace Tomographer { namespace Logger {
  // Traits for OriginedLogger
  template<typename BaseLoggerType_>
  struct LoggerTraits<OriginedLogger<BaseLoggerType_> > : LoggerTraits<BaseLoggerType_>
  {
    enum {
      //! Logger will delegate calls for current level() to base logger
      HasOwnGetLevel = 1
    };
  };
} } // namespaces

/*
namespace tomo_internal {
template<typename LoggerType>
struct OriginedLoggerGetPrefix {
  static inline std::string prefix(LoggerType & ) { return std::string(); }
};
template<typename BaseLoggerType>
struct OriginedLoggerGetPrefix<OriginedLogger<BaseLoggerType> > {
  typedef OriginedLogger<BaseLoggerType> LoggerType;
  static inline std::string prefix(LoggerType & logger) {
    return OriginedLoggerGetPrefix<BaseLoggerType>::prefix(logger.baselogger())
      + logger.origin_prefix() + logger.glue();
  }
};
} // namespace tomo_internal
*/

template<typename BaseLoggerType_>
class OriginedLogger : public Tomographer::Logger::LoggerBase<OriginedLogger<BaseLoggerType_> >
{
public:
  typedef BaseLoggerType_ BaseLoggerType;

private:
  typedef Tomographer::Logger::LoggerBase<OriginedLogger> Base_;

  const std::string _origin_prefix;
  const std::string _glue;

  BaseLoggerType & _baselogger;

public:
  OriginedLogger(const std::string & origin_fn_name, BaseLoggerType & logger_)
    : _origin_prefix(origin_fn_name), _glue("::"), _baselogger(logger_)
  {
  }
  OriginedLogger(const std::string & origin_prefix, const std::string & glue, BaseLoggerType & logger_)
    : _origin_prefix(origin_prefix), _glue(glue), _baselogger(logger_)
  {
  }
  OriginedLogger(const OriginedFilterOriginSpec & spec, BaseLoggerType & logger_)
    : _origin_prefix(spec.origin_prefix.to_string()), _glue(spec.glue.to_string()), _baselogger(logger_)
  {
  }

  inline std::string origin_prefix() const { return _origin_prefix; }
  inline std::string glue() const { return _glue; }

  inline BaseLoggerType & baselogger() { return _baselogger; };

  inline OriginedLogger<OriginedLogger<BaseLoggerType> > sublogger(const std::string & new_prefix)
  {
    return OriginedLogger<OriginedLogger<BaseLoggerType> >(new_prefix, *this);
  }
  inline OriginedLogger<OriginedLogger<BaseLoggerType> > sublogger(const std::string & new_prefix,
								   const std::string & new_glue)
  {
    return OriginedLogger<OriginedLogger<BaseLoggerType> >(new_prefix, new_glue, *this);
  }

  template<typename... Args>
  inline void longdebug(Args... a) { log<Tomographer::Logger::LONGDEBUG>(a...); }
  template<typename... Args>
  inline void debug(Args... a) { log<Tomographer::Logger::DEBUG>(a...); }
  template<typename... Args>
  inline void info(Args... a) { log<Tomographer::Logger::INFO>(a...); }
  template<typename... Args>
  inline void warning(Args... a) { log<Tomographer::Logger::WARNING>(a...); }
  template<typename... Args>
  inline void error(Args... a) { log<Tomographer::Logger::ERROR>(a...); }

  template<int Level, typename... Args>
  inline void log(Args... args)
  {
    Base_::template log<Level>("", args...);
  }


  // relay calls to base logger

  inline std::string get_origin(const char * origin) const
  {
    return ( origin == NULL || origin[0] == 0
	     ? _origin_prefix
	     : _origin_prefix + _glue + origin );
  }

  //! Emit a log by relaying to the base logger
  inline void emit_log(int level, const char * origin, const std::string& msg)
  {
    // this might also be called if we have a sublogger. In that case, if we have a
    // sublogger, then use their prefix.
    _baselogger.emit_log(level, get_origin(origin).c_str(), msg);
  }

  //! Get the base logger's set level.
  inline int level() const
  {
    return _baselogger.level();
  }

  //! If relevant for the base logger, filter the messages by origin from the base logger.
  TOMOGRAPHER_ENABLED_IF(Tomographer::Logger::LoggerTraits<BaseLoggerType>::HasFilterByOrigin)
  inline bool filter_by_origin(int level, const char * origin)
  {
    return _baselogger.filter_by_origin(level, get_origin(origin).c_str());
  }
};


class test_origin_logger
{
  OriginedLogger<Tomographer::Logger::BufferLogger> _logger;

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
    OriginedLogger<decltype(_logger)> logger(TOMO_ORIGIN, _logger);
    
    logger.debug("Hi there!");
    for (int k = 0; k < 10; ++k) {
      logger.longdebug("Number = %d", k);
    }
  }
};









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


BOOST_AUTO_TEST_CASE(origined_logger)
{
  Tomographer::Logger::BufferLogger b(Tomographer::Logger::LONGDEBUG);

  {
    test_origin_logger tst(b);
    tst.some_method();
  }
  
  BOOST_MESSAGE(b.get_contents());
  // ............
}



BOOST_AUTO_TEST_SUITE_END() // test_loggers_basic



