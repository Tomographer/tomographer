
#ifndef TOMOPY_COMMON_H
#define TOMOPY_COMMON_H

#include <cstdio>

#include <boost/python.hpp>

#include "pylogger.h"

// must be included at top, before any other Eigen header (sets eigen_assert() macro)
// but it uses python logging, so after "pylogger.h"
#include "eigpyconv.h"

#include <Eigen/Eigen>

#include <tomographer2/tools/cxxutil.h>
#include <tomographer2/tools/loggers.h>


typedef double RealType;
typedef int CountIntType;




#endif
