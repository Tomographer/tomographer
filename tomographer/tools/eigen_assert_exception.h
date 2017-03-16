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

#ifndef TOMOGRAPHER_TOOLS_EIGEN_ASSERT_EXCEPTION_H
#define TOMOGRAPHER_TOOLS_EIGEN_ASSERT_EXCEPTION_H

/** \file eigen_assert_exception.h
 *
 * \brief Define tools for Eigen's \a eigen_assert() to throw an exception instead of
 * assert'ing.
 *
 * \note This header must be included BEFORE any Eigen header!
 *
 * \note If you define the preprocessor symbol \c TOMOGRAPHER_EIGEN_ASSERT_EXCEPTION
 * before including this file, then the macro \c eigen_assert() is defined such that it
 * throws an exception upon failure. (In this case, \c eigen_assert() is defined to call
 * \ref eigen_assert_throw_exception().)
 *
 */

#include <string>
#include <stdexcept>

#include <tomographer/tools/cxxdefs.h>

namespace Tomographer {
namespace Tools {

/** \brief Exception that is thrown upon failed \c eigen_assert
 *
 * This is useful, e.g. for the test suites.
 *
 * \note Eigen's \ref Eigen::CommaInitializer "CommaInitializer" has an assertion check in
 * its destructor, which in C++11 are automatically \c noexcept.  This causes warnings
 * under g++6, and the code will call terminate() at runtime if the assertion fails.
 */
TOMOGRAPHER_EXPORT class EigenAssertException : public std::exception
{
  std::string _msg;
public:
  EigenAssertException(const std::string& msg, const std::string& file, const std::size_t line)
    : _msg("eigen_assert() failed: `" + msg + "' at " + file + ", line " + std::to_string(line))
  {
  }
  virtual ~EigenAssertException() noexcept {}

  virtual const char * what() const noexcept { return _msg.c_str(); }
};

} // namespace Tools
} // namespace Tomographer



// define some Eigen internal magic (undocumented, hope they don't change this in the
// future) in case there is a eigen_assert() in a destructor (e.g. CommaInitializer)
#define VERIFY_RAISES_ASSERT 1
namespace Eigen {
typedef Tomographer::Tools::EigenAssertException eigen_assert_exception;
}


/** \brief Macro like \a eigen_assert(), but which throws an exception.
 *
 * You can use this macro in a definition of \c "eigen_assert()". If the assertion
 * condition fails, then an exception of type \ref
 * Tomographer::Tools::EigenAssertException is thrown.
 */
#define eigen_assert_throw_exception(x)         \
  if (!(x)) {                                                           \
    tomographer_eigen_assert_failure_cleanup();                         \
    throw (::Tomographer::Tools::EigenAssertException(#x, __FILE__, __LINE__)); \
  }

/** \brief Gets called when we use eigen assertion exceptions and when eigen_assert() fails
 *
 * This function does nothing. However, you can override this function depending on the
 * context, by locally defining another function called \a
 * %tomographer_eigen_assert_failure_cleanup().  Because eigen_assert_throw_exception() is
 * a macro which, in its expansion, invokes \a
 * %tomographer_eigen_assert_failure_cleanup(), the closest function context-wise is
 * invoked.
 *
 * This mechanism should be used in case classes must clean up after an eigen assertion
 * failure.  The problem is the following: for instance, suppose a class \a X has calls to
 * eigen_assert() in its destructor (note that such destructors must be declared
 * <code>noexcept(false)</code> explicitly, as per C++11 standard). Now imaging that there
 * is an other eigen_assert() failure somewhere else in one of \a X's methods, causing an
 * exception to be thrown. If the instance of \a X goes out of scope before the exception
 * is caught, it is highly likely that the eigen_assert() in the destructor also fails. In
 * this case, no exception is thrown and std::terminate() is called (with g++ 6 and
 * clang++; not sure what the C++ standard mandates).  To solve this, \a X should provide
 * a private method called \a %tomographer_eigen_assert_failure_cleanup() which sets the
 * class back to a consistent state, after which eigen_assert() tests in the destructor
 * won't fail.
 *
 * This problem arises in particular for Eigen's Eigen::CommaInitializer class; we use the
 * above mechanism in a patched class in the tests to avoid this problem (see
 * <code>test/test_tomographer.h</code>).
 *
 */
inline void tomographer_eigen_assert_failure_cleanup() { }



#ifdef TOMOGRAPHER_EIGEN_ASSERT_EXCEPTION
// override Eigen's eigen_assert() macro
#undef eigen_assert
#define eigen_assert(x) eigen_assert_throw_exception(x)
#endif


#endif
