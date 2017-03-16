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

#ifndef TOMOGRAPHER_DENSEDM_DENSELLH_H
#define TOMOGRAPHER_DENSEDM_DENSELLH_H

#include <cstddef>
#include <cmath>

#include <random>

#include <boost/math/constants/constants.hpp>

/** \file densellh.h
 *
 * \brief Some declarations for the DenseLLH type interface
 *
 * See the \ref pageInterfaceDenseLLH
 */

namespace Tomographer {
namespace DenseDM {

/** \brief Possible ways a \ref pageInterfaceDenseLLH "DenseLLH-compatible type" can
 *         calculate the llh function
 */
enum {

  /** \brief The DenseLLH-compatible type cannot calculate the LLH function; it is
   *         useless.
   */
  LLHCalcTypeInvalid = 0,
  
  /** \brief The DenseLLH-compatible object exposes a method \c logLikelihoodRho(), taking as
   *         argument a (const ref to a) DMTypes::MatrixType
   */
  LLHCalcTypeRho = 1,

  /** \brief The DenseLLH-compatible object exposes a method \c logLikelihoodX(), taking as
   *         argument a (const ref to a) DMTypes::VectorParamType
   */
  LLHCalcTypeX = 2

};








/** \brief Exception class notifying of invalid measurement data
 *
 * Implementations of a \ref pageInterfaceDenseLLH compliant type may throw this exception
 * to notify that the provided measurement data is invalid.
 *
 *
 */
TOMOGRAPHER_EXPORT class InvalidMeasData : public std::exception
{
  std::string _msg;
  std::string _fullmsg;
public:
  /** \brief Constructor with error message
   *
   * The string \c "Invalid Measurement Data: " is prepended to the given message to
   * construct the \ref fullMsg() which is also returned by \ref what().
   */
  InvalidMeasData(const std::string& msg)
    : _msg(msg), _fullmsg("Invalid Measurement Data: " + _msg) { }

  /** \brief Constructor with error message (given as rvalue reference)
   */
  InvalidMeasData(std::string&& msg)
    : _msg(std::move(msg)), _fullmsg("Invalid Measurement Data: " + _msg) { }

  virtual ~InvalidMeasData() throw() { }

  //! Get the message provided to the constructor
  inline std::string msg() const noexcept { return _msg; }
  //! Get the full error message
  inline std::string fullMsg() const noexcept { return _fullmsg; }
    
  //! Get the full error message as a pointer to a C string
  virtual const char * what() const noexcept { return _msg.c_str(); }
};



} // namespace DenseDM
} // namespace Tomographer



#endif
