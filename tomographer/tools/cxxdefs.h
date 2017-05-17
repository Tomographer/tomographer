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

#include <boost/predef.h>

// NOTE: DO NOT INCLUDE EIGEN HEADERS IN THIS FILE! For instance eigen_assert_exception.h
// depends on cxxdefs.h not including any Eigen headers.




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




// -----------------------------------------------------------------------------
// Mark stuff as deprecated
// -----------------------------------------------------------------------------

//
// This is giving me headaches with different compilers & compatibility ... just document
// stuff as being deprecated.
//

// #ifdef TOMOGRAPHER_PARSED_BY_DOXYGEN

// /** \brief Mark a class declaration as deprecated
//  *
//  * Use for instance as:
//  * \code
//  *    template<typename CountType>
//  *    TOMOGRAPHER_DEPRECATED_CLASS(MyObject) : public Base<CountType> { ... }
//  * \endcode
//  */
// #  define TOMOGRAPHER_DEPRECATED_CLASS(X)  class X

// /** \brief Mark a class declaration as deprecated
//  *
//  * Use for instance as:
//  * \code
//  *    template<typename CountType>
//  *    TOMOGRAPHER_DEPRECATED_STRUCT(MyObject) : public Base<CountType> { ... }
//  * \endcode
//  */
// #  define TOMOGRAPHER_DEPRECATED_STRUCT(X)  struct X

// /** \brief Mark a function declaration as deprecated
//  *
//  * Use as:
//  * \code
//  *   TOMOGRAPHER_DEPRECATED_FUNC(double f(int x, int y)) {
//  *     return (double)x / y;
//  *   }
//  * \endcode
//  */
// #  define TOMOGRAPHER_DEPRECATED_FUNC(FuncDecl) FuncDecl

// /** \brief Mark a "using x = y" declaration as deprecated
//  *
//  * Use as:
//  * \code
//  *    // using MyAlias = OriginalType
//  *    TOMOGRAPHER_DEPRECATED_USING(MyAlias, OriginalType);
//  * \endcode
//  */
// #  define TOMOGRAPHER_DEPRECATED_USING(X, Y)  using X = Y

// #else
// // implementation:

// #if defined(__has_cpp_attribute)
// #  if __has_cpp_attribute(deprecated)
// #    define _tomographer_has_cxx_deprecated_attribute
// #  endif
// #endif

// # if defined(_tomographer_has_cxx_deprecated_attribute)
// #  define TOMOGRAPHER_DEPRECATED [[deprecated]]
// #  define TOMOGRAPHER_DEPRECATED_FUNC(...) TOMOGRAPHER_DEPRECATED __VA_ARGS__
// #  define TOMOGRAPHER_DEPRECATED_CLASS(...) class TOMOGRAPHER_DEPRECATED __VA_ARGS__
// #  define TOMOGRAPHER_DEPRECATED_STRUCT(...) struct TOMOGRAPHER_DEPRECATED __VA_ARGS__
// #  define TOMOGRAPHER_DEPRECATED_USING(X, ...)  using X TOMOGRAPHER_DEPRECATED = __VA_ARGS__
// # elif defined(__GNUC__)
// #  define TOMOGRAPHER_DEPRECATED __attribute__((deprecated))
// #  define TOMOGRAPHER_DEPRECATED_FUNC(...) __VA_ARGS__ TOMOGRAPHER_DEPRECATED
// #  define TOMOGRAPHER_DEPRECATED_CLASS(...) class TOMOGRAPHER_DEPRECATED __VA_ARGS__
// #  define TOMOGRAPHER_DEPRECATED_STRUCT(...) struct TOMOGRAPHER_DEPRECATED __VA_ARGS__
// #  define TOMOGRAPHER_DEPRECATED_USING(X, ...)  using TOMOGRAPHER_DEPRECATED X = __VA_ARGS__
// # elif defined(_MSC_VER)
// #  define TOMOGRAPHER_DEPRECATED __declspec(deprecated)
// #  define TOMOGRAPHER_DEPRECATED_FUNC(...) __VA_ARGS__ TOMOGRAPHER_DEPRECATED
// #  define TOMOGRAPHER_DEPRECATED_CLASS(...) class TOMOGRAPHER_DEPRECATED __VA_ARGS__
// #  define TOMOGRAPHER_DEPRECATED_STRUCT(...) struct TOMOGRAPHER_DEPRECATED __VA_ARGS__
// #  define TOMOGRAPHER_DEPRECATED_USING(X, ...)  TOMOGRAPHER_DEPRECATED using X = __VA_ARGS__
// # else
// #  define TOMOGRAPHER_DEPRECATED_FUNC(...) __VA_ARGS__
// #  define TOMOGRAPHER_DEPRECATED_CLASS(...) class  __VA_ARGS__
// #  define TOMOGRAPHER_DEPRECATED_STRUCT(...) struct  __VA_ARGS__
// #  define TOMOGRAPHER_DEPRECATED_USING(X, ...)  using X = __VA_ARGS__
// # endif

// #endif




// -----------------------------------------------------------------------------
// Compiler information
// -----------------------------------------------------------------------------

// see http://www.boost.org/doc/libs/1_62_0/doc/html/predef/reference.html#predef.reference.boost_comp_compiler_macros

#ifdef TOMOGRAPHER_PARSED_BY_DOXYGEN
/** \brief Expands to a std::string with a brief descritpion of the compiler used (including version)
 *
 */
#define TOMOGRAPHER_COMPILER_INFO_STR /* ... */
#else
// implementation of TOMOGRAPHER_COMPILER_INFO_STR:
#if BOOST_COMP_BORLAND
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("Borland C++ %x", __BORLANDC__)
#elif BOOST_COMP_CLANG
#  ifdef __apple_build_version__
#    define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("Apple LLVM/clang++ %s", __clang_version__)
#  else
#    define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("LLVM/clang++ %s", __clang_version__)
#  endif
#elif BOOST_COMP_COMO
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("Comeau C++ %d", __COMO_VERSION__)
#elif BOOST_COMP_DEC
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("Compaq C/C++ %d", __DECCXX_VER)
#elif BOOST_COMP_DIAB
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("Diab C/C++ %d", __VERSION_NUMBER__)
#elif BOOST_COMP_DMC
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("Digital Mars %x", __DMC__)
#elif BOOST_COMP_SYSC
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("Dignus Systems/C++ %d", __SYSC_VER__)
#elif BOOST_COMP_EDG
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("EDG C++ Frontend %d", __EDG_VERSION__)
#elif BOOST_COMP_PATH
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("EKOpath %d.%d.%d",  __PATHCC__, __PATHCC_MINOR__, __PATHCC_PATCHLEVEL__)
#elif BOOST_COMP_GNUC
#  ifdef __MINGW32__
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("MinGW GCC %d.%d.%d", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
#  else
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("Gnu GCC %d.%d.%d", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
#  endif
#elif BOOST_COMP_GCCXML
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("GCC XML")
#elif BOOST_COMP_GHS
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("Green Hills C/C++ %d", __GHS_VERSION_NUMBER__)
#elif BOOST_COMP_HPACC
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("HP aC++ %d", __HP_aCC)
#elif BOOST_COMP_IAR
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("IAR C/C++ %d", __VER__)
#elif BOOST_COMP_IBM
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("IBM XL C/C++ %d", __IBMCPP__)
#elif BOOST_COMP_INTEL
#  ifdef __INTEL_COMPILER_BUILD_DATE
#    define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("Intel C/C++ %d (%d)", __INTEL_COMPILER, __INTEL_COMPILER_BUILD_DATE)
#  else
#    define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("Intel C/C++ %d", __INTEL_COMPILER)
#  endif
#elif BOOST_COMP_KCC
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("Kai C++ %d", __KCC_VERSION)
#elif BOOST_COMP_LLVM // not covered by BOOST_COMP_CLANG???
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("<LLVM>")
#elif BOOST_COMP_HIGHC
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("MetaWare High C/C++")
#elif BOOST_COMP_MWERKS
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("Metrowerks CodeWarrior %x", __MWERKS__)
#elif BOOST_COMP_MRI
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("Microtec C/C++")
#elif BOOST_COMP_MPW
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("MPW C++ %x", __MRC__)
#elif BOOST_COMP_PALM
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("Palm C/C++ %x", _PACC_VER)
#elif BOOST_COMP_PGI
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("Portland Group C/C++ %d.%d.%d",  __PGIC__, __PGIC_MINOR__, __PGIC_PATCHLEVEL__)
#elif BOOST_COMP_SGI
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("SGI MIPSpro %d", _SGI_COMPILER_VERSION)
#elif BOOST_COMP_SUNPRO
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("Oracle Solaris Studio %x", __SUNPRO_CC)
#elif BOOST_COMP_TENDRA
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("TenDRA C/C++")
#elif BOOST_COMP_MSVC
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("Microsoft Visual C/C++ %d", _MSC_FULL_VER)
#elif BOOST_COMP_WATCOM
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("Watcom C++ %d", __WATCOMC__)
#elif defined(__VERSION__)
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("<unknown compiler, version %s>", __VERSION__)
#else
#  define TOMOGRAPHER_COMPILER_INFO_STR Tomographer::Tools::fmts("<unknown compiler>")
#endif
// end implementation of TOMOGRAPHER_COMPILER_INFO_STR:
#endif






#endif
