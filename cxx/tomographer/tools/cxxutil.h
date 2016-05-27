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

#ifndef TOMOGRAPHER_TOOLS_UTILS_H
#define TOMOGRAPHER_TOOLS_UTILS_H

/** \file tomographer/tools/util.h
 *
 * \brief Some C++ utilities, with a tad of C++11 tricks.
 *
 */

#include <type_traits>

#include <Eigen/Core> // Eigen::Dynamic

#include <tomographer/tools/conststr.h>


// -----------------------------------------------------------------------------
// Define functions with printf-safe arguments, with compiler-generated warnings
// -----------------------------------------------------------------------------


// The PRINTFX_ARGS_SAFE macros are defined here, so that in the future we may support a
// wider range of compilers which may not understand the __attribute__(format)
// instruction.


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
#define PRINTF1_ARGS_SAFE  __attribute__ ((__format__ (__printf__, 1, 2)))

//! See \ref PRINTF1_ARGS_SAFE
#define PRINTF2_ARGS_SAFE  __attribute__ ((__format__ (__printf__, 2, 3)))

//! See \ref PRINTF1_ARGS_SAFE
#define PRINTF3_ARGS_SAFE  __attribute__ ((__format__ (__printf__, 3, 4)))

//! See \ref PRINTF1_ARGS_SAFE
#define PRINTF4_ARGS_SAFE  __attribute__ ((__format__ (__printf__, 4, 5)))



// -----------------------------------------------------------------------------
// Some C++ helpers
// -----------------------------------------------------------------------------



/** \brief Tool for static assertions without message.
 *
 * Simply use as message to C++11's \a static_assert() a stringified version of the
 * expression itself.
 */
#define TOMO_STATIC_ASSERT_EXPR(...)				\
  static_assert(__VA_ARGS__, #__VA_ARGS__)



namespace Tomographer
{
namespace Tools
{

// taken from http://stackoverflow.com/a/25510879/1694896

namespace tomo_internal {
  template <typename F>
  struct FinalAction {
    FinalAction(F f) : clean_{f} {}
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
template<typename T_, bool IsDynamic_, T_ StaticValue_>
class static_or_dynamic
{

public:
  // definitions here are for the static case, the dynamic case will be specialized below.
  TOMO_STATIC_ASSERT_EXPR(IsDynamic_ == false) ;

  //! Type of the value we are storing
  typedef T_ T;
  //! Whether this value is flexible at run-time (dynamic), or fixed at compile-time (static)
  static constexpr bool IsDynamic = IsDynamic_;
  //! The value, if stored at compile-time, or \ref Eigen::Dynamic.
  static constexpr T StaticValue = StaticValue_;

  //! Default Constructor. Only if the value is stored at compile-time.
  inline static_or_dynamic() { }
  /** \brief Constructor with an explicit value.
   *
   * The value \a val must equal the compile-time fixed value, if any, or you'll get an
   * assert failure.
   */
  inline explicit static_or_dynamic(T val) {
    eigen_assert(val == StaticValue);
    (void)val; // silence "unused parameter" warning if eigen_assert() gets optimized out
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

/** \brief Template Specialization -- see \ref static_or_dynamic<T_,IsDynamic_,StaticValue_>
 *
 * Specialization for the case if the value is known only at runtime.
 */
template<typename T_, T_ StaticValue_>
class static_or_dynamic<T_, true, StaticValue_>
{
public:

  //! Type of the value we are storing. See \ref static_or_dynamic<T_,IsDynamic_,StaticValue_>::T
  typedef T_ T;
  //! Whether this value is flexible at run-time (dynamic), or fixed at compile-time (static).
  static constexpr bool IsDynamic = true;

  
  //! No default constructor.
  static_or_dynamic() = delete;
  /** \brief Constructor which initializes the value to \a val.
   *
   * The stored value may not be subsequently changed.
   */
  inline explicit static_or_dynamic(T val) : _dyn_value(val) { }


  //! See \ref static_or_dynamic<typename T_, int Value>::value()
  inline T value() const { return _dyn_value; }

  //! See \ref static_or_dynamic<typename T_, int Value>::operator()()
  inline T operator()() const { return value(); }

private:

  //! The dynamically stored value
  const T _dyn_value;
};


// -----------------------------------------------------------------------------


/** \brief Utility that stores a data type if a compile-time flag is enabled.
 *
 * If \a enabled is \c true, then this type defines a public member \a value of type \a T.
 *
 * The constructor always accepts any number of arguments. They are either ignored (if
 * nothing is stored), or relayed on to the \a value's constructor.
 *
 */
template<typename T_, bool enabled>
struct store_if_enabled
{
  //! The type we're storing
  typedef T_ T;
  //! Whether we're storing a value or not -- by default no
  static constexpr bool is_enabled = false;

  //! Constructor
  template<typename... Args>
  explicit store_if_enabled(Args...) { }
};
/** \brief Specialization of \ref store_if_enabled<T_,enabled> for if we're storing a
 *         value
 *
 */
template<typename T_>
struct store_if_enabled<T_, true>
{
  //! The type we're storing
  typedef T_ T;
  //! Whether we're storing a value or not -- yes.
  static constexpr bool is_enabled = true;

  //! This property keeps the value we're supposed to store.
  T value;

  //! Constructor. Any arguments are passed to the value's constructor.
  template<typename... ArgTypes>
  explicit store_if_enabled(const ArgTypes& ...  args) : value(args...) { }
};

/** \brief C++ Stream operators for \ref store_if_enabled<T,enabled>
 *
 * Produces human-readable output.
 */
template<typename T>
inline std::ostream & operator<<(std::ostream & str, store_if_enabled<T, false> /*val*/)
{
  return str << "[-]";
}
/** \brief C++ Stream operators for \ref store_if_enabled<T,enabled>
 *
 * Produces human-readable output.
 */
template<typename T>
inline std::ostream & operator<<(std::ostream & str, store_if_enabled<T, true> val)
{
  return str << val.value;
}



// -----------------------------------------------------------------------------

/** \brief Return true if the argument is a power of two, false otherwise.
 */
inline constexpr bool is_power_of_two(int N)
{
  // taken from http://stackoverflow.com/q/10585450/1694896
  return N && !(N & (N - 1));
}




// -----------------------------------------------------------------------------



/** \brief Expands to the current function name or signature
 *
 * For \ref Logger::LocalLogger, use \ref TOMO_ORIGIN.
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





// -----------------------------------------------------------------------------

#ifndef TOMOGRAPHER_PARSED_BY_DOXYGEN
// WARNING!!! CHECK OUT  http://stackoverflow.com/q/29363532/1694896
// FOR VERY SUBTLE BUGS....... :( :(   -- TEST WITH INTEL ICC!!
#define TOMOGRAPHER_ENABLED_IF(...)					\
  template<bool _dummy__enabledif = false,				\
	   typename std::enable_if<_dummy__enabledif || (__VA_ARGS__), bool>::type \
                                                        _dummy__enabledif2 = false>
#define TOMOGRAPHER_ENABLED_IF_TMPL(...)				\
  bool _dummy__enabledif = false,				\
  typename std::enable_if<_dummy__enabledif || (__VA_ARGS__), bool>::type \
                                                 _dummy__enabledif2 = true
#endif


} // namespace Tools
} // namespace Tomographer



#endif
