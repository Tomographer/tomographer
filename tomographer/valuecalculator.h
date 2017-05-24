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

#include <tuple>

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
// helper for the helper for getValue() 
//
template<typename ValueType, typename... ValueCalculators>
struct MplxVC_getval_helper_helper
{
  template<int I, typename PointType>
  static inline ValueType callGetValue(const std::array<void*,sizeof...(ValueCalculators)> & _valcalcs, PointType && x)
  {
    return ( (typename std::tuple_element<I, std::tuple<ValueCalculators...> >::type *) _valcalcs[I])
        -> getValue(std::forward<PointType>(x));
  }
};

//
// helper for getValue() 
//
template<int NumValueCalculators, typename ValueType, typename... ValueCalculators>
struct MplxVC_getval_helper
{
  template<typename PointType, int IterI = 0,
	   TOMOGRAPHER_ENABLED_IF_TMPL(IterI < NumValueCalculators)>
  static inline ValueType getValue(const int _i, const std::array<void*, sizeof...(ValueCalculators)> & _valcalcs, PointType&& x)
  {
    if (IterI == _i) {
      return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<IterI>(_valcalcs, x) ;
    }
    return getValue<PointType, IterI+1>(_i, _valcalcs, std::forward<PointType>(x));
  }
  //
  template<typename PointType, int IterI = 0,
	   TOMOGRAPHER_ENABLED_IF_TMPL(IterI == NumValueCalculators)>
  static inline ValueType getValue(const int, const std::array<void*, sizeof...(ValueCalculators)> & , PointType&& )
  {
    tomographer_assert(false && "Invalid i: i>=NumValueCalculators or i<0");
    return ValueType();// silence ICC "missing return statement" warning
  }
};

// a few specializations, for optimizing a chain of if's to a faster switch statement (?)
template<typename ValueType, typename... ValueCalculators>
struct MplxVC_getval_helper<1, ValueType, ValueCalculators...>
{
  TOMO_STATIC_ASSERT_EXPR(sizeof...(ValueCalculators) == 1) ;
  template<typename PointType>
  static inline ValueType getValue(const int _i, const std::array<void*, sizeof...(ValueCalculators)> & _valcalcs, PointType&& x)
  {
    tomographer_assert(_i == 0 && "i != 0 but NumValueCalculators == 1");(void)_i;
    return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<0>(_valcalcs, x) ;
  }
};
//--
template<typename ValueType, typename... ValueCalculators>
struct MplxVC_getval_helper<2, ValueType, ValueCalculators...>
{
  TOMO_STATIC_ASSERT_EXPR(sizeof...(ValueCalculators) == 2) ;
  template<typename PointType>
  static inline ValueType getValue(const int _i, const std::array<void*, sizeof...(ValueCalculators)> & _valcalcs, PointType&& x)
  {
    tomographer_assert(_i >= 0 && _i < (int)sizeof...(ValueCalculators) && "i out of range");
    switch (_i) {
    case 0:     return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<0>(_valcalcs, x) ;
    default:    return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<1>(_valcalcs, x) ;
    }
  }
};
//--
template<typename ValueType, typename... ValueCalculators>
struct MplxVC_getval_helper<3, ValueType, ValueCalculators...>
{
  TOMO_STATIC_ASSERT_EXPR(sizeof...(ValueCalculators) == 3) ;
  template<typename PointType>
  static inline ValueType getValue(const int _i, const std::array<void*, sizeof...(ValueCalculators)> & _valcalcs, PointType&& x)
  {
    tomographer_assert(_i >= 0 && _i < (int)sizeof...(ValueCalculators) && "i out of range");
    switch (_i) {
    case 0:     return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<0>(_valcalcs, x) ;
    case 1:     return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<1>(_valcalcs, x) ;
    default:    return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<2>(_valcalcs, x) ;
    }
  }
};
//--
template<typename ValueType, typename... ValueCalculators>
struct MplxVC_getval_helper<4, ValueType, ValueCalculators...>
{
  TOMO_STATIC_ASSERT_EXPR(sizeof...(ValueCalculators) == 4) ;
  template<typename PointType>
  static inline ValueType getValue(const int _i, const std::array<void*, sizeof...(ValueCalculators)> & _valcalcs, PointType&& x)
  {
    tomographer_assert(_i >= 0 && _i < (int)sizeof...(ValueCalculators) && "i out of range");
    switch (_i) {
    case 0:    return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<0>(_valcalcs, x) ;
    case 1:    return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<1>(_valcalcs, x) ;
    case 2:    return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<2>(_valcalcs, x) ;
    default:   return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<3>(_valcalcs, x) ;
    }
  }
};
//--
template<typename ValueType, typename... ValueCalculators>
struct MplxVC_getval_helper<5, ValueType, ValueCalculators...>
{
  TOMO_STATIC_ASSERT_EXPR(sizeof...(ValueCalculators) == 5) ;
  template<typename PointType>
  static inline ValueType getValue(const int _i, const std::array<void*, sizeof...(ValueCalculators)> & _valcalcs, PointType&& x)
  {
    tomographer_assert(_i >= 0 && _i < (int)sizeof...(ValueCalculators) && "i out of range");
    switch (_i) {
    case 0:    return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<0>(_valcalcs, x) ;
    case 1:    return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<1>(_valcalcs, x) ;
    case 2:    return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<2>(_valcalcs, x) ;
    case 3:    return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<3>(_valcalcs, x) ;
    default:   return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<4>(_valcalcs, x) ;
    }
  }
};
//--
template<typename ValueType, typename... ValueCalculators>
struct MplxVC_getval_helper<6, ValueType, ValueCalculators...>
{
  TOMO_STATIC_ASSERT_EXPR(sizeof...(ValueCalculators) == 6) ;
  template<typename PointType>
  static inline ValueType getValue(const int _i, const std::array<void*, sizeof...(ValueCalculators)> & _valcalcs, PointType&& x)
  {
    tomographer_assert(_i >= 0 && _i < (int)sizeof...(ValueCalculators) && "i out of range");
    switch (_i) {
    case 0:    return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<0>(_valcalcs, x) ;
    case 1:    return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<1>(_valcalcs, x) ;
    case 2:    return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<2>(_valcalcs, x) ;
    case 3:    return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<3>(_valcalcs, x) ;
    case 4:    return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<4>(_valcalcs, x) ;
    default:   return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<5>(_valcalcs, x) ;
    }
  }
};

} // namespace tomo_internal





/** \brief A ValueCalculator-instance which the choice of which ValueCalculator to use at
 *         run-time
 *
 * \since Changed in %Tomographer 5.0: Design slightly different; not all value
 *        calculators are instantiated; Constructor now takes lambdas (or callables) which
 *        can create individual value calculators.
 */
template<typename ValueType_, typename... ValueCalculators>
class TOMOGRAPHER_EXPORT MultiplexorValueCalculator
  : public virtual Tools::NeedOwnOperatorNew<ValueCalculators...>::ProviderType
{
public:
  typedef ValueType_ ValueType;

  static constexpr int NumValueCalculators = sizeof...(ValueCalculators);
  static constexpr int NumStaticallyOptimizedIfs = 6;

  //  typedef std::tuple<ValueCalculators&...> ValueCalculatorsRefTupleType;
  typedef std::tuple<ValueCalculators...> ValueCalculatorsTupleType;

private:
  std::array<void*,NumValueCalculators> _valcalcs;

  const int _i;

  
  template<int I = 0, typename TupleCreators = void,
           typename std::enable_if<(I<NumValueCalculators),bool>::type = true>
  inline void _create_Ith_valcalc(int i, TupleCreators && creators) {
    if (i == I) {
      _valcalcs[(std::size_t)i] = (void*)std::get<I>(creators)();
    }
    _create_Ith_valcalc<I+1>(i, std::forward<TupleCreators>(creators));
  }
  template<int I = 0, typename TupleCreators = void,
           typename std::enable_if<(I==NumValueCalculators),bool>::type = true>
  inline void _create_Ith_valcalc(int , TupleCreators && ) { }

  template<int I = 0, typename std::enable_if<(I<NumValueCalculators),bool>::type = true>
  inline void _delete_all() {
    if (_valcalcs[I] != NULL) {
      delete (typename std::tuple_element<I, ValueCalculatorsTupleType>::type *) _valcalcs[I];
    }
    _delete_all<I+1>();
  }
  template<int I = 0, typename std::enable_if<(I==NumValueCalculators),bool>::type = true>
  inline void _delete_all() { }

  template<int I = 0, typename std::enable_if<(I<NumValueCalculators),bool>::type = true>
  inline void _create_Ith_copy(int i, const void * src) {
    tomographer_assert( src != NULL && "Copy constructor invoked with invalid other object.") ;
    if (I == i) {
      typedef typename std::tuple_element<I, ValueCalculatorsTupleType>::type  ValueCalculator;
      _valcalcs[(std::size_t)i] = new ValueCalculator( *(const ValueCalculator*)src ) ; // copy constructor
    }
    _create_Ith_copy<I+1>(i, src);
  }
  template<int I = 0, typename std::enable_if<(I==NumValueCalculators),bool>::type = true>
  inline void _create_Ith_copy(int , const void * ) { }
  
public:

  /** \brief Constructor.
   * 
   * The first parameter, \a i, selects which value calculator is applied. (The whole idea
   * is that this value can be selected at run-time.)
   *
   * Specify as argument a list of callables, which are responsible for creating each
   * value calculator.  Each callable should take no arguments and create a
   * ValueCalculator of the corresponding type using the \c new operator.  Only the
   * callable corresponding to the chosen \a i will be called.  The created
   * ValueCalculator will be automatically deleted.
   */
  template<typename... CreatorFns>
  inline MultiplexorValueCalculator(const int i, CreatorFns&&... creators)
    : _valcalcs(), _i(i)
  {
    _valcalcs.fill(NULL);
    tomographer_assert(0 <= i);
    tomographer_assert(i < NumValueCalculators);
    _create_Ith_valcalc(i, std::forward_as_tuple(creators...));
  }

  /** \brief Destructor. Frees resources
   */
  ~MultiplexorValueCalculator()
  {
    _delete_all();
  }

  /** \brief Move constructor.
   *
   * Steal the other guy's pointers.
   */
  inline MultiplexorValueCalculator(MultiplexorValueCalculator && x)
    : _valcalcs(x._valcalcs), _i(x._i)
  {
    x._valcalcs.fill(NULL) ;
  }

  /** \brief Copy constructor.
   *
   * Create the corresponding valcalc using its copy constructor
   */
  inline MultiplexorValueCalculator(const MultiplexorValueCalculator & other)
    : _valcalcs(), _i(other._i)
  {
    _valcalcs.fill(NULL) ;
    _create_Ith_copy(_i, other._valcalcs[(std::size_t)_i]);
  }

  //! Get a particular value calculator [static index]
  template<int I>
  inline const typename std::tuple_element<I, ValueCalculatorsTupleType>::type * getValueCalculator() const
  {
    return (typename std::tuple_element<I, ValueCalculatorsTupleType>::type *) _valcalcs[I];
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
