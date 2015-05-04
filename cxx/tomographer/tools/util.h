
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
tomo_internal::FinalAction<F> finally(F f)
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



} // namespace Tools
} // namespace Tomographer



#endif
