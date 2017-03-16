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


#ifndef TOMOGRAPHER_TOOLS_BOOST_TEST_LOGGER_H
#define TOMOGRAPHER_TOOLS_BOOST_TEST_LOGGER_H

/** \file boost_test_logger.h
 *
 * \brief Provide a logger class for when using the Boost Unit Test Framework (see \ref
 *        Tomographer::Logger::BoostTestLogger).
 */


#include <tomographer/tools/loggers.h>


namespace Tomographer { namespace Logger {

class BoostTestLogger;

// traits for BoostTestLogger
template<>
struct LoggerTraits<BoostTestLogger> : DefaultLoggerTraits
{
  enum {
    IsThreadSafe = 0, StaticMinimumImportanceLevel = -1
  };
};

/** \brief  A logger we can use in tests, which calls \c BOOST_TEST_MESSAGE()
 *
 * A logger implementation for use when writing tests with the Boost Unit Test Framework.
 *
 * This logger simply relays all messages to the \c BOOST_TEST_MESSAGE() macro to display
 * testing output.
 *
 * Specify a logging level to the constructor, by default it is \c DEBUG.
 */
TOMOGRAPHER_EXPORT class BoostTestLogger : public Tomographer::Logger::LoggerBase<BoostTestLogger>
{
public:
  BoostTestLogger(int level = Tomographer::Logger::DEBUG)
    : Tomographer::Logger::LoggerBase<BoostTestLogger>(level)
  {
  }
  inline void setLevel(int level)
  {
    // call the protected LoggerBase<StderrLogger>::setLogLevel()
    setLogLevel(level);
  }
  inline void emitLog(int level, const char * origin,
                      const std::string & msg)
  {
    BOOST_TEST_MESSAGE("(" << Tomographer::Logger::LogLevel(level) << ")" << "[" << origin << "] " << msg) ;
  }
};




} } // namespaces



#endif
