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
  static inline ValueType callGetValue(const void * valcalc, PointType && x) // const version
  {
    return ( (const typename std::tuple_element<I, std::tuple<ValueCalculators...> >::type *) valcalc)
        -> getValue(std::forward<PointType>(x));
  }
  template<int I, typename PointType>
  static inline ValueType callGetValue(void * valcalc, PointType && x) // non-const version
  {
    return ( (typename std::tuple_element<I, std::tuple<ValueCalculators...> >::type *) valcalc)
        -> getValue(std::forward<PointType>(x));
  }
};

//
// helper for getValue() 
//
template<int NumValueCalculators, typename ValueType, typename... ValueCalculators>
struct MplxVC_getval_helper
{
  template<typename PointType, typename VoidPtr, // VoidPtr is `void*' or `const void*'
           int IterI = 0, TOMOGRAPHER_ENABLED_IF_TMPL(IterI < NumValueCalculators)>
  static inline ValueType getValue(const int _i, VoidPtr _valcalc, PointType&& x)
  {
    if (IterI == _i) {
      return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<IterI>(_valcalc, x) ;
    }
    return getValue<PointType, VoidPtr, IterI+1>(_i, _valcalc, std::forward<PointType>(x));
  }
  //
  template<typename PointType, typename VoidPtr, // VoidPtr is `void*' or `const void*'
           int IterI = 0, TOMOGRAPHER_ENABLED_IF_TMPL(IterI == NumValueCalculators)>
  static inline ValueType getValue(const int, VoidPtr, PointType&& )
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
  template<typename PointType, typename VoidPtr>
  static inline ValueType getValue(const int _i, VoidPtr _valcalc, PointType&& x)
  {
    tomographer_assert(_i == 0 && "i != 0 but NumValueCalculators == 1");(void)_i;
    return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<0>(_valcalc, x) ;
  }
};
//--
template<typename ValueType, typename... ValueCalculators>
struct MplxVC_getval_helper<2, ValueType, ValueCalculators...>
{
  TOMO_STATIC_ASSERT_EXPR(sizeof...(ValueCalculators) == 2) ;
  template<typename PointType, typename VoidPtr>
  static inline ValueType getValue(const int _i, VoidPtr _valcalc, PointType&& x)
  {
    tomographer_assert(_i >= 0 && _i < (int)sizeof...(ValueCalculators) && "i out of range");
    switch (_i) {
    case 0:
      return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<0>(_valcalc, x) ;
    default:
      return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<1>(_valcalc, x) ;
    }
  }
};
//--
template<typename ValueType, typename... ValueCalculators>
struct MplxVC_getval_helper<3, ValueType, ValueCalculators...>
{
  TOMO_STATIC_ASSERT_EXPR(sizeof...(ValueCalculators) == 3) ;
  template<typename PointType, typename VoidPtr>
  static inline ValueType getValue(const int _i, VoidPtr _valcalc, PointType&& x)
  {
    tomographer_assert(_i >= 0 && _i < (int)sizeof...(ValueCalculators) && "i out of range");
    switch (_i) {
    case 0:
      return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<0>(_valcalc, x) ;
    case 1:
      return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<1>(_valcalc, x) ;
    default:
      return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<2>(_valcalc, x) ;
    }
  }
};
//--
template<typename ValueType, typename... ValueCalculators>
struct MplxVC_getval_helper<4, ValueType, ValueCalculators...>
{
  TOMO_STATIC_ASSERT_EXPR(sizeof...(ValueCalculators) == 4) ;
  template<typename PointType, typename VoidPtr>
  static inline ValueType getValue(const int _i, VoidPtr _valcalc, PointType&& x)
  {
    tomographer_assert(_i >= 0 && _i < (int)sizeof...(ValueCalculators) && "i out of range");
    switch (_i) {
    case 0:
      return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<0>(_valcalc, x) ;
    case 1:
      return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<1>(_valcalc, x) ;
    case 2:
      return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<2>(_valcalc, x) ;
    default:
      return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<3>(_valcalc, x) ;
    }
  }
};
//--
template<typename ValueType, typename... ValueCalculators>
struct MplxVC_getval_helper<5, ValueType, ValueCalculators...>
{
  TOMO_STATIC_ASSERT_EXPR(sizeof...(ValueCalculators) == 5) ;
  template<typename PointType, typename VoidPtr>
  static inline ValueType getValue(const int _i, VoidPtr _valcalc, PointType&& x)
  {
    tomographer_assert(_i >= 0 && _i < (int)sizeof...(ValueCalculators) && "i out of range");
    switch (_i) {
    case 0:
      return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<0>(_valcalc, x) ;
    case 1:
      return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<1>(_valcalc, x) ;
    case 2:
      return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<2>(_valcalc, x) ;
    case 3:
      return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<3>(_valcalc, x) ;
    default:
      return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<4>(_valcalc, x) ;
    }
  }
};
//--
template<typename ValueType, typename... ValueCalculators>
struct MplxVC_getval_helper<6, ValueType, ValueCalculators...>
{
  TOMO_STATIC_ASSERT_EXPR(sizeof...(ValueCalculators) == 6) ;
  template<typename PointType, typename VoidPtr>
  static inline ValueType getValue(const int _i, VoidPtr * _valcalc, PointType&& x)
  {
    tomographer_assert(_i >= 0 && _i < (int)sizeof...(ValueCalculators) && "i out of range");
    switch (_i) {
    case 0:
      return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<0>(_valcalc, x) ;
    case 1:
      return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<1>(_valcalc, x) ;
    case 2:
      return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<2>(_valcalc, x) ;
    case 3:
      return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<3>(_valcalc, x) ;
    case 4:
      return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<4>(_valcalc, x) ;
    default:
      return MplxVC_getval_helper_helper<ValueType,ValueCalculators...>::template callGetValue<5>(_valcalc, x) ;
    }
  }
};

} // namespace tomo_internal





/** \brief A ValueCalculator implementation which the choice of which
 *         ValueCalculator to use at run-time
 *
 * A MultiplexorValueCalculator is a proxy \ref pageInterfaceValueCalculator
 * "ValueCalculator" (which you can use, e.g., in \ref
 * ValueHistogramMHRWStatsCollector), which allows you to choose one among
 * several ValueCalculator types to use at runtime.  The possible choices of
 * ValueCalculator s has to be defined at compile-time as template parameters.
 * The constructor of the MultiplexorValueCalculator takes as many "creator
 * functions" as defined ValueCalculator types, and calls only the one which
 * we'd like to use.
 *
 * The choice of the ValueCalculator is fixed upon instantiation of the
 * MultiplexorValueCalculator. I.e., once decided, the choice of which
 * ValueCalculator to use cannot be changed.
 *
 * \tparam ValueType the ValueType returned by the MultiplexorValueCalculator's
 *         getValue().  All ValueCalculator s must have ValueTypes which can be
 *         implicitily converted to this type.  (See \ref
 *         pageInterfaceValueCalculator)
 *
 * \tparam ValueCalculators... The list of ValueCalculator types among which to
 *         choose at runtime.
 *
 * For example, if you woule like to leave open at compile-time whether to use a
 * TrDistToRefCalculator or a FidelityToRefCalculator, you could write something
 * along the lines of:
 * \code
 *   typedef MultiplexorValueCalculator<double,
 *                                      TrDistToRefCalculator<...>,
 *                                      FidelityToRefCalculator<...> >
 *     MyValueCalculator;
 *
 *   ...
 *
 *   // choose which value calculator to use at run-time
 *   int which_value_calculator =
 *     use_trace_distance ? 0 : 1;  // 0 or 1 is index in the list of
 *                                  // possible ValueCalculators
 *   MyValueCalculator valcalc(
 *     which_value_calculator,
 *     [&]() { return new TrDistToRefCalculator<...>(...); }
 *     [&]() { return new FidelityTORefCalculator<...>(...); }
 *     ) ;
 *
 *   ...
 * \endcode
 *
 * \since Changed in %Tomographer 5.0: Design slightly different; not all value
 *        calculators are instantiated; Constructor now takes lambdas (or
 *        callables) which can create individual value calculators.
 *
 * \since Changed in %Tomographer 5.4: Now correctly handles both const and
 *        non-const calls to getValue()
 */
template<typename ValueType_, typename... ValueCalculators>
class TOMOGRAPHER_EXPORT MultiplexorValueCalculator
  : public virtual Tools::NeedOwnOperatorNew<ValueCalculators...>::ProviderType
{
public:
  //! Value type returned by getValue() (see \ref pageInterfaceValueCalculator)
  typedef ValueType_ ValueType;

  //! Number of ValueCalculators given as template parameters
  static constexpr int NumValueCalculators = sizeof...(ValueCalculators);

  /** \brief Internal implementation detail, don't use
   * 
   * \internal
   *
   * Sequence of if-else's are optimized to a switch statement below this number
   * of ValueCalculators.
   */
  static constexpr int NumStaticallyOptimizedIfs = 6;

  //  typedef std::tuple<ValueCalculators&...> ValueCalculatorsRefTupleType;
  typedef std::tuple<ValueCalculators...> ValueCalculatorsTupleType;

private:

  // pointer to the actual ValueCalculator instance in use, or NULL
  void * const _valcalc;

  // the index of the actual ValueCalculator in use in the list of possible ones
  const int _i;

  // helper functions

  template<typename TupleCreators>
  static inline void * _create_Ith_valcalc(int i, TupleCreators && creators)
  {
    tomographer_assert(0 <= i);
    tomographer_assert(i < NumValueCalculators);
    return _create_Ith_valcalc_a(i, std::forward<TupleCreators>(creators));
  }

  template<int I = 0, typename TupleCreators = void,
           typename std::enable_if<(I<NumValueCalculators),bool>::type = true>
  static inline void * _create_Ith_valcalc_a(int i, TupleCreators && creators)
  {
    if (i == I) {
      typename std::tuple_element<I, ValueCalculatorsTupleType>::type * valcalc =
        std::get<I>(creators)();
      return (void *) valcalc;
    }
    return _create_Ith_valcalc_a<I+1>(i, std::forward<TupleCreators>(creators));
  }
  template<int I = 0, typename TupleCreators = void,
           typename std::enable_if<(I==NumValueCalculators),bool>::type = true>
  static inline void * _create_Ith_valcalc_a(int , TupleCreators && ) { return NULL; }


  template<int I = 0, typename std::enable_if<(I<NumValueCalculators),bool>::type = true>
  inline void _delete_all()
  {
    if (_i == I && _valcalc != NULL) {
      delete (typename std::tuple_element<I, ValueCalculatorsTupleType>::type *) _valcalc;
    }
    _delete_all<I+1>();
  }
  template<int I = 0, typename std::enable_if<(I==NumValueCalculators),bool>::type = true>
  inline void _delete_all() { }


  static inline void * _create_Ith_copy(int i, void * src)
  {
    tomographer_assert(0 <= i);
    tomographer_assert(i < NumValueCalculators);
    tomographer_assert( src != NULL && "Copy constructor invoked with invalid other object." ) ;

    return _create_Ith_copy_a<0>(i, src);
  }

  template<int I = 0, TOMOGRAPHER_ENABLED_IF_TMPL(I<NumValueCalculators)>
  inline static void * _create_Ith_copy_a(int i, void * src) {
    if (I == i) {
      typedef typename std::tuple_element<I, ValueCalculatorsTupleType>::type  ValueCalculator;
      return new ValueCalculator( *(const ValueCalculator*)src ) ; // copy constructor
    }
    return _create_Ith_copy_a<I+1>(i, src);
  }
  template<int I = 0, TOMOGRAPHER_ENABLED_IF_TMPL(I==NumValueCalculators)>
  inline static void * _create_Ith_copy_a(int , void * ) { return NULL; }
  
public:

  /** \brief Constructor.
   * 
   * The first parameter \a i selects which value calculator is applied, given
   * as an index in the list of possible ValueCalculators (starting at zero, of
   * course).
   *
   * The \a creators... are a list of callables which are responsible for
   * instantiating the required value calculator.  Each callable should take no
   * arguments and create a ValueCalculator of the corresponding type using the
   * \c new operator.  Only the callable corresponding to the chosen \a i will
   * be called.  The created ValueCalculator will be automatically deleted.
   */
  template<typename... CreatorFns>
  inline MultiplexorValueCalculator(const int i, CreatorFns&&... creators)
    : _valcalc(_create_Ith_valcalc(i, std::forward_as_tuple(creators...))), _i(i)
  {
  }

  /** \brief Destructor. Frees resources
   */
  ~MultiplexorValueCalculator()
  {
    _delete_all();
  }

  // No move constructor because our members are const, and we would have to
  // make sure we don't double-free the value calculator.

  /** \brief Copy constructor.
   *
   * Create the corresponding valcalc using its copy constructor
   */
  inline MultiplexorValueCalculator(const MultiplexorValueCalculator & other)
    : _valcalc(_create_Ith_copy(other._i, other._valcalc)), _i(other._i)
  {
  }

  /** \brief Get a particular value calculator [static index] (const)
   *
   * \returns The pointer to the corresponding ValueCalculator, or NULL if the
   * run-time value calculactor chosen is not equal to \a I.
   */
  template<int I>
  inline const typename std::tuple_element<I, ValueCalculatorsTupleType>::type * getValueCalculator() const
  {
    if (_i != I) {
      return NULL;
    }
    return (const typename std::tuple_element<I, ValueCalculatorsTupleType>::type *) _valcalc;
  }

  /** \brief Get a particular value calculator [static index] (non-const)
   *
   * \returns The pointer to the corresponding ValueCalculator, or NULL if the
   * run-time value calculactor chosen is not equal to \a I.
   */
  template<int I>
  inline typename std::tuple_element<I, ValueCalculatorsTupleType>::type * getValueCalculator()
  {
    if (_i != I) {
      return NULL;
    }
    return (typename std::tuple_element<I, ValueCalculatorsTupleType>::type *) _valcalc;
  }

  /** \brief The main method which computes the value according to the
   *         pre-chosen ValueCalculator (const version)
   *
   * Calculates the value according to the ValueCalculator instance which was
   * chosen at constructor-time (via the index \a i).
   */
  template<typename PointType>
  inline ValueType getValue(PointType&& x) const
  {
    return tomo_internal::MplxVC_getval_helper<
      NumValueCalculators,ValueType,ValueCalculators...
      >::getValue(_i, (const void*)_valcalc, std::forward<PointType>(x));
  }

  /** \brief The main method which computes the value according to the
   *         pre-chosen ValueCalculator (non-const version)
   *
   * Calculates the value according to the ValueCalculator instance which was
   * chosen at constructor-time (via the index \a i).
   */
  template<typename PointType>
  inline ValueType getValue(PointType&& x)
  {
    return tomo_internal::MplxVC_getval_helper<
      NumValueCalculators,ValueType,ValueCalculators...
      >::getValue(_i, _valcalc, std::forward<PointType>(x));
  }

};



} // namespace Tomographer




#endif
