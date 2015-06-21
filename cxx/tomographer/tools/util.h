
#ifndef TOMOGRAPHER_TOOLS_UTILS_H
#define TOMOGRAPHER_TOOLS_UTILS_H

#include <type_traits>

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
 *   auto delete_ptr = finally([ptr] { delete ptr; });
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




/** \brief Test whether the given value is positive or zero.
 *
 * This helper is useful to silence warnings in templates about `comparision of unsigned
 * >= 0 is always true'.
 */
template<typename X>
inline typename std::enable_if<std::is_unsigned<X>::value, bool>::type is_positive(const X /* val */)
{
  return true;
}
//! See \ref is_positive()
template<typename X>
inline typename std::enable_if<!std::is_unsigned<X>::value, bool>::type is_positive(const X val)
{
  return val >= 0;
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
 * \tparam T the type of the value to store.
 * \tparam Value the (integer) value to store at compile-time. If this value is \a
 *         Eigen::Dynamic, then the value will be stored at runtime.
 *
 */
template<typename T_, int Value>
class static_or_dynamic
{
public:
  typedef T_ T;
  static constexpr T ValueAtCTime = Value;

  inline static_or_dynamic() { }
  inline explicit static_or_dynamic(T val) {
    assert(val == ValueAtCTime);
    (void)val; // silence "unused parameter" warning
  }

  inline T value() const { return ValueAtCTime; }
  inline T operator()() const { return ValueAtCTime; }
};

template<typename T_>
class static_or_dynamic<T_, Eigen::Dynamic>
{
public:
  typedef T_ T;
  static constexpr T ValueAtCTime = Eigen::Dynamic;
  
  static_or_dynamic() = delete;  // no default constructor.
  inline explicit static_or_dynamic(T val) : _dyn_value(val) { }

  inline T value() const { return _dyn_value; }
  inline T operator()() const { return value(); }
private:
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
  typedef T_ T;
  static constexpr bool is_enabled = false;

  template<typename... Args>
  explicit store_if_enabled(Args...) { }
};
template<typename T_>
struct store_if_enabled<T_, true>
{
  typedef T_ T;
  static constexpr bool is_enabled = true;

  T value;

  template<typename... ArgTypes>
  explicit store_if_enabled(const ArgTypes& ...  args) : value(args...) { }
};

template<typename T>
inline std::ostream & operator<<(std::ostream & str, store_if_enabled<T, false> /*val*/)
{
  return str << "[-]";
}
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


} // namespace Tools
} // namespace Tomographer



#endif
