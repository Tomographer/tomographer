
#ifndef TOMOPY_COMMON_H
#define TOMOPY_COMMON_H

#include <cstdio>

#include <boost/python.hpp>

#ifdef EIGEN_NO_DEBUG
#error "TomographerPy requires enabled Eigen assertions, otherwise `TomographerCxxError` won't be raised as documented."
#endif

// DEBUGGING ONLY: set TOMOGRAPHERPY_DEBUG_EIGEN_ASSERT_CAUSES_ABORT to cause eigen_assert() failures to abort() and dump core
#ifndef TOMOGRAPHERPY_DEBUG_EIGEN_ASSERT_CAUSES_ABORT
#define TOMOGRAPHER_EIGEN_ASSERT_EXCEPTION
#endif
#include <tomographer/tools/eigen_assert_exception.h>


//#include <tomographerpy/eigpyconv.h>

#include <Eigen/Eigen>

#include <tomographer/tools/cxxutil.h>
#include <tomographer/tools/loggers.h>

#include <tomographerpy/pylogger.h>



// get a demangle() function from Boost, either with boost::core::demangle() (boost >=
// 1.56) or boost::units::detail::demangle() (boost before that)
#include <boost/version.hpp>
#if BOOST_VERSION >= 105600
#include <boost/core/demangle.hpp>
#else
#include <boost/units/detail/utility.hpp>
namespace boost { namespace core {  using boost::units::detail::demangle; } }
#endif



typedef double RealType;
typedef int CountIntType;

namespace Py {

typedef Eigen::Matrix<RealType, Eigen::Dynamic, 1> RealVectorType;
typedef Eigen::Matrix<RealType, Eigen::Dynamic, Eigen::Dynamic> RealMatrixType;
typedef Eigen::Matrix<std::complex<RealType>, Eigen::Dynamic, 1> CplxVectorType;
typedef Eigen::Matrix<std::complex<RealType>, Eigen::Dynamic, Eigen::Dynamic> CplxMatrixType;

typedef Eigen::Matrix<CountIntType, Eigen::Dynamic, 1> CountIntVectorType;

};


#endif
