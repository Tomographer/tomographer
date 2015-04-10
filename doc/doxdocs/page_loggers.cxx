
/** \page pageLoggers Logging and Loggers
 *
 * Tomographer provides a lightweight framework for logging, i.e. producing messages which
 * inform the user and/or developer about what the program is doing. Objects who would
 * like to log messages take a template type parameter \a Logger, and an instance of such
 * a type usually provided to its constructor. The \a Logger type must be a subclass of
 * \ref Tomographer::LoggerBase.
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
 * This last idiom is also very useful if producing the log message itself is rather
 * resource-consuming. Imagine you wish to pretty-print a histogram for debugging:
 * \code
 *   logger.debug("origin_fn()", [&histogram](std::ostream& stream) {
 *                  stream << "Histogram: \n" << histogram.pretty_print();
 *                });
 * \endcode
 * The call to the lambda, and thus to <code>histogram.pretty_print()</code>, will only be
 * performed if the logger will indeed eventually print the message.
 *
 * A logger may be also directly queried whether a message at a given log level will be
 * emitted or discarded:
 * \code
 *   if (logger.enabled_for(Logger::INFO)) {
 *     logger.info(...); // will emit message and not discard it
 *   }
 * \endcode
 *
 * You may also query directly the level of a logger, but this information should only be
 * used to propagate a level further, e.g. to some proxy logger:
 * \code
 *   ProxyLogger proxy(&logger, logger.level());
 * \endcode
 * Don't rely on <code>logger.level()</code> directly to know whether a message will be
 * emitted: the <code>enabled_for(level)</code> approach is more flexible.
 *
 * Specific topics:
 *  - \subpage pageCustomLogger
 */