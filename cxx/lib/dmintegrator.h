
#ifndef DMINTEGRATOR_H
#define DMINTEGRATOR_H

#include "integrator.h"


template<typename Rng, typename TomoProblem, typename Logger>
class DMStateSpaceRandomWalk
{
public:
  typedef typename TomoProblem::MatrQ MatrQ;
  typedef typename TomoProblem::LLHValueType LLHValueType;
  typedef typename MatrQ::MatrixType MatrixType;
  typedef typename MatrQ::VectorParamType VectorParamType;
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
                         const MatrixType & startpt, TomoProblem & tomo, Rng & rng, Logger & log_)
    : _n_sweep(n_sweep), _n_therm(n_therm), _n_run(n_run),
      _step_size(step_size),
      _tomo(tomo),
      _rng(rng),
      _normal_distr_rnd(0.0, 1.0),
      _log(log_),
      cur_T(startpt)
  {
  }

  inline size_t n_sweep() const { return _n_sweep; }
  inline size_t n_therm() const { return _n_therm; }
  inline size_t n_run() const { return _n_run; }

  inline void init() {
    _log.longdebug("DMStateSpaceRandomWalk::init()");
    num_accepted = 0;
    num_live_points = 0;
    curptlogval = fnlogval(cur_T);
  }
  inline void thermalizing_done() {
    _log.longdebug("DMStateSpaceRandomWalk::thermalizing_done()");
  }
  inline void done() {
    _log.longdebug("DMStateSpaceRandomWalk::done()");
  }

  inline double fnlogval(const MatrixType & T)
  {
    MatrixType rho = _tomo.matq.initMatrixType();
    rho.noalias() = T*T.adjoint();

    VectorParamType x = _tomo.matq.initVectorParamType();
    param_herm_to_x(x, rho);

    return -0.5 * _tomo.calc_llh(x);
  }

  inline void move(size_t k, bool is_thermalizing, bool is_live_iter)
  {
    _log.longdebug("DMStateSpaceRandomWalk::move(%lu, %d, %d)", k, is_thermalizing, is_live_iter);

    MatrixType new_T = _tomo.matq.initMatrixType();

    MatrixType DeltaT = _tomo.matq.initMatrixType();

    // iterate over rows first, as Eigen stores in column-major order by default
    for (size_t j = 0; j < _tomo.matq.dim(); ++j) {
      for (size_t k = 0; k < _tomo.matq.dim(); ++k) {
        DeltaT(k,j) = typename MatrQ::ComplexScalar(_normal_distr_rnd(_rng), _normal_distr_rnd(_rng));
      }
    }

    //std::cout << "Delta:\n" << Delta <<"\n";
    new_T.noalias() = cur_T + _step_size * DeltaT;

    //std::cout << "Tnew:\n" << Tnew <<"\n" << "Tnew.norm()=" << Tnew.norm()<<"\n";
    // renormalize to "project" onto the large T-space sphere
    new_T /= new_T.norm(); //Matrix<>.norm() is Frobenius norm.


    LLHValueType newptlogval = fnlogval(new_T);

    // accept move?
    bool accept = false;
    double a = 0;
    if (newptlogval > curptlogval) {
      accept = true;
    } else {
      a = exp(newptlogval - curptlogval);
      typename Rng::result_type r = _rng();
      if ( r-_rng.min() <= a*(_rng.max()-_rng.min()) ) {
        accept = true;
      }
    }

    // track acceptance ratio, except if we are thermalizing
    if (!is_thermalizing) {
      num_accepted += accept ? 1 : 0;
      ++num_live_points;
    }

    if (accept) {
      // update the internal state of the random walk
      cur_T = new_T;
      curptlogval = newptlogval;
    }

    _log.longdebug("  #%lu: accept=%d, a=%g, newptlogval=%g [llh=%g], accept_ratio=%g",
                   k, accept, a, newptlogval, -2*newptlogval, (double)num_accepted/num_live_points);
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





#endif
