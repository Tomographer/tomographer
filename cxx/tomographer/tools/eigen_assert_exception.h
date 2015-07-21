
#ifndef TOMOGRAPHER_TOOLS_EIGEN_ASSERT_EXCEPTION_H
#define TOMOGRAPHER_TOOLS_EIGEN_ASSERT_EXCEPTION_H

/** \file
 *
 * \brief Define tools for Eigen's \a eigen_assert() to throw an exception instead of
 * assert'ing.
 *
 * \note This header must be included BEFORE any Eigen header!
 */

#include <string>
#include <stdexcept>

namespace Tomographer {
namespace Tools {

/** \brief Exception that is thrown upon failed \c eigen_assert
 *
 * This is useful, e.g. for the test suites.
 */
class eigen_assert_exception : public std::exception
{
  std::string _msg;
public:
  eigen_assert_exception(const std::string& msg, const std::string& file, const std::size_t line)
    : _msg("eigen_assert() failed: `" + msg + "' at " + file + ", line " + std::to_string(line))
  {
  }
  virtual ~eigen_assert_exception() noexcept {}

  virtual const char * what() const noexcept { return _msg.c_str(); }
};

} // namespace Tools
} // namespace Tomographer



#define eigen_assert_throw_exception(x)         \
  if (!(x)) { throw (::Tomographer::Tools::eigen_assert_exception(#x, __FILE__, __LINE__)); }
  

#ifdef TOMOGRAPHER_EIGEN_ASSERT_EXCEPTION
// override Eigen's eigen_assert() macro
#undef eigen_assert
#define eigen_assert(x) eigen_assert_throw_exception(x)
#endif


#endif
