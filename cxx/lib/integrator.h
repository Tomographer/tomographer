
#ifndef INTEGRATOR_H
#define INTEGRATOR_H

#include <limits>
#include <random>

#include "matrq.h"
#include "tomoproblem.h"
#include "loggers.h"


template<typename Rng, typename TomoProblem, typename Logger>
class DMStateSpaceRandomWalk
{
public:
  typedef typename TomoProblem::MatrQ MatrQ;
  typedef typename TomoProblem::LLHValueType LLHValueType;
  typedef typename MatrQ::MatrixType MatrixType;
  typedef typename MatrQ::RealScalar RealScalar;

private:
  const size_t _n_sweep;
  const size_t _n_therm;
  const size_t _n_run;
  const double _step_size;

  TomoProblem & _tomo;
  Rng & _rng;
  std::normal_distribution<LLHValueType> _normal_distr_rnd;

  Logger & _log;

  MatrixType cur_T;
  LLHValueType curptlogval;

  size_t num_accepted;
  size_t num_live_points;

public:

  DMStateSpaceRandomWalk(size_t n_sweep, size_t n_therm, size_t n_run, RealScalar step_size,
                         const MatrixType & startpt, TomoProblem & tomo, Rnd & rng, Logger & log)
    : _n_sweep(n_sweep), _n_therm(n_therm), _n_run(n_run),
      _step_size(step_size),
      cur_T(startpt),
      _tomo(tomo),
      _rng(rng),
      _normal_distr_rnd(0.0, 1.0),
      _log(log)
  {
  }

  inline size_t n_sweep() const { return _n_sweep; }
  inline size_t n_therm() const { return _n_therm; }
  inline size_t n_run() const { return _n_run; }

  inline void init() const {
    log.longdebug("DMStateSpaceRandomWalk::init()");
    num_accepted = 0;
    num_live_points = 0;
    curptlogval = fnlogval(cur_T);
  }
  inline void thermalizing_done() const {
    log.longdebug("DMStateSpaceRandomWalk::thermalizing_done()");
  }
  inline void done() const {
    log.longdebug("DMStateSpaceRandomWalk::done()");
  }

  inline double fnlogval(const MatrixType & T)
  {
    MatrixType rho = _matq.initMatrixType();
    rho.noalias() = T*T.adjoint();

    VectorParamType x = _matq.initVectorParamType();
    param_herm_to_x(x, rho);

    return -0.5 * _tomo.calc_llh(x);
  }

  inline void move(size_t k, bool is_thermalizing, bool is_live_iter)
  {
    log.longdebug("DMStateSpaceRandomWalk::move(%d, %d, %d)", k, is_thermalizing, is_live_iter);

    MatrixType new_T = _matrq.initMatrixType();

    MatrixType DeltaT = _matrq.initMatrixType();

    // iterate over rows first, as Eigen stores in column-major order by default
    for (int j = 0; j < _matrq.dim(); ++j) {
      for (int k = 0; k < _matrq.dim(); ++k) {
        DeltaT(k,j) = MatrQ::ComplexType(_normal_distr_rnd(_rng), _normal_distr_rnd(_rng));
      }
    }

    //std::cout << "Delta:\n" << Delta <<"\n";
    new_T.noalias() = cur_T + step_size * DeltaT;

    //std::cout << "Tnew:\n" << Tnew <<"\n" << "Tnew.norm()=" << Tnew.norm()<<"\n";
    // renormalize to "project" onto the large T-space sphere
    new_T /= Tnew.norm(); //Matrix<>.norm() is Frobenius norm.


    LLHValueType newptlogval = fnlogval(newpt);

    // accept move?
    bool accept = false;
    if (newptlogval > curptlogval) {
      accept = true;
    } else {
      double a = exp(newptlogval - curptlogval);
      auto r = rng();
      if ( r-rng.min() <= a*(rng.max()-rng.min()) ) {
        accept = true;
      }
    }

    // track acceptance ratio
    num_accepted += accept ? 1 : 0;
    ++num_live_points;

    if (accept) {
      // update the internal state of the random walk
      cur_T = new_T;
      curptlogval = newptlogval;
    }
  }

  inline double acceptance_ratio() const
  {
    return (double) num_accepted / num_live_points;
  }

  inline void process(size_t k)
  {
    log.longdebug("DMStateSpaceRandomWalk::process(%d)", k);
  }

 
};





template<typename RandomWalk_>
class MetropolisWalkerBase
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

      bool is_live_iter = (k % Nsweep == 0);
      
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
