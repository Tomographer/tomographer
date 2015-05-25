
#ifndef TOMOGRAPHER_TOOLS_EIGEN_ASSERT_EXCEPTION_H
#define TOMOGRAPHER_TOOLS_EIGEN_ASSERT_EXCEPTION_H

// must be included BEFORE any Eigen header

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
  eigen_assert_exception(const std::string& msg)
    : _msg("eigen_assert() failed: `" + msg + "'")
  {
  }
  virtual ~eigen_assert_exception() noexcept {}

  virtual const char * what() const noexcept { return _msg.c_str(); }
};

} // namespace Tools
} // namespace Tomographer


// override Eigen's eigen_assert() macro
#undef eigen_assert
#define eigen_assert(x) \
  if (!(x)) { throw (::Tomographer::Tools::eigen_assert_exception(#x)); }


#include <Eigen/Core>


#endif
