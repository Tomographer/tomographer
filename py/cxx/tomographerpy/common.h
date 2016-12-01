
#ifndef TOMOPY_COMMON_H
#define TOMOPY_COMMON_H

#include <cstdio>

#include <boost/python.hpp>

#define TOMOGRAPHER_EIGEN_ASSERT_EXCEPTION
#include <tomographer2/tools/eigen_assert_exception.h>

#include "eigpyconv.h"

#include <Eigen/Eigen>

#include <tomographer2/tools/cxxutil.h>
#include <tomographer2/tools/loggers.h>

#include "pylogger.h"



typedef double RealType;
typedef int CountIntType;



// Python/C++ logger:

// to allow for debugging
//extern Tomographer::Logger::FileLogger tpy_logger(std::stderr, Tomographer::Logger::INFO, true /* display_origin */);
// to be fast
//extern Tomographer::Logger::VacuumLogger tpy_logger;
// by default: slow, but really good integration to Python logging
extern PyLogger tpy_logger;


// the type of the C++ logger we are using -- for any of the above
typedef decltype(tpy_logger) TPyLoggerType;


#endif
