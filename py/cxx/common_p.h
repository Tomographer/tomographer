

#ifndef TOMOPY_COMMON_P_H
#define TOMOPY_COMMON_P_H


#include <tomographerpy/common.h>
#include <tomographerpy/pylogger.h>


// Python/C++ logger: -- Stick to PyLogger, really, as some parts of the code may break otherwise
//
// The logger object instance is defined in tomographerpy.cxx

// to allow for debugging:
//extern Tomographer::Logger::FileLogger tpy_logger(std::stderr, Tomographer::Logger::INFO, true /* display_origin */);
// to be fast:
//extern Tomographer::Logger::VacuumLogger tpy_logger;
// by default: slow, but really good integration to Python logging:
extern PyLogger tpy_logger;



// the type of the C++ logger we are using -- for any of the above
typedef decltype(tpy_logger) TPyLoggerType;



#endif
