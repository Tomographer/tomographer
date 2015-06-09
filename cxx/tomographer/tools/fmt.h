#ifndef TOMOGRAPHER_TOOLS_FMT_H
#define TOMOGRAPHER_TOOLS_FMT_H

#include <cstdio>
#include <cstdarg>

#include <chrono>
#include <string>
#include <sstream> // stringstream
#include <iostream>

#include <tomographer/tools/util.h> // PRINTFN_ARGS_SAFE


namespace Tomographer
{
namespace Tools
{



//
// =============================================================================
//
// Formatting Utilities
//


/** \brief Exception for bad \c printf format
 *
 * This exception is raised when \ref fmts() or \ref vfmts() are called with a bad format
 * argument, causing \c vsnprintf() to return a negative number.
 */
class bad_fmts_format : public std::exception
{
  std::string msg;
public:
  //! Construct an exception for bad printf formatting. Provide an error message here.
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
  int nsize = std::vsnprintf(buffer, size, fmt, ap1);
  if (nsize < 0) {
    // failure: bad format probably
    throw bad_fmts_format("vsnprintf("+std::string(fmt)+") failure: code="+std::to_string(nsize));
  }
  if(size <= nsize) {
    // buffer too small: delete buffer and try again
    delete[] buffer;
    size = nsize+1; // +1 for "\0"
    buffer = new char[size];
    nsize = std::vsnprintf(buffer, size, fmt, vl);
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
 * This is typically useful if you want to feed in C++ objects to a printf()-like
 * function.
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




// =============================================================================


/** \brief Format a number of seconds into a human-readable string
 *
 * This formats the duration, given in seconds, into a human-readable string with hours,
 * minutes seconds and milliseconds.
 *
 * \param seconds the duration in seconds
 */
inline std::string fmt_duration(double seconds)
{
  // split `seconds' into integral part and fractional part
  double dt_i_d;
  double dt_f = std::modf(seconds, &dt_i_d);
  int dt_i = (int)(dt_i_d+0.5);
  
  return fmts("%d:%02d:%02d.%03d", dt_i/3600, (dt_i/60)%60, dt_i%60, (int)(dt_f*1000+0.5));
};

/** \brief Format a \a std::chrono::duration into a human-readable string
 *
 * This formats the duration into a human-readable string with hours, minutes seconds and
 * milliseconds.
 *
 * \param dt the duration, a \a std::chrono::duration 
 *
 * See also \ref fmt_duration(double)
 */
template<typename Rep, typename Period>
inline std::string fmt_duration(std::chrono::duration<Rep, Period> dt)
{
  typedef std::chrono::duration<Rep, Period> Duration;

  // delta-time, in seconds and fraction of seconds
  double seconds = (double)dt.count() * Duration::period::num / Duration::period::den ;

  return fmt_duration(seconds);
};



} // namespace Tools
} // namespace Tomographer


#endif
