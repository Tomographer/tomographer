/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
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
#include <cassert>

#include <limits>
#include <random>
#include <tuple>
#include <utility>
#include <type_traits>

#include <tomographer/qit/matrq.h>
#include <tomographer/qit/util.h>
#include <tomographer/qit/param_herm_x.h>
#include <tomographer/tomoproblem.h>
#include <tomographer/histogram.h>
#include <tomographer/tools/loggers.h>
#include <tomographer/mhrw_bin_err.h>


/** \file mhrw.h
 * \brief Routines for performing a Metropolis-Hastings random walk.
 *
 */


namespace Tomographer {


/** \brief Base class performing an abstract random walk
 *
 * This class takes care to update the state of a random walk for it to perform
 * thermalizing sweeps, followed by "live" runs.
 *
 * The random walk is composed of <em>iterations</em>. There are \c n_sweep iterations per
 * "sweep". This number should be approximately chosen such that \c step_size * n_sweep is
 * of order of the size of the state space. (This is in order to better decorrelate the
 * samples.)
 *
 * Initially, a number \c n_therm <em>thermalizing</em> sweeps are performed. No samples
 * are taken during this period, and this is meant for a Metropolis random walk to find
 * its "valley".
 *
 * After the thermalizing runs, a number of <em>run</em> sweeps are performed, in which a
 * <em>live</em> sample is taken at the last iteration of each sweep.
 *
 * This class doesn't actually keep the state of the random walk, nor does it have any
 * notion about which space is being explored. All of this is delegated to an object
 * instance of type \a RandomWalk, which must implement the \a RandomWalk type interface
 * (see \ref pageInterfaceRandomWalk).
 *
 * \note You should not instantiate this class. Just call its static \ref run() method.
 *
 * You might not even need to refer to this class directly. For example, you can call
 * directly \ref DMStateSpaceRandomWalk::run().
 */
template<typename RandomWalk_>
struct RandomWalkBase
{
  typedef RandomWalk_ RandomWalk;
  typedef typename RandomWalk::CountIntType CountIntType;

  /** \brief Run the random walk. (pun intended)
   *
   * This will repeatedly call <code>move()</code> on the random walk object \a rw, first
   * a number of times for the thermalizing runs, then a number of times for the "live"
   * runs. The corresponding callbacks in \a rw will be called as documented in \ref
   * pageInterfaceRandomWalk and in this class documentation.
   */
  static void run(RandomWalk & rw)
  {
    const CountIntType Nsweep = rw.n_sweep();
    const CountIntType Ntherm = rw.n_therm();
    const CountIntType Nrun = rw.n_run();

    rw.init();

    CountIntType k;

    const CountIntType num_thermalize = Nsweep*Ntherm;

    for (k = 0; k < num_thermalize; ++k) {
      // calculate a candidate jump point and see if we accept the move
      rw.move(k, true, false);
    }


    rw.thermalizing_done();

    const CountIntType num_run = Nsweep*Nrun;

    CountIntType n = 0; // number of live samples

    for (k = 0; k < num_run; ++k) {

      bool is_live_iter = ((k+1) % Nsweep == 0);
      
      // calculate a candidate jump point and see if we accept the move
      rw.move(k, false, is_live_iter);

      if (is_live_iter) {
        rw.process_sample(k, n);
        ++n;
      }

    }

    rw.done();
    return;
  }

private:
  RandomWalkBase() { } // static-only class
};







// -----------------



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


namespace tomo_internal {

  /** \internal
   * \brief Helper class for \ref MHRandomWalk
   *
   * Helps out \ref MHRandomWalk decide how to calculate the Metropolis-Hastings function
   * value ratio statically, without having to resort to a run-time \c if or \c switch
   * construct.
   */
  template<typename MHWalker, int UseFnSyntaxType>
  struct MHRandomWalk_helper_decide_jump
  {
    /** \internal
     * \brief Type of a point in the random walk
     */
    typedef typename MHWalker::PointType PointType;
    /** \internal
     * \brief Type of the function value returned by the \c MHWalker object
     */
    typedef typename MHWalker::FnValueType FnValueType;

    /** \internal
     * \brief get a value for the given point
     *
     * Depending on the \c UseFnSyntaxType template parameter, this is either the function
     * value, its logarithm, or a dummy value.
     */
    static inline FnValueType get_ptval(MHWalker & mhwalker, const PointType & curpt)
    {
      assert(0 && "UNKNOWN UseFnSyntaxType: Not implemented");
    }
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
    static inline double get_a_value(MHWalker & /*mhwalker*/, const PointType & /*newpt*/, FnValueType /*newptval*/,
                                     const PointType & /*curpt*/, FnValueType /*curptval*/)
    {
      assert(0 && "UNKNOWN UseFnSyntaxType: Not implemented");
      return 0;
    }
  };

  /** \internal
   * Template specialization of \ref MHRandomWalk_helper_decide_jump.
   */
  template<typename MHWalker>
  struct MHRandomWalk_helper_decide_jump<MHWalker, MHUseFnValue>
  {
    typedef typename MHWalker::PointType PointType;
    typedef typename MHWalker::FnValueType FnValueType;

    static inline FnValueType get_ptval(MHWalker & mhwalker, const PointType & curpt)
    {
      return mhwalker.fnval(curpt);
    }
    static inline double get_a_value(MHWalker & /*mhwalker*/, const PointType & /*newpt*/, double newptval,
                                     const PointType & /*curpt*/, double curptval)
    {
      return (newptval / curptval);
    }
  };

  /** \internal
   * Template specialization of \ref MHRandomWalk_helper_decide_jump.
   */
  template<typename MHWalker>
  struct MHRandomWalk_helper_decide_jump<MHWalker, MHUseFnLogValue>
  {
    typedef typename MHWalker::PointType PointType;
    typedef typename MHWalker::FnValueType FnValueType;

    static inline FnValueType get_ptval(MHWalker & mhwalker, const PointType & curpt)
    {
      return mhwalker.fnlogval(curpt);
    }
    static inline double get_a_value(MHWalker & /*mhwalker*/, const PointType & /*newpt*/, FnValueType newptval,
                                     const PointType & /*curpt*/, FnValueType curptval)
    {
      return (newptval > curptval) ? 1.0 : exp(newptval - curptval);
    }
  };

  /** \internal
   * Template specialization of \ref MHRandomWalk_helper_decide_jump.
   */
  template<typename MHWalker>
  struct MHRandomWalk_helper_decide_jump<MHWalker, MHUseFnRelativeValue>
  {
    typedef typename MHWalker::PointType PointType;
    typedef int FnValueType; // dummy FnValueType

    static inline FnValueType get_ptval(MHWalker & /*mhwalker*/, const PointType & /*curpt*/)
    {
      return 0;
    }
    static inline double get_a_value(MHWalker & mhwalker, const PointType & newpt, FnValueType /*newptval*/,
                                     const PointType & curpt, FnValueType /*curptval*/)
    {
      return mhwalker.fnrelval(newpt, curpt);
    }
  };
};



/** \brief A Metropolis-Hastings Random Walk
 *
 * Implements a <a
 * href="http://en.wikipedia.org/wiki/Metropolis%E2%80%93Hastings_algorithm">Metropolis-Hastings
 * random walk</a>. This takes care of accepting or rejecting a new point and taking samples.
 *
 * The \a MHWalker is responsible for dealing with the state space, providing a new
 * proposal point and calculating the function value at different points. See \ref
 * pageInterfaceMHWalker.
 *
 * A \a MHRWStatsCollector takes care of collecting useful data during a random walk. It
 * should be a type implementing a \a MHRWStatsCollector interface, see \ref
 * pageInterfaceMHRWStatsCollector.
 *
 */
template<typename Rng_, typename MHWalker_, typename MHRWStatsCollector_, typename LoggerType_,
         typename CountIntType_ = unsigned int>
class MHRandomWalk
{
public:
  //! Random number generator type (see C++ std::random)
  typedef Rng_ Rng;
  //! The random walker type which knows about the state space and jump function
  typedef MHWalker_ MHWalker;
  //! The stats collector type (see \ref pageInterfaceMHRWStatsCollector)
  typedef MHRWStatsCollector_ MHRWStatsCollector;
  //! The logger type (see \ref pageLoggers)
  typedef LoggerType_ LoggerType;
  //! The type used for counting numbers of iterations (see, e.g. \ref n_sweep())
  typedef CountIntType_ CountIntType;

  //! The type of a point in the random walk
  typedef typename MHWalker::PointType PointType;
  //! The type of a step size of the random walk
  typedef typename MHWalker::RealScalar RealScalar;

  //! The type of the Metropolis-Hastings function value. (See class documentation)
#ifndef TOMOGRAPHER_PARSED_BY_DOXYGEN
  typedef typename tomo_internal::MHRandomWalk_helper_decide_jump<MHWalker,MHWalker::UseFnSyntaxType>::FnValueType
      FnValueType;
#else
  typedef _FnValueType FnValueType;
#endif

  enum {
    //! How to calculate the Metropolis-Hastings jump probability ratio (see class documentation)
    UseFnSyntaxType = MHWalker::UseFnSyntaxType
  };

private:
  const CountIntType _n_sweep;
  const CountIntType _n_therm;
  const CountIntType _n_run;
  const RealScalar _step_size;

  Rng & _rng;
  MHWalker & _mhwalker;
  MHRWStatsCollector & _stats;
  LoggerType & _logger;

  //! Current point
  PointType curpt;
  /** \brief Current function value at current point. This is a dummy value if
   * <code>UseFnSyntaxType==MHUseFnRelativeValue</code>.
   */
  FnValueType curptval;

  /** \brief Keeps track of the total number of accepted moves during the "live" runs
   * (i.e., not thermalizing). This is used to track the acceptance ratio (see \ref
   * acceptance_ratio())
   */
  CountIntType num_accepted;
  /** \brief Keeps track of the total number of moves during the "live" runs, i.e. not
   * thermalizing. This is used to track the acceptance ratio (see \ref acceptance_ratio())
   */
  CountIntType num_live_points;

public:

  //! Simple constructor, initializes the given fields
  MHRandomWalk(CountIntType n_sweep, CountIntType n_therm, CountIntType n_run, RealScalar step_size,
	       MHWalker & mhwalker, MHRWStatsCollector & stats,
               Rng & rng, LoggerType & logger_)
    : _n_sweep(n_sweep), _n_therm(n_therm), _n_run(n_run),
      _step_size(step_size),
      _rng(rng),
      _mhwalker(mhwalker),
      _stats(stats),
      _logger(logger_),
      curpt(),
      curptval(),
      num_accepted(0),
      num_live_points(0)
  {
    _logger.debug("MHRandomWalk", "constructor(). n_sweep=%lu, n_therm=%lu, n_run=%lu, step_size=%g",
	       (unsigned long)n_sweep, (unsigned long)n_therm, (unsigned long)n_run, (double)step_size);
  }

  //! Required for \ref pageInterfaceRandomWalk. Number of iterations in a sweep.
  inline CountIntType n_sweep() const { return _n_sweep; }
  //! Required for \ref pageInterfaceRandomWalk. Number of thermalizing sweeps.
  inline CountIntType n_therm() const { return _n_therm; }
  //! Required for \ref pageInterfaceRandomWalk. Number of live run sweeps.
  inline CountIntType n_run() const { return _n_run; }

  //! Get the step size of the random walk.
  inline RealScalar step_size() const { return _step_size; }

  /** \brief Required for \ref pageInterfaceRandomWalk. Resets counts and relays to the
   * MHWalker and the MHRWStatsCollector.
   */
  inline void init()
  {
    num_accepted = 0;
    num_live_points = 0;

    // starting point
    curpt = _mhwalker.startpoint();
    curptval = tomo_internal::MHRandomWalk_helper_decide_jump<MHWalker,UseFnSyntaxType>::get_ptval(_mhwalker, curpt);

    _mhwalker.init();
    _stats.init();
  }
  /** \brief Required for \ref pageInterfaceRandomWalk. Relays to the MHWalker and the
   * MHRWStatsCollector.
   */
  inline void thermalizing_done()
  {
    _mhwalker.thermalizing_done();
    _stats.thermalizing_done();
  }
  /** \brief Required for \ref pageInterfaceRandomWalk. Relays to the MHWalker and the
   * MHRWStatsCollector.
   */
  inline void done()
  {
    _mhwalker.done();
    _stats.done();
  }

  /** \brief Access the current state of the random walk
   *
   * \returns the current point the random walk is located at.
   */
  inline const PointType & get_curpt() const
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
  inline const FnValueType & get_curptval() const
  {
    return curptval;
  }

  /** \brief Force manual state of random walk
   *
   * This may be called to force setting the current state of the random walk to the given
   * point \c pt.
   */
  inline void set_curpt(const PointType& pt)
  {
    curpt = pt;
    curptval = tomo_internal::MHRandomWalk_helper_decide_jump<MHWalker,UseFnSyntaxType>::get_ptval(_mhwalker, curpt);
    _logger.longdebug("MHRandomWalk",
                   streamstr("set_curpt(): set internal state. Value = "<<curptval<<"; Point =\n"<<pt<<"\n"));
                   
  }

  /** \brief Required for \ref pageInterfaceRandomWalk. Processes a single move in the random walk.
   *
   * This function gets a new move proposal from the MHWalker object, and calculates the
   * \a a value, which tells us with which probability we should accept the move. This \a
   * a value is calculated according to the documentation in \ref
   * labelMHWalkerUseFnSyntaxType "Role of UseFnSyntaxType".
   */
  inline void move(CountIntType k, bool is_thermalizing, bool is_live_iter)
  {
    // The reason `step_size` is passed to jump_fn instead of leaving jump_fn itself
    // handle the step size, is that we might in the future want to dynamically adapt the
    // step size according to the acceptance ratio. That would have to be done in this
    // class.
    PointType newpt = _mhwalker.jump_fn(curpt, _step_size);

    FnValueType newptval;

    newptval = tomo_internal::MHRandomWalk_helper_decide_jump<MHWalker,UseFnSyntaxType>::get_ptval(_mhwalker, newpt);

    double a = tomo_internal::MHRandomWalk_helper_decide_jump<MHWalker,UseFnSyntaxType>::get_a_value(
        _mhwalker, newpt, newptval, curpt, curptval
        );

    // accept move?
    bool accept = true;
    if (a < 1.0) {
      accept = bool( _rng()-_rng.min() <= a*(_rng.max()-_rng.min()) );
    }

    // track acceptance ratio, except if we are thermalizing
    if (!is_thermalizing) {
      num_accepted += accept ? 1 : 0;
      ++num_live_points;
    }

    _stats.raw_move(k, is_thermalizing, is_live_iter, accept, a, newpt, newptval, curpt, curptval, *this);

    _logger.longdebug("MHRandomWalk",
                   "%s%3lu: %s a=%-7.2g, newptval=%5.4g [llh=%.4g], curptval=%5.4g [llh=%.4g]   accept_ratio=%s",
                   (is_thermalizing?"T":"#"),
                   (unsigned long)k, accept?"AC":"RJ", (double)a, (double)newptval, -2.0*newptval,
		   (double)curptval, -2.0*curptval,
                   (!is_thermalizing?Tools::fmts("%.2g", acceptance_ratio()).c_str():"N/A"));

    if (accept) {
      // update the internal state of the random walk
      curpt = newpt;
      curptval = newptval;
    }
  }

  /** \brief Query whether we have any statistics about acceptance ratio. This is \c
   * false, for example, during the thermalizing runs.
   */
  inline bool has_acceptance_ratio() const
  {
    return (num_live_points > 0);
  }
  /** \brief Return the acceptance ratio so far.
   */
  template<typename RatioType = double>
  inline RatioType acceptance_ratio() const
  {
    return (RatioType) num_accepted / num_live_points;
  }

  /** \brief Required for \ref pageInterfaceRandomWalk. Process a new live sample in the
   * random walk. Relays the call to the \a MHRWStatsCollector (see \ref
   * pageInterfaceMHRWStatsCollector).
   */
  inline void process_sample(CountIntType k, CountIntType n)
  {
    _stats.process_sample(k, n, curpt, curptval, *this);
  }
 

  inline void run()
  {
    RandomWalkBase<MHRandomWalk>::run(*this);
  }
};





// -----------------





/** \brief A simple MHRWStatsCollector interface which combines several stats collectors
 *
 * A \ref MHRandomWalk object expects one instance of a \a MHRWStatsCollector (see \ref
 * pageInterfaceMHRWStatsCollector); in case you wish to provide several stats collectors,
 * you should use a MultipleMHRWStatsCollectors instance which combines all your preferred
 * stats collectors.
 *
 * The obscure variadic templating of this class should not scare you&mdash;it's
 * relatively straightforward to use:
 * \code
 *   // some stat collectors
 *   MyStatCollector1 statcoll1(..);
 *   MyStatCollector2 statcoll2(..);
 *   MyStatCollector3 statcoll3(..);
 *
 *   // we combine them into one single "stat collector interface", which we can then
 *   // give as argument to the MHRandomWalk object:
 *   MultipleMHRWStatsCollectors<MyStatCollector1,MyStatCollector2,MyStatCollector3>
 *       multistatcollector(statcoll1, statcoll2, statcoll3);
 *
 *   MHRandomWalk<...> myrandomwalk(..., multistatcollector,...);
 *   // now, the random walk will call the callbacks in `multistatcollector', which
 *   // will relay the callbacks to all the given stat collectors `statcoll*'.
 * \endcode
 *
 * The number of stat collectors that were defined is accessible through the constant
 * enumeration value \ref NumStatColl.
 *
 */
template<typename... MHRWStatsCollectors>
class MultipleMHRWStatsCollectors
{
public:
  typedef std::tuple<MHRWStatsCollectors&...> MHRWStatsCollectorsRefTupleType;
  typedef std::tuple<MHRWStatsCollectors...> MHRWStatsCollectorsTupleType;

  //! The number of stats collectors we are tracking
  static constexpr int NumStatColl = sizeof...(MHRWStatsCollectors);

private:
  MHRWStatsCollectorsRefTupleType statscollectors;

public:

  MultipleMHRWStatsCollectors(MHRWStatsCollectors&... statscollectors_)
    : statscollectors(statscollectors_...)
  {
  }

  // method to get a particular stats collector
  template<int I>
  inline const typename std::tuple_element<I, MHRWStatsCollectorsTupleType>::type & getStatsCollector()
  {
    return std::get<I>(statscollectors);
  }

  // init() callback

  template<int I = 0>
  inline typename std::enable_if<I < NumStatColl, void>::type init()
  {
    std::get<I>(statscollectors).init();
    init<I+1>();
  }
  template<int I = 0>
  inline typename std::enable_if<I == NumStatColl, void>::type init()
  {
  }

  // thermalizing_done() callback

  template<int I = 0>
  inline typename std::enable_if<I < NumStatColl, void>::type thermalizing_done()
  {
    std::get<I>(statscollectors).thermalizing_done();
    thermalizing_done<I+1>();
  }
  template<int I = 0>
  inline typename std::enable_if<I == NumStatColl, void>::type thermalizing_done()
  {
  }

  // done() callback

  template<int I = 0>
  inline typename std::enable_if<I < NumStatColl, void>::type done()
  {
    std::get<I>(statscollectors).done();
    done<I+1>();
  }
  template<int I = 0>
  inline typename std::enable_if<I == NumStatColl, void>::type done()
  {
  }


  // raw_move() callback

  template<typename CountIntType, typename PointType, typename FnValueType, typename MHRandomWalk, int I = 0>
  inline typename std::enable_if<I < NumStatColl, void>::type raw_move(
      CountIntType k, bool is_thermalizing, bool is_live_iter, bool accepted,
      double a, const PointType & newpt, FnValueType newptval,
      const PointType & curpt, FnValueType curptval,
      MHRandomWalk & rw
      )
  {
    std::get<I>(statscollectors).raw_move(
        k, is_thermalizing, is_live_iter, accepted, a,
        newpt, newptval, curpt, curptval, rw
        );
    raw_move<CountIntType, PointType, FnValueType, MHRandomWalk, I+1>(
        k, is_thermalizing, is_live_iter, accepted, a,
        newpt, newptval, curpt, curptval, rw
        );
  }
  template<typename CountIntType, typename PointType, typename FnValueType, typename MHRandomWalk, int I = 0>
  inline typename std::enable_if<I == NumStatColl, void>::type raw_move(
      CountIntType, bool, bool, bool, double, const PointType &, FnValueType,
      const PointType &, FnValueType, MHRandomWalk &
      )
  {
  }


  // process_sample() callback

  template<typename CountIntType, typename PointType, typename FnValueType, typename MHRandomWalk, int I = 0>
  inline typename std::enable_if<I < NumStatColl, void>::type process_sample(
      CountIntType k, CountIntType n, const PointType & curpt, FnValueType curptval, MHRandomWalk & rw
      )
  {
    std::get<I>(statscollectors).process_sample(k, n, curpt, curptval, rw);
    process_sample<CountIntType, PointType, FnValueType, MHRandomWalk, I+1>(k, n, curpt, curptval, rw);
  }

  template<typename CountIntType, typename PointType, typename FnValueType, typename MHRandomWalk, int I = 0>
  inline typename std::enable_if<I == NumStatColl, void>::type process_sample(
      CountIntType, CountIntType, const PointType &, FnValueType, MHRandomWalk &
      )
  {
  }

};




// -----------------



/** \brief A StatsCollector which builds a histogram of values calculated with a
 * ValueCalculator for each data sample point
 *
 * \a ValueHistogramMHRWStatsCollector complies both with the \ref
 * pageInterfaceMHRWStatsCollector and the \ref pageInterfaceResultable.
 *
 * This stats collector is suitable for tracking statistics during a \ref MHRandomWalk.
 *
 * The ValueCalculator is a type expected to implement the \ref
 * pageInterfaceValueCalculator.
 *
 */
template<typename ValueCalculator_,
	 typename LoggerType = Logger::VacuumLogger,
	 typename HistogramType_ = UniformBinsHistogram<typename ValueCalculator_::ValueType>
	 >
class ValueHistogramMHRWStatsCollector
{
public:
  /** \brief The type which calculates the interesting value. Should be of type interface \ref
   * pageInterfaceValueCalculator.
   */
  typedef ValueCalculator_ ValueCalculator;

  //! The type to use to represent a calculated distance
  typedef typename ValueCalculator::ValueType ValueType;

  //! The type of the histogram. Usually a \ref UniformBinsHistogram with \a ValueType range type
  typedef HistogramType_ HistogramType;

  //! Required for compliance with \ref pageInterfaceResultable type
  typedef HistogramType_ Result;

  //! Structure which holds the parameters of the histogram we're recording
  typedef typename HistogramType::Params HistogramParams;

private:

  //! Store the histogram
  HistogramType _histogram;

  /** \brief The value calculator which we will invoke to get the interesting value.
   *
   * The type should implement the \ref pageInterfaceValueCalculator interface.
   */
  ValueCalculator _vcalc;

  LoggerType & _logger;

public:
  //! Simple constructor, initializes with the given values
  ValueHistogramMHRWStatsCollector(HistogramParams histogram_params,
				   const ValueCalculator & vcalc,
				   LoggerType & logger)
    : _histogram(histogram_params),
      _vcalc(vcalc),
      _logger(logger)
  {
  }

  //! Get the histogram data collected so far. See \ref HistogramType.
  inline const HistogramType & histogram() const
  {
    return _histogram;
  }

  /** \brief Get the histogram data collected so far. This method is needed for \ref
   * pageInterfaceResultable compliance.
   */
  inline const Result & getResult() const
  {
    return _histogram;
  }

  // stats collector part

  //! Part of the \ref pageInterfaceMHRWStatsCollector. Initializes the histogram to zeros.
  inline void init()
  {
    // reset our array
    _histogram.reset();
  }
  //! Part of the \ref pageInterfaceMHRWStatsCollector. No-op.
  inline void thermalizing_done()
  {
  }
  /** \brief Part of the \ref pageInterfaceMHRWStatsCollector.
   *
   * If you call this function with \a PrintHistogram=true (the default), then this will
   * display the final histogram in the logger at logging level \a Logger::LONGDEBUG.
   *
   * If this function is called with \a PrintHistogram=false, then this is a no-op.
   */
  template<bool PrintHistogram = true>
  inline void done()
  {
    if (PrintHistogram) {
      if (_logger.enabled_for(Logger::LONGDEBUG)) {
	// _logger.longdebug("ValueHistogramMHRWStatsCollector", "done()");
	_logger.longdebug("ValueHistogramMHRWStatsCollector",
		       "Done walking & collecting stats. Here's the histogram:\n"
		       + _histogram.pretty_print());
      }
    }
  }

  //! Part of the \ref pageInterfaceMHRWStatsCollector. No-op.
  template<typename CountIntType, typename PointType, typename LLHValueType, typename MHRandomWalk>
  void raw_move(CountIntType k, bool /*is_thermalizing*/, bool /*is_live_iter*/, bool /*accepted*/,
                double /*a*/, const PointType & /*newpt*/, LLHValueType /*newptval*/,
                const PointType & /*curpt*/, LLHValueType /*curptval*/, MHRandomWalk & /*mh*/)
  {
    _logger.longdebug("ValueHistogramMHRWStatsCollector", "raw_move(): k=%lu", (unsigned long)k);
  }

  //! Part of the \ref pageInterfaceMHRWStatsCollector. Records the sample in the histogram.
  template<typename CountIntType, typename PointType, typename LLHValueType, typename MHRandomWalk>
  std::size_t process_sample(CountIntType k, CountIntType n, const PointType & curpt,
                             LLHValueType /*curptval*/, MHRandomWalk & /*mh*/)
  {
    ValueType val = _vcalc.getValue(curpt);

    _logger.longdebug("ValueHistogramMHRWStatsCollector", "in process_sample(): k=%lu, n=%lu, val=%.4g",
                   (unsigned long)k, (unsigned long)n, val);

    return _histogram.record(val);

    //_logger.longdebug("ValueHistogramMHRWStatsCollector", "process_sample() finished");
  }
 

};



/** \brief Traits-like class for ValueHistogramWithBinningMHRWStatsCollector.
 *
 * Collects the template parameters for use with \ref
 * ValueHistogramWithBinningMHRWStatsCollector.
 *
 * Provides also some trait properties, such as the corresponding result type \ref Result.
 */
template<typename ValueCalculator_,
         typename CountIntType_ = int,
         typename CountRealAvgType_ = double,
         int NumTrackValues_ = Eigen::Dynamic,
         int NumLevels_ = Eigen::Dynamic
	 >
struct ValueHistogramWithBinningMHRWStatsCollectorParams
{
  /** \brief The type of the \ref pageInterfaceValueCalculator which calculates the value
   * of which we're collecting a histogram. */
  typedef ValueCalculator_ ValueCalculator;
  /** \brief Type used to count the number of hits in each bin. */
  typedef CountIntType_ CountIntType;
  /** \brief Type used to store the averages of the histogram bins. */
  typedef CountRealAvgType_ CountRealAvgType;

  //! Number of values we're tracking, i.e. the number of bins in the histogram [for now]
  static constexpr int NumTrackValues = NumTrackValues_;
  //! The number of levels in the binning analysis. See \ref BinningAnalysis.
  static constexpr int NumLevels = NumLevels_;

  //! The type of a value calculated by the \ref pageInterfaceValueCalculator.
  typedef typename ValueCalculator::ValueType ValueType;
  
  //! The relevant \ref BinningAnalysis parameters for us.
  typedef BinningAnalysisParams<ValueType,NumTrackValues,NumLevels,false/*StoreBinSums*/,CountIntType>
  BinningAnalysisParamsType;

  /** \brief The Base Histogram Type.
   *
   * The type of the histogram which simply stores the bin counts.
   */
  typedef UniformBinsHistogram<typename ValueCalculator::ValueType, CountIntType> BaseHistogramType;
  /** \brief The Final Histogram Type (with error bars).
   *
   */
  typedef UniformBinsHistogramWithErrorBars<typename ValueCalculator::ValueType, CountRealAvgType> HistogramType;
  typedef typename HistogramType::Params HistogramParams;

  /** \brief Result type of the corresponding ValueHistogramWithBinningMHRWStatsCollector
   *
   * Stores a histogram with error bars, detailed information about error bars at different binning
   * levels, and information about the convergence of these error bars.
   */
  struct Result
  {
    //! Simple default constructor (e.g. to use as std::vector<Result>).
    explicit Result()
      : hist(), error_levels(), converged_status()
    {
    }

    //! Constructor which initializes the fields from the histogram and binning analysis type.
    template<typename BinningAnalysisType>
    Result(HistogramParams p, const BinningAnalysisType & b)
      : hist(p),
	error_levels(b.num_track_values(), b.num_levels()+1),
	converged_status(Eigen::ArrayXi::Constant(b.num_track_values(), BinningAnalysisType::UNKNOWN_CONVERGENCE))
    {
      eigen_assert(converged_status.rows() == b.num_track_values() && converged_status.cols() == 1);
    }

    //! Histogram, already with error bars
    HistogramType hist;
    //! Detailed error bars for all binning levels
    typename BinningAnalysisParamsType::BinSumSqArray error_levels;
    /** \brief Information of convergence status of the error bars (see e.g. \ref
     * BinningAnalysisParamsType::CONVERGED)
     */
    Eigen::ArrayXi converged_status;

    //! Dump values, error bars and convergence status in human-readable form into ostream
    inline void dump_convergence_analysis(std::ostream & str) const
    {
      for (int k = 0; k < converged_status.size(); ++k) {
	str << "\tval[" << std::setw(3) << k << "] = "
	    << std::setw(12) << hist.bins(k)
	    << " +- " << std::setw(12) << hist.delta(k);
	if (converged_status(k) == BinningAnalysisParamsType::CONVERGED) {
	  str << "  [CONVERGED]";
	} else if (converged_status(k) == BinningAnalysisParamsType::NOT_CONVERGED) {
	  str << "  [NOT CONVERGED]";
	} else if (converged_status(k) == BinningAnalysisParamsType::UNKNOWN_CONVERGENCE) {
	  str << "  [UNKNOWN]";
	} else {
	  str << "  [UNKNOWN CONVERGENCE STATUS: " << converged_status(k) << "]";
	}
	str << "\n";
      }
    }

    //! Dump values, error bars and convergence status in human-readable form as string
    inline std::string dump_convergence_analysis() const
    {
      std::stringstream ss;
      dump_convergence_analysis(ss);
      return ss.str();
    }

  };

};

/** \brief Collect a histogram of values from a MH random walk, with binning analysis.
 *
 * The \a Params template parameter must be a
 * ValueHistogramWithBinningMHRWStatsCollectorParams with the relevant template
 * parameters.
 *
 */
template<typename Params,
	 typename LoggerType_ = Logger::VacuumLogger
         >
class ValueHistogramWithBinningMHRWStatsCollector
{
public:

  //! See \ref ValueHistogramWithBinningMHRWStatsCollectorParams::ValueCalculator .
  typedef typename Params::ValueCalculator ValueCalculator;
  //! See \ref ValueHistogramWithBinningMHRWStatsCollectorParams::CountIntType .
  typedef typename Params::CountIntType CountIntType;
  //! See \ref ValueHistogramWithBinningMHRWStatsCollectorParams::CountRealAvgType .
  typedef typename Params::CountRealAvgType CountRealAvgType;
  
  /** \brief Somewhere where this object may log what it's doing. */
  typedef LoggerType_ LoggerType;

  //! See \ref ValueHistogramWithBinningMHRWStatsCollectorParams::BaseHistogramType .
  typedef typename Params::BaseHistogramType BaseHistogramType;
  //! See \ref ValueHistogramWithBinningMHRWStatsCollectorParams::HistogramParams .
  typedef typename Params::HistogramParams HistogramParams;
    
  //! See \ref ValueHistogramWithBinningMHRWStatsCollectorParams::ValueType .
  typedef typename Params::ValueType ValueType;
  
  //! See \ref ValueHistogramWithBinningMHRWStatsCollectorParams::BinningAnalysisParamsType .
  typedef typename Params::BinningAnalysisParamsType BinningAnalysisParamsType;
  //! See \ref ValueHistogramWithBinningMHRWStatsCollectorParams::BinningAnalysisType .
  typedef BinningAnalysis<BinningAnalysisParamsType, LoggerType> BinningAnalysisType;
    
  //! See \ref ValueHistogramWithBinningMHRWStatsCollectorParams::NumTrackValues .
  static constexpr int NumTrackValuesCTime = Params::NumTrackValues;
  //! See \ref ValueHistogramWithBinningMHRWStatsCollectorParams::NumLevels .
  static constexpr int NumLevelsCTime = Params::NumLevels;

  //! See \ref ValueHistogramWithBinningMHRWStatsCollectorParams::Result .
  typedef typename Params::Result Result;

  /** \brief This is the natural ValueHistogramMHRWStatsCollector type on which we're
   *         adding error bars.
   */
  typedef ValueHistogramMHRWStatsCollector<
    ValueCalculator,
    LoggerType,
    BaseHistogramType
    > ValueHistogramMHRWStatsCollectorType;
  
private:
    
  ValueHistogramMHRWStatsCollectorType value_histogram;

  BinningAnalysisType binning_analysis;

  LoggerType & logger;

  Result result;

public:
    
  ValueHistogramWithBinningMHRWStatsCollector(HistogramParams histogram_params,
                                              const ValueCalculator & vcalc,
                                              int num_levels,
                                              LoggerType & logger_)
    : value_histogram(histogram_params, vcalc, logger_),
      binning_analysis(histogram_params.num_bins, num_levels, logger_),
      logger(logger_),
      result(histogram_params, binning_analysis)
  {
    logger.longdebug("ValueHistogramWithBinningMHRWStatsCollector", "constructor()");
  }
  
  //! Get the histogram data collected so far. See \ref BaseHistogramType .
  inline const BaseHistogramType & histogram() const
  {
    return value_histogram.histogram();
  }

  inline const BinningAnalysisType & get_binning_analysis() const
  {
    return binning_analysis;
  }

  /** \brief Get the final histogram data. This method is needed for \ref
   * pageInterfaceResultable compliance.
   *
   * This will only yield a valid value AFTER the all the data has been collected and \ref
   * done() was called.
   */
  inline const Result & getResult() const
  {
    return result;
  }

  // stats collector part

  //! Part of the \ref pageInterfaceMHRWStatsCollector. Initializes the histogram to zeros.
  inline void init()
  {
    value_histogram.init();
  }
  //! Part of the \ref pageInterfaceMHRWStatsCollector. No-op.
  inline void thermalizing_done()
  {
    value_histogram.thermalizing_done();
  }
  //! Finalize the data collection. Part of the \ref pageInterfaceMHRWStatsCollector.
  inline void done()
  {
    logger.longdebug("ValueHistogramWithBinningMHRWStatsCollector::done()", "finishing up ...");

    value_histogram.template done<false>();

    const BaseHistogramType & h = value_histogram.histogram();
    result.hist.params = h.params;
    CountRealAvgType normalization = h.bins.sum() + h.off_chart; // need ALL samples, because that's
								 // what the binning analysis sees
    result.hist.bins = h.bins.template cast<CountRealAvgType>() / normalization;
    result.error_levels = binning_analysis.calc_error_levels(result.hist.bins);
    result.hist.delta = result.error_levels.col(binning_analysis.num_levels()).template cast<CountRealAvgType>();
    result.hist.off_chart = h.off_chart / normalization;

    result.converged_status = binning_analysis.determine_error_convergence(result.error_levels);

    logger.debug("ValueHistogramWithBinningMHRWStatsCollector", [&,this](std::ostream & str) {
        str << "Binning analysis: bin sqmeans at different binning levels are:\n"
            << binning_analysis.get_bin_sqmeans() << "\n"
	    << "\t-> so the error bars at different binning levels are:\n"
	    << result.error_levels << "\n"
	    << "\t-> convergence analysis: \n";
	result.dump_convergence_analysis(str);
	str << "\t... and just for you, here is the final histogram:\n" << result.hist.pretty_print() << "\n";
      });
  }

  //! Part of the \ref pageInterfaceMHRWStatsCollector. No-op.
  template<typename CountIntType2, typename PointType, typename LLHValueType, typename MHRandomWalk>
  inline void raw_move(CountIntType2 k, bool is_thermalizing, bool is_live_iter, bool accepted,
		       double a, const PointType & newpt, LLHValueType newptval,
		       const PointType & curpt, LLHValueType curptval, MHRandomWalk & mh)
  {
    value_histogram.raw_move(k, is_thermalizing, is_live_iter, accepted, a, newpt, newptval, curpt, curptval, mh);
  }

  //! Part of the \ref pageInterfaceMHRWStatsCollector. Records the sample in the histogram.
  template<typename CountIntType2, typename PointType, typename LLHValueType, typename MHRandomWalk>
  inline void process_sample(CountIntType2 k, CountIntType2 n, const PointType & curpt,
			     LLHValueType curptval, MHRandomWalk & mh)
  {
    std::size_t histindex = value_histogram.process_sample(k, n, curpt, curptval, mh);
    binning_analysis.process_new_values(
	can_basis_vec<Eigen::Array<ValueType,Eigen::Dynamic,1> >(histindex, value_histogram.histogram().num_bins())
	);
  }

};






/** \brief Template, specializable class to get status reports from stats collectors.
 *
 * Specialize this class for your stats collector to be able to provide a short status
 * report. Just provide 2-3 lines with the most important information enough to provide a
 * very basic overview, not a full-length report.
 */
template<typename MHRWStatsCollector_>
struct MHRWStatsCollectorStatus
{
  typedef MHRWStatsCollector_ MHRWStatsCollector;

  static constexpr bool CanProvideStatus = false;

  /** \brief Prepare a string which reports the status of the given stats collector
   *
   * Don't end your string with a newline, it will be added automatically.
   */
  static inline std::string getStatus(const MHRWStatsCollector * /*stats*/)
  {
    return std::string();
  }
};



/** \brief Provide status reporting for a MultipleMHRWStatsCollectors
 *
 */
template<typename... Args>
struct MHRWStatsCollectorStatus<MultipleMHRWStatsCollectors<Args... > >
{
  typedef MultipleMHRWStatsCollectors<Args... > MHRWStatsCollector;

  static constexpr int NumStatColl = MHRWStatsCollector::NumStatColl;
  
  static constexpr bool CanProvideStatus = true;

  template<int I = 0, typename std::enable_if<(I < NumStatColl), bool>::type dummy = true>
  static inline std::string getStatus(const MHRWStatsCollector * stats)
  {
    typedef typename std::tuple_element<I, typename MHRWStatsCollector::MHRWStatsCollectorsTupleType>::type
      ThisStatsCollector;
    return
      (MHRWStatsCollectorStatus<ThisStatsCollector>::CanProvideStatus
       ? (MHRWStatsCollectorStatus<ThisStatsCollector>::getStatus(stats->template getStatsCollector<I>())
	  + ((I < (NumStatColl-1)) ? std::string("\n") : std::string()))
       : std::string())
      + getStatus<I+1>(stats);
  };

  template<int I = 0, typename std::enable_if<(I == NumStatColl), bool>::type dummy = true>
  static inline std::string getStatus(const MHRWStatsCollector * stats)
  {
    return std::string();
  }

};



/** \brief Provide status reporting for a ValueHistogramMHRWStatsCollector
 *
 */
template<typename ValueCalculator_,
	 typename LoggerType_,
	 typename HistogramType_
	 >
struct MHRWStatsCollectorStatus<ValueHistogramMHRWStatsCollector<ValueCalculator_, LoggerType_, HistogramType_> >
{
  typedef ValueHistogramMHRWStatsCollector<ValueCalculator_, LoggerType_, HistogramType_> MHRWStatsCollector;
  
  static constexpr bool CanProvideStatus = true;

  static inline std::string getStatus(const MHRWStatsCollector * stats)
  {
    const int maxbarwidth = 50;

    typedef typename MHRWStatsCollector::HistogramType HistogramType;

    return "Histogram: " + histogram_short_bar<HistogramType>(stats->histogram(), true, maxbarwidth);
  }
};

/** \brief Provide status reporting for a ValueHistogramWithBinningMHRWStatsCollector
 *
 */
template<typename Params_,
	 typename LoggerType_
	 >
struct MHRWStatsCollectorStatus<ValueHistogramWithBinningMHRWStatsCollector<Params_, LoggerType_> >
{
  typedef ValueHistogramWithBinningMHRWStatsCollector<Params_, LoggerType_> MHRWStatsCollector;
  
  static constexpr bool CanProvideStatus = true;

  static inline std::string getStatus(const MHRWStatsCollector * stats)
  {
    const int maxbarwidth = 50;

    typedef typename MHRWStatsCollector::BaseHistogramType BaseHistogramType;
    const BaseHistogramType & histogram = stats->histogram();

    // calculate the error bars at different levels, to determine convergence status.
    typedef typename MHRWStatsCollector::BinningAnalysisType BinningAnalysisType;
    //typedef typename MHRWStatsCollector::CountRealAvgType CountRealAvgType;
    typedef typename BinningAnalysisType::ValueType ValueType;
    const auto& binning_analysis = stats->get_binning_analysis();
    Eigen::Array<ValueType, Eigen::Dynamic, 1> binmeans(histogram.num_bins());
    binmeans = histogram.bins.template cast<ValueType>() /
      (ValueType)(histogram.bins.sum() + histogram.off_chart);

    auto error_levels = binning_analysis.calc_error_levels(binmeans);
    auto conv_status = binning_analysis.determine_error_convergence(error_levels);

    int n_cnvg = 0;
    int n_unknown = 0;
    int n_not_cnvg = 0;
    for (std::size_t k = 0; k < (std::size_t)histogram.num_bins(); ++k) {
      if (conv_status(k) == BinningAnalysisType::CONVERGED) {
	++n_cnvg;
      } else if (conv_status(k) == BinningAnalysisType::NOT_CONVERGED) {
	++n_not_cnvg;
      } else {
	++n_unknown;
      }
    }

    return tomo_internal::histogram_short_bar_fmt<BaseHistogramType>(histogram, "", maxbarwidth)
      + Tools::fmts("   err: (cnvg/?/fail) %d/%d/%d", n_cnvg, n_unknown, n_not_cnvg);
  }
};



} // namespace Tomographer








#endif
