
#ifndef INTEGRATOR_H
#define INTEGRATOR_H

#include <cassert>
#include <limits>
#include <random>
#include <tuple>
#include <utility>

#include <tomographer/qit/matrq.h>
#include <tomographer/tomoproblem.h>
#include <tomographer/param_herm_x.h>
#include <tomographer/loggers.h>







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
 * notion about which space is being explored. The \c RandomWalk type, and the \c rw
 * object instance, are responsible for actually implementing the random walk. It should
 * keep the current state of the random walk in memory and update it when the \c move()
 * function is called. \c RandomWalk needs to provide the following members, which are
 * called at the appropriate times:
 *
 * <ul><li> <code>size_t RandomWalk::n_sweep()</code>
 *          Number of iterations that compose a "sweep".
 *     <li> <code>size_t RandomWalk::n_therm()</code>
 *          Number of thermalizing sweeps to perform.
 *     <li> <code>size_t RandomWalk::n_run()</code>
 *          Number of live sweeps to perofrm.
 *     <li> <code>void RandomWalk::move(size_t k, bool is_thermalizing, bool is_live_iter)</code>
 *          Is called to perform a new random walk iteration. The \c rw object is
 *          responsible for keeping the current state of the random walk in memory,
 *          and for processing a jump function. This method should update the internal
 *          state of the random walk. This function does not return anything. \c k is the
 *          raw iteration count, starting at zero (and which is reset to zero after the
 *          thermalizing sweeps). \c is_thermalizing is \c true during the thermalizing
 *          runs, \c false otherwise. \c is_live_iter is \c true when a live sample is
 *          taken, only once every sweep after the thermalization runs.
 *     <li> <code>void RandomWalk::process_sample(size_t k)</code>
 *          Is called for each "live" point for which a sample should be taken. The point
 *          in question is the current state of the random walk. This only happens after
 *          thermalization, and at the last iteration of a sweep.
 *     <li> <code>void RandomWalk::init()</code>
 *          Is called before starting the random walk. The RandomWalk may perform custom
 *          last-minute initializations if required.
 *     <li> <code>void RandomWalk::thermalizing_done()</code>
 *          Is called after the thermalizing runs and before starting the live runs.
 *     <li> <code>void RandomWalk::done()</code>
 *          Is called after the random walk is finished. This happens after the given
 *          number of iterations were reached.
 * </ul>
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

  static void run(RandomWalk & rw)
  {

    const size_t Nsweep = rw.n_sweep();
    const size_t Ntherm = rw.n_therm();
    const size_t Nrun = rw.n_run();

    rw.init();

    size_t k;

    const size_t num_thermalize = Nsweep*Ntherm;

    for (k = 0; k < num_thermalize; ++k) {
      // calculate a candidate jump point and see if we accept the move
      rw.move(k, true, false);
    }


    rw.thermalizing_done();

    const size_t num_run = Nsweep*Nrun;

    for (k = 0; k < num_run; ++k) {

      bool is_live_iter = ((k+1) % Nsweep == 0);
      
      // calculate a candidate jump point and see if we accept the move
      rw.move(k, false, is_live_iter);

      if (is_live_iter) {
        rw.process_sample(k);
      }

    }

    rw.done();
    return;
  }

private:
  RandomWalkBase() { } // static-only class
};








enum {
  MHUseFnValue = 1,    // use MHWalker::fnval(newpt)
  MHUseFnLogValue,     // use MHWalker::fnlogval(newpt)
  MHUseFnRelativeValue // use MHWalker::fnrelval(newpt, curpt)
};


namespace tomo_internal {
  template<typename MHWalker, int UseFnSyntaxType>
  struct MHRandomWalk_helper_decide_jump
  {
    typedef typename MHWalker::PointType PointType;
    typedef int FnValueType; // dummy FnValueType

    static inline double get_ptval(MHWalker & mhwalker, const PointType & curpt)
    {
      assert(0 && "UNKNOWN UseFnSyntaxType: Not implemented");
    }
    static inline double get_a_value(MHWalker & /*mhwalker*/, const PointType & /*newpt*/, double /*newptval*/,
                                     const PointType & /*curpt*/, double /*curptval*/)
    {
      assert(0 && "UNKNOWN UseFnSyntaxType: Not implemented");
    }
  };

  template<typename MHWalker>
  struct MHRandomWalk_helper_decide_jump<MHWalker, MHUseFnValue>
  {
    typedef typename MHWalker::PointType PointType;
    typedef typename MHWalker::FnValueType FnValueType;

    static inline double get_ptval(MHWalker & mhwalker, const PointType & curpt)
    {
      return mhwalker.fnval(curpt);
    }
    static inline double get_a_value(MHWalker & /*mhwalker*/, const PointType & /*newpt*/, double newptval,
                                     const PointType & /*curpt*/, double curptval)
    {
      return (newptval / curptval);
    }
  };

  template<typename MHWalker>
  struct MHRandomWalk_helper_decide_jump<MHWalker, MHUseFnLogValue>
  {
    typedef typename MHWalker::PointType PointType;
    typedef typename MHWalker::FnValueType FnValueType;

    static inline double get_ptval(MHWalker & mhwalker, const PointType & curpt)
    {
      return mhwalker.fnlogval(curpt);
    }
    static inline double get_a_value(MHWalker & /*mhwalker*/, const PointType & /*newpt*/, double newptval,
                                     const PointType & /*curpt*/, double curptval)
    {
      return (newptval > curptval) ? 1.0 : exp(newptval - curptval);
    }
  };

  template<typename MHWalker>
  struct MHRandomWalk_helper_decide_jump<MHWalker, MHUseFnRelativeValue>
  {
    typedef typename MHWalker::PointType PointType;
    typedef int FnValueType; // dummy FnValueType

    static inline double get_ptval(MHWalker & /*mhwalker*/, const PointType & /*curpt*/)
    {
      return 0.0;
    }
    static inline double get_a_value(MHWalker & mhwalker, const PointType & newpt, double /*newptval*/,
                                     const PointType & curpt, double /*curptval*/)
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
 * .....................
 *
 */
template<typename Rng, typename MHWalker, typename RWStatsCollector, typename Log>
class MHRandomWalk
{
public:
  typedef typename MHWalker::PointType PointType;
  typedef typename MHWalker::RealScalar RealScalar;

  typedef typename tomo_internal::MHRandomWalk_helper_decide_jump<MHWalker,MHWalker::UseFnSyntaxType>::FnValueType
          FnValueType;
  enum {
    UseFnSyntaxType = MHWalker::UseFnSyntaxType
  };

private:
  const size_t _n_sweep;
  const size_t _n_therm;
  const size_t _n_run;
  const RealScalar _step_size;

  Rng & _rng;
  MHWalker & _mhwalker;
  RWStatsCollector & _stats;
  Log & _log;

  PointType curpt;
  FnValueType curptval;

  size_t num_accepted;
  size_t num_live_points;

public:

  MHRandomWalk(size_t n_sweep, size_t n_therm, size_t n_run, RealScalar step_size,
               const PointType & startpt, MHWalker & mhwalker, RWStatsCollector & stats,
               Rng & rng, Log & log_)
    : _n_sweep(n_sweep), _n_therm(n_therm), _n_run(n_run),
      _step_size(step_size),
      _mhwalker(mhwalker),
      _stats(stats),
      _log(log_),
      _rng(rng),
      curpt(startpt)
  {
  }

  inline size_t n_sweep() const { return _n_sweep; }
  inline size_t n_therm() const { return _n_therm; }
  inline size_t n_run() const { return _n_run; }

  inline void init()
  {
    num_accepted = 0;
    num_live_points = 0;

    curptval = tomo_internal::MHRandomWalk_helper_decide_jump<MHWalker,UseFnSyntaxType>::get_ptval(_mhwalker, curpt);

    _mhwalker.init();
    _stats.init();
  }
  inline void thermalizing_done()
  {
    _mhwalker.thermalizing_done();
    _stats.thermalizing_done();
  }
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
    _log.longdebug("MHRandomWalk",
                   streamstr("set_curpt(): set internal state. Value = "<<curptval<<"; Point =\n"<<pt<<"\n"));
                   
  }

  inline void move(size_t k, bool is_thermalizing, bool is_live_iter)
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

    _log.longdebug("MHRandomWalk",
                   "%s%3lu: %s a=%-7.2g, newptval=%5.4g [llh=%.4g], curptval=%5.4g [llh=%.4g]   accept_ratio=%s",
                   (is_thermalizing?"T":"#"),
                   k, accept?"AC":"RJ", a, newptval, -2*newptval, curptval, -2*curptval,
                   (!is_thermalizing?fmts("%.2g", acceptance_ratio()).c_str():"N/A"));

    if (accept) {
      // update the internal state of the random walk
      curpt = newpt;
      curptval = newptval;
    }
  }

  inline double acceptance_ratio() const
  {
    return (double) num_accepted / num_live_points;
  }

  inline void process_sample(size_t k)
  {
    _stats.process_sample(k, curpt, curptval, *this);
  }
 
};




namespace tomo_internal {
  // see http://stackoverflow.com/a/6894436/1694896 : iterating over a tuple

  /*
  template<typename... Tp>
  struct TupleIterator
  {
    template<std::size_t I = 0, typename FuncT, typename... FnArgs>
    static inline typename std::enable_if<I == sizeof...(Tp), void>::type
    for_each(std::tuple<Tp&...> & , FuncT , const FnArgs & ... )
    {
    }

    template<std::size_t I = 0, typename FuncT, typename... FnArgs>
    static inline typename std::enable_if<I < sizeof...(Tp), void>::type
    for_each(std::tuple<Tp&...>& tup, FuncT f, const FnArgs & ... args)
    {
      f(std::get<I>(tup), args...);
      for_each<I + 1, FuncT, Tp...>(t, f);
    }
  };
  */
  
  /*
  template<std::size_t I = 0, typename FuncT, typename... Tp>
  inline typename std::enable_if<I == sizeof...(Tp), void>::type
  for_each(std::tuple<Tp...> & , FuncT ) // Unused arguments are given no names.
  {
  }

  template<std::size_t I = 0, typename FuncT, typename... Tp>
  inline typename std::enable_if<I < sizeof...(Tp), void>::type
  for_each(std::tuple<Tp...>& t, FuncT f)
  {
    f(std::get<I>(t));
    for_each<I + 1, FuncT, Tp...>(t, f);
  }
  */
};




template<typename... RWStatsCollectors>
class MultipleRWStatsCollectors
{
  std::tuple<RWStatsCollectors&...>  statscollectors;

  enum {
    NumStatColl = sizeof...(RWStatsCollectors)
  };

public:
  MultipleRWStatsCollectors(RWStatsCollectors&... statscollectors_)
    : statscollectors(statscollectors_...)
  {
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

  template<typename PointType, typename FnValueType, typename MHRandomWalk, int I = 0>
  inline typename std::enable_if<I < NumStatColl, void>::type raw_move(
      size_t k, bool is_thermalizing, bool is_live_iter, bool accepted,
      double a, const PointType & newpt, FnValueType newptval,
      const PointType & curpt, FnValueType curptval,
      MHRandomWalk & rw
      )
  {
    std::get<I>(statscollectors).raw_move(k, is_thermalizing, is_live_iter, accepted, a,
                                          newpt, newptval, curpt, curptval, rw);
    raw_move<PointType, FnValueType, MHRandomWalk, I+1>(k, is_thermalizing, is_live_iter, accepted, a,
                                                        newpt, newptval, curpt, curptval, rw);
  }
  template<typename PointType, typename FnValueType, typename MHRandomWalk, int I = 0>
  inline typename std::enable_if<I == NumStatColl, void>::type raw_move(
      size_t, bool, bool, bool, double, const PointType &, FnValueType,
      const PointType &, FnValueType, MHRandomWalk &
      )
  {
  }


  // process_sample() callback

  template<typename PointType, typename FnValueType, typename MHRandomWalk, int I = 0>
  inline typename std::enable_if<I < NumStatColl, void>::type process_sample(
      size_t k, const PointType & curpt, FnValueType curptval, MHRandomWalk & rw
      )
  {
    std::get<I>(statscollectors).process_sample(k, curpt, curptval, rw);
    process_sample<PointType, FnValueType, MHRandomWalk, I+1>(k, curpt, curptval, rw);
  }

  template<typename PointType, typename FnValueType, typename MHRandomWalk, int I = 0>
  inline typename std::enable_if<I == NumStatColl, void>::type process_sample(
      size_t, const PointType &, FnValueType, MHRandomWalk &
      )
  {
  }

};













#endif
