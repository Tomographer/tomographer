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

#ifndef TOMOGRAPHER_TOOLS_UTILS_H
#define TOMOGRAPHER_TOOLS_UTILS_H

/** \file cxxutil.h
 *
 * \brief Some C++ utilities, with a tad of C++11 tricks.
 *
 */

#include <cassert>
#include <cstddef>
#include <cstdlib>

#include <iostream>
#include <complex>
#include <type_traits>
#include <exception>
#include <stdexcept>

#include <tomographer/tools/cxxdefs.h>
#include <tomographer/tools/conststr.h>

#include <Eigen/Core>

// -----------------------------------------------


namespace Tomographer
{
namespace Tools
{

// taken from http://stackoverflow.com/a/25510879/1694896

namespace tomo_internal {
  template <typename F>
  struct FinalAction {
    FinalAction(F f) : clean_(f) {}
    ~FinalAction() { clean_(); }
    F clean_;
  };
} // namespace tomo_internal

/** \brief implementation of a \c finally clause, somewhat like in Python
 *
 * Example Usage:
 * \code
 *   SomeResource * ptr = new SomeResource(..)
 *   auto delete_ptr = finally([ptr]() { delete ptr; });
 *   // Now, the pointer ptr will be 'delete'd at end of the current block.
 *   ...
 *
 * \endcode
 */
template <typename F>
inline tomo_internal::FinalAction<F> finally(F f)
{
  return tomo_internal::FinalAction<F>(f);
}






// -----------------------------------------------------------------------------


/** \brief A type which stores a value possibly known at compile-time.
 *
 * This is an implementation inspired by Eigen's mechanism of compile-time known
 * matrix/vector sizes.
 *
 * This class declares a private member property which stores a dynamic value given to the
 * constructor, except if a value was already given as template parameter in which case we
 * take for granted that this value will always be the one we want at runtime.
 *
 * \tparam T_ the type of the value to store.
 * \tparam IsDynamic_ whether to store a dynamical value, or to store the static, fixed
 *         compile-time value.
 * \tparam StaticValue_ to store at compile-time.  This value will be ignored if
 *         \c IsDynamic is \c false. 
 *
 */
template<typename T_, bool IsDynamic_, T_ StaticValue_ = T_()>
TOMOGRAPHER_EXPORT class StaticOrDynamic
{

public:
  // definitions here are for the static case, the dynamic case will be specialized below.
  TOMO_STATIC_ASSERT_EXPR(IsDynamic_ == false) ;

  //! Type of the value we are storing
  typedef T_ T;
  //! Whether this value is flexible at run-time (dynamic), or fixed at compile-time (static)
  static constexpr bool IsDynamic = IsDynamic_;
  //! The value, if stored at compile-time.
  static constexpr T StaticValue = StaticValue_;

  //! Default Constructor. Only if the value is stored at compile-time.
  inline StaticOrDynamic() { }
  /** \brief Constructor with an explicit value.
   *
   * The value \a val must equal the compile-time fixed value, if any, or you'll get an
   * assert failure.
   */
  inline explicit StaticOrDynamic(T val) {
    tomographer_assert(val == StaticValue);
    (void)val; // silence "unused parameter" warning if tomographer_assert() gets optimized out
  }

  /** \brief Get the value stored.
   *
   * The same call syntax works both for dynamic and compile-time storage.
   */
  inline T value() const { return StaticValue; }

  /** \brief Get the value stored.
   *
   * The same call syntax works both for dynamic and compile-time storage.
   */
  inline T operator()() const { return StaticValue; }
};
// static properties
template<typename T_, bool IsDynamic_, T_ StaticValue_>
constexpr bool StaticOrDynamic<T_, IsDynamic_, StaticValue_>::IsDynamic;
template<typename T_, bool IsDynamic_, T_ StaticValue_>
constexpr typename StaticOrDynamic<T_, IsDynamic_, StaticValue_>::T StaticOrDynamic<T_, IsDynamic_, StaticValue_>::StaticValue;

/** \brief Template Specialization -- see \ref StaticOrDynamic<T_,IsDynamic_,StaticValue_>
 *
 * Specialization for the case if the value is known only at runtime.
 */
template<typename T_, T_ StaticValue_>
TOMOGRAPHER_EXPORT class StaticOrDynamic<T_, true, StaticValue_>
{
public:

  //! Type of the value we are storing. See \ref StaticOrDynamic<T_,IsDynamic_,StaticValue_>::T
  typedef T_ T;
  //! Whether this value is flexible at run-time (dynamic), or fixed at compile-time (static).
  static constexpr bool IsDynamic = true;

  
  //! No default constructor.
  StaticOrDynamic() = delete;
  /** \brief Constructor which initializes the value to \a val.
   *
   * The stored value may not be subsequently changed.
   */
  inline explicit StaticOrDynamic(T val) : _dyn_value(val) { }


  //! See \ref StaticOrDynamic<T_,IsDynamic_,StaticValue_>::value()
  inline T value() const { return _dyn_value; }

  //! See \ref StaticOrDynamic<T_,IsDynamic_,StaticValue_>::operator()()
  inline T operator()() const { return value(); }

private:

  //! The dynamically stored value
  const T _dyn_value;
};
// static properties
template<typename T_, T_ StaticValue_>
constexpr bool StaticOrDynamic<T_, true, StaticValue_>::IsDynamic;


// -----------------------------------------------------------------------------


/** \brief Utility that stores a data type if a compile-time flag is enabled.
 *
 * If \a enabled is \c true, then this type defines a public member \a value of type \a T.
 * It is not declared constant, and can be used as a regular variable.
 *
 * The constructor always accepts any number of arguments. They are either ignored (if
 * nothing is stored), or relayed on to the \a value's constructor.
 *
 */
template<typename T_, bool enabled>
TOMOGRAPHER_EXPORT struct StoreIfEnabled
{
  //! The type we're storing
  typedef T_ T;
  //! Whether we're storing a value or not -- by default no
  static constexpr bool IsEnabled = false;

  //! Constructor
  template<typename... Args>
  explicit StoreIfEnabled(Args...) { }
};
// static properties
template<typename T_, bool enabled>
constexpr bool StoreIfEnabled<T_, enabled>::IsEnabled;
/** \brief Specialization of \ref StoreIfEnabled<T_,enabled> for if we're storing a
 *         value
 *
 */
template<typename T_>
TOMOGRAPHER_EXPORT struct StoreIfEnabled<T_, true>
{
  //! The type we're storing
  typedef T_ T;
  //! Whether we're storing a value or not -- yes.
  static constexpr bool IsEnabled = true;

  //! This property keeps the value we're supposed to store.
  T value;

  //! Constructor. Any arguments are passed to the value's constructor.
  template<typename... ArgTypes>
  explicit StoreIfEnabled(const ArgTypes& ...  args) : value(args...) { }
};
// static properties:
template<typename T_>
constexpr bool StoreIfEnabled<T_, true>::IsEnabled;

/** \brief C++ Stream operators for \ref StoreIfEnabled
 *
 * Produces human-readable output.
 *
 * This implementation kicks in if the value is not streamable, and simply outputs the
 * fixed string \c "[-]".
 */
template<typename T>
inline std::ostream & operator<<(std::ostream & str, const StoreIfEnabled<T, false>& /*val*/)
{
  str << "[-]";
  return str;
}
/** \brief C++ Stream operators for \ref StoreIfEnabled
 *
 * Produces human-readable output.
 *
 * This implementation kicks in if the value is indeed streamable, and outputs the given
 * value into the stream.
 */
template<typename T>
inline std::ostream & operator<<(std::ostream & str, const StoreIfEnabled<T, true>& val)
{
  str << val.value;
  return str;
}



// -----------------------------------------------------------------------------


/** \brief Return true if the argument is a power of two, false otherwise.
 */
template<typename IntType = int>
inline constexpr bool isPowerOfTwo(IntType N)
{
  // taken from http://stackoverflow.com/q/10585450/1694896
  return N && !(N & (N - 1));
}


// -----------------------------------------------------------------------------


/** \brief The Real scalar type corresponding to a std::complex type
 *
 * If the type \a Scalar is of the form \a std::complex<T>, then this struct has a single
 * member \a type which is an alias of \a T.
 *
 * If the type \a Scalar is not an \a std::complex<T> type, then the member \a type is
 * just an alias of \a Scalar.
 */
template<typename Scalar>
struct ComplexRealScalar {
  typedef Scalar type;
};

//! Implementation of \ref ComplexRealScalar for complex types.
template<typename RealScalar>
struct ComplexRealScalar<std::complex<RealScalar> >
{
  typedef RealScalar type;
};




/** \brief Test whether the given value is positive or zero.
 *
 * This helper is useful to silence warnings in templates about `comparision of unsigned
 * >= 0 is always true'.
 */
template<typename X>
inline typename std::enable_if<std::is_unsigned<X>::value, bool>::type isPositive(const X /* val */)
{
  return true;
}
//! See \ref isPositive()
template<typename X>
inline typename std::enable_if<!std::is_unsigned<X>::value, bool>::type isPositive(const X val)
{
  return val >= 0;
}





// -----------------------------------------------------------------------------
// Define functions with printf-safe arguments, with compiler-generated warnings
// -----------------------------------------------------------------------------


// The PRINTFX_ARGS_SAFE macros are defined here, so that in the future we may support a
// wider range of compilers which may not understand the __attribute__(format)
// instruction.


// g++ or clang++ (or doxygen :-) )
#if defined(__GNUC__) || defined(__clang__) || defined(TOMOGRAPHER_PARSED_BY_DOXGEN)
/** \brief attributes for a function accepting printf-like variadic arguments
 *
 * Put this macro in front of custom functions which accept printf-like formatted
 * arguments. The attributes which this macro expands to instructs the compiler to warn
 * the user against wrong printf formats.
 *
 * Use the macro PRINTF1_ARGS_SAFE if the format string is the first argument of the
 * function and the arguments immediately follow, i.e. if the function has signature
 * \code
 *   <return-type> function(const char * fmt, ...);
 * \endcode
 * If you have other arguments preceeding the format string, use the respective macros
 * \ref PRINTF2_ARGS_SAFE, \ref PRINTF3_ARGS_SAFE or \ref PRINTF4_ARGS_SAFE. Note that for
 * non-static class members there is always the implicit \c this parameter, so you should
 * count the arguments starting from 2, not 1.
 *
 * See also the `format' attribute at
 * https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html#Common-Function-Attributes .
 */
#  define PRINTF1_ARGS_SAFE  __attribute__ ((__format__ (__printf__, 1, 2)))
//! See \ref PRINTF1_ARGS_SAFE
#  define PRINTF2_ARGS_SAFE  __attribute__ ((__format__ (__printf__, 2, 3)))
//! See \ref PRINTF1_ARGS_SAFE
#  define PRINTF3_ARGS_SAFE  __attribute__ ((__format__ (__printf__, 3, 4)))
//! See \ref PRINTF1_ARGS_SAFE
#  define PRINTF4_ARGS_SAFE  __attribute__ ((__format__ (__printf__, 4, 5)))
#else
#  define PRINTF1_ARGS_SAFE
#  define PRINTF2_ARGS_SAFE
#  define PRINTF3_ARGS_SAFE
#  define PRINTF4_ARGS_SAFE
#endif




#if TOMOGRAPHER_PARSED_BY_DOXYGEN
/**  Force stack realign for some functions, where the stacks may not aligned by default
 * (e.g. when using threads for Windows---set this attribute to any external entry points
 * as http://stackoverflow.com/a/6718067/1694896 as well as
 * http://www.peterstock.co.uk/games/mingw_sse/)
 *
 * \todo Look up the exact conditions (compilers, platforms, stack sizes etc.) to make
 *       sure this macro has a correct value in all cases.
 */
#  define TOMOGRAPHER_CXX_STACK_FORCE_REALIGN
#else // TOMOGRAPHER_PARSED_BY_DOXYGEN
//
#if defined(__GNUC__) || defined(__clang__)
#  if defined(__MINGW32__) || defined(__MINGW64__)
#    define TOMOGRAPHER_CXX_STACK_FORCE_REALIGN __attribute__((force_align_arg_pointer,noinline))
#  else
#    define TOMOGRAPHER_CXX_STACK_FORCE_REALIGN
#  endif
#elif defined(__ICC)
// but ICC is fine
#  define TOMOGRAPHER_CXX_STACK_FORCE_REALIGN
#else
#  warning "You are using an unknown compiler. You may run into memory alignment problems... Good luck!"
#  define TOMOGRAPHER_CXX_STACK_FORCE_REALIGN
#endif
//
#endif // TOMOGRAPHER_PARSED_BY_DOXYGEN


// Internal hacks...
// ---------------------------

// these are useful when writing SFINAE C++/C++11 hacks
namespace tomo_internal {
template<typename Enabledtype = void> struct sfinae_no { typedef int no[1]; };
template<typename EnabledType = void> struct sfinae_yes { typedef int yes[2]; };
} // namespace tomo_internal





/** \brief Return a suitable width for displaying stuff on the standard output
 *
 * If max_width > 0, return that value as is.
 *
 * If max_width <= 0, return the width of the screen (or default width) as given by the
 * environment variable \c COLUMNS, minus the absolute value of the given number. [E.g. if
 * the screen width is 100, then if maxwidth=-4, then return 96.]
 */
inline int getWidthForTerminalOutput(int max_width = 0)
{
  if (max_width > 0) {
    return max_width;
  }
  // max_width <= 0:
  const int offset = max_width;
  // decide of a maximum width to display
#if defined(__MINGW32__) || defined(__MINGW64__)
  max_width = 80; // Windows terminals are usually 80 chars wide
#else
  max_width = 100; // default maximum width
#endif
  // Note that $COLUMNS is not in the environment usually, so you have to set it manually
  // with e.g.
  //     shell> export COLUMNS=$COLUMNS
  const char * cols_s = std::getenv("COLUMNS");
  if (cols_s != NULL) {
    max_width = std::atoi(cols_s);
  }
  max_width += offset; // if we had given, e.g. maxwidth=-4
  return max_width;
}






// -----------------------------------------------------------------------------
// Function Name-Related Stuff
// -----------------------------------------------------------------------------



/** \brief Expands to the current function name or signature
 *
 * For \ref Tomographer::Logger::LocalLogger, use \ref TOMO_ORIGIN.
 */
#define TOMO_FUNCTION __PRETTY_FUNCTION__



namespace tomo_internal {
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
}; // helper struct
} // namespace tomo_internal


/** \brief Extract the function name from its signature.
 *
 * The argument is parsed as a function signature, and the name of the function (including
 * scope such as namespaces and classes) is returned.
 */
constexpr inline conststr extractFuncName(const conststr & funcname)
{
  return tomo_internal::extractFuncName_helper::extract(funcname);
}



} // namespace Tools
} // namespace Tomographer




/** \brief Define a simple exception class
 *
 * This macro defines a class named \a ClassName which derives from \a std::exception.
 * The constructor is given a string (the error message) which it prefixes with the given
 * \a ErrPrefix to form the full error message.
 *
 * The exception class consists of:
 *
 * - a constructor taking a single argument of type \a std::string.  The constructor
 *   stores the string internally, prefixing it with the error prefix.
 *
 * - the usual method <code>const char * what() const noexcept</code> retrieves the string
 *   passed to the constructor (including the error prefix).
 *
 * - another method <code>std::string msg() const noexcept</code> allows to retrieve the
 *   message string as an \a std::string (also including the error prefix).
 *
 * Example usage:
 * \code
 *   TOMOGRAPHER_DEFINE_MSG_EXCEPTION(invalid_input, "Invalid input: ") ;
 *   ...
 *   try {
 *     ...
 *     if (dim != matrix.rows()) {  // for example
 *       throw invalid_input("Matrix dimensions are incorrect!");
 *     }
 *     ...
 *   } catch (const std::exception & e) {
 *     std::cerr << "An error occurred, aborting.\n"
 *               << e.what() << "\n";
 *     ::exit(1);
 *   }
 * \endcode
 *
 */
#define TOMOGRAPHER_DEFINE_MSG_EXCEPTION(ClassName, ErrPrefix)          \
  TOMOGRAPHER_EXPORT class ClassName : public std::exception {          \
    std::string _msg;                                                   \
  public:                                                               \
    ClassName(std::string msg) : _msg(std::string(ErrPrefix) + std::move(msg)) { } \
    virtual ~ClassName() throw() { }                                    \
    std::string msg() const { return _msg; }                            \
    const char * what() const throw() { return _msg.c_str(); }          \
  };

/** \brief Define a simple exception class
 *
 * In contrast to \ref TOMOGRAPHER_DEFINE_MSG_EXCEPTION, the newly defined class does not
 * itself store the error message.  Rather, it passes it on to the base class \a BaseClass
 * which is responsible of storing it and returning a meaningful \a what() string.
 *
 * There is also no \a msg() method provided.
 */
#define TOMOGRAPHER_DEFINE_MSG_EXCEPTION_BASE(ClassName, ErrPrefix, BaseClass) \
  TOMOGRAPHER_EXPORT class ClassName : public BaseClass {               \
  public:                                                               \
    ClassName(std::string msg) : BaseClass(std::string(ErrPrefix) + std::move(msg)) { } \
    virtual ~ClassName() throw() { }                                    \
  };



namespace Tomographer {
namespace Tools {

/** \brief Ensure that a condition is met, or throw an exception
 *
 * If \a condition is \a true, no action is taken.  If \a condition is \a false, then an
 * instance of the exception \a ExceptionClass is thrown.  The exception is instantiated
 * using \a message as parameter.
 *
 * \tparam ExceptionClass is the exception to throw if the given condition is \a false
 *
 * \param condition controls whether to throw an error (if its value is \a false)
 *
 * \param message is the string to pass on to the exception class if condition is \a false
 */
template<typename ExceptionClass = std::runtime_error>
inline void tomographerEnsure(bool condition, std::string message) {
  if (!condition) {
    throw ExceptionClass(message);
  }
}


} // namespace Tools
} // namespace Tomographer



#endif
