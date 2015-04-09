
/** \page pageCustomLogger Write a New Custom Logger
 *
 * To write a new, custom \c Logger class, you need to do the following:
 *
 *  - define your logger class and write the code that actually emits the log;
 *
 *  - define logger traits to characterize your logger.
 *
 * A simplistic example (simplified version of \ref Tomographer::SimpleFoutLogger) of a
 * logger is given below.
 *
 * \code
 *   class StderrLogger : public Tomographer::LoggerBase<StderrLogger>
 *   {
 *   public:
 *     StderrLogger(int level = Logger::INFO) : Tomographer::LoggerBase<StderrLogger>(level)
 *     {
 *     }
 *
 *     // Change the log level
 *     //
 *     // WARNING: This method is not thread-safe!
 *     //
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
 *   namespace Tomographer {
 *     // Traits for StderrLogger
 *     template<>
 *     struct LoggerTraits<StderrLogger>
 *     {
 *       enum {
 *         // fprintf is actually thread-safe, so our logger is thread-safe
 *         IsThreadSafe = 1,
 *         // set this to a particular level to unconditinally discard any
 *         // message logged with strictly lower importance level.
 *         StaticMinimumImportanceLevel = -1
 *       };
 *     };
 *   }
 * \endcode
 */

