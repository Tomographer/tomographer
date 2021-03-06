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

#ifndef TOMOGRAPHER_TOOLS_FMT_H
#define TOMOGRAPHER_TOOLS_FMT_H

/** \file fmt.h
 *
 * \brief Utilities for formatting strings.
 *
 */

#include <cstdio>
#include <cstdarg>

#include <chrono>
#include <string>
#include <sstream> // stringstream
#include <iostream>
#include <utility> // std::declval
#include <vector>
#include <iomanip>
#include <functional>

#include <tomographer/tools/cxxutil.h> // PRINTFN_ARGS_SAFE


namespace Tomographer {
namespace Tools {


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
class TOMOGRAPHER_EXPORT BadFmtsFormat : public std::exception
{
  std::string msg;
public:
  //! Construct an exception for bad printf formatting. Provide an error message here.
  BadFmtsFormat(const std::string& msg_) : msg(msg_) { }
  ~BadFmtsFormat() throw() { }

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
  
  // on MinGW, can't find std::vsnprintf() because for some reason it's declared in the
  // global scope... so just import std namespace here and use generic vsnprintf() call.
  using namespace std;

  std::size_t size = 10;
  char * buffer = new char[size];
  va_list ap1;
  va_copy(ap1, vl);
  int nsize = vsnprintf(buffer, size, fmt, ap1);
  if (nsize < 0) {
    // failure: bad format probably
    throw BadFmtsFormat("vsnprintf("+std::string(fmt)+") failure: code="+std::to_string(nsize));
  }
  if(size <= (std::size_t)nsize) {
    // buffer too small: delete buffer and try again
    delete[] buffer;
    size = (std::size_t)nsize+1; // +1 for "\0"
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



//
// utility to stream an object, if possible
//

namespace tomo_internal {
// see http://stackoverflow.com/a/9154394/1694896
// see sfinae_yes and sfinae_no in cxxutil.h
template<typename T> static auto test_has_stream_op(int)
  -> typename sfinae_yes<decltype(*((std::ostream*)(NULL)) << *((T*)(NULL)))>::yes&;
template<typename T> static auto test_has_stream_op(long)
  -> typename sfinae_no<>::no&;
// template<typename T, int dummy_value,
// 	 typename EnabledIfType = decltype(std::declval<std::ostream&>() << std::declval<T>())>
// struct test_has_stream_op { static constexpr bool value = true; };
// template<typename T, long dummy_value>
// struct test_has_stream_op<T,dummy_value,void> { static constexpr bool value = false; };
// template<typename T>
// struct has_stream_op {
//   static constexpr bool value = test_has_stream_op<T, 0>::value;
// };
} // namespace tomo_internal


/** \brief Traits class to see whether an object exposes a \c "<<" operator overload for
 *         std::ostream
 *
 * Use the expression
 * \code
 *    hasOStream<T>::value
 * \endcode
 * to test whether a type \a T can be streamed into a std::ostream.  This value is a
 * compile-time boolean and can be used in template parameters.
 */
template<typename T>
struct TOMOGRAPHER_EXPORT hasOStreamOp {
  static constexpr bool value = (sizeof(tomo_internal::test_has_stream_op<T>(0))
				 == sizeof(typename tomo_internal::sfinae_yes<>::yes));
};


namespace tomo_internal {
//! Helper for streamIfPossible(sophisticated)
template<typename T_>
struct StreamIfPossibleWrapper
{
  //! The type we are wrapping -- the template parameter
  typedef T_ T;
  /** \brief Constructor.
   *
   */
  StreamIfPossibleWrapper(
      const T& obj_,
      std::string before_,
      std::string after_,
      std::string alternative_
      )
    : obj(obj_), before(before_), after(after_), alternative(alternative_)
  {
  }

  //! Const reference to the object we are wrapping to the ostream
  const T & obj;

  //! Text to display before the object's string representation, if \a T is streamable
  const std::string before;
  //! Text to display after the object's string representation, if \a T is streamable
  const std::string after;
  //! Text to display if \a T is not streamable
  const std::string alternative;

  // internal :
  
  //! Stream currently wrapped object into the given stream. (Internal)
  template<typename OStream, TOMOGRAPHER_ENABLED_IF_TMPL(!hasOStreamOp<T>::value)>
  void stream_into(OStream&& stream) const
  {
    stream << alternative;
  }
  //! Stream currently wrapped object into the given stream. (Internal)
  template<typename OStream, TOMOGRAPHER_ENABLED_IF_TMPL(hasOStreamOp<T>::value)>
  void stream_into(OStream&& stream) const
  {
    stream << before << obj << after;
  }
};
//! implementation of sending a StreamIfPossibleWrapper through a std::ostream
template<typename T>
inline std::ostream & operator<<(std::ostream & stream, const StreamIfPossibleWrapper<T>& wobj)
{
  wobj.stream_into(stream);
  return stream;
}
//! implementation of sending a StreamIfPossibleWrapper through a std::ostream
template<typename T>
inline std::ostream&& operator<<(std::ostream&& stream, const StreamIfPossibleWrapper<T>& wobj)
{
  wobj.stream_into(stream);
  return std::move(stream);
}
}



#ifndef TOMOGRAPHER_PARSED_BY_DOXYGEN
template<typename T, TOMOGRAPHER_ENABLED_IF_TMPL(!hasOStreamOp<T>::value)>
inline std::string streamIfPossible(const T& /*obj*/)
{
  // obj is not streamable
  return std::string("<")+typeid(T).name()+std::string(">");
}
template<typename T, TOMOGRAPHER_ENABLED_IF_TMPL(hasOStreamOp<T>::value)>
inline const T& streamIfPossible(const T& obj)
{
  return obj;
}
template<typename T>
inline tomo_internal::StreamIfPossibleWrapper<T>
streamIfPossible(const T& obj, std::string before,
		 std::string after = std::string(),
		 std::string alternative = "<"+std::string(typeid(T).name())+">")
{
  return tomo_internal::StreamIfPossibleWrapper<T>(obj, std::move(before),
						   std::move(after), std::move(alternative));
}
#else
/** \brief Utility to stream an object, but only if \c "<<" overload exists
 *
 * You may use this function in a sequence of objects passed to a std::ostream, so that
 * the object gets printed if it is streamable and it is ignored if not.  The return type
 * of this function is guaranteed to be streamable for any type \a T.
 *
 * A typical use of this function is to generate debugging messages in generic templated
 * classes for which the data type is not certain to be streamable.
 *
 * Example use:
 * \code
 *   Eigen::Matrix2d m(Eigen::Matrix2d::Identity());
 *   struct NotStreamable { int a, b, c;  };
 *   NotStreamable x;
 *   std::cout << "The identity matrix is = \n" << streamIfPossible(m) << "\n"
 *             << "And the value of x is = " << streamIfPossible(x) << "\n";
 * \endcode
 * Would output:
 * 
 */
template<typename T>
inline _Unspecified streamIfPossible(const T& obj)
{
}
/** \brief A slightly more sophisticated alternative to \ref streamIfPossible()
 *
 * With this wrapper, you can also specify text to add before and after the object if it
 * is streamed, and you can specify the alternative text to display if the object is not
 * streamable.
 *
 * Example:
 * \code
 *   MyObjectType obj(..);
 *   std::cout << "Hello !"
 *             << StreamIfPossibleWrapper(obj, "\nobject is = ", "\n", " [not streamable]\n")
 *             << "Done !\n";
 * \endcode
 * might give you either
 * <pre>
 *   Hello!
 *   object is = ..... string representation of MyObjectType .....
 *   Done !
 * </pre>
 * if the object type \a MyObjectType is streamable, or
 * <pre>
 *   Hello! [not streamable]
 *   Done !
 * </pre>
 * if it is not.
 *
 * Specify the object const reference in \a obj, the string to display before the
 * object's string representation (\a before), the string to display after the object's
 * string representation (\a after), and the alternative string to display if the object
 * is not streamable (\a alternative).
 */
template<typename T>
inline _Unspecified streamIfPossible(const T& obj,
				     std::string before,
				     std::string after = std::string(),
				     std::string alternative = "<"+std::string(typeid(T).name())+">")
{
}
#endif







// =============================================================================


/** \brief Format a number of seconds into a human-readable string
 *
 * This formats the duration, given in seconds, into a human-readable string with hours,
 * minutes seconds and milliseconds.
 *
 * \param seconds the duration in seconds
 */
inline std::string fmtDuration(double seconds)
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
 * See also \ref fmtDuration(double)
 */
template<typename Rep, typename Period>
inline std::string fmtDuration(std::chrono::duration<Rep, Period> dt)
{
  typedef std::chrono::duration<Rep, Period> Duration;

  // delta-time, in seconds and fraction of seconds
  double seconds = (double)dt.count() * Duration::period::num / Duration::period::den ;

  return fmtDuration(seconds);
};





/** \brief Minimal tool for formatting console stuff with fixed line width
 *
 */
class TOMOGRAPHER_EXPORT ConsoleFormatterHelper
{
public:
  /** \brief Constructor
   *
   * \param width is the target with of the stuff you want to output.  If \a width is
   *        zero, then an appropriate default is chosen including an attempt to detect the
   *        current console width.
   *
   */
  ConsoleFormatterHelper(int width = 0)
    : _columns((std::size_t)Tomographer::Tools::getWidthForTerminalOutput(width))
  {
  }

  //! The number of character columns (as specified to the constructor or detected)
  inline std::size_t columns() const { return _columns; }

  /** \brief Produce a centered string
   *
   * \returns a string of columns() characters, with \a x displayed in the middle of the
   *          line.  A newline is added.  If \a x is wider than the line length, it is
   *          returned in full with a newline appended.
   */
  inline std::string centerLine(std::string x)
  {
    if (x.size() > columns()) {
      return std::move(x) + "\n";
    }
    const std::size_t r = columns() - x.size();
    const std::size_t rleft = r/2;
    const std::size_t rright = r - rleft; // may differ from r/2 if r is odd
    return std::string(rleft, ' ') + std::move(x) + std::string(rright, ' ') + "\n";
  }

  /** \brief Produce a right-aligned string
   *
   * \returns a string of columns() characters, with \a x displayed at the right of the
   *          line.  A newline is added.  If \a x is wider than the line length, it is
   *          returned in full with a newline appended.
   */
  inline std::string rightLine(std::string x)
  {
    if (x.size() > columns()) {
      return std::move(x) + "\n";
    }
    const std::size_t r = columns() - x.size();
    return std::string(r, ' ') + std::move(x) + "\n";
  }

  /** \brief Produce a left-aligned string
   *
   * \returns a string of columns() characters, with \a x displayed at the left of the
   *          line.  The string is padded with spaces and a final newline.  If \a x is
   *          wider than the line length, it is returned in full with a newline appended.
   */
  inline std::string leftLine(std::string x)
  {
    if (x.size() > columns()) {
      return std::move(x) + "\n";
    }
    const std::size_t r = columns() - x.size();
    return std::move(x) + std::string(r, ' ') + "\n";
  }
  
  inline std::string hrule(char pattern = '-')
  {
    // remember that std::string(10, '-') constructs 10 copies of '-', that is : "----------".
    return std::string(columns(), pattern) + std::string("\n");
  }
  
private:
  const std::size_t _columns;
};


// =============================================================================

// Helper

namespace tomo_internal {
class LazyOStreamCallback {
public:
  template<typename CallFn>
  LazyOStreamCallback(CallFn && fn_) : fn(fn_) { }

  std::function<void(std::ostream&)> fn;
};
inline std::ostream & operator<<(std::ostream & stream, const LazyOStreamCallback & lazy)
{
  lazy.fn(stream);
  return stream;
}
} // tomo_internal


// =============================================================================

// Simple word wrapping

class SimpleWordWrapper
{
public:
  SimpleWordWrapper(std::size_t w_,
                    std::string init_ = "",
                    int indent_ = 0,
                    int first_indent_ = 0,
                    bool hard_wrap_long_lines_ = false,
                    bool obey_newlines_ = false)
    : w(w_),
      init(std::move(init_)),
      indent(indent_),
      first_indent(first_indent_),
      hard_wrap_long_lines(hard_wrap_long_lines_),
      obey_newlines(obey_newlines_)
  {
  }

  std::vector<std::string> rawWrapLines(const std::string & text) const
  {
    std::vector<std::string> output;
    std::size_t pos = 0;

    std::string startline;
    int startline_len = first_indent + (int)init.size();

    auto finish_line = [&output,&startline,&startline_len,this](std::string linetoend = "") {
      std::string full_line = std::move(startline) + std::move(linetoend);
      if (full_line.size()) {
        output.push_back(full_line);
      }
      startline = "";
      startline_len = indent;
    };
    auto add_startline = [&startline,&startline_len](std::string morestartline) {
      startline += std::move(morestartline);
      startline_len += morestartline.size();
    };

    while (pos < text.size()) {

      std::size_t this_w = (std::size_t)std::max((int)w - startline_len, 1);

      // build the next line of the output

      if (text[pos] == ' ' || text[pos] == '\n') {
        bool new_paragraph = false;
        while (pos < text.size() && (text[pos] == ' ' || text[pos] == '\n')) {
          if (text[pos] == '\n') {
            new_paragraph = true;
          }
          ++ pos; // ignore leading spaces
        }
        if (new_paragraph) {
          finish_line();
          output.push_back("");
          continue;
        }
      }

      std::size_t nxtpos = text.find_first_of('\n', pos);
      if (nxtpos != std::string::npos && nxtpos < pos+this_w) { // have a newline on this line
        if (obey_newlines) {
          // hard newline within next line, break there
          finish_line( text.substr(pos, nxtpos-pos) );
          pos = nxtpos+1;
          continue;
        } else {
          // might be a paragraph ending, so continue re-parsing from here
          add_startline( text.substr(pos, nxtpos-pos) );
          pos = nxtpos+1;
          continue;
        }
      }

      if (text.size() < pos + this_w) {
        // fits in one line
        finish_line( text.substr(pos) );
        pos = text.size();
        break;
      }

      // starting at this_w from pos, look back for first space or newline
      std::size_t lpos = text.find_last_of(" \n", pos+this_w);
      if (lpos == std::string::npos || lpos < pos) { // not found within this line
        // line too long with no spaces
        if (hard_wrap_long_lines) {
          finish_line( text.substr(pos, this_w) );
          pos += this_w;
          continue;
        }
        // otherwise, look for next space and break there
        std::size_t nxtpos = text.find_first_of(" \n", pos+this_w);
        if (nxtpos == std::string::npos) {
          // all up to the end is one super long word
          finish_line( text.substr(pos) );
          pos = text.size();
          break;
        }
        // ok, wrapping a bit further off the line keeping the long word intact
        finish_line( text.substr(pos, nxtpos-pos) );
        pos = nxtpos+1;
        continue;
      }
      // wrap line normally at last space
      finish_line( text.substr(pos, lpos-pos) );
      pos = lpos+1;

      continue;
    }
    finish_line();
    
    // if last line is new paragraph, remove it
    if (output.size() && output.back().size() == 0) {
      output.pop_back();
    }

    return output;
  }

  tomo_internal::LazyOStreamCallback wrapped(const std::string & text) const
  {
    std::vector<std::string> lines = rawWrapLines(text);
    return tomo_internal::LazyOStreamCallback([lines,this](std::ostream & stream) {
        for (std::size_t k = 0; k < lines.size(); ++k) {
          if (k == 0) {
            stream << std::string((std::size_t)first_indent, ' ') << init << lines[0] << "\n";
          } else {
            stream << std::string((std::size_t)indent, ' ') << lines[k] << "\n";
          }
        }
      });
  }

  std::size_t getWidth() const { return w; }
  std::string getInit() const { return init; }
  int getIndent() const { return indent; }
  int getFirstIndent() const { return first_indent; }
  bool getHardWrapLongLines() const { return hard_wrap_long_lines; }
  bool getObeyNewlines() const { return obey_newlines; }

private:
  std::size_t w;
  std::string init;
  int indent;
  int first_indent;
  bool hard_wrap_long_lines;
  bool obey_newlines;
};


// =============================================================================

// Format footnotes in std::ostream


/** \brief Helper to format footnotes in Fmt
 *
 * 
 */
class FmtFootnotes
{
private:
  std::vector<std::string> ftlist;
public:
  FmtFootnotes() : ftlist{}
  {
  }
  
  tomo_internal::LazyOStreamCallback addFootNote(std::string footnote)
  {
    return tomo_internal::LazyOStreamCallback([footnote,this](std::ostream & stream) {
        stream << "[" << std::to_string(ftlist.size()+1) << "]";
        ftlist.push_back(footnote);
      }) ;
  }

  void addSilentFootNote(std::size_t no, std::string footnote)
  {
    (void)no; // silence "unused" warning

    // Footnotes numbering starts at 1
    tomographer_assert(no-1 == ftlist.size()
                       && "Your footnotes have changed numbering! Check your silent footnotes.") ;

    ftlist.push_back(footnote);
  }

  const std::vector<std::string> & getFootNotes() const { return ftlist; }

  tomo_internal::LazyOStreamCallback wrapped(std::size_t width = 80) const
  {
    return tomo_internal::LazyOStreamCallback([width,this](std::ostream & stream) {
        const std::vector<std::string> & ftlist = getFootNotes();
        int w = (int)(std::ceil(std::log10((double)ftlist.size()))+0.01);
        for (std::size_t k = 0; k < ftlist.size(); ++k) {
          std::string head = "["+std::to_string(1+k)+"] ";
          SimpleWordWrapper wrapper(width, head, w+4, 1);
          stream << wrapper.wrapped(ftlist[k]);
        }
      });
  }
};

inline std::ostream & operator<<(std::ostream & stream, const FmtFootnotes & f)
{
  const std::vector<std::string> & ftlist = f.getFootNotes();
  int w = (int)(std::ceil(std::log10((double)ftlist.size()))+0.01);
  const std::string nlindent = "\n" + std::string((std::size_t)w+4, ' ');
  for (std::size_t k = 0; k < ftlist.size(); ++k) {
    std::string s = ftlist[k];

    // string replace "\n" by "\n<indent spaces>" -- see https://stackoverflow.com/a/3418285/1694896
    size_t start_pos = 0;
    while (s.size() && (start_pos = s.find("\n", start_pos)) != std::string::npos) {
        s.replace(start_pos, 1, nlindent);
        start_pos += nlindent.length();
    }

    // output this footnote
    stream << " [" << std::setw(w) << (k+1) << "] " << s << "\n";
  }

  return stream;
}




} // namespace Tools
} // namespace Tomographer


#endif
