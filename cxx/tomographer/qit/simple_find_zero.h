

#ifndef SIMPLE_FIND_ZERO_H
#define SIMPLE_FIND_ZERO_H

#include <cmath>
#include <cstdio>
#include <limits>

#include <tomographer/tools/loggers.h>

namespace Tomographer
{
namespace Tools
{


/** \brief Simple zero-finding algorithm
 *
 * Not robust. The function should be nice (ideally e.g. convex). This uses the <a
 * href="http://en.wikipedia.org/wiki/Inverse_quadratic_interpolation#The_method">Inverse
 * Quadratic Interpolation Method</a>.
 *
 * Additionally, if the function returns a \a NaN at some point, attempts are made to
 * recover back into the domain of the function.
 *
 * If a callback is provided, then the method <code>call(k, newpt, newval)</code> of the
 * callback object is called for each iteration, with the arguments \a k = iteration
 * number, \a newpt = the new X point, \a newval = the function value at the new point.
 */
template<typename ValueType, typename PointType, typename Fn, typename LoggerType = Logger::VacuumLogger>
inline static PointType simpleFindZero(const Fn & fn, PointType pt1, PointType pt2,
				       int maxiters = 50,
				       ValueType valtolerance = 1e-6,
				       ValueType * final_value = NULL,
				       int * final_numiters = NULL,
				       LoggerType & logger = Logger::vacuum_logger)
{
  int num_recover_iters = 0;
  int max_recover_iters = 10*maxiters;

  // initialize to invalid values
  if (final_value != NULL) {
    *final_value = std::numeric_limits<ValueType>::quiet_NaN();
  }
  if (final_numiters != NULL) {
    *final_numiters = -1;
  }

  // start now
  ValueType val1 = fn(pt1);
  ValueType val2 = fn(pt2);
  while (std::isnan(val2) && num_recover_iters < max_recover_iters) {
    logger.longdebug("simpleFindZero()", [pt2](std::ostream & str) {
	str << "function returned NaN for given pt2 = " << pt2 << ", attempting to recover...";
      });
    // make pt2 closer to pt1
    pt2 = pt1 + (pt2 - pt1) / 2.0;
    val2 = fn(pt2);
    ++num_recover_iters;
  }

  if (std::fabs(val2 - val1) <= valtolerance) {
    logger.warning("simpleFindZero()", [pt1,pt2,val1,val2](std::ostream & str) {
	str << "the two initial points x1="<<pt1<<" and x2="<<pt2<<" give values too close: y1="<<val1
	    << " and y2=" << val2;
      });
    return std::numeric_limits<ValueType>::quiet_NaN();
  }

  PointType pt3 = pt1  -  val1 * (pt2 - pt1)/(val2 - val1);
  ValueType val3 = fn(pt3);
  if (std::isnan(val3) || (std::fabs(val3 - val2) <= valtolerance ||
			   std::fabs(val3 - val1) <= valtolerance)) {
    logger.longdebug("simpleFindZero()", [pt3,val3](std::ostream & str) {
	str << "function returned NaN, or value too close to x1 or x2, for guessed x3=" << pt3
	    << " (y3="<<val3<<"), attempting to recover...";
      });
    // pick point between pt1 and pt2
    pt3 = pt1 + (pt2 - pt1) / 2.0;
    val3 = fn(pt3);
  }

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

    while (std::isnan(newval) && num_recover_iters < max_recover_iters) {
      // try to recover.
      logger.longdebug("simpleFindZero()", [newpt](std::ostream & str) {
	  str << "function returned NaN for new point " << newpt << ", attempting to recover...";
	});
      newpt = pt3 + (newpt - pt3) / 2;
      newval = fn(newpt);
      ++num_recover_iters;
    }

    logger.longdebug("simpleFindZero()", [&](std::ostream& str) {
	str << "Iter #" << k << ": x = " << newpt << "   y = " << newval;
      });

    // if the result is precise enough, return it.
    // same if we need to stop because we've exceeded maximum recovery attempts
    if (std::fabs(newval) < valtolerance || num_recover_iters >= max_recover_iters) {
      if (final_value != NULL) {
	*final_value = newval;
      }
      if (final_numiters != NULL) {
	*final_numiters = k;
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
