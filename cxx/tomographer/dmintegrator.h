
#ifndef DMINTEGRATOR_H
#define DMINTEGRATOR_H

#include <tomographer/qit/util.h>
#include <tomographer/qit/dist.h>
#include <tomographer/integrator.h>
#include <tomographer/loggers.h>


template<typename Rng, typename TomoProblem, typename StatsCollector, typename Log>
class DMStateSpaceRandomWalk
{
public:
  typedef typename TomoProblem::MatrQ MatrQ;
  typedef typename TomoProblem::LLHValueType LLHValueType;
  typedef typename MatrQ::MatrixType MatrixType;
  typedef typename MatrQ::VectorParamType VectorParamType;
  typedef typename MatrQ::RealScalar RealScalar;
  typedef typename MatrQ::ComplexScalar ComplexScalar;

  typedef MatrixType PointType;
  typedef LLHValueType FnValueType;
  enum {
    UseFnSyntaxType = MHUseFnLogValue
  };

private:

  TomoProblem & _tomo;
  Rng & _rng;
  std::normal_distribution<RealScalar> _normal_distr_rnd;

  Log & _log;
  
  typedef MHRandomWalk<Rng,DMStateSpaceRandomWalk<Rng,TomoProblem,StatsCollector,Log>,StatsCollector,Log>
          OurMHRandomWalk;

  OurMHRandomWalk _mhrw;

public:

  DMStateSpaceRandomWalk(size_t n_sweep, size_t n_therm, size_t n_run, RealScalar step_size,
                         const MatrixType & startpt, TomoProblem & tomo, Rng & rng, StatsCollector & stats,
                         Log & log_)
    : _tomo(tomo),
      _rng(rng),
      _normal_distr_rnd(0.0, 1.0),
      _log(log_),
      _mhrw(n_sweep, n_therm, n_run, step_size, startpt, *this, stats, _rng, log_)
  {
  }

  inline void init()
  {
    if (_mhrw.get_curpt().norm() < 1e-3) {
      // zero matrix given: means to choose random starting point
      MatrixType T;// = _tomo.matq.initMatrixType();

      T = dense_random<MatrixType>(
          _rng, _normal_distr_rnd, _tomo.matq.dim(), _tomo.matq.dim()
          );
      T = T/T.norm(); // normalize to be on surface of the sphere

      printf("T = \n%s\n", streamcstr(T));

      _mhrw.set_curpt(T);
    }
  }
  inline void thermalizing_done()
  {
  }

  inline void done()
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

  //  inline LLHValueType fnval(const MatrixType &) { return -999; }
  //  inline LLHValueType fnrelval(const MatrixType &, const MatrixType &) { return -999; }


  inline MatrixType jump_fn(const MatrixType& cur_T, RealScalar step_size)
  {
    MatrixType DeltaT = dense_random<MatrixType>(
        _rng, _normal_distr_rnd, _tomo.matq.dim(), _tomo.matq.dim()
        );

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

};




template<typename MatrQ, typename Log>
class FidelityHistogramStatsCollector
{
public:
  typedef typename MatrQ::MatrixType MatrixType;

private:
  double _fid_min;
  double _fid_max;
  Eigen::ArrayXi _fid_bin_counts;

  MatrixType _ref_T;

  Log & _log;

public:
  FidelityHistogramStatsCollector(double fid_min, double fid_max, int num_bins,
                                  const MatrixType & ref_T, MatrQ /*mq*/,
                                  Log & logger)
    : _fid_min(fid_min), _fid_max(fid_max),
      _fid_bin_counts(num_bins), // resize our array.
      _ref_T(ref_T),
      _log(logger)
  {
  }


  // stats collector part

  inline void init()
  {
    // reset our array
    _fid_bin_counts.setZero();
  }
  inline void thermalizing_done()
  {
  }
  inline void done()
  {
    if (_log.enabled_for(Logger::LONGDEBUG)) {
      // _log.longdebug("FidelityHistogramStatsCollector", "done()");
      std::string hist;
      size_t Ntot = _fid_bin_counts.size();
      double barscale = (1+_fid_bin_counts.maxCoeff()) / 80.0; // full bar is 80 chars wide
      // _log.longdebug("FidelityHistogramStatsCollector", "done(): Ntot=%lu, barscale=%g", Ntot, barscale);
      for (size_t k = 0; k < Ntot; ++k) {
        hist += fmts("%-6.4g | %3d %s\n", _fid_min + k*(_fid_max-_fid_min)/Ntot,
                     _fid_bin_counts(k), std::string((int)(_fid_bin_counts(k)/barscale+0.5), '*').c_str());
      }
      _log.longdebug("FidelityHistogramStatsCollector",
                     "Done walking & collecting stats. Here's the histogram:\n"+hist);
    }
  }

  template<typename LLHValueType, typename MHRandomWalk>
  void raw_move(size_t k, bool is_thermalizing, bool is_live_iter, bool accept,
                double a, const MatrixType & newpt, LLHValueType newptval, const MatrixType & curpt,
                LLHValueType curptval, MHRandomWalk & mh)
  {
    _log.longdebug("FidelityHistogramStatsCollector", "raw_move(): k=%lu", k);
  }

  template<typename LLHValueType, typename MHRandomWalk>
  void process_sample(size_t k, const MatrixType & curpt, LLHValueType curptval, MHRandomWalk & mh)
  {
    double fid = fidelity_T(curpt, _ref_T);

    _log.longdebug("FidelityHistogramStatsCollector", "in process_sample(): k=%lu, fid=%.4g", k, fid);

    if (fid < _fid_min || fid >= _fid_max) {
      _log.longdebug("FidelityHistogramStatsCollector", "fidelity %.3g out of histogram range [%.3g, %.3g]",
                     fid, _fid_min, _fid_max);
      return;
    }

    ++_fid_bin_counts( (int)((fid-_fid_min) / (_fid_max-_fid_min) * _fid_bin_counts.size()) );

    //_log.longdebug("FidelityHistogramStatsCollector", "process_sample() finished");
  }
 

};



#endif
