
#ifndef TOMOPY_COMMON_H
#define TOMOPY_COMMON_H

#include <cstdio>

#include <boost/python.hpp>

#ifdef EIGEN_NO_DEBUG
#error "TomographerPy requires enabled Eigen assertions, otherwise `TomographerCxxError` won't be raised as documented."
#endif

#define TOMOGRAPHER_EIGEN_ASSERT_EXCEPTION
#include <tomographer/tools/eigen_assert_exception.h>


//#include <tomographerpy/eigpyconv.h>

#include <Eigen/Eigen>

#include <tomographer/tools/cxxutil.h>
#include <tomographer/tools/loggers.h>

#include <tomographerpy/pylogger.h>


typedef double RealType;
typedef int CountIntType;

// the main exception object
extern PyObject * TomographerCxxError;



#endif
