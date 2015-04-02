
#ifndef DMINTEGRATOR_H
#define DMINTEGRATOR_H

#include <tomographer/qit/util.h>
#include <tomographer/qit/dist.h>
#include <tomographer/integrator.h>
#include <tomographer/loggers.h>
#include <tomographer/histogram.h>


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

  const TomoProblem & _tomo;
  Rng & _rng;
  std::normal_distribution<RealScalar> _normal_distr_rnd;

  Log & _log;
  
  typedef MHRandomWalk<Rng,DMStateSpaceRandomWalk<Rng,TomoProblem,StatsCollector,Log>,StatsCollector,Log>
          OurMHRandomWalk;

  OurMHRandomWalk _mhrw;

public:

  DMStateSpaceRandomWalk(size_t n_sweep, size_t n_therm, size_t n_run, RealScalar step_size,
                         const MatrixType & startpt, const TomoProblem & tomo, Rng & rng, StatsCollector & stats,
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

      // DEBUG: printf("T = \n%s\n", streamcstr(T));

      _mhrw.set_curpt(T);
    }
    _log.debug("DMStateSpaceRandomWalk", "Starting random walk");
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
    RandomWalkBase<OurMHRandomWalk>::run(_mhrw);
  }

};




template<typename MatrQ, typename FidelityValueType>
struct FidelityHistogramStatsCollectorInfo
{
  typedef typename MatrQ::MatrixType MatrixType;
  typedef UniformBinsHistogram<FidelityValueType> HistogramType;
  typedef typename HistogramType::Params HistogramParams;
};

template<typename MatrQ_, typename FidelityValueType_, typename Log>
class FidelityHistogramStatsCollector
{
public:
  typedef MatrQ_ MatrQ;
  typedef FidelityValueType_ FidelityValueType;

  typedef typename FidelityHistogramStatsCollectorInfo<MatrQ, FidelityValueType>::MatrixType MatrixType;
  typedef typename FidelityHistogramStatsCollectorInfo<MatrQ, FidelityValueType>::HistogramType HistogramType;
  typedef typename FidelityHistogramStatsCollectorInfo<MatrQ, FidelityValueType>::HistogramParams HistogramParams;

private:

  HistogramType _histogram;

  MatrixType _ref_T;

  Log & _log;

public:
  FidelityHistogramStatsCollector(FidelityValueType fid_min, FidelityValueType fid_max, int num_bins,
                                  const MatrixType & ref_T, MatrQ /*mq*/,
                                  Log & logger)
    : _histogram(fid_min, fid_max, num_bins),
      _ref_T(ref_T),
      _log(logger)
  {
  }
  FidelityHistogramStatsCollector(HistogramParams histogram_params,
                                  const MatrixType & ref_T, MatrQ /*mq*/,
                                  Log & logger)
    : _histogram(histogram_params),
      _ref_T(ref_T),
      _log(logger)
  {
  }


  inline const HistogramType & histogram() const
  {
    return _histogram;
  }

  // stats collector part

  inline void init()
  {
    // reset our array
    _histogram.reset();
  }
  inline void thermalizing_done()
  {
  }
  inline void done()
  {
    if (_log.enabled_for(Logger::LONGDEBUG)) {
      // _log.longdebug("FidelityHistogramStatsCollector", "done()");
      _log.longdebug("FidelityHistogramStatsCollector",
                     "Done walking & collecting stats. Here's the histogram:\n"
                     + _histogram.pretty_print());
    }
  }

  template<typename LLHValueType, typename MHRandomWalk>
  void raw_move(size_t k, bool /*is_thermalizing*/, bool /*is_live_iter*/, bool /*accepted*/,
                double /*a*/, const MatrixType & /*newpt*/, LLHValueType /*newptval*/,
                const MatrixType & /*curpt*/, LLHValueType /*curptval*/, MHRandomWalk & /*mh*/)
  {
    _log.longdebug("FidelityHistogramStatsCollector", "raw_move(): k=%lu", k);
  }

  template<typename LLHValueType, typename MHRandomWalk>
  void process_sample(size_t k, const MatrixType & curpt, LLHValueType /*curptval*/, MHRandomWalk & /*mh*/)
  {
    FidelityValueType fid = fidelity_T(curpt, _ref_T);

    _log.longdebug("FidelityHistogramStatsCollector", "in process_sample(): k=%lu, fid=%.4g", k, fid);

    _histogram.record(fid);

    //_log.longdebug("FidelityHistogramStatsCollector", "process_sample() finished");
  }
 

};








namespace DMIntegratorTasks
{


  template<typename TomoProblem, typename FidelityValueType = double>
  struct CData
  {
    typedef typename FidelityHistogramStatsCollectorInfo<typename TomoProblem::MatrQ,
                                                         FidelityValueType>::HistogramParams
    /*..*/  HistogramParams;

    CData(const TomoProblem &prob_, int base_seed_ = 0,
             HistogramParams hparams = HistogramParams())
      : prob(prob_), base_seed(base_seed_), histogram_params(hparams)
    {
    }

    // the data:

    //! the Tomography data (POVM effects, frequencies, etc.)
    TomoProblem prob;

    //! Parameter of the random walk -- number of iterations per sweep
    size_t n_sweep;
    //! Parameter of the random walk -- number of thermalizing sweeps
    size_t n_therm;
    //! Parameter of the random walk -- number of "live" sweeps
    size_t n_run;
    //! Parameter of the random walk -- step size of the random walk
    typename TomoProblem::MatrQ::RealScalar step_size;

    //! A base random seed from which each run seed will be derived
    int base_seed;

    //! Which histogram to record (min, max, numbins)
    HistogramParams histogram_params;
  };

  /**
   *
   */
  template<typename TomoProblem, typename Logger, typename Rng = std::mt19937, typename FidelityValueType = double>
  struct MHRandomWalkTask
  {
    int _seed;
    Logger _log;

    typedef FidelityHistogramStatsCollector<typename TomoProblem::MatrQ, FidelityValueType, Logger>
    /*..*/ FidStatsCollector;
    FidStatsCollector fidstats;

    typedef CData<TomoProblem, FidelityValueType> OurCData;

    /** \brief Returns the random seed to seed the random number generator with.
     *
     * This simply returns \code pcdata->base_seed + k \endcode
     */
    static inline int get_input(int k, const OurCData * pcdata)
    {
      return pcdata->base_seed + k;
    }

    MHRandomWalkTask(int inputseed, const OurCData * pcdata, Logger & log)
      : _seed(inputseed),
        _log(log),
        fidstats(pcdata->histogram_params, pcdata->prob.T_MLE, pcdata->prob.matq, _log)
    {
    }

    inline void run(const OurCData * pcdata, Logger & /*log*/)
    {
      
      typedef DMStateSpaceRandomWalk<Rng,TomoProblem,FidStatsCollector,Logger>
        OurRandomWalk;
      
      Rng rng(_seed); // seeded random number generator
      
      OurRandomWalk rwalk(
          // MH random walk parameters
          pcdata->n_sweep,
          pcdata->n_therm,
          pcdata->n_run,
          pcdata->step_size,
          // starting point: zero matrix means random starting point
          pcdata->prob.matq.initMatrixType(),
          // the tomo problem data
          pcdata->prob,
          // rng, stats collector and logger
          rng,
          fidstats,
          _log);
      
      rwalk.run();
    }
  };

  template<typename HistogramType_>
  struct MHRandomWalkResultsCollector
  {
    typedef HistogramType_ HistogramType;
    typedef typename HistogramType::Params HistogramParams;

    HistogramType final_histogram;

    MHRandomWalkResultsCollector(HistogramParams p)
      : final_histogram(p)
    {
    }

    inline void init(size_t num_runs, size_t n_chunk, const void * pcdata) const
    {
    }

    inline void run_finished() const
    {
    }

    template<typename TomoProblem, typename Rng, typename FidelityValueType>
    inline void collect_results(const MHRandomWalkTask<TomoProblem,Rng,FidelityValueType>& t)
    {
      final_histogram.bins += t.fidstats.histogram().bins;
      final_histogram.off_chart += t.fidstats.histogram().off_chart;
    }
  };


}




#endif
