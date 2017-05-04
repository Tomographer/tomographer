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

#include <tomographer/tools/loggers.h>
#include <tomographer/tools/fmt.h>
#include <tomographer/tools/cxxutil.h>
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
const double MHRWAcceptanceRatioRecommendedMin = 0.2;
/** \brief Maximal recommended acceptance ratio */
const double MHRWAcceptanceRatioRecommendedMax = 0.4;


/** \brief An MHWalkerParams type which just stores a step size
 *
 * This type is a simple struct which stores a step size. It should be used as the \a
 * WalkerParams typedef of a \ref pageInterfaceMHWalker compliant type which only needs a
 * step size as parameter.
 */
template<typename StepRealType_ = double>
TOMOGRAPHER_EXPORT struct MHWalkerParamsStepSize
{
  typedef StepRealType_ StepRealType;

  MHWalkerParamsStepSize() : step_size() { }
  MHWalkerParamsStepSize(StepRealType step_size_) : step_size(step_size_) { }

  StepRealType step_size;
};

template<typename StepRealType>
std::ostream & operator<<(std::ostream & stream, MHWalkerParamsStepSize<StepRealType> p)
{
  return stream << "step_size=" << p.step_size;
}


/** \brief Specify the parameters of a Metropolis-Hastings random walk
 *
 * Specifies the parameters of a Metropolis-Hastings random walk (number of thermalization
 * runs, sweep size, number of live runs, step size).
 */
template<typename CountIntType_ = int, typename MHWalkerParams_ = MHWalkerParamsStepSize<double> >
TOMOGRAPHER_EXPORT struct MHRWParams
{
  typedef CountIntType_ CountIntType;
  typedef MHWalkerParams_ MHWalkerParams;

  explicit MHRWParams()
    : mhwalker_params(), n_sweep(0), n_therm(0), n_run(0)
  {
  }
  MHRWParams(MHWalkerParams mhwalker_params_, CountIntType n_sweep_, CountIntType n_therm_, CountIntType n_run_)
    : mhwalker_params(mhwalker_params_), n_sweep(n_sweep_), n_therm(n_therm_), n_run(n_run_)
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
std::ostream & operator<<(std::ostream & str, const MHRWParams<CountIntType,MHWalkerParams> & p)
{
  str << "MHRWParams(" << p.mhwalker_params << ",n_sweep=" << p.n_sweep
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
 */
enum MHWalkerParamsAdjustmentStrategy {
  //! Never adjust the parameters of the random walk
  MHWalkerParamsDoNotAdjust = 0,

  //! Adjustments are enabled during thermalization sweeps
  MHWalkerParamsAdjustWhileThermalizing = 0x01,
  //! Adjustments are enabled during live (running) sweeps
  MHWalkerParamsAdjustWhileRunning = 0x02,
  //! Adjustments are enabled during both thermalization and live (running) sweeps
  MHWalkerParamsAdjustWhileThermalizingAndRunning =
    MHWalkerParamsAdjustWhileThermalizing | MHWalkerParamsAdjustWhileRunning,

  //! Adjustemnts should be performed after each individual iteration
  MHWalkerParamsAdjustEveryIteration = 0x10,
  //! Adjustemnts should be performed after a sample is taken (during live runs only)
  MHWalkerParamsAdjustEverySample = 0x20,

  //! Adjustemnts should be performed only while thermalizing, after each individual iteration
  MHWalkerParamsAdjustEveryIterationWhileThermalizing =
    MHWalkerParamsAdjustEveryIteration | MHWalkerParamsAdjustWhileThermalizing,

  //! Adjustemnts should be performed all the time, after each individual iteration
  MHWalkerParamsAdjustAllTheTime =
    MHWalkerParamsAdjustEveryIteration | MHWalkerParamsAdjustWhileThermalizingAndRunning
};

/** \brief A \ref pageInterfaceMHWalkerParamsAdjuster which does not adjust anything
 *
 * No adjustments are performed whatsoever. The MHWalkerParams may be of any type.
 */
class MHWalkerParamsNoAdjuster
{
public:
  enum { AdjustmentStrategy = MHWalkerParamsDoNotAdjust };

  MHWalkerParamsNoAdjuster() { }

  template<typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType>
  inline void initParams(const MHRWParamsType & /* params */, const MHWalker & /* mhwalker */,
                         const MHRandomWalkType & /* mhrw */) const { }

  template<typename MHRWParamsType, typename MHWalker, typename MHRandomWalkType>
  inline void thermalizingDone(const MHRWParamsType & /* params */, const MHWalker & /* mhwalker */,
                               const MHRandomWalkType & /* mhrw */) const { }

  template<bool IsThermalizing, bool IsAfterSample, typename MHRWParamsType, typename CountIntType,
           typename MHWalker, typename MHRandomWalkType>
  inline void adjustParams(const MHRWParamsType & /* params */, const MHWalker & /* mhwalker */,
                           CountIntType /* iter_k */, const MHRandomWalkType & /* mhrw */) const
  {
  }
};


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
 * \tparam LoggerType is a logger type to which log messages can be generated (see \ref
 *         pageLoggers)
 *
 * \tparam CountIntType is an integer type used to count the number of iterations.  You'll
 *         want to use \a int here, unless you really want to pursue exceptionally long
 *         random walks.
 *
 */
template<typename Rng_, typename MHWalker_, typename MHRWStatsCollector_, typename LoggerType_,
         typename CountIntType_ = int, typename MHWalkerParamsAdjuster_ = MHWalkerParamsNoAdjuster>
TOMOGRAPHER_EXPORT class MHRandomWalk
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
  typedef MHRWParams<CountIntType, MHWalkerParams> MHRWParamsType;

  //! The type which will take care of dynamically adjusting the parameters of the random walk
  typedef MHWalkerParamsAdjuster_ MHWalkerParamsAdjuster;
  enum { MHWalkerParamsAdjusterStrategy = MHWalkerParamsAdjuster::AdjustmentStrategy };

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
  typename tomo_internal::const_type_helper<MHRWParamsType,(int)MHWalkerParamsAdjusterStrategy==(int)MHWalkerParamsDoNotAdjust>::type _n;

  Rng & _rng;
  MHWalker & _mhwalker;
  MHRWStatsCollector & _stats;
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


  MHWalkerParamsAdjuster _mhwalker_params_adjuster;


public:

  //! Simple constructor, initializes the given fields
  MHRandomWalk(MHWalkerParams mhwalker_params, CountIntType n_sweep, CountIntType n_therm, CountIntType n_run,
	       MHWalker & mhwalker, MHRWStatsCollector & stats,
               Rng & rng, LoggerType & logger_, MHWalkerParamsAdjuster mhwalker_params_adjuster = MHWalkerParamsAdjuster())
    : _n(mhwalker_params, n_sweep, n_therm, n_run),
      _rng(rng),
      _mhwalker(mhwalker),
      _stats(stats),
      _logger(TOMO_ORIGIN, logger_),
      curpt(),
      curptval(),
      num_accepted(0),
      num_live_points(0),
      _mhwalker_params_adjuster(mhwalker_params_adjuster)
  {
    _logger.debug([&](std::ostream & stream) {
	stream << "constructor(). n_sweep=" << n_sweep << ", mhwalker_params=" << mhwalker_params
	       << "n_therm=" << n_therm << ", n_run=" << n_run;
      });
  }
  //! Simple constructor, initializes the given fields
  template<typename MHRWParamsType>
  MHRandomWalk(MHRWParamsType&& n_rw,
	       MHWalker & mhwalker, MHRWStatsCollector & stats,
               Rng & rng, LoggerType & logger_, MHWalkerParamsAdjuster mhwalker_params_adjuster = MHWalkerParamsAdjuster())
    : _n(std::forward<MHRWParamsType>(n_rw)),
      _rng(rng),
      _mhwalker(mhwalker),
      _stats(stats),
      _logger(TOMO_ORIGIN, logger_),
      curpt(),
      curptval(),
      num_accepted(0),
      num_live_points(0),
      _mhwalker_params_adjuster(mhwalker_params_adjuster)
  {
    _logger.debug([&](std::ostream & s) { s << "constructor(). mhrw parameters = " << _n; });
  }

  MHRandomWalk(const MHRandomWalk & other) = delete;


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

    _mhwalker_params_adjuster.initParams(_n, _mhwalker, *this);

    _logger.longdebug("_init() done.");
  }
  /** \brief Relays to the MHWalker and the MHRWStatsCollector.
   */
  inline void _thermalizing_done()
  {
    _mhwalker.thermalizingDone();
    _stats.thermalizingDone();

    _mhwalker_params_adjuster.thermalizingDone(_n, _mhwalker, *this);

    _logger.longdebug("_thermalizing_done() done.");
  }
  /** \brief Relays to the MHWalker and the MHRWStatsCollector.
   */
  inline void _done()
  {
    _mhwalker.done();
    _stats.done();
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
    return (newptval > curptval) ? 1.0 : exp(newptval - curptval);
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
  void _adjustparams_afteriter(CountIntType iter_k)
  {
    _mhwalker_params_adjuster.template adjustParams<IsThermalizing, false>(_n, _mhwalker, iter_k, *this);
  }
  void _adjustparams_aftersample(CountIntType iter_k)
  {
    _mhwalker_params_adjuster.template adjustParams<false, true>(_n, _mhwalker, iter_k, *this);
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
	s << "Starting random walk, sweep size = " << _n.n_sweep << ", mhwalker params = " << _n.mhwalker_params
	  << ", # therm sweeps = " << _n.n_therm << ", # live sweeps = " << _n.n_run;
      });

    const CountIntType num_thermalize = _n.n_sweep * _n.n_therm;

    for (k = 0; k < num_thermalize; ++k) {
      // calculate a candidate jump point and see if we accept the move
      _move<true>(k, false);
      _adjustparams_afteriter<true>(k);
    }

    _thermalizing_done();

    _logger.longdebug("Thermalizing done, starting live runs.");

    const CountIntType num_run = _n.n_sweep * _n.n_run;

    CountIntType n = 0; // number of live samples

    for (k = 0; k < num_run; ++k) {

      bool is_live_iter = ((k+1) % _n.n_sweep == 0);
      
      // calculate a candidate jump point and see if we accept the move
      _move<false>(k, is_live_iter);
      _adjustparams_afteriter<false>(k);

      if (is_live_iter) {
        _process_sample(k, n);
        ++n;
        _adjustparams_aftersample(k);
      }

    }

    _done();

    _logger.longdebug("Random walk completed.");

    return;
  }
};



} // namespace Tomographer




#endif
