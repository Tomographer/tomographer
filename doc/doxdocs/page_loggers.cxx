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


/** \page pageLoggers Logging and Loggers
 *
 * %Tomographer provides a lightweight framework for logging, i.e. producing messages
 * which inform the user and/or developer about what the program is doing. Objects who
 * would like to log messages take a template type parameter \a Logger, and an instance of
 * such a type usually provided to its constructor. The \a Logger type must be a subclass
 * of \ref Tomographer::Logger::LoggerBase.
 *
 * <h2>Basic Usage</h2>
 *
 * Log messages have different levels of importance, which are \ref
 * Tomographer::Logger::ERROR, \ref Tomographer::Logger::WARNING, \ref
 * Tomographer::Logger::INFO, \ref Tomographer::Logger::DEBUG, and \ref
 * Tomographer::Logger::LONGDEBUG. Please read the documentation for these levels and make
 * sure you choose the right level for your messages.
 *
 * To log messages into a logger object \c logger, one simply calls, depending on the
 * level, one of for example:
 * \code
 *   logger.longdebug("origin_fn()", "Iteration k=%d, new value is = %.4g", k, newvalue);
 *   logger.debug("origin_fn()", "Starting new instance of a random walk with %d iterations", num_iterations);
 *   logger.info("origin_fn()", "Data successfully read, dim = %d", data->dim);
 *   logger.warning("origin_fn()", "Failed to nice() process to value %d", nice_val);
 *   logger.error("origin_fn()", "Can't read file `%s': %s", fname.c_str(), strerror(errno));
 * \endcode
 *
 * As you see, for each level, you have a corresponding method to call. The first argument
 * is a constant string literal which should specify where the message originated from. It
 * need not be displayed by all loggers, but is really helpful to trace bugs and track
 * down where the program actually is.
 *
 * Your messages should not end in newlines. Newlines will automatically be added by
 * loggers which log into files and/or the console.
 *
 * <h2>Formatting Flavors</h2>
 *
 * Each of the above methods comes in different flavors, depending on whether you would
 * like to print a message which is already formatted, to have your message formatted \c
 * printf -style, or to use C++ streams to display your message:
 * \code
 * logger.debug(const char * origin, const char * fmt, ...); // printf-style
 * logger.debug(const char * origin, const std::string & msg); // already formatted
 * logger.debug(const char * origin, Fn fn); // lambda callback
 * \endcode
 * The last variant takes a callable argument and calls it with an
 * <code>std::ostream&</code> argument. The function should then stream whatever it wants
 * into the stream. This is particularly useful with C++11 lambdas, for example:
 * \code
 * logger.debug("origin_fn()", [value](std::ostream& stream) {
 *                stream << "Value is = " << value;
 *              });
 * \endcode
 *
 * This last idiom is also very useful if producing the log message itself is 
 * resource-consuming. Imagine you wish to pretty-print a histogram for debugging:
 * \code
 *   logger.debug("origin_fn()", [&histogram](std::ostream& stream) {
 *                  stream << "Histogram: \n" << histogram.pretty_print();
 *                });
 * \endcode
 * The call to the lambda, and thus to <code>histogram.pretty_print()</code>, will only be
 * performed if the logger will indeed eventually print the message.
 *
 * \warning When using printf-style logging methods in templated classes, remember to
 *          always cast the value to a known type, or else your code may be invalid for a
 *          different type. It is preferrable in such cases to use the C++ lambda
 *          stream mechanism.  Compare:
 *          \code
 *            template<typename ValType = double> my_function(ValType value) { 
 *              // INVALID if ValType=float,int,... (likely crash!) :
 *              logger.debug("my_function()", "value is %f", value);
 *              // Safe, although possibly inappropriate format (say if ValType=int):
 *              logger.debug("my_function()", "value is %f", (double)value);
 *              // Safe:
 *              logger.debug("my_function()", [&](std::ostream & stream) {
 *                  stream << "value is " << value;
 *                });
 *            }
 *          \endcode
 *
 *
 * <h2>Scoped Logger with (Semi-)Automatic Origin Handling</h2>
 *
 * To avoid specifying the \a origin parameter for repeated calls within the same class or
 * function, you may use a Tomographer::Logger::LocalLogger, where you set the origin once
 * in the constructor and don't specify it later on. Also, you may use it recursively.  In
 * the following example, the origin of the log messages are automatically set to \c
 * "my_function()" and \c "my_function()/some_callback[lambda]", respectively:
 * \code
 *   template<typename LoggerType>
 *   int my_function(int value, LoggerType & baselogger) {
 *     auto logger = makeLocalLogger(TOMO_ORIGIN, baselogger);
 *     logger.debug("value is %d", value);
 *  
 *     auto some_callback = [&](std::string some_other_value) {
 *       auto innerlogger = logger.sublogger("some_callback[lambda]");
 *       innerlogger.debug([&](std::ostream& str) {
 *           str << "Inside callback: " << some_other_value;
 *         });
 *     };
 *     some_callback("42");
 *   }
 * \endcode
 *
 *
 * <h2>Querying/Setting the Logger Level</h2>
 *
 * Most loggers store their own level. This might not be the case, however, for example
 * for a proxy logger which relays calls to another logger. Such loggers don't "store"
 * their runtime level but are capable of querying it. This is controlled by the logger
 * traits, see \ref pageCustomLogger.
 *
 * Any logger may be directly queried whether a message at a given log level will be
 * emitted or discarded:
 * \code
 *   if (logger.enabled_for(Logger::INFO)) {
 *     logger.info(...); // will emit message and not discard it
 *   }
 * \endcode
 *
 * \note In order to prepare a log message only if it is to be displayed, it is preferable
 *       not to use \a enabled_for(), but to provide a callback which accepts a reference
 *       to a C++ stream as explained above.  In this case, the callback is only invoked
 *       if the message is actually going to be emitted, and can take into account more
 *       specific message filtering (such as filtering by origin). 
 * 
 * The level of a logger, stored or queried, may be obtained with the \ref
 * Tomographer::Logger::LoggerBase<Derived>::level() method. But <b>don't abuse</b> of
 * this, usually there is no need to query the level of a logger, and it is much
 * preferable to see if the logger is enabled for a particular level with \ref
 * Tomographer::Logger::LoggerBase<Derived>::enabled_for().
 *
 * Also, by default there is no public \c setLevel() method, in case your logger's level
 * is statically fixed or otherwise can't be changed, or if you need a thread-safe logger.
 * Some classes provide however their own API for changing the logger level: For example,
 * \ref Tomographer::Logger::FileLogger "FileLogger" provides a method \ref
 * Tomographer::Logger::FileLogger::setLevel() "setLevel(level)".
 *
 * Specific topics:
 *  - \subpage pageCustomLogger
 */
