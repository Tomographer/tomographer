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

namespace Tomographer {
namespace Tools {

/** \brief Exception that is thrown upon failed \c eigen_assert
 *
 * This is useful, e.g. for the test suites.
 */
class EigenAssertException : public std::exception
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



/** \brief Macro like \a eigen_assert(), but which throws an exception.
 *
 * You can use this macro in a definition of \c "eigen_assert()". If the assertion
 * condition fails, then an exception of type \ref
 * Tomographer::Tools::EigenAssertException is thrown.
 */
#define eigen_assert_throw_exception(x)         \
  if (!(x)) { throw (::Tomographer::Tools::EigenAssertException(#x, __FILE__, __LINE__)); }
  

#ifdef TOMOGRAPHER_EIGEN_ASSERT_EXCEPTION
// override Eigen's eigen_assert() macro
#undef eigen_assert
#define eigen_assert(x) eigen_assert_throw_exception(x)
#endif


#endif
