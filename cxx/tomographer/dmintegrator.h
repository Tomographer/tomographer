
#ifndef DMINTEGRATOR_H
#define DMINTEGRATOR_H

#include <tomographer/integrator.h>


template<typename Rng, typename TomoProblem, typename Logger>
class DMStateSpaceRandomWalk
{
public:
  typedef typename TomoProblem::MatrQ MatrQ;
  typedef typename TomoProblem::LLHValueType LLHValueType;
  typedef typename MatrQ::MatrixType MatrixType;
  typedef typename MatrQ::VectorParamType VectorParamType;
  typedef typename MatrQ::RealScalar RealScalar;

  typedef MatrixType PointType;
  typedef LLHValueType FnValueType;
  enum {
    UseFnSyntaxType = MHUseFnLogValue
  };

private:

  TomoProblem & _tomo;
  Rng & _rng;
  std::normal_distribution<RealScalar> _normal_distr_rnd;

  Logger & _log;
  
  typedef MHRandomWalk<Rng,DMStateSpaceRandomWalk<Rng,TomoProblem,Logger>,DMStateSpaceRandomWalk<Rng,TomoProblem,Logger>,Logger> OurMHRandomWalk;

  OurMHRandomWalk _mhrw;

public:

  DMStateSpaceRandomWalk(size_t n_sweep, size_t n_therm, size_t n_run, RealScalar step_size,
                         const MatrixType & startpt, TomoProblem & tomo, Rng & rng, Logger & log_)
    : _tomo(tomo),
      _rng(rng),
      _normal_distr_rnd(0.0, 1.0),
      _log(log_),
      _mhrw(n_sweep, n_therm, n_run, step_size, startpt, *this, *this, _rng, log_)
  {
  }

  inline LLHValueType fnlogval(const MatrixType & T)
  {
    MatrixType rho = _tomo.matq.initMatrixType();
    rho.noalias() = T*T.adjoint();

    VectorParamType x = _tomo.matq.initVectorParamType();
    param_herm_to_x(x, rho);

    LLHValueType llhval = -0.5 * _tomo.calc_llh(x);
    // _log.longdebug("fnlogval(%s) = %g\n", streamcstr(x.transpose()), llhval);
    return llhval;
  }

  inline LLHValueType fnval(const MatrixType &) { return -999; }
  inline LLHValueType fnrelval(const MatrixType &, const MatrixType &) { return -999; }


  inline MatrixType jump_fn(const MatrixType& cur_T, RealScalar step_size)
  {
    MatrixType DeltaT = _tomo.matq.initMatrixType();
    // iterate over rows first, as Eigen stores in column-major order by default
    for (size_t j = 0; j < _tomo.matq.dim(); ++j) {
      for (size_t k = 0; k < _tomo.matq.dim(); ++k) {
        DeltaT(k,j) = typename MatrQ::ComplexScalar(_normal_distr_rnd(_rng), _normal_distr_rnd(_rng));
      }
    }

    MatrixType new_T = _tomo.matq.initMatrixType();

    new_T.noalias() = cur_T + step_size * DeltaT;

    // renormalize to "project" onto the large T-space sphere
    new_T /= new_T.norm(); //Matrix<>.norm() is Frobenius norm.

    //    _log.longdebug("jump_fn(): step_size=%g, cur_T =\n%s\nDeltaT = \n%s\nnew_T = \n%s",
    //                   step_size, streamstr(cur_T).c_str(), streamstr(DeltaT).c_str(),
    //                   streamstr(new_T).c_str());

    // hope for Return Value Optimization by compiler
    return new_T;
  }


  void run()
  {
    MetropolisWalkerBase<OurMHRandomWalk>::run(_mhrw);
  }


  // stats collector part

  inline void init()
  {
  }
  inline void thermalizing_done()
  {
  }
  inline void done()
  {
  }

  void raw_move(size_t k, bool is_thermalizing, bool is_live_iter, bool accept,
                double a, const MatrixType& newpt, LLHValueType newptval, const MatrixType& curpt,
                LLHValueType curptval, OurMHRandomWalk & mh)
  {
    _log.longdebug("DMStateSpaceRandomWalk", "raw_move(): k=%lu", k);
  }

 
};





#endif
