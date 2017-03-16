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

#ifndef TOMOGRAPHER_TOOLS_DEFS_H
#define TOMOGRAPHER_TOOLS_DEFS_H

/** \file cxxdefs.h
 *
 * \brief Basic definitions, which may have to be defined before any Eigen headers or
 * further utilities are included.
 *
 */


#include <cassert>
#include <cstddef>
#include <cstdlib>


// NOTE: DO NOT INCLUDE EIGEN HEADERS IN THIS FILE! For instance eigen_assert_exception.h
// depends on cxxutil.h not including any Eigen headers.




// -----------------------------------------------------------------------------
// Some general useful definitions
// -----------------------------------------------------------------------------


//
// Included in front of all public symbols.  If you need, you may define this to
// e.g. "__attribute__((visibility("default")))" or "__declspec(dllexport)" to export the
// definition in the library
//
#ifndef TOMOGRAPHER_EXPORT
#define TOMOGRAPHER_EXPORT
#endif


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


#ifndef tomographer_assert
/** \brief Assertion test macro
 */
#define tomographer_assert(...) eigen_assert((__VA_ARGS__) && "assert: " #__VA_ARGS__)
#endif



#ifndef TOMOGRAPHER_PARSED_BY_DOXYGEN
// WARNING!!! CHECK OUT  http://stackoverflow.com/q/29363532/1694896
// FOR VERY SUBTLE BUGS :( :(   -- TEST WITH INTEL ICC!!
#define TOMOGRAPHER_ENABLED_IF(...)					\
  template<bool _dummy__enabledif = false,				\
	   typename std::enable_if<_dummy__enabledif || (__VA_ARGS__), bool>::type \
                                                        _dummy__enabledif2 = false>
#define TOMOGRAPHER_ENABLED_IF_TMPL(...)				\
  bool _dummy__enabledif = false,				\
  typename std::enable_if<_dummy__enabledif || (__VA_ARGS__), bool>::type \
                                                 _dummy__enabledif2 = true

/** \brief Altenative to \ref TOMOGRAPHER_ENABLED_IF_TMPL()
 *
 * Use this alternative to \ref TOMOGRAPHER_ENABLED_IF_TMPL() in case you get compiler
 * errors about "repeated default value for argument _dummy__enabledif" because of another
 * similar declaration just before.
 */
#define TOMOGRAPHER_ENABLED_IF_TMPL_REPEAT(...)				\
  bool _dummy__enabledif,						\
  typename std::enable_if<_dummy__enabledif || (__VA_ARGS__), bool>::type \
			  _dummy__enabledifAlt2 = true
#endif







#endif
