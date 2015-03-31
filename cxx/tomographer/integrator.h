
#ifndef INTEGRATOR_H
#define INTEGRATOR_H

#include <limits>
#include <random>

#include <tomographer/matrq.h>
#include <tomographer/tomoproblem.h>
#include <tomographer/param_herm_x.h>
#include <tomographer/loggers.h>


enum {
  MHUseFnValue = 1,    // use MHWalker::fnval(newpt)
  MHUseFnLogValue,     // use MHWalker::fnlogval(newpt)
  MHUseFnRelativeValue // use MHWalker::fnrelval(newpt, curpt)
};

template<typename Rng, typename MHWalker, typename StatsCollector, typename Logger>
class MHRandomWalk
{
public:
  typedef typename MHWalker::PointType PointType;
  typedef typename MHWalker::RealScalar RealScalar;

  // yes, sorry: MHWalker must define FnValueType even if UseFnSyntaxType ==
  // MHUseFnRelative. It may be a dummy type (e.g. int) though in that case.
  typedef typename MHWalker::FnValueType FnValueType;
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
  StatsCollector & _stats;
  Logger & _log;

  PointType curpt;
  FnValueType curptval;

  size_t num_accepted;
  size_t num_live_points;

public:

  MHRandomWalk(size_t n_sweep, size_t n_therm, size_t n_run, RealScalar step_size,
               const PointType & startpt, MHWalker & mhwalker, StatsCollector & stats,
               Rng & rng, Logger & log_)
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
    switch (UseFnSyntaxType) {
    case MHUseFnLogValue:
      curptval = _mhwalker.fnlogval(curpt);
      break;
    case MHUseFnValue:
      curptval = _mhwalker.fnval(curpt);
      break;
    case MHUseFnRelativeValue:
    default:
      curptval = 0;
      break;
    }
    _stats.init();
  }
  inline void thermalizing_done()
  {
    _stats.thermalizing_done();
  }
  inline void done()
  {
    _stats.done();
  }

  inline void move(size_t k, bool is_thermalizing, bool is_live_iter)
  {
    // The reason `step_size` is passed to jump_fn instead of leaving jump_fn itself
    // handle the step size, is that we might in the future want to dynamically adapt the
    // step size according to the acceptance ratio. That would have to be done in this
    // class.
    PointType newpt = _mhwalker.jump_fn(curpt, _step_size);

    FnValueType newptval;
    double a = 1.0;
    switch (UseFnSyntaxType) {
    case MHUseFnLogValue:
      newptval = _mhwalker.fnlogval(newpt);
      a = (newptval > curptval) ? 1.0 : exp(newptval - curptval);
      break;
    case MHUseFnValue:
      newptval = _mhwalker.fnval(newpt);
      a = newptval / curptval;
      break;
    case MHUseFnRelativeValue:
      newptval = 0.0;
      a = _mhwalker.fnrelval(newpt, curpt);
      break;
    default:
      assert(0 && "Unknown UseFnSyntaxType!");
    }

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
                   "%s%3lu: %s a=%-7.2g, newptval=%5.4g [llh=%.4g], curptval=%5.4g [llh=%.4g]   accept_ratio=%.2g",
                   (is_thermalizing?"T":"#"),
                   k, accept?"AC":"RJ", a, newptval, -2*newptval, curptval, -2*curptval,
                   acceptance_ratio());

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

  inline void process(size_t k)
  {
  }
 
};







template<typename RandomWalk_>
struct MetropolisWalkerBase
{
  typedef RandomWalk_ RandomWalk;

  static void run(RandomWalk & rw)
  {
    // rw should provide the methods:
    //
    //   size_t rw.n_sweep()
    //   size_t rw.n_therm()
    //   size_t rw.n_run()
    //
    //   void rw.init()  -- called at the very beginning
    //   void rw.thermalizing_done() -- called after thermalizing run, before "live" run
    //   void rw.done()  -- called at the very end
    //
    //   void rw.move(size_t k, bool is_thermalizing, bool is_live_iter)
    //       -- chooses a random next point and decides whether to accept new point or
    //       not, using standard Metropolis move rules. This involves calculating some
    //       function for these points usually. Then, if we accept the move, update the
    //       internal state to the new point.
    //
    // rw should keep a current state (current point in the walk) in memory. The function
    // `move()` refers to this state.
    //
    // For collecting results, rw should provide the following methods:
    //
    //   void rw.process(size_t k)
    //       -- called for each "live" point (i.e. not thermalizing, and after full sweep)
    //
    //

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
        rw.process(k);
      }

    }

    rw.done();
    return;
  }
};
















#endif
