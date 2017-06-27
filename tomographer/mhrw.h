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

#ifndef _TOMOGRAPHER_MHRW_H
#define _TOMOGRAPHER_MHRW_H

#include <cstddef>

#include <limits>
#include <random>
#include <iomanip>
#include <type_traits>

#include <tomographer/tools/loggers.h>
#include <tomographer/tools/fmt.h>
#include <tomographer/tools/cxxutil.h>
#include <tomographer/tools/statusprovider.h>
#include <tomographer/multiproc.h>
#include <tomographer/mhrw_bin_err.h>
#include <tomographer/tools/needownoperatornew.h>



/** \file mhrw.h
 * \brief Routines for performing a Metropolis-Hastings random walk.
 *
 */


namespace Tomographer {





enum {
  /** \brief Provides the MH function value at each point (see \ref
   * labelMHWalkerUseFnSyntaxType "Role of UseFnSyntaxType")
   */
  MHUseFnValue = 1,
  /** \brief Provides the logarithm MH function value at each point (see \ref
   * labelMHWalkerUseFnSyntaxType "Role of UseFnSyntaxType")
   */
  MHUseFnLogValue,
  /** \brief Provides directly the ratio of the function values for two consecutive points
   * of the MH random walk (see \ref labelMHWalkerUseFnSyntaxType
   * "Role of UseFnSyntaxType")
   */
  MHUseFnRelativeValue
};



// note: const implies static linkage, see http://stackoverflow.com/q/2268749/1694896
//
/** \brief Minimal recommended acceptance ratio */
constexpr const double MHRWAcceptanceRatioRecommendedMin = 0.2;
/** \brief Maximal recommended acceptance ratio */
constexpr const double MHRWAcceptanceRatioRecommendedMax = 0.4;


/** \brief An MHWalkerParams type which just stores a step size
 *
 * This type is a simple struct which stores a step size. It should be used as the \a
 * WalkerParams typedef of a \ref pageInterfaceMHWalker compliant type which only needs a
 * step size as parameter.
 */
template<typename StepRealType_ = double>
struct TOMOGRAPHER_EXPORT MHWalkerParamsStepSize
{
  typedef StepRealType_ StepRealType;

  MHWalkerParamsStepSize() : step_size() { }
  MHWalkerParamsStepSize(StepRealType step_size_) : step_size(step_size_) { }

  StepRealType step_size;
};

template<typename StepRealType>
inline std::ostream & operator<<(std::ostream & stream, MHWalkerParamsStepSize<StepRealType> p)
{
  return stream << "step_size=" << p.step_size;
}


/** \brief Specify the parameters of a Metropolis-Hastings random walk
 *
 * Specifies the parameters of a Metropolis-Hastings random walk (number of thermalization
 * runs, sweep size, number of live runs, step size).
 *
 * \since
 * Changed in Tomographer 5.0: This class now stores an arbitrary type for the parameters
 * of the MHWalker type. Note the new template parameter order!!
 */
template<typename MHWalkerParams_ = MHWalkerParamsStepSize<double>, typename CountIntType_ = int >
struct TOMOGRAPHER_EXPORT MHRWParams
{
  typedef MHWalkerParams_ MHWalkerParams;
  typedef CountIntType_ CountIntType;

  explicit MHRWParams()
    : mhwalker_params(), n_sweep(0), n_therm(0), n_run(0)
  {
  }
  template<typename MHWalkerParamsInit>
  MHRWParams(MHWalkerParamsInit && mhwalker_params_,
             CountIntType n_sweep_, CountIntType n_therm_, CountIntType n_run_)
    : mhwalker_params(std::forward<MHWalkerParamsInit>(mhwalker_params_)),
      n_sweep(n_sweep_), n_therm(n_therm_), n_run(n_run_)
  {
  }

  template<typename MHWalkerParamsOtherType, typename CountIntOtherType>
  MHRWParams(const MHRWParams<MHWalkerParamsOtherType, CountIntOtherType> & copy)
    : mhwalker_params(copy.mhwalker_params),
      n_sweep(copy.n_sweep), n_therm(copy.n_therm), n_run(copy.n_run)
  {
  }

  /** \brief The parameters of the mh-walker, typically just the step size of the random walk
   */
  MHWalkerParams mhwalker_params;

  /** \brief The number of individual updates to collect together in a "sweep"
   */
  CountIntType n_sweep;

  /** \brief Number of thermalization sweeps
   */
  CountIntType n_therm;

  /** \brief Number of live sweeps
   */
  CountIntType n_run;
};



template<typename CountIntType, typename MHWalkerParams>
inline std::ostream & operator<<(std::ostream & str, const MHRWParams<MHWalkerParams,CountIntType> & p)
{
  str << "MHRWParams(" << p.mhwalker_params << ";n_sweep=" << p.n_sweep
      << ",n_therm=" << p.n_therm << ",n_run=" << p.n_run << ")";
  return str;
}



/** \brief Describe how frequently the parameters of the random walk should be dynamically adjusted
 *
 * The value of an AdjustmentStrategy may be a binary OR value of several bits.  The
 * strategy needs to specify:
 *
 *   - Whether the adjustments are to be enabled only during thermalization sweeps, only
 *     during live run sweeps, or during both
 *
 *   - Whether the adjustments are to be performed after an individual iteration (a single
 *     move in the random walk), or only after processing a live sample (only during live
 *     runs)
 *
 *
 * \since Added in %Tomographer 5.0.
 */
enum MHRWControllerAdjustmentStrategy {
  //! Never adjust the parameters of the random walk
  MHRWControllerDoNotAdjust = 0,

  //! Adjustments are enabled during thermalization sweeps
  MHRWControllerAdjustWhileThermalizing = 0x01,
  //! Adjustments are enabled during live (running) sweeps
  MHRWControllerAdjustWhileRunning = 0x02,
  //! Adjustments are enabled during both thermalization and live (running) sweeps
  MHRWControllerAdjustWhileThermalizingAndRunning =
    MHRWControllerAdjustWhileThermalizing | MHRWControllerAdjustWhileRunning,

  /** \brief Mask out bits which decide at which random walk stage (thermalizing and/or
   *         running) adjustments are performed
   */
  MHRWControllerAdjustRWStageMASK = 0x0f,

  //! Adjustemnts should be performed after each individual iteration
  MHRWControllerAdjustEveryIteration = 0x10,
  //! Adjustemnts should be performed after a sample is taken (during live runs only)
  MHRWControllerAdjustEverySample = 0x20,

  /** \brief Mask out bits which decide at which frequency (after each iteration and/or
   *         after each sample) adjustments are performed
   */
  MHRWControllerAdjustFrequencyMASK = 0xf0,


  //! Adjustemnts should be performed only while thermalizing, after each individual iteration
  MHRWControllerAdjustEveryIterationWhileThermalizing =
    MHRWControllerAdjustEveryIteration | MHRWControllerAdjustWhileThermalizing,

  //! Adjustemnts should be performed all the time, after each individual iteration
  MHRWControllerAdjustEveryIterationAlways =
    MHRWControllerAdjustEveryIteration | MHRWControllerAdjustWhileThermalizingAndRunning
};




/** \brief Helper class to invoke a \ref pageInterfaceMHRWController 's callbacks
 *
 * Invoke the callbacks on the controller, respecting its \a AdjustmentStrategy flags.
 */
template<typename MHRWControllerType_>
struct MHRWControllerInvoker
{
  typedef MHRWControllerType_ MHRWControllerType;
  enum { AdjustmentStrategy = MHRWControllerType::AdjustmentStrategy };

private:
  constexpr static bool _enabled_callback(bool IsThermalizing, bool IsAfterSample)
  {
    return
      ( ((AdjustmentStrategy & MHRWControllerAdjustWhileThermalizing) && IsThermalizing) ||
        ((AdjustmentStrategy & MHRWControllerAdjustWhileRunning) && !IsThermalizing) )
      &&
      ( ((AdjustmentStrategy & MHRWControllerAdjustEverySample) && IsAfterSample) ||
        ((AdjustmentStrategy & MHRWControllerAdjustEveryIteration) && !IsAfterSample) )
      ;
  }

public:

  template<typename... Args>
  static inline void invokeInit(MHRWControllerType & x, Args && ... args) {
    x.init(std::forward<Args>(args)...) ;
  }

  template<typename MHRWParamsType, typename MHWalker, typename CountIntType, typename MHRandomWalkType>
  static inline bool invokeAllowDoneThermalization(MHRWControllerType & x,
                                                   const MHRWParamsType & params, // ensure this is const
                                                   const MHWalker & mhwalker,
                                                   CountIntType iter_k,
                                                   const MHRandomWalkType & mhrw)
  {
    return x.allowDoneThermalization(params, mhwalker, iter_k, mhrw);
  }

  template<typename MHRWParamsType, typename MHWalker, typename CountIntType, typename MHRandomWalkType>
  static inline bool invokeAllowDoneRuns(MHRWControllerType & x,
                                         const MHRWParamsType & params, // ensure this is const
                                         const MHWalker & mhwalker,
                                         CountIntType iter_k,
                                         const MHRandomWalkType & mhrw)
  {
    return x.allowDoneRuns(params, mhwalker, iter_k, mhrw);
  }


  template<bool IsThermalizing, bool IsAfterSample, typename ... Args>
  static inline
  typename std::enable_if<_enabled_callback(IsThermalizing,IsAfterSample), void>::type
  invokeAdjustParams(MHRWControllerType & x, Args && ... args)
  {
    x.template adjustParams<IsThermalizing, IsAfterSample>(std::forward<Args>(args)...);
  }
  template<bool IsThermalizing, bool IsAfterSample, typename ... Args>
  static inline
  typename std::enable_if<!_enabled_callback(IsThermalizing,IsAfterSample), void>::type
  invokeAdjustParams(MHRWControllerType & , Args && ... )
  {
    // callback disabled
  }

};




namespace tomo_internal {

template<unsigned int AdjustmentStrategy, unsigned int OtherAdjustmentStrategy>
struct controller_flags_compatible {
  static constexpr bool value =
    // adjustments are done on different stages of the random walk
    ((AdjustmentStrategy & OtherAdjustmentStrategy & MHRWControllerAdjustRWStageMASK) == 0) ;
};


template<bool RestOk, unsigned int ProcessedAdjustmentStrategyFlags,
         typename... MHRWControllerTypes>
struct controllers_compatible_helper;

template<bool RestOk, unsigned int ProcessedAdjustmentStrategyFlags, typename MHRWControllerAType,
         typename... MHRWControllerRestTypes>
struct controllers_compatible_helper<RestOk, ProcessedAdjustmentStrategyFlags, MHRWControllerAType,
                                   MHRWControllerRestTypes...>
  : controllers_compatible_helper<
      RestOk && controller_flags_compatible<MHRWControllerAType::AdjustmentStrategy,
                                            ProcessedAdjustmentStrategyFlags>::value ,
      ProcessedAdjustmentStrategyFlags | MHRWControllerAType::AdjustmentStrategy,
      MHRWControllerRestTypes...
  > { };

template<bool RestOk, unsigned int ProcessedAdjustmentStrategyFlags>
struct controllers_compatible_helper<RestOk, ProcessedAdjustmentStrategyFlags> {
  static constexpr bool value = RestOk;
  static constexpr unsigned int CombinedAdjustmentStrategy = ProcessedAdjustmentStrategyFlags;
};


template<typename... MHRWControllerTypes>
struct controllers_compatible : controllers_compatible_helper<true, 0, MHRWControllerTypes...> { };

} // namespace tomo_internal







/** \brief A \ref pageInterfaceMHRWController which combines several independent
 *         random walk controllers
 *
 * The random walk controllers must be \a compatible.  Two controllers \a A and \a B are
 * \a compatible if they perform adjustments at different stages of the random walk (e.g.,
 * one during thermalization and the other during the live runs) as given by their \a
 * AdjustmentStrategy flags.
 *
 * The \a allowDoneRuns() and \a allowDoneThermalization() callbacks do not affect whether
 * controllers are compatible.  The thermalization runs end only after all the controllers'
 * \a allowDoneThermalization() callbacks return \a true, and similarly the live runs end
 * only after all the controllers' \a allowDoneRuns() callbacks return \a true.
 *
 * \since Added in %Tomographer 5.0.
 */
template<typename ... MHRWControllerTypes>
class TOMOGRAPHER_EXPORT MHRWMultipleControllers
{
public:

  typedef std::tuple<MHRWControllerTypes...> TupleType;
  typedef std::tuple<MHRWControllerTypes&...> TupleRefType;

  static constexpr int NumControllers = sizeof...(MHRWControllerTypes) ;

  TOMO_STATIC_ASSERT_EXPR( tomo_internal::controllers_compatible<MHRWControllerTypes...>::value ) ;

private:
  TupleRefType controllers;

public:
  enum {
    AdjustmentStrategy =
      tomo_internal::controllers_compatible<MHRWControllerTypes...>::CombinedAdjustmentStrategy
  };


  MHRWMultipleControllers(MHRWControllerTypes&... controllers_)
    : controllers(controllers_...)
  {
  }


  template<int I>
  inline const typename std::tuple_element<I, TupleRefType>::type getController() const
  {
    return std::get<I>(controllers) ;
  }


  template<typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType,
           int I = 0, TOMOGRAPHER_ENABLED_IF_TMPL(I < NumControllers)>
  inline void init(MHRWParamsType & params, const MHWalker & mhwalker,
                   const MHRandomWalkType & mhrw)
  {
    std::get<I>(controllers).init(params, mhwalker, mhrw);
    init<MHRWParamsType, MHWalker, MHRandomWalkType, I+1>(params, mhwalker, mhrw);
  }
  template<typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType,
           int I = 0, TOMOGRAPHER_ENABLED_IF_TMPL(I == NumControllers)>
  inline void init(const MHRWParamsType &, const MHWalker &, const MHRandomWalkType &) const
  {
  }

  template<typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType,
           int I = 0, TOMOGRAPHER_ENABLED_IF_TMPL(I < NumControllers)>
  inline void thermalizingDone(MHRWParamsType & params, const MHWalker & mhwalker,
                               const MHRandomWalkType & mhrw)
  {
    std::get<I>(controllers).thermalizingDone(params, mhwalker, mhrw);
    thermalizingDone<MHRWParamsType, MHWalker, MHRandomWalkType, I+1>(params, mhwalker, mhrw);
  }
  template<typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType,
           int I = 0, TOMOGRAPHER_ENABLED_IF_TMPL(I == NumControllers)>
  inline void thermalizingDone(const MHRWParamsType & , const MHWalker & ,
                               const MHRandomWalkType & ) const
  {
  }

  template<typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType,
           int I = 0, TOMOGRAPHER_ENABLED_IF_TMPL(I < NumControllers)>
  inline void done(MHRWParamsType & params, const MHWalker & mhwalker,
                   const MHRandomWalkType & mhrw)
  {
    std::get<I>(controllers).done(params, mhwalker, mhrw);
    done<MHRWParamsType, MHWalker, MHRandomWalkType, I+1>(params, mhwalker, mhrw);
  }
  template<typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType,
           int I = 0, TOMOGRAPHER_ENABLED_IF_TMPL(I == NumControllers)>
  inline void done(const MHRWParamsType & , const MHWalker & ,
                   const MHRandomWalkType & ) const
  {
  }

  template<bool IsThermalizing, bool IsAfterSample,
           typename MHRWParamsType, typename CountIntType,
           typename MHWalker, typename MHRandomWalkType,
           int I = 0, TOMOGRAPHER_ENABLED_IF_TMPL(I < NumControllers)>
  inline void adjustParams(MHRWParamsType & params, const MHWalker & mhwalker,
                           CountIntType iter_k, const MHRandomWalkType & mhrw)
  {
    // only one of the calls to adjustParmas() should actually go through; we've checked
    // this above with a static_assert
    MHRWControllerInvoker<typename std::tuple_element<I, TupleType>::type>
      ::template invokeAdjustParams<IsThermalizing,IsAfterSample>(
          std::get<I>(controllers), params, mhwalker, iter_k, mhrw
          );
    adjustParams<IsThermalizing,IsAfterSample,MHRWParamsType,CountIntType,MHWalker,MHRandomWalkType,I+1>(
        params, mhwalker, iter_k, mhrw
        );
  }
  template<bool IsThermalizing, bool IsAfterSample,
           typename MHRWParamsType, typename CountIntType,
           typename MHWalker, typename MHRandomWalkType,
           int I = 0, TOMOGRAPHER_ENABLED_IF_TMPL(I == NumControllers)>
  inline void adjustParams(const MHRWParamsType &, const MHWalker &,
                           CountIntType , const MHRandomWalkType &) const
  {
  }

  template<typename MHRWParamsType, typename MHWalker, typename CountIntType, typename MHRandomWalkType,
           int I = 0, TOMOGRAPHER_ENABLED_IF_TMPL(I < NumControllers)>
  inline bool allowDoneThermalization(const MHRWParamsType & params, const MHWalker & mhwalker,
                                      CountIntType iter_k, const MHRandomWalkType & mhrw)
  {
    return std::get<I>(controllers).allowDoneThermalization(params, mhwalker, iter_k, mhrw) &&
      allowDoneThermalization<MHRWParamsType,MHWalker,CountIntType,MHRandomWalkType,I+1>(
          params, mhwalker, iter_k, mhrw
          ) ;
  }
  template<typename MHRWParamsType, typename MHWalker, typename CountIntType, typename MHRandomWalkType,
           int I = 0, TOMOGRAPHER_ENABLED_IF_TMPL(I == NumControllers)>
  inline bool allowDoneThermalization(const MHRWParamsType & , const MHWalker & ,
                                      CountIntType , const MHRandomWalkType & ) const
  {
    return true;
  }

  template<typename MHRWParamsType, typename MHWalker, typename CountIntType, typename MHRandomWalkType,
           int I = 0, TOMOGRAPHER_ENABLED_IF_TMPL(I < NumControllers)>
  inline bool allowDoneRuns(const MHRWParamsType & params, const MHWalker & mhwalker,
                            CountIntType iter_k, const MHRandomWalkType & mhrw)
  {
    return std::get<I>(controllers).allowDoneRuns(params, mhwalker, iter_k, mhrw) &&
      allowDoneRuns<MHRWParamsType,MHWalker,CountIntType,MHRandomWalkType,I+1>(
          params, mhwalker, iter_k, mhrw
          ) ;
  }
  template<typename MHRWParamsType, typename MHWalker, typename CountIntType, typename MHRandomWalkType,
           int I = 0, TOMOGRAPHER_ENABLED_IF_TMPL(I == NumControllers)>
  inline bool allowDoneRuns(const MHRWParamsType & , const MHWalker & ,
                            CountIntType , const MHRandomWalkType & ) const
  {
    return true;
  }
};
template<typename ... MHRWControllerTypes>
constexpr int MHRWMultipleControllers<MHRWControllerTypes...>::NumControllers;


/** \brief Convenience function to create a MHRWMultipleControllers (using template
 *         argument deduction)
 *
 * \since Added in %Tomographer 5.0
 */
template<typename... MHRWControllerTypes>
inline MHRWMultipleControllers<MHRWControllerTypes...>
mkMHRWMultipleControllers(MHRWControllerTypes & ... controllers)
{
  return MHRWMultipleControllers<MHRWControllerTypes...>( controllers ... ) ;
}



/** \brief A \ref pageInterfaceMHRWController which does not adjust anything
 *
 * No adjustments are performed whatsoever. The MHWalkerParams may be of any type.
 *
 * \since Added in %Tomographer 5.0.
 */
typedef MHRWMultipleControllers<> MHRWNoController;







namespace tomo_internal {
// const_type_helper: a single typedef member, 'type', which is declared as 'T' or 'const
// T' depending on whether use_const=false or true
template<typename T, bool use_const>
struct const_type_helper {
  typedef T type;
};
template<typename T>
struct const_type_helper<T, true> {
  typedef const T type;
};

template<typename MHWalker, bool has_FnValueType>
struct helper_FnValueType_or_dummy {
  typedef typename MHWalker::FnValueType type;
};
template<typename MHWalker>
struct helper_FnValueType_or_dummy<MHWalker,false> {
  typedef int type; // dummy
};
} // namespace tomo_internal


/** \brief A Metropolis-Hastings Random Walk
 *
 * Implements a <a
 * href="http://en.wikipedia.org/wiki/Metropolis%E2%80%93Hastings_algorithm">Metropolis-Hastings
 * random walk</a>. This takes care of accepting or rejecting a new point and taking samples.
 *
 * This class takes care to update the state of a random walk for it to perform
 * thermalizing sweeps, followed by "live" runs.
 *
 * The random walk is composed of \em iterations. There are \c n_sweep iterations per
 * "sweep". This number should be approximately chosen such that <code>step_size *
 * n_sweep</code> is of order of the size of the state space. (This is in order to better
 * decorrelate the samples.)
 *
 * Initially, a number \c n_therm of \em thermalizing sweeps are performed. No samples
 * are taken during this period, and this is meant for a Metropolis random walk to find
 * its "valley".
 *
 * After the thermalizing runs, a number of <em>run</em> sweeps are performed, in which a
 * <em>live</em> sample is taken at the last iteration of each sweep.
 *
 * \since Since Tomographer 5.0: added \a MHRWController.
 *
 * \tparam Rng is a C++ random number generator (for example, \ref std::mt19937)
 *
 * \tparam MHWalker is responsible for dealing with the state space, providing a new
 *         proposal point and calculating the function value at different points. See \ref
 *         pageInterfaceMHWalker.
 *
 * \tparam MHRWStatsCollector takes care of collecting useful data during a random
 *         walk. It should be a type implementing a \a MHRWStatsCollector interface, see
 *         \ref pageInterfaceMHRWStatsCollector.
 *
 * \tparam MHRWController a \ref pageInterfaceMHRWController compliant
 *         type which may dynamically adjust the parameters of the random walker.  Just
 *         specify \ref MHRWNoController if you don't need the parameters to be
 *         dynamically adjusted.
 *
 * \tparam LoggerType is a logger type to which log messages can be generated (see \ref
 *         pageLoggers)
 *
 * \tparam CountIntType is an integer type used to count the number of iterations.  You'll
 *         want to use \a int here, unless you really want to pursue exceptionally long
 *         random walks.
 *
 */
template<typename Rng_, typename MHWalker_, typename MHRWStatsCollector_,
         typename MHRWController_ = MHRWNoController,
         typename LoggerType_ = Logger::VacuumLogger,
         typename CountIntType_ = int>
class TOMOGRAPHER_EXPORT MHRandomWalk
  : public virtual Tools::NeedOwnOperatorNew<typename MHWalker_::PointType>::ProviderType
{
public:
  //! Random number generator type (see C++ std::random)
  typedef Rng_ Rng;
  //! The random walker type which knows about the state space and jump function
  typedef MHWalker_ MHWalker;
  //! The stats collector type (see \ref pageInterfaceMHRWStatsCollector)
  typedef MHRWStatsCollector_ MHRWStatsCollector;
  //! The logger type which will be provided by user to constructor (see \ref pageLoggers)
  typedef LoggerType_ LoggerType;
  /** \brief The type used for counting numbers of iterations (see, e.g. \ref nSweep() or \ref
   *         MHRWParams) */
  typedef CountIntType_ CountIntType;

  //! The type of a point in the random walk
  typedef typename MHWalker::PointType PointType;
  //! The parameters type of the MHWalker implememtation, typically just a double storing the step size of the random walk
  typedef typename MHWalker::WalkerParams MHWalkerParams;

  //! The struct which can hold the parameters of this random walk
  typedef MHRWParams<MHWalkerParams, CountIntType> MHRWParamsType;

  //! The type which will take care of dynamically adjusting the parameters of the random walk
  typedef MHRWController_ MHRWController;
  enum { MHRWControllerStrategy = MHRWController::AdjustmentStrategy };

  //! The MHRWControllerInvoker for our random walk controller, for convenience
  typedef MHRWControllerInvoker<MHRWController> MHRWControllerInvokerType;

  //! The type of the Metropolis-Hastings function value. (See class documentation)
#ifndef TOMOGRAPHER_PARSED_BY_DOXYGEN
  typedef typename tomo_internal::helper_FnValueType_or_dummy<
    MHWalker,(int)MHWalker::UseFnSyntaxType!=(int)MHUseFnRelativeValue
    >::type  FnValueType;
#else
  typedef _FnValueType FnValueType;
#endif

  enum {
    //! How to calculate the Metropolis-Hastings jump probability ratio (see class documentation)
    UseFnSyntaxType = MHWalker::UseFnSyntaxType
  };

private:
  // declare const if no adjustments are to be made. This expands to "MHRWParamsType _n;"
  // or "const MHRWParamsType _n;"
  typename tomo_internal::const_type_helper<
    MHRWParamsType,
    (int)MHRWControllerStrategy==(int)MHRWControllerDoNotAdjust
    >::type _n;

  Rng & _rng;
  MHWalker & _mhwalker;
  MHRWStatsCollector & _stats;
  MHRWController & _mhrw_controller;

  Logger::LocalLogger<LoggerType> _logger;

  //! Current point
  PointType curpt;
  /** \brief Current function value at current point. This is a dummy value if
   * <code>UseFnSyntaxType==MHUseFnRelativeValue</code>.
   */
  FnValueType curptval;

  /** \brief Keeps track of the total number of accepted moves during the "live" runs
   * (i.e., not thermalizing). This is used to track the acceptance ratio (see \ref
   * acceptanceRatio())
   */
  CountIntType num_accepted;
  /** \brief Keeps track of the total number of moves during the "live" runs, i.e. not
   * thermalizing. This is used to track the acceptance ratio (see \ref acceptanceRatio())
   */
  CountIntType num_live_points;


public:

  //! Simple constructor, initializes the given fields
  MHRandomWalk(MHWalkerParams mhwalker_params, CountIntType n_sweep, CountIntType n_therm, CountIntType n_run,
	       MHWalker & mhwalker, MHRWStatsCollector & stats, MHRWController & mhrw_controller,
               Rng & rng, LoggerType & logger_)
    : _n(mhwalker_params, n_sweep, n_therm, n_run),
      _rng(rng),
      _mhwalker(mhwalker),
      _stats(stats),
      _mhrw_controller(mhrw_controller),
      _logger(TOMO_ORIGIN, logger_),
      curpt(),
      curptval(),
      num_accepted(0),
      num_live_points(0)
  {
    _logger.debug([&](std::ostream & stream) {
	stream << "constructor(). n_sweep=" << n_sweep << ", mhwalker_params=" << mhwalker_params
	       << "n_therm=" << n_therm << ", n_run=" << n_run;
      });
  }
  //! Simple constructor, initializes the given fields
  template<typename MHRWParamsTypeInit>
  MHRandomWalk(MHRWParamsTypeInit&& n_rw,
	       MHWalker & mhwalker, MHRWStatsCollector & stats, MHRWController & mhrw_controller,
               Rng & rng, LoggerType & logger_)
    : _n(std::forward<MHRWParamsTypeInit>(n_rw)),
      _rng(rng),
      _mhwalker(mhwalker),
      _stats(stats),
      _mhrw_controller(mhrw_controller),
      _logger(TOMO_ORIGIN, logger_),
      curpt(),
      curptval(),
      num_accepted(0),
      num_live_points(0)
  {
    _logger.debug([&](std::ostream & s) { s << "constructor(). mhrw parameters = " << _n; });
  }

  MHRandomWalk(const MHRandomWalk & other) = delete;


  //! Access the stats collector
  inline const MHRWStatsCollector & statsCollector() const { return _stats; }

  //! Access the random walk controller
  inline const MHRWController & mhrwController() const { return _mhrw_controller; }


  //! The parameters of the random walk.
  inline MHRWParamsType mhrwParams() const { return _n; }

  //! Get the MHWalker parameters
  inline MHWalkerParams mhWalkerParams() const { return _n.mhwalker_params; }

  //! Number of iterations in a sweep.
  inline CountIntType nSweep() const { return _n.n_sweep; }
  //! Number of thermalizing sweeps.
  inline CountIntType nTherm() const { return _n.n_therm; }
  //! Number of live run sweeps.
  inline CountIntType nRun() const { return _n.n_run; }


  /** \brief Query whether we have any statistics about acceptance ratio. This is \c
   * false, for example, during the thermalizing runs.
   */
  inline bool hasAcceptanceRatio() const
  {
    return (num_live_points > 0);
  }
  /** \brief Return the acceptance ratio so far.
   */
  template<typename RatioType = double>
  inline RatioType acceptanceRatio() const
  {
    return RatioType(num_accepted) / RatioType(num_live_points);
  }


  /** \brief Access the current state of the random walk
   *
   * \returns the current point the random walk is located at.
   */
  inline const PointType & getCurrentPoint() const
  {
    return curpt;
  }

  /** \brief Access the current function value of the random walk
   *
   * \returns the current value of the function in the current state of the random walk.
   *
   * \warning the meaning of this value depends on \c MHRandomWalk::UseFnSyntaxType. It is
   * either the value of the function, its logarithm, or a dummy value.
   */
  inline const FnValueType & getCurrentPointValue() const
  {
    return curptval;
  }

  /** \brief Force manual state of random walk
   *
   * This may be called to force setting the current state of the random walk to the given
   * point \c pt.
   */
  inline void setCurrentPoint(const PointType& pt)
  {
    curpt = pt;
    curptval = _get_ptval(curpt);
    _logger.longdebug([&](std::ostream & s) {
	s << "setCurrentPoint: set internal state. Value = " << curptval << "; Point =\n" << pt << "\n";
      });
  }


private:

  /** \brief Resets counts and relays to the MHWalker and the MHRWStatsCollector.
   */
  inline void _init()
  {
    num_accepted = 0;
    num_live_points = 0;

    // starting point
    curpt = _mhwalker.startPoint();
    curptval = _get_ptval(curpt);

    _mhwalker.init();
    _stats.init();

    _mhrw_controller.init(_n, _mhwalker, *this);

    _logger.longdebug("_init() done.");
  }
  /** \brief Relays to the MHWalker and the MHRWStatsCollector.
   */
  inline void _thermalizing_done()
  {
    _mhwalker.thermalizingDone();
    _stats.thermalizingDone();

    _mhrw_controller.thermalizingDone(_n, _mhwalker, *this);

    _logger.longdebug("_thermalizing_done() done.");
  }
  /** \brief Relays to the MHWalker and the MHRWStatsCollector.
   */
  inline void _done()
  {
    _mhwalker.done();
    _stats.done();

    _mhrw_controller.done(_n, _mhwalker, *this);

    _logger.longdebug("_done() done.");
  }

  /** \brief Processes a single move in the random walk.
   *
   * This function gets a new move proposal from the MHWalker object, and calculates the
   * \a a value, which tells us with which probability we should accept the move. This \a
   * a value is calculated according to the documentation in \ref
   * labelMHWalkerUseFnSyntaxType "Role of UseFnSyntaxType".
   */
  template<bool IsThermalizing>
  inline void _move(CountIntType k, bool is_live_iter)
  {
    _logger.longdebug("_move()");
    // The reason `mhwalker_params` is passed to jump_fn instead of leaving jump_fn itself
    // handle the step size, is that we might in the future want to dynamically adapt the
    // step size according to the acceptance ratio. That would have to be done in this
    // class.
    const PointType newpt = _mhwalker.jumpFn(curpt, _n.mhwalker_params);

    const FnValueType newptval = _get_ptval(newpt);

    const double a = _get_a_value(newpt, newptval, curpt, curptval);

    // accept move?
    bool accept = ( a >= 1.0 ? true : bool( _rng()-_rng.min() <= a*(_rng.max()-_rng.min()) ) ) ;

    // track acceptance ratio, except if we are thermalizing
    if (!IsThermalizing) {
      num_accepted += accept ? 1 : 0;
      ++num_live_points;
    }

    _stats.rawMove(k, IsThermalizing, is_live_iter, accept, a, newpt, newptval, curpt, curptval, *this);

    _logger.longdebug([&](std::ostream & stream) {
	stream << (IsThermalizing?"T":"#") << std::setw(3) << k << ": " << (accept?"AC":"RJ") << " "
	       << std::setprecision(4)
	       << "a=" << std::setw(5) << a << ", newptval=" << std::setw(5) << newptval
	       << ", curptval=" << std::setw(5) << curptval << ", accept_ratio="
	       << (!IsThermalizing ? Tools::fmts("%.2g", this->acceptanceRatio()) : std::string("N/A"))
	       << Tools::streamIfPossible(curpt, "\ncurpt = ", "", "");
      });

    if (accept) {
      // update the internal state of the random walk
      curpt = newpt;
      curptval = newptval;
    }
    _logger.longdebug("_move() done.");
  }

  /** \brief Required for \ref pageInterfaceRandomWalk. Process a new live sample in the
   * random walk. Relays the call to the \a MHRWStatsCollector (see \ref
   * pageInterfaceMHRWStatsCollector).
   */
  inline void _process_sample(CountIntType k, CountIntType n)
  {
    _stats.processSample(k, n, curpt, curptval, *this);
    _logger.longdebug("_process_sample() done.");
  }


  //
  // utilities for getting the fn value at a specific point, and comparing & getting the
  // "a"-value for the jump
  //

#ifdef TOMOGRAPHER_PARSED_BY_DOXYGEN
  /** \internal
   * \brief get a value for the given point
   *
   * Depending on the \c UseFnSyntaxType template parameter, this is either the function
   * value, its logarithm, or a dummy value.
   */
  inline FnValueType _get_ptval(PointType curpt) const { }
  /** \internal
   * \brief calculate the MH function ratio between two points
   *
   * This function calculates and returns the \f$ a \f$ parameter which is the ratio of
   * the function value between the new, proposal point and the current point. Recall,
   * if \f$ a < 1 \f$ then the new point should be accepted with probability \f$ a \f$,
   * else the new point should always be accepted.
   *
   * \note this function may return simply 1 if we are sure that \f$ a > 1 \f$.
   *
   * The returnd value of \f$ a \f$ is calculated differently depending on the value of
   * the \c UseFnSyntaxType template parameter.
   */
  inline double _get_a_value(PointType newpt, FnValueType newptval,
                             PointType curpt, FnValueType curptval) const { }
#else
  // the actual implementation:

  // Case UseFnSyntaxType==MHUseFnValue
  template<typename PtType, TOMOGRAPHER_ENABLED_IF_TMPL(UseFnSyntaxType == MHUseFnValue)>
  inline FnValueType _get_ptval(PtType && curpt) const
  {
    return _mhwalker.fnVal(curpt);
  }
  template<typename PtType1, typename PtType2, TOMOGRAPHER_ENABLED_IF_TMPL(UseFnSyntaxType == MHUseFnValue)>
  inline double _get_a_value(PtType1 && /*newpt*/, FnValueType newptval,
                             PtType2 && /*curpt*/, FnValueType curptval) const
  {
    return ((double)newptval) / curptval;
  }

  // Case UseFnSyntaxType==MHUseFnLogValue
  template<typename PtType, TOMOGRAPHER_ENABLED_IF_TMPL(UseFnSyntaxType == MHUseFnLogValue)>
  inline FnValueType _get_ptval(PtType && curpt) const
  {
    return _mhwalker.fnLogVal(curpt);
  }
  template<typename PtType1, typename PtType2, TOMOGRAPHER_ENABLED_IF_TMPL(UseFnSyntaxType == MHUseFnLogValue)>
  inline double _get_a_value(PtType1 && /*newpt*/, FnValueType newptval,
                             PtType2 && /*curpt*/, FnValueType curptval) const
  {
    using namespace std;
    return (newptval > curptval) ? 1.0 : exp(double(newptval - curptval));
  }

  // case UseFnSyntaxType==MHUseFnRelativeValue
  template<typename PtType, TOMOGRAPHER_ENABLED_IF_TMPL(UseFnSyntaxType == MHUseFnRelativeValue)>
  inline FnValueType _get_ptval(PtType && /*curpt*/) const
  {
    return 0;
  }
  template<typename PtType1, typename PtType2, TOMOGRAPHER_ENABLED_IF_TMPL(UseFnSyntaxType == MHUseFnRelativeValue)>
  inline double _get_a_value(PtType1 && newpt, FnValueType /*newptval*/,
                             PtType2 && curpt, FnValueType /*curptval*/) const
  {
    using namespace std;
    return _mhwalker.fnRelVal(std::forward<PtType1>(newpt), std::forward<PtType2>(curpt));
  }

#endif


  // adjustments
  template<bool IsThermalizing>
  inline void _controller_adjust_afteriter(CountIntType iter_k)
  {
    MHRWControllerInvokerType::template invokeAdjustParams<IsThermalizing, false>(
        _mhrw_controller, _n, _mhwalker, iter_k, *this
        );
  }
  inline void _controller_adjust_aftersample(CountIntType iter_k)
  {
    MHRWControllerInvokerType::template invokeAdjustParams<false, true>(
        _mhrw_controller, _n, _mhwalker, iter_k, *this
        );
  }
  inline bool _controller_allow_therm_done(CountIntType iter_k)
  {
    return MHRWControllerInvokerType::template invokeAllowDoneThermalization(
        _mhrw_controller, _n, _mhwalker, iter_k, *this
        );
  }
  inline bool _controller_allow_runs_done(CountIntType iter_k)
  {
    return MHRWControllerInvokerType::template invokeAllowDoneRuns(
        _mhrw_controller, _n, _mhwalker, iter_k, *this
        );
  }

public:

  /** \brief Run the random walk. (pun intended)
   *
   * This will take care of the full random walk.  The specified number of thermalizing
   * sweeps will be run, followed by a number of "live" sweeps where one sample is taken
   * per sweep.
   */
  void run()
  {
    _init();

    CountIntType k;

    _logger.longdebug([&](std::ostream & s) {
	s << "Starting random walk, parameters are = " << _n;
      });

    // keep the this expression explicit in the condition, because it may be updated by
    // the controller. (The compiler should optimize the const value anyway if no
    // controller is set because _n is declared const in that case.)
    for ( k = 0 ; (k < (_n.n_sweep * _n.n_therm)) || !_controller_allow_therm_done(k) ; ++k ) {
      _move<true>(k, false);
      _controller_adjust_afteriter<true>(k);
    }

    _thermalizing_done();

    _logger.longdebug("Thermalizing done, starting live runs.");

    CountIntType n = 0; // number of live samples

    // keep the this expression explicit in the condition, because it may be updated by
    // the controller. (The compiler should optimize the const value anyway if no
    // controller is set because _n is declared const in that case.)
    for (k = 0 ; (k < (_n.n_sweep * _n.n_run)) || !_controller_allow_runs_done(k) ; ++k) {

      bool is_live_iter = ((k+1) % _n.n_sweep == 0);
      
      // calculate a candidate jump point and see if we accept the move
      _move<false>(k, is_live_iter);
      _controller_adjust_afteriter<false>(k);

      if (is_live_iter) {
        _process_sample(k, n);
        ++n;
        _controller_adjust_aftersample(k);
      }

    }

    _done();

    _logger.longdebug("Random walk completed.");

    return;
  }
};














/** \brief Status Report structure representing the status of a \ref MHRandomWalk
 *
 * This struct can store information about the current status of a \ref MHRandomWalk while
 * it is running.
 *
 * This is particularly useful when multiprocessing tasks, for example, with \ref
 * MultiProc::OMP::TaskDispatcher and requesting status reports as with \ref
 * MultiProc::OMP::TaskDispatcher::requestStatusReport().
 */
template<typename MHRWParamsType>
struct TOMOGRAPHER_EXPORT MHRWStatusReport : public MultiProc::TaskStatusReport
{
  typedef typename MHRWParamsType::CountIntType IterCountIntType;

  /** \brief Constructor which initializes all fields */
  MHRWStatusReport(double fdone = 0.0, const std::string & msg = std::string(),
                   IterCountIntType kstep_ = 0, MHRWParamsType mhrw_params_ = MHRWParamsType(),
                   double acceptance_ratio_ = 0.0)
    : MultiProc::TaskStatusReport(fdone, msg),
    kstep(kstep_),
    mhrw_params(mhrw_params_),
    acceptance_ratio(acceptance_ratio_),
    n_total_iters(mhrw_params.n_sweep*(mhrw_params.n_therm + mhrw_params.n_run))
  {
  }

  /** \brief the current iteration number */
  IterCountIntType kstep;

  /** \brief the parameters of the random walk
   *
   * Stores the number of iterations that form a sweep (\a n_sweep), the total number
   * of thermalization sweeps (\a n_therm), the total number of live run sweeps (\a
   * n_run) as well as the step size of the random walk (\a mhwalker_params).
   *
   * See also \ref MHRandomWalk
   */
  MHRWParamsType mhrw_params;

  /** \brief the current acceptance ratio of the random walk (see
   *    \ref Tomographer::MHRandomWalk::acceptanceRatio()
   * ) */
  double acceptance_ratio;

  /** \brief the total number of iterations required for this random walk
   *
   * This is calculated as \f$
   *  \textit{nTotalIters} = \textit{nSweep} \times \left( \textit{nTherm} + \textit{nRun} \right)
   * \f$.
   */
  IterCountIntType n_total_iters;

  template<typename MHRandomWalkType, typename MHRWStatsCollectorType, typename MHRWControllerType>
  static inline MHRWStatusReport
  createFromRandWalkStatInfo(IterCountIntType k, bool is_thermalizing,
                             const MHRandomWalkType & rw,
                             const MHRWStatsCollectorType & stats_collector,
                             const MHRWControllerType & mhrw_controller) {
    // prepare & provide status report
    IterCountIntType totiters = rw.nSweep()*(rw.nTherm()+rw.nRun());
    // k restarts at zero after thermalization, so account for that:
    IterCountIntType kreal = is_thermalizing ? k : k + rw.nSweep()*rw.nTherm();
    double fdone = ( is_thermalizing ? (double)k : (double)k + rw.nSweep()*rw.nTherm() ) / (double)totiters;
    double accept_ratio = std::numeric_limits<double>::quiet_NaN();
    bool warn_accept_ratio = false;
    std::string accept_ratio_msg;
    if (rw.hasAcceptanceRatio()) {
      accept_ratio = rw.acceptanceRatio();
      warn_accept_ratio = (accept_ratio > MHRWAcceptanceRatioRecommendedMax ||
                           accept_ratio < MHRWAcceptanceRatioRecommendedMin);
      accept_ratio_msg = std::string("  [") + (warn_accept_ratio ? "!!** " : "") +
        std::string("accept ratio=") + Tools::fmts("%.2f", accept_ratio) +
        (warn_accept_ratio ? " **!!" : "") + "]";
    }
    //
    // "therm. sweep NNN/NNN [+rn:NNN]: XX.XX% done"
    // "run sweep    NNN/NNN [+th:NNN]: XX.XX% done [accept ratio=0.25]"
    //
    std::string msg;
    if (is_thermalizing) {
      msg = "therm. sweep " + std::to_string(k / rw.nSweep()) + "/" + std::to_string(rw.nTherm())
        + " [+rn:" + std::to_string(rw.nRun()) + "]";
    } else {
      msg = "run sweep    " + std::to_string(k / rw.nSweep()) + "/" + std::to_string(rw.nRun())
        + " [+th:" + std::to_string(rw.nTherm()) + "]";
    }
    msg += " : " + Tools::fmts("%5.2f", fdone*100.0) + "% done";
    if (accept_ratio_msg.size()) {
      msg += accept_ratio_msg;
    }
    // std::string msg = Tools::fmts(
    //     "%s %lu/(%lu=%lu*(%lu+%lu)) : %5.2f%% done  %s",
    //     ( ! is_thermalizing
    //       ? "iteration"
    //       : "[therm.] "),
    //     (unsigned long)kreal, (unsigned long)totiters, (unsigned long)rw.nSweep(),
    //     (unsigned long)rw.nTherm(), (unsigned long)rw.nRun(),
    //     fdone*100.0,
    //     accept_ratio_msg.c_str()
    //     );
    const std::string nlindent = "\n    ";
    if (Tools::StatusQuery<MHRWStatsCollectorType>::CanProvideStatusLine) {
      const std::string s = Tools::StatusQuery<MHRWStatsCollectorType>::getStatusLine(&stats_collector);
      if (s.size()) {
        msg += nlindent;
        for (std::size_t j = 0; j < s.size(); ++j) {
          if (s[j] == '\n') {
            msg += nlindent;
          } else {
            msg += s[j];
          }
        }
      }
    }
    if (Tools::StatusQuery<MHRWControllerType>::CanProvideStatusLine) {
      const std::string s = Tools::StatusQuery<MHRWControllerType>::getStatusLine(&mhrw_controller);
      if (s.size()) {
        msg += nlindent;
        for (std::size_t j = 0; j < s.size(); ++j) {
          if (s[j] == '\n') {
            msg += nlindent;
          } else {
            msg += s[j];
          }
        }
      }
    }
    return MHRWStatusReport(fdone, msg, kreal, rw.mhrwParams(), accept_ratio);
  }

};










//
// Specialize Tomographer::Tools::StatusProvider  for our MHRWMultipleControllers
//


namespace Tools {

/** \brief Provide status reporting for a MHRWMultipleControllers
 *
 */
template<typename... Args>
struct TOMOGRAPHER_EXPORT StatusProvider<MHRWMultipleControllers<Args... > >
{
  typedef MHRWMultipleControllers<Args... > MHRWControllerType;
  typedef MHRWControllerType StatusableObject;

  static constexpr int NumControllers = MHRWControllerType::NumControllers;
  
  static constexpr bool CanProvideStatusLine = true;

  template<int I = 0, typename std::enable_if<(I < NumControllers), bool>::type dummy = true>
  static inline std::string getStatusLine(const MHRWControllerType * ctrl)
  {
    typedef typename std::tuple_element<I, typename MHRWControllerType::TupleType>::type
      ThisController;
    return
      _joinnl( (StatusQuery<ThisController>::CanProvideStatusLine
                ? StatusQuery<ThisController>::getStatusLine(& ctrl->template getController<I>())
                : std::string()),
               getStatusLine<I+1>(ctrl) );
  };

  template<int I = 0, typename std::enable_if<(I == NumControllers), bool>::type dummy = true>
  static inline std::string getStatusLine(const MHRWControllerType * )
  {
    return std::string();
  }

private:
  static inline std::string _joinnl(std::string a, std::string b) {
    if (a.size() && b.size()) {
      return std::move(a) + "\n" + std::move(b);
    }
    return std::move(a) + std::move(b); // one of these guys is empty
  }
};
// static members:
template<typename... Args>
constexpr int StatusProvider<MHRWMultipleControllers<Args... > >::NumControllers;
template<typename... Args>
constexpr bool StatusProvider<MHRWMultipleControllers<Args... > >::CanProvideStatusLine;


} // namespace Tools








} // namespace Tomographer




#endif
