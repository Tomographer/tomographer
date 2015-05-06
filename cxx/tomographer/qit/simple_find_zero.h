

#ifndef SIMPLE_FIND_ZERO_H
#define SIMPLE_FIND_ZERO_H

#include <cmath>
#include <cstdio>

namespace Tomographer
{
namespace Tools
{


/** \brief Simple zero-finding algorithm
 *
 * Not robust. The function should be nice (ideally e.g. convex). This uses the <a
 * href="http://en.wikipedia.org/wiki/Inverse_quadratic_interpolation#The_method">Inverse
 * Quadratic Interpolation Method</a>.
 */
template<typename ValueType, typename PointType, typename Fn>
inline static PointType simpleFindZero(const Fn & fn, PointType pt1, PointType pt2,
				       int maxiters = 50,
				       ValueType valtolerance = 1e-6,
				       ValueType * final_value = NULL,
				       int * final_numiters = NULL)
{
  ValueType val1 = fn(pt1);
  ValueType val2 = fn(pt2);
  PointType pt3 = pt1  -  val1 * (pt2 - pt1)/(val1 - val2);
  ValueType val3 = fn(pt3);

  //  std::printf("x1=%g, val1=%g\n", (double)pt1, (double)val1);
  //  std::printf("x2=%g, val2=%g\n", (double)pt2, (double)val2);
  //  std::printf("x3=%g, val3=%g\n", (double)pt3, (double)val3);
  
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

    // if the result is precise enough, return it.
    if (std::abs(newval) < valtolerance) {
      *final_value = newval;
      *final_numiters = maxiters;
      return newpt;
    }
    // recurrence
    pt1 = pt2; val1 = val2;
    pt2 = pt3; val2 = val3;
    pt3 = newpt; val3 = newval;
  }

  *final_value = newval;
  *final_numiters = maxiters;
  return newpt;
}



} // namespace Tools
} // namespace Tomographer



#endif
