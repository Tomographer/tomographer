
#ifndef INTEGRATOR_H
#define INTEGRATOR_H

#include <limits>
#include <random>

#include "matrq.h"
#include "tomoproblem.h"
#include "param_herm_x.h"
#include "loggers.h"



/*
template<typename Rng, typename MHWalker>
class MHRandomWalk
{
public:
  typedef typename MHWalker::PointType PointType;
  typedef typename MHWalker::RealScalar RealScalar;
  typedef typename MHWalker::FnValueType FnValuetype;
  enum {
    UseFnSyntaxType = MHWalker::UseFnSyntaxType
  };
  enum {
    UseFnValue = 1, // MHWalker::fnval(newpt)
    UseFnLogValue,  // MHWalker::fnlogval(newpt)
    UseFnRelativeValue // MHWalker::fnrelval(newpt, curpt)
  };

private:
  const size_t _n_sweep;
  const size_t _n_therm;
  const size_t _n_run;
  const RealScalar _step_size;

  Rng & _rng;
  MHWalker & _mhwalker;

  PointType curpt;
  FnValueType curfnval;

  size_t num_accepted;
  size_t num_live_points;

public:

  DMStateSpaceRandomWalk(size_t n_sweep, size_t n_therm, size_t n_run, RealScalar step_size,
                         const PointType & startpt, MHWalker & mhwalker, Rng & rng)
    : _n_sweep(n_sweep), _n_therm(n_therm), _n_run(n_run),
      _step_size(step_size),
      _mhwalker(mhwalker),
      _rng(rng),
      curpt(startpt)
  {
  }

  inline size_t n_sweep() const { return _n_sweep; }
  inline size_t n_therm() const { return _n_therm; }
  inline size_t n_run() const { return _n_run; }

  inline void init() {
    num_accepted = 0;
    num_live_points = 0;
    switch (UseFnSyntaxType) {
    case UseFnLogValue:
      curptval = _mhwalker.fnlogval(curpt);
      break;
    case UseFnValue:
      curptval = _mhwalker.fnval(curpt);
      break;
    case UseFnRelativeValue:
    default:
      curptval = 0;
      break;
    }
  }
  inline void thermalizing_done() {
  }
  inline void done() {
  }

  inline void move(size_t k, bool is_thermalizing, bool is_live_iter)
  {
    PointType newpt = _mhwalker.jumpFn(curpt, _step_size);

    FnValueType newptval;
    double a = 1.0;
    switch (UseFnSytaxType) {
    case UseFnLogValue:
      newptval = _mhwalker.fnlogval(curpt);
      a = (newptlogval > curptlogval) ? 1.0 : exp(newptlogval - curptlogval);
      break;
    case UseFnValue:
      newptval = _mhwalker.fnval(curpt);
      a = newptlogval / curptlogval;
      break;
    case UseFnRelativeValue:
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

    if (accept) {
      // update the internal state of the random walk
      curpt = newpt;
      curptlogval = newptlogval;
    }

    _log.longdebug("  #%lu: accept=%d, a=%g, newptval=%g, accept_ratio=%g",
                   k, accept, a, newptval, (double)num_accepted/num_live_points);
  }

  inline double acceptance_ratio() const
  {
    return (double) num_accepted / num_live_points;
  }

  inline void process(size_t k)
  {
    _log.longdebug("DMStateSpaceRandomWalk::process(%lu)", k);
  }

 
};

*/







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
