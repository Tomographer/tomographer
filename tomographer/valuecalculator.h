/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
 * Copyright (c) 2017 Caltech, Institute for Quantum Information and Matter, Philippe Faist
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef TOMOGRAPHER_VALUECALCULATOR_H
#define TOMOGRAPHER_VALUECALCULATOR_H


#include <tomographer/tools/cxxutil.h>
#include <tomographer/tools/needownoperatornew.h>


/** \file valuecalculator.h
 *
 * \brief Some elementary definitions which relate to the \ref
 * pageInterfaceValueCalculator.
 *
 */


namespace Tomographer {



namespace tomo_internal {

//
// helper for getValue() 
//
template<int NumValueCalculators, typename ValueType, typename... ValueCalculators>
struct MplxVC_getval_helper
{
  template<typename PointType, int IterI = 0,
	   TOMOGRAPHER_ENABLED_IF_TMPL(IterI < NumValueCalculators)>
  static inline ValueType getValue(const int _i, const std::tuple<ValueCalculators...> & _valcalcs, PointType&& x)
  {
    if (IterI == _i) {
      return std::get<IterI>(_valcalcs).getValue(std::forward<PointType>(x));
    }
    return getValue<PointType, IterI+1>(_i, _valcalcs, std::forward<PointType>(x));
  }
  //
  template<typename PointType, int IterI = 0,
	   TOMOGRAPHER_ENABLED_IF_TMPL(IterI == NumValueCalculators)>
  static inline ValueType getValue(const int, const std::tuple<ValueCalculators...> & , PointType&& )
  {
    tomographer_assert(false && "Invalid i: i>=NumValueCalculators or i<0");
    return ValueType();// silence ICC "missing return statement" warning
  }
};
// a few specializations, for optimizing a chain of if's to a faster switch statement
template<typename ValueType, typename... ValueCalculators>
struct MplxVC_getval_helper<1, ValueType, ValueCalculators...>
{
  TOMO_STATIC_ASSERT_EXPR(sizeof...(ValueCalculators) == 1) ;
  template<typename PointType>
  static inline ValueType getValue(const int _i, const std::tuple<ValueCalculators...> & _valcalcs, PointType&& x)
  {
    tomographer_assert(_i == 0 && "i != 0 but NumValueCalculators == 1");(void)_i;
    return std::get<0>(_valcalcs).getValue(std::forward<PointType>(x));
  }
};
//--
template<typename ValueType, typename... ValueCalculators>
struct MplxVC_getval_helper<2, ValueType, ValueCalculators...>
{
  TOMO_STATIC_ASSERT_EXPR(sizeof...(ValueCalculators) == 2) ;
  template<typename PointType>
  static inline ValueType getValue(const int _i, const std::tuple<ValueCalculators...> & _valcalcs, PointType&& x)
  {
    tomographer_assert(_i >= 0 && _i < (int)sizeof...(ValueCalculators) && "i out of range");
    switch (_i) {
    case 0: return std::get<0>(_valcalcs).getValue(std::forward<PointType>(x));
    default: return std::get<1>(_valcalcs).getValue(std::forward<PointType>(x));
    }
  }
};
//--
template<typename ValueType, typename... ValueCalculators>
struct MplxVC_getval_helper<3, ValueType, ValueCalculators...>
{
  TOMO_STATIC_ASSERT_EXPR(sizeof...(ValueCalculators) == 3) ;
  template<typename PointType>
  static inline ValueType getValue(const int _i, const std::tuple<ValueCalculators...> & _valcalcs, PointType&& x)
  {
    tomographer_assert(_i >= 0 && _i < (int)sizeof...(ValueCalculators) && "i out of range");
    switch (_i) {
    case 0: return std::get<0>(_valcalcs).getValue(std::forward<PointType>(x));
    case 1: return std::get<1>(_valcalcs).getValue(std::forward<PointType>(x));
    default: return std::get<2>(_valcalcs).getValue(std::forward<PointType>(x));
    }
  }
};
//--
template<typename ValueType, typename... ValueCalculators>
struct MplxVC_getval_helper<4, ValueType, ValueCalculators...>
{
  TOMO_STATIC_ASSERT_EXPR(sizeof...(ValueCalculators) == 4) ;
  template<typename PointType>
  static inline ValueType getValue(const int _i, const std::tuple<ValueCalculators...> & _valcalcs, PointType&& x)
  {
    tomographer_assert(_i >= 0 && _i < (int)sizeof...(ValueCalculators) && "i out of range");
    switch (_i) {
    case 0: return std::get<0>(_valcalcs).getValue(std::forward<PointType>(x));
    case 1: return std::get<1>(_valcalcs).getValue(std::forward<PointType>(x));
    case 2: return std::get<2>(_valcalcs).getValue(std::forward<PointType>(x));
    default: return std::get<3>(_valcalcs).getValue(std::forward<PointType>(x));
    }
  }
};
//--
template<typename ValueType, typename... ValueCalculators>
struct MplxVC_getval_helper<5, ValueType, ValueCalculators...>
{
  TOMO_STATIC_ASSERT_EXPR(sizeof...(ValueCalculators) == 5) ;
  template<typename PointType>
  static inline ValueType getValue(const int _i, const std::tuple<ValueCalculators...> & _valcalcs, PointType&& x)
  {
    tomographer_assert(_i >= 0 && _i < (int)sizeof...(ValueCalculators) && "i out of range");
    switch (_i) {
    case 0: return std::get<0>(_valcalcs).getValue(std::forward<PointType>(x));
    case 1: return std::get<1>(_valcalcs).getValue(std::forward<PointType>(x));
    case 2: return std::get<2>(_valcalcs).getValue(std::forward<PointType>(x));
    case 3: return std::get<3>(_valcalcs).getValue(std::forward<PointType>(x));
    default: return std::get<4>(_valcalcs).getValue(std::forward<PointType>(x));
    }
  }
};
//--
template<typename ValueType, typename... ValueCalculators>
struct MplxVC_getval_helper<6, ValueType, ValueCalculators...>
{
  TOMO_STATIC_ASSERT_EXPR(sizeof...(ValueCalculators) == 6) ;
  template<typename PointType>
  static inline ValueType getValue(const int _i, const std::tuple<ValueCalculators...> & _valcalcs, PointType&& x)
  {
    tomographer_assert(_i >= 0 && _i < (int)sizeof...(ValueCalculators) && "i out of range");
    switch (_i) {
    case 0: return std::get<0>(_valcalcs).getValue(std::forward<PointType>(x));
    case 1: return std::get<1>(_valcalcs).getValue(std::forward<PointType>(x));
    case 2: return std::get<2>(_valcalcs).getValue(std::forward<PointType>(x));
    case 3: return std::get<3>(_valcalcs).getValue(std::forward<PointType>(x));
    case 4: return std::get<4>(_valcalcs).getValue(std::forward<PointType>(x));
    default: return std::get<5>(_valcalcs).getValue(std::forward<PointType>(x));
    }
  }
};

} // namespace tomo_internal






/** \brief A ValueCalculator-instance which the choice of which ValueCalculator to use at
 *         run-time
 *
 *
 */
template<typename ValueType_, typename... ValueCalculators>
TOMOGRAPHER_EXPORT class MultiplexorValueCalculator
  : public virtual Tools::NeedOwnOperatorNew<ValueCalculators...>::ProviderType
{
public:
  typedef ValueType_ ValueType;

  static constexpr int NumValueCalculators = sizeof...(ValueCalculators);
  static constexpr int NumStaticallyOptimizedIfs = 6;

  //  typedef std::tuple<ValueCalculators&...> ValueCalculatorsRefTupleType;
  typedef std::tuple<ValueCalculators...> ValueCalculatorsTupleType;

private:
  ValueCalculatorsTupleType _valcalcs;

  const int _i;

public:

  /** \brief Constructor.
   * 
   * The first parameter, \a i, selects which value calculator is applied. (The whole idea
   * is that this value can be selected at run-time.)
   *
   * Specify as argument a list of value calculator instances, in the same order as the
   * template parameters.
   */
  template<typename... Args>
  inline MultiplexorValueCalculator(const int i, Args&&... valcalcs)
    : _valcalcs(std::forward<Args>(valcalcs)...), _i(i)
  {
  }

  //! Get a particular value calculator [static index]
  template<int I>
  inline const typename std::tuple_element<I, ValueCalculatorsTupleType>::type & getValueCalculator() const
  {
    return std::get<I>(_valcalcs);
  }

  //! Calculate the value at the given point, using the I-th value calculator
  template<int I, typename PointType>
  inline ValueType getValueI(PointType&& x) const
  {
    return getValueCalculator<I>().getValue(std::forward<PointType>(x));
  }

  /** \brief The main method which computes the value according to the pre-chosen ValueCalculator
   *
   * Calculates the value according to the ValueCalculator instance which was chosen at
   * constructor-time (via the index \a i).
   */
  template<typename PointType>
  inline ValueType getValue(PointType&& x) const
  {
    return tomo_internal::MplxVC_getval_helper<
      NumValueCalculators,ValueType,ValueCalculators...
      >::getValue(_i, _valcalcs, std::forward<PointType>(x));
  }

};



} // namespace Tomographer




#endif
