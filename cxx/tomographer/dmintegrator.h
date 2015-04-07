
#ifndef DMINTEGRATOR_H
#define DMINTEGRATOR_H

#include <cstddef>

#include <tomographer/qit/util.h>
#include <tomographer/qit/dist.h>
#include <tomographer/integrator.h>
#include <tomographer/loggers.h>
#include <tomographer/histogram.h>

namespace Tomographer {


template<typename Rng, typename TomoProblem, typename RWStatsCollector, typename Log>
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
  
  typedef MHRandomWalk<Rng,DMStateSpaceRandomWalk<Rng,TomoProblem,RWStatsCollector,Log>,RWStatsCollector,Log>
          OurMHRandomWalk;

  OurMHRandomWalk _mhrw;

public:

  DMStateSpaceRandomWalk(unsigned int n_sweep, unsigned int n_therm, unsigned int n_run, RealScalar step_size,
                         const MatrixType & startpt, const TomoProblem & tomo, Rng & rng, RWStatsCollector & stats,
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
struct FidelityHistogramRWStatsCollectorInfo
{
  typedef typename MatrQ::MatrixType MatrixType;
  typedef UniformBinsHistogram<FidelityValueType> HistogramType;
  typedef typename HistogramType::Params HistogramParams;
};

template<typename MatrQ_, typename FidelityValueType_, typename Log>
class FidelityHistogramRWStatsCollector
{
public:
  typedef MatrQ_ MatrQ;
  typedef FidelityValueType_ FidelityValueType;

  typedef typename FidelityHistogramRWStatsCollectorInfo<MatrQ, FidelityValueType>::MatrixType MatrixType;
  typedef typename FidelityHistogramRWStatsCollectorInfo<MatrQ, FidelityValueType>::HistogramType HistogramType;
  typedef typename FidelityHistogramRWStatsCollectorInfo<MatrQ, FidelityValueType>::HistogramParams HistogramParams;

private:

  HistogramType _histogram;

  MatrixType _ref_T;

  Log & _log;

public:
  FidelityHistogramRWStatsCollector(FidelityValueType fid_min, FidelityValueType fid_max, int num_bins,
                                  const MatrixType & ref_T, MatrQ /*mq*/,
                                  Log & logger)
    : _histogram(fid_min, fid_max, num_bins),
      _ref_T(ref_T),
      _log(logger)
  {
  }
  FidelityHistogramRWStatsCollector(HistogramParams histogram_params,
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
      // _log.longdebug("FidelityHistogramRWStatsCollector", "done()");
      _log.longdebug("FidelityHistogramRWStatsCollector",
                     "Done walking & collecting stats. Here's the histogram:\n"
                     + _histogram.pretty_print());
    }
  }

  template<typename LLHValueType, typename MHRandomWalk>
  void raw_move(unsigned int k, bool /*is_thermalizing*/, bool /*is_live_iter*/, bool /*accepted*/,
                double /*a*/, const MatrixType & /*newpt*/, LLHValueType /*newptval*/,
                const MatrixType & /*curpt*/, LLHValueType /*curptval*/, MHRandomWalk & /*mh*/)
  {
    _log.longdebug("FidelityHistogramRWStatsCollector", "raw_move(): k=%lu", (unsigned long)k);
  }

  template<typename LLHValueType, typename MHRandomWalk>
  void process_sample(unsigned int k, const MatrixType & curpt, LLHValueType /*curptval*/, MHRandomWalk & /*mh*/)
  {
    FidelityValueType fid = fidelity_T(curpt, _ref_T);

    _log.longdebug("FidelityHistogramRWStatsCollector", "in process_sample(): k=%lu, fid=%.4g",
                   (unsigned long)k, fid);

    _histogram.record(fid);

    //_log.longdebug("FidelityHistogramRWStatsCollector", "process_sample() finished");
  }
 

};








namespace DMIntegratorTasks
{

  /** \brief Data needed to be accessible to the working code
   *
   * Stores the tomography data, as well as parameters to the random walk and ranges for
   * the fidelity histgram to take.
   */
  template<typename TomoProblem, typename FidelityValueType = double>
  struct CData
  {
    typedef typename FidelityHistogramRWStatsCollectorInfo<typename TomoProblem::MatrQ,
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
    unsigned int n_sweep;
    //! Parameter of the random walk -- number of thermalizing sweeps
    unsigned int n_therm;
    //! Parameter of the random walk -- number of "live" sweeps
    unsigned int n_run;
    //! Parameter of the random walk -- step size of the random walk
    typename TomoProblem::MatrQ::RealScalar step_size;

    //! A base random seed from which each run seed will be derived
    int base_seed;

    //! Which histogram to record (min, max, numbins)
    HistogramParams histogram_params;
  };

  /** \brief Random Walk on the space of density matrices, collecting fidelity histogram
   * statistics
   *
   * This class can be used with \ref MultiProc::run_omp_tasks(), for example.
   */
  template<typename TomoProblem, typename Logger, typename Rng = std::mt19937, typename FidelityValueType = double>
  struct MHRandomWalkTask
  {
    int _seed;
    Logger _log;

    typedef FidelityHistogramRWStatsCollector<typename TomoProblem::MatrQ, FidelityValueType, Logger>
    /*..*/ FidRWStatsCollector;
    FidRWStatsCollector fidstats;

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
      
      typedef DMStateSpaceRandomWalk<Rng,TomoProblem,FidRWStatsCollector,Logger>
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

  /** \brief Collect results from MHRandomWalkTask's
   *
   * This is meant to be used in a task dispatcher environment, e.g. 
   * \ref MultiProc::run_omp_tasks().
   */
  template<typename HistogramType_>
  struct MHRandomWalkResultsCollector
  {
    typedef HistogramType_ HistogramType;
    typedef typename HistogramType::Params HistogramParams;

    const HistogramParams params;
    Eigen::ArrayXd final_histogram;
    Eigen::ArrayXd std_dev;
    double off_chart;
    unsigned int num_histograms;

    MHRandomWalkResultsCollector(HistogramParams p)
      : params(p)
    {
    }

    inline void init(unsigned int /* num_runs */, unsigned int /* n_chunk */, const void * /* pcdata */)
    {
      final_histogram = Eigen::ArrayXd::Zero(params.num_bins);
      std_dev = Eigen::ArrayXd::Zero(params.num_bins);
      num_histograms = 0;
      off_chart = 0;
    }

    inline void run_finished()
    {
      final_histogram /= num_histograms;
      std_dev /= num_histograms;
      off_chart /= num_histograms;

      // std_dev = sqrt(< X^2 > - < X >^2) / sqrt(Nrepeats)
      auto finhist2 = final_histogram*final_histogram; // for array, this is c-wise product
      std_dev = ( (std_dev - finhist2) / num_histograms ).sqrt();
    }

    template<typename TomoProblem, typename Rng, typename FidelityValueType>
    inline void collect_results(const MHRandomWalkTask<TomoProblem,Rng,FidelityValueType>& t)
    {
      // final_histogram collects the sum of the histograms
      // std_dev for now collects the sum of squares. std_dev will be normalized in run_finished().
      auto newbins = t.fidstats.histogram().bins.template cast<double>();
      final_histogram += newbins;
      std_dev += newbins * newbins ; // for arrays, this is c-wise product : square of each value
      off_chart += t.fidstats.histogram().off_chart;
      ++num_histograms;
    }

    inline std::string pretty_print(const unsigned int max_width = 100) const
    {
      std::string s;
      assert(final_histogram.size() >= 0);
      std::size_t Ntot = (std::size_t)final_histogram.size();
      // max_width - formatting widths (see below) - some leeway
      const unsigned int max_bar_width = max_width - (6+3+4+5+4+5) - 5;
      double barscale = (1.0+final_histogram.maxCoeff()) / max_bar_width; // full bar is max_bar_width chars wide
      assert(barscale > 0);
      auto val_to_bar_len = [max_bar_width,barscale](double val) -> unsigned int {
        if (val < 0) {
          val = 0;
        }
        unsigned int l = (unsigned int)(val/barscale+0.5);
        if (l >= max_bar_width) {
          return max_bar_width-1;
        }
        return l;
      };
      auto fill_str_len = [val_to_bar_len](std::string & s, double valstart, double valend, char c, char cside) {
        unsigned int vs = val_to_bar_len(valstart);
        unsigned int ve = val_to_bar_len(valend);
        assert(vs < s.size() && ve < s.size());
        for (unsigned int j = vs+1; j < ve; ++j) {
          s[j] = c;
        }
        s[vs] = cside;
        s[ve] = cside;
      };

      for (std::size_t k = 0; k < Ntot; ++k) {
        assert(final_histogram(k) >= 0);
        std::string sline(max_bar_width, ' ');
        fill_str_len(sline, 0.0, final_histogram(k) - std_dev(k), '*', '*');
        fill_str_len(sline, final_histogram(k) - std_dev(k), final_histogram(k) + std_dev(k), '-', '|');

        s += fmts("%-6.4g | %s    %5.1f +- %5.1f\n",
                  params.min + k*(params.max-params.min)/Ntot,
                  sline.c_str(),
                  final_histogram(k), std_dev(k)
                  );
      }
      if (off_chart > 1e-6) {
        s += fmts("   ... with another (average) %.4g points off chart.\n", (double)off_chart);
      }
      return s;
    }
  };


}


} // namespace Tomographer


#endif
