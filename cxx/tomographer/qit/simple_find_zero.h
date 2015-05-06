

#ifndef SIMPLE_FIND_ZERO_H
#define SIMPLE_FIND_ZERO_H

#include <cmath>
#include <cstdio>

#include <tomographer/tools/loggers.h>

namespace Tomographer
{
namespace Tools
{

/* * \brief a no-op static callback class.
 *
 * For use, for example, with \ref simpleFindZero().
 *
 */ /*
struct EmptyCallback {
  EmptyCallback() { }
  template<typename... Args>
  inline void call(Args... args) { }
};


template<typename LoggerType>
struct SimpleFindZeroLoggerCallback {
  LoggerType & logger;
  SimpleFindZeroLoggerCallback(LoggerType & logger_) : logger(logger_) { }

  template<typename PointType, typename ValueType>
  inline void call(int k, PointType x, ValueType y) const
  {
    logger.longdebug("simpleFindZero()", [&](std::ostream& str) {
	str << "Iter #" << k << ": x = " << x << "   y = " << y;
      });
  }
};
    */

/** \brief Simple zero-finding algorithm
 *
 * Not robust. The function should be nice (ideally e.g. convex). This uses the <a
 * href="http://en.wikipedia.org/wiki/Inverse_quadratic_interpolation#The_method">Inverse
 * Quadratic Interpolation Method</a>.
 *
 * If a callback is provided, then the method <code>call(k, newpt, newval)</code> of the
 * callback object is called for each iteration, with the arguments \a k = iteration
 * number, \a newpt = the new X point, \a newval = the function value at the new point.
 */
template<typename ValueType, typename PointType, typename Fn, typename LoggerType = VacuumLogger>
inline static PointType simpleFindZero(const Fn & fn, PointType pt1, PointType pt2,
				       int maxiters = 50,
				       ValueType valtolerance = 1e-6,
				       ValueType * final_value = NULL,
				       int * final_numiters = NULL,
				       LoggerType & logger = vacuum_logger)
{
  ValueType val1 = fn(pt1);
  ValueType val2 = fn(pt2);
  PointType pt3 = pt1  -  val1 * (pt2 - pt1)/(val2 - val1);
  ValueType val3 = fn(pt3);

  //  std::printf("x1=%g, val1=%g\n", (double)pt1, (double)val1);
  //  std::printf("x2=%g, val2=%g\n", (double)pt2, (double)val2);
  //  std::printf("x3=%g, val3=%g\n", (double)pt3, (double)val3);
  logger.longdebug("simpleFindZero()", [&](std::ostream& str) {
      str << "Starting with\n"
	  << "\t x1="<<pt1<<"    \tf(x1)="<<val1<<"\n"
	  << "\t x2="<<pt2<<"    \tf(x2)="<<val2<<"\n"
	  << "\t --> x3="<<pt3<<"\tf(x3)="<<val3;
    });
  
  PointType newpt;
  ValueType newval;
  
  for (int k = 0; k < maxiters; ++k) {
    // iterate x_{n+1} = ...
    newpt = pt1 * val2*val3 / ((val1 - val2)*(val1 - val3))
      + pt2 * val1*val3 / ((val2 - val1)*(val2 - val3))
      + pt3 * val1*val2 / ((val3 - val1)*(val3 - val2)) ;

    // and calculate f(x_{n+1})
    newval = fn(newpt);

    //    std::printf("k=%d, newpt=%g, newval=%g\n", k, (double)newpt, (double)newval);
    //iter_callback.call(k, newpt, newval);
    logger.longdebug("simpleFindZero()", [&](std::ostream& str) {
	str << "Iter #" << k << ": x = " << newpt << "   y = " << newval;
      });

    // if the result is precise enough, return it.
    if (std::abs(newval) < valtolerance) {
      if (final_value != NULL) {
	*final_value = newval;
      }
      if (final_numiters != NULL) {
	*final_numiters = maxiters;
      }
      return newpt;
    }
    // recurrence
    pt1 = pt2; val1 = val2;
    pt2 = pt3; val2 = val3;
    pt3 = newpt; val3 = newval;
  }

  if (final_value != NULL) {
    *final_value = newval;
  }
  if (final_numiters != NULL) {
    *final_numiters = maxiters;
  }
  return newpt;
}



} // namespace Tools
} // namespace Tomographer



#endif
