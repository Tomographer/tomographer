
/** \page pageCustomLogger Write a New Custom Logger
 *
 * To write a new, custom \c Logger class, you need to do the following:
 *
 *  - define your logger class and write the code that actually emits the log;
 *
 *  - define logger traits to characterize your logger.
 *
 * A simplistic example (simplified version of \ref SimpleFoutLogger) of a logger is given
 * below.
 *
 * \note In the future, we might have to add a member like "StaticMinimumLevel" in the
 *     traits class to statically reject low-level (verbose) messages which are anyway not
 *     wanted.
 *
 * \code
 *   class StderrLogger : public LoggerBase<StderrLogger>
 *   {
 *   public:
 *     StderrLogger(int level = Logger::INFO) : LoggerBase<StderrLogger>(level)
 *     {
 *     }
 *
 *     /// \brief Change the log level
 *     ///
 *     /// \warning This method is not thread-safe!
 *     ///
 *     inline void setLevel(int level)
 *     {
 *       // change the protected LoggerBase<StderrLogger>::_level
 *       _level = level;
 *     }
 *   
 *     inline void emit_log(int level, const char * origin, const std::string & msg)
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
 *
 *   // Traits for StderrLogger -- fprintf is actually thread-safe.
 *   template<>
 *   struct LoggerTraits<StderrLogger>
 *   {
 *     enum {
 *       IsThreadSafe = 1
 *     };
 *   };
 * \endcode
 */

