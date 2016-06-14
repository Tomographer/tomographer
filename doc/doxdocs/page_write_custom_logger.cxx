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



/** \page pageCustomLogger Write a New Custom Logger
 *
 * To write a new, custom \c Logger class, you need to do the following:
 *
 *  - define your logger class and write the code that actually emits the log;
 *
 *  - define logger traits to characterize your logger.
 *
 * A simplistic example (simplified version of \ref Tomographer::Logger::FileLogger)
 * of a logger is given below.
 *
 * \code
 *
 *   class StderrLogger; // forward declaration
 *   namespace Tomographer { namespace Logger {
 *     // Traits for StderrLogger
 *     template<>
 *     struct LoggerTraits<StderrLogger> : DefaultLoggerTraits
 *     {
 *       enum {
 *         // fprintf is actually thread-safe, so our logger is thread-safe
 *         IsThreadSafe = 1,
 *         // set this to a particular level to unconditinally discard any
 *         // message logged with strictly lower importance level.
 *         StaticMinimumImportanceLevel = -1
 *       };
 *     };
 *   } } // namespaces
 *
 *   class StderrLogger : public Tomographer::Logger::LoggerBase<StderrLogger>
 *   {
 *   public:
 *     StderrLogger(int level = Tomographer::Logger::INFO)
 *       : Tomographer::Logger::LoggerBase<StderrLogger>(level)
 *     {
 *     }
 *
 *     // Change the log level
 *     //
 *     // WARNING: This method is not thread-safe!
 *     //
 *     inline void setLevel(int level)
 *     {
 *       // call the protected LoggerBase<StderrLogger>::setLogLevel()
 *       setLogLevel(level);
 *     }
 *   
 *     inline void emit_log(int level, const char * origin,
 *                          const std::string & msg)
 *     {
 *       std::string finalmsg;
 *
 *       if (level == Logger::ERROR) {
 *         finalmsg = "ERROR: ";
 *       } else if (level == Logger::WARNING) {
 *         finalmsg = "Warning: ";
 *       }
 *       
 *       finalmsg += "["+std::string(origin)+"] " + msg;
 *   
 *       // display the log message
 *       fprintf(fp, "%s\n", finalmsg.c_str());
 *     }
 *   };
 * \endcode
 *
 */

