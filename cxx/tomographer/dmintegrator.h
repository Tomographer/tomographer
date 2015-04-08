
#ifndef DMINTEGRATOR_H
#define DMINTEGRATOR_H

#include <cstddef>

#include <tomographer/qit/util.h>
#include <tomographer/qit/dist.h>
#include <tomographer/integrator.h>
#include <tomographer/loggers.h>
#include <tomographer/histogram.h>
#include <tomographer/multiproc.h>


namespace Tomographer {


template<typename TomoProblem, typename Rng, typename RWStatsCollector, typename Log,
         typename CountIntType = unsigned int>
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
  
  typedef MHRandomWalk<Rng,DMStateSpaceRandomWalk<TomoProblem,Rng,RWStatsCollector,Log>,RWStatsCollector,
                       Log,CountIntType>
          OurMHRandomWalk;

  OurMHRandomWalk _mhrw;

public:

  DMStateSpaceRandomWalk(CountIntType n_sweep, CountIntType n_therm, CountIntType n_run, RealScalar step_size,
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


template<typename TomoProblem, typename Rng, typename RWStatsCollector, typename Log,
         typename CountIntType = unsigned int>
DMStateSpaceRandomWalk<TomoProblem,Rng,RWStatsCollector,Log,CountIntType>
makeDMStateSpaceRandomWalk(CountIntType n_sweep, CountIntType n_therm, CountIntType n_run,
                           typename TomoProblem::MatrQ::RealScalar step_size,
                           const typename TomoProblem::MatrQ::MatrixType & startpt,
                           const TomoProblem & tomo, Rng & rng, RWStatsCollector & stats,
                           Log & log_)
{
  return DMStateSpaceRandomWalk<TomoProblem,Rng,RWStatsCollector,Log,CountIntType>(
      n_sweep, n_therm, n_run, step_size, startpt, tomo, rng, stats, log_
      );
}





/** \brief Stores information about how to acquire a fidelity histogram (e.g. during a
 * random walk).
 *
 * See \ref FidelityHistogramMHRWStatsCollector
 */
template<typename MatrQ, typename FidelityValueType>
struct FidelityHistogramMHRWStatsCollectorTraits
{
  typedef typename MatrQ::MatrixType MatrixType;
  typedef UniformBinsHistogram<FidelityValueType> HistogramType;
  typedef typename HistogramType::Params HistogramParams;
};

/** \brief A StatsCollector which builds a histogram of fidelities to a reference point
 *
 * This stats collector is suitable for tracking statistics during a \ref MHRandomWalk.
 */
template<typename MatrQ_, typename FidelityValueType_, typename Log>
class FidelityHistogramMHRWStatsCollector
{
public:
  typedef MatrQ_ MatrQ;
  typedef FidelityValueType_ FidelityValueType;

  typedef typename FidelityHistogramMHRWStatsCollectorTraits<MatrQ, FidelityValueType>::MatrixType MatrixType;
  typedef typename FidelityHistogramMHRWStatsCollectorTraits<MatrQ, FidelityValueType>::HistogramType HistogramType;
  typedef typename FidelityHistogramMHRWStatsCollectorTraits<MatrQ, FidelityValueType>::HistogramParams HistogramParams;

private:

  HistogramType _histogram;

  MatrixType _ref_T;

  Log & _log;

public:
  FidelityHistogramMHRWStatsCollector(FidelityValueType fid_min, FidelityValueType fid_max, int num_bins,
                                  const MatrixType & ref_T, MatrQ /*mq*/,
                                  Log & logger)
    : _histogram(fid_min, fid_max, num_bins),
      _ref_T(ref_T),
      _log(logger)
  {
  }
  FidelityHistogramMHRWStatsCollector(HistogramParams histogram_params,
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
      // _log.longdebug("FidelityHistogramMHRWStatsCollector", "done()");
      _log.longdebug("FidelityHistogramMHRWStatsCollector",
                     "Done walking & collecting stats. Here's the histogram:\n"
                     + _histogram.pretty_print());
    }
  }

  template<typename LLHValueType, typename MHRandomWalk>
  void raw_move(unsigned int k, bool /*is_thermalizing*/, bool /*is_live_iter*/, bool /*accepted*/,
                double /*a*/, const MatrixType & /*newpt*/, LLHValueType /*newptval*/,
                const MatrixType & /*curpt*/, LLHValueType /*curptval*/, MHRandomWalk & /*mh*/)
  {
    _log.longdebug("FidelityHistogramMHRWStatsCollector", "raw_move(): k=%lu", (unsigned long)k);
  }

  template<typename LLHValueType, typename MHRandomWalk>
  void process_sample(unsigned int k, const MatrixType & curpt, LLHValueType /*curptval*/, MHRandomWalk & /*mh*/)
  {
    FidelityValueType fid = fidelity_T(curpt, _ref_T);

    _log.longdebug("FidelityHistogramMHRWStatsCollector", "in process_sample(): k=%lu, fid=%.4g",
                   (unsigned long)k, fid);

    _histogram.record(fid);

    //_log.longdebug("FidelityHistogramMHRWStatsCollector", "process_sample() finished");
  }
 

};








/** \brief Definitions for running multiple random walks and collecting fidelity statistics
 *
 * Provides class definitions for interfacing with a task manager/dispatcher (see \ref
 * pageTaskManagerDispatcher).
 *
 */
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
    typedef typename FidelityHistogramMHRWStatsCollectorTraits<typename TomoProblem::MatrQ,
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
   * This class can be used with \ref MultiProc::OMPTaskDispatcher, for example.
   */
  template<typename TomoProblem, typename Logger, typename Rng = std::mt19937,
           typename FidelityValueType = double, typename CountIntType = unsigned int>
  struct MHRandomWalkTask
  {
    /** \brief Status Report for a \ref MHRandomWalkTask
     *
     * This struct can store information about the current status of a \ref
     * MHRandomWalkTask while it is running.
     *
     * This is for use with, for example, \ref OMPTaskDispatcher::request_status_report().
     */
    struct StatusReport : MultiProc::StatusReport {
      /** \brief Constructor which initializes all fields */
      StatusReport(double fdone, const std::string & msg, CountIntType kstep_, CountIntType n_sweep_,
                   CountIntType n_therm_, CountIntType n_run_, double acceptance_ratio_)
        : MultiProc::StatusReport(fdone, msg),
          kstep(kstep_),
          n_sweep(n_sweep_),
          n_therm(n_therm_),
          n_run(n_run_),
          acceptance_ratio(acceptance_ratio_),
          n_total_iters(n_sweep*(n_therm+n_run))
      {
      }
      /** \brief the current iteration number */
      const CountIntType kstep;
      /** \brief the number of iterations that form a sweep (see \ref MHRandomWalk) */
      const CountIntType n_sweep;
      /** \brief the number of thermalization sweeps (see \ref MHRandomWalk) */
      const CountIntType n_therm;
      /** \brief the number of live run sweeps (see \ref MHRandomWalk) */
      const CountIntType n_run;
      /** \brief the current acceptance ratio of the random walk (see \ref
       *     Tomographer::MHRandomWalk::acceptance_ratio()) */
      const double acceptance_ratio;
      /** \brief the total number of iterations required for this random walk
       *
       * This is calculated as \f$
       *  \textit{nTotalIters} = \textit{nSweep} \times \left( \textit{nTherm} + \textit{nRun} \right)
       * \f$.
       */
      const CountIntType n_total_iters;
    };
    /** \brief Typedef for \ref StatusReport
     *
     * This is needed by, e.g. \ref OMPTaskDispatcher.
     */
    typedef StatusReport StatusReportType;

    /** \brief convenience typedef for the \ref DMIntegratorTasks::CData which we will use */
    typedef CData<TomoProblem, FidelityValueType> CDataType;

    /** \brief convenience typedef for our \ref FidelityHistogramMHRWStatsCollector which we use here */
    typedef FidelityHistogramMHRWStatsCollector<typename TomoProblem::MatrQ, FidelityValueType, Logger>
    /*..*/ FidelityHistogramMHRWStatsCollectorType;

  private:
    /** \brief how to seed the random number generator for this particular task (input) */
    typename Rng::result_type _seed;

    /** \brief a logger to which we can log messages */
    Logger _log;

    /** \brief our \ref FidelityHistogramMHRWStatsCollector instance */
    FidelityHistogramMHRWStatsCollectorType fidstats;

  public:

    /** \brief Returns a random seed to seed the random number generator with for run #k
     *
     * This simply returns \code pcdata->base_seed + k \endcode
     *
     * This should be considered as the input of the k-th task. Each task must of course
     * have a different seed, otherwise we will just repeat the same "random" walks!
     */
    static inline int get_input(int k, const CDataType * pcdata)
    {
      return pcdata->base_seed + k;
    }

    /** \brief Constructs the MHRandomWalkTask
     *
     * You should never need to call this directly, except if you're writing a task
     * manager/dispatcher (e.g. \ref OMPTaskDispatcher)
     */
    MHRandomWalkTask(int inputseed, const CDataType * pcdata, Logger & log)
      : _seed(inputseed),
        _log(log),
        fidstats(pcdata->histogram_params, pcdata->prob.T_MLE, pcdata->prob.matq, _log)
    {
    }

    /** \brief Run this task
     *
     * Runs this task, i.e. instantiates a \ref DMStateSpaceRandomWalk with the provided
     * inputs and data, and runs it.
     *
     * This also takes care to call the task manager interface's
     * <code>status_report_requested()</code> and submits a status report if required. See
     * e.g. \ref OMPTaskDispatcher::request_status_report().
     */
    template<typename TaskManagerIface>
    inline void run(const CDataType * pcdata, Logger & /*log*/, TaskManagerIface * tmgriface)
    {
      Rng rng(_seed); // seeded random number generator

      typedef StatusReportCheck<TaskManagerIface> OurStatusReportCheck;

      OurStatusReportCheck statreportcheck(this, tmgriface);
      
      MultipleMHRWStatsCollectors<FidelityHistogramMHRWStatsCollectorType, OurStatusReportCheck>
        statscollectors(fidstats, statreportcheck);

      auto rwalk = makeDMStateSpaceRandomWalk(
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
          statscollectors,
          _log);
      
      rwalk.run();
    }


    inline const FidelityHistogramMHRWStatsCollectorType & get_fid_stats() const
    {
      return fidstats;
    }

  private:
    /** \internal
     * \brief helper to provide status report
     *
     * This is in fact a StatsCollector.
     */
    template<typename TaskManagerIface>
    struct StatusReportCheck
    {
      StatusReportCheck(MHRandomWalkTask * mhrwtask_, TaskManagerIface *tmgriface_)
        : mhrwtask(mhrwtask_), tmgriface(tmgriface_)
      { }

      MHRandomWalkTask *mhrwtask;
      TaskManagerIface *tmgriface;

      inline void init() { }
      inline void thermalizing_done() { }
      inline void done() { }

      template<typename PointType, typename FnValueType, typename MHRandomWalk>
      inline void raw_move(
          CountIntType k, bool is_thermalizing, bool, bool, double, const PointType &, FnValueType,
          const PointType &, FnValueType, MHRandomWalk & rw
          )
      {
        // see if we should provide a status report
        //        fprintf(stderr, "StatusReportCheck::raw_move(): testing for status report requested!\n");
        if (tmgriface->status_report_requested()) {
          // prepare & provide status report
          CountIntType totiters = rw.n_sweep()*(rw.n_therm()+rw.n_run());
          double fdone = (double)k/totiters;
          std::string msg;
          msg = fmts(
              "iteration %s %lu/(%lu=%lu*(%lu+%lu)) : %5.2f%% done  [accept ratio=%.2f]",
              (is_thermalizing ? "[T]" : "   "),
              (unsigned long)k, (unsigned long)totiters, (unsigned long)rw.n_sweep(),
              (unsigned long)rw.n_therm(), (unsigned long)rw.n_run(),
              fdone*100.0,
              (rw.has_acceptance_ratio() ? rw.acceptance_ratio() : std::nan(""))
              );
          tmgriface->submit_status_report(StatusReport(fdone, msg, k, rw.n_sweep(), rw.n_therm(),
                                                       rw.n_run(), rw.acceptance_ratio()));
        }
        //        fprintf(stderr, "StatusReportCheck::raw_move(): done\n");
      }

      template<typename PointType, typename FnValueType, typename MHRandomWalk>
      inline void process_sample(CountIntType, const PointType &, FnValueType, MHRandomWalk &)
      {
      }
    };
  };

  /** \brief Collect results from MHRandomWalkTask's
   *
   * This is meant to be used in a task dispatcher environment, e.g. 
   * \ref MultiProc::OMPTaskDispatcher.
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
      auto& histogram = t.get_fid_stats().histogram();
      auto newbins = histogram.bins.template cast<double>();
      final_histogram += newbins;
      std_dev += newbins * newbins ; // for arrays, this is c-wise product : square of each value
      off_chart += histogram.off_chart;
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
