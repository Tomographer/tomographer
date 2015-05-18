
#ifndef DMLLHINTEGRATOR_H
#define DMLLHINTEGRATOR_H

#include <cstddef>
#include <cmath>

#include <string>
#include <random>
#include <limits>

#include <tomographer/qit/util.h>
#include <tomographer/qit/dist.h>
#include <tomographer/tools/fmt.h>
#include <tomographer/tools/loggers.h>
#include <tomographer/integrator.h>
#include <tomographer/histogram.h>
#include <tomographer/multiproc.h>


namespace Tomographer {


/** \brief A random walk in the density matrix space of a Hilbert state space of a quantum
 * system
 *
 * The random walk explores in a Haar-invariant way the density operators on a Hilbert
 * space of a given dimension.
 *
 * \tparam TomoProblem the tomography data, expected to be a priori a \ref
 *      IndepMeasTomoProblem
 *
 * \tparam Rng a \c std::random random number \a generator (such as \ref std::mt19937)
 *
 * \tparam MHRWStatsCollector a type implementing a StatsCollector interface (\ref
 *      pageInterfaceMHRWStatsCollector)
 */
template<typename TomoProblem, typename Rng, typename MHRWStatsCollector, typename Log,
         typename CountIntType = unsigned int>
class DMStateSpaceLLHRandomWalk
{
public:
  //! The data types of our problem
  typedef typename TomoProblem::MatrQ MatrQ;
  //! The loglikelihood function value type (see \ref IndepMeasTomoProblem)
  typedef typename TomoProblem::LLHValueType LLHValueType;
  //! The matrix type for a density operator on our quantum system
  typedef typename MatrQ::MatrixType MatrixType;
  //! Type of an X-parameterization of a density operator (see \ref param_x_to_herm())
  typedef typename MatrQ::VectorParamType VectorParamType;
  //! The real scalar corresponding to our data types. Usually a \c double.
  typedef typename MatrQ::RealScalar RealScalar;
  //! The complex real scalar corresponding to our data types. Usually a \c std::complex<double>.
  typedef typename MatrQ::ComplexScalar ComplexScalar;

  //! Provided for MHRandomWalk. A point in our random walk = a density matrix
  typedef MatrixType PointType;
  //! Provided for MHRandomWalk. The function value type is the loglikelihood value type
  typedef LLHValueType FnValueType;
  enum {
    /** \brief We will calculate the log-likelihood function, which is the logarithm of
     * the Metropolis-Hastings function we should be calculating
     */
    UseFnSyntaxType = MHUseFnLogValue
  };

private:

  const TomoProblem & _tomo;
  Rng & _rng;
  std::normal_distribution<RealScalar> _normal_distr_rnd;

  Log & _log;
  
  typedef MHRandomWalk<Rng,DMStateSpaceLLHRandomWalk<TomoProblem,Rng,MHRWStatsCollector,Log>,MHRWStatsCollector,
                       Log,CountIntType>
          OurMHRandomWalk;

  OurMHRandomWalk _mhrw;

public:

  /** \brief Constructor which just initializes the given fields
   *
   * If you provide a zero \a startpt here, then a random starting point will be chosen
   * using the \a rng random number generator to generate a random point on the sphere.
   */
  DMStateSpaceLLHRandomWalk(CountIntType n_sweep, CountIntType n_therm, CountIntType n_run,
			    RealScalar step_size, const MatrixType & startpt,
			    const TomoProblem & tomo, Rng & rng, MHRWStatsCollector & stats,
			    Log & log_)
    : _tomo(tomo),
      _rng(rng),
      _normal_distr_rnd(0.0, 1.0),
      _log(log_),
      _mhrw(n_sweep, n_therm, n_run, step_size, startpt, *this, stats, _rng, log_)
  {
  }

  /** \brief Provided for \ref MHRandomWalk. Initializes some fields and prepares for a
   *     random walk.
   *
   * \note If the \a startpt given to the constructor is zero (or has very small norm),
   * then a uniformly random starting point is chosen. The starting point is chosen
   * randomly according to the Haar-invariant measure on the state space. This corresponds
   * to choosing a uniform point on the sphere corresponding to the T-parameterization of
   * the density matrices.
   */
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
    _log.debug("DMStateSpaceLLHRandomWalk", "Starting random walk");
  }

  //! Callback for after thermalizing is done. No-op.
  inline void thermalizing_done()
  {
  }

  //! Callback for after random walk is finished. No-op.
  inline void done()
  {
  }

  /** \brief Calculate the logarithm of the Metropolis-Hastings function value.
   *
   * \return <code>-0.5 * (-2-log-likelihood)</code>, where the
   * <code>-2-log-likelihood</code> is computed using \ref
   * IndepMeasTomoProblem::calc_llh()
   */
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

  //! Decides of a new point to jump to for the random walk
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

  /** \brief Starts the random walk. Simply calls the static \ref RandomWalkBase::run()
   * method for our random walk.
   */
  void run()
  {
    RandomWalkBase<OurMHRandomWalk>::run(_mhrw);
  }

};


/** \brief Instantiate an object of type \ref DMStateSpaceLLHRandomWalk.
 *
 * This is useful to avoid having to spell out the full template parameters, and let the
 * function call deduce them.
 *
 * Example:
 * 
 * \code
 *   DMStateSpaceLLHRandomWalk<MyTomoProblem,MyRng,MyMHRWStatsCollector,MyLog,
 *                          MyCountIntType>  myDmStateSpaceRandomWalk(
 *          n_sweep, n_therm, n_run, step_size, startpt, tomo, rng, stats, log
 *       );
 *   // is equivalent to:
 *   auto myDmStateSpaceRandomWalk = makeDMStateSpaceLLHRandomWalk(
 *          n_sweep, n_therm, n_run, step_size, startpt, tomo, rng, stats, log
 *       );
 * \endcode
 */
template<typename TomoProblem, typename Rng, typename MHRWStatsCollector, typename Log,
         typename CountIntType = unsigned int>
DMStateSpaceLLHRandomWalk<TomoProblem,Rng,MHRWStatsCollector,Log,CountIntType>
makeDMStateSpaceLLHRandomWalk(CountIntType n_sweep, CountIntType n_therm, CountIntType n_run,
                           typename TomoProblem::MatrQ::RealScalar step_size,
                           const typename TomoProblem::MatrQ::MatrixType & startpt,
                           const TomoProblem & tomo, Rng & rng, MHRWStatsCollector & stats,
                           Log & log_)
{
  return DMStateSpaceLLHRandomWalk<TomoProblem,Rng,MHRWStatsCollector,Log,CountIntType>(
      n_sweep, n_therm, n_run, step_size, startpt, tomo, rng, stats, log_
      );
}



/** \brief Calculate the fidelity to a reference state for each sample
 *
 */
template<typename TomoProblem_, typename FidValueType_ = double>
class FidelityToRefCalculator
{
public:
  typedef TomoProblem_ TomoProblem;
  typedef typename TomoProblem::MatrQ MatrQ;
  typedef typename MatrQ::MatrixType MatrixType;

  //! For TomoValueCalculator interface : value type
  typedef FidValueType_ ValueType;

private:
  MatrixType _ref_T;

public:
  //! Constructor, take the reference state to be the MLE
  FidelityToRefCalculator(const TomoProblem & tomo)
    : _ref_T(tomo.matq.initMatrixType())
  {
    _ref_T = tomo.T_MLE;
  }
  //! Constructor, the reference state is T_ref (in \ref pageParamsT)
  FidelityToRefCalculator(const TomoProblem & tomo, const MatrixType & T_ref)
    : _ref_T(tomo.matq.initMatrixType())
  {
    _ref_T = T_ref;
  }

  inline ValueType getValueT(const MatrixType & T) const
  {
    return fidelity_T<ValueType>(T, _ref_T);
  }
};

/** \brief Calculate the trace distance to a reference state for each sample
 *
 */
template<typename TomoProblem_, typename TrDistValueType_ = double>
class TrDistToRefCalculator
{
public:
  typedef TomoProblem_ TomoProblem;
  typedef typename TomoProblem::MatrQ MatrQ;
  typedef typename MatrQ::MatrixType MatrixType;

  //! For TomoValueCalculator interface : value type
  typedef TrDistValueType_ ValueType;

private:
  MatrixType _ref_rho;

public:
  //! Constructor, take the reference state to be the MLE
  TrDistToRefCalculator(const TomoProblem & tomo)
    : _ref_rho(tomo.matq.initMatrixType())
  {
    _ref_rho = tomo.rho_MLE;
  }
  //! Constructor, the reference state is \a rho_ref
  TrDistToRefCalculator(const TomoProblem & tomo, const MatrixType& rho_ref)
    : _ref_rho(tomo.matq.initMatrixType())
  {
    _ref_rho = rho_ref;
  }

  inline ValueType getValueRho(const MatrixType & rho) const
  {
    return 0.5 * (rho - _ref_rho).jacobiSvd().singularValues().sum();
  }
};

/** \brief Calculate expectation value of an observable for each sample
 *
 */
template<typename TomoProblem_>
class ObservableValueCalculator
{
public:
  typedef TomoProblem_ TomoProblem;
  typedef typename TomoProblem::MatrQ MatrQ;
  typedef typename MatrQ::MatrixType MatrixType;
  typedef typename MatrQ::VectorParamType VectorParamType;

  //! For TomoValueCalculator interface : value type
  typedef typename MatrQ::RealScalar ValueType;

private:
  const TomoProblem & _tomo;

  //! The observable we wish to watch the exp value with (in \ref pageParamsX)
  VectorParamType _A_x;

  //  MatrixType _A; // not needed

public:
  //! Constructor directly accepting \a A as a hermitian matrix
  ObservableValueCalculator(const TomoProblem & tomo, const MatrixType & A)
    : _tomo(tomo),
      _A_x(tomo.matq.initVectorParamType())
  {
    param_herm_to_x(_A_x, A);
  }

  //! Constructor directly accepting the X parameterization of \a A
  ObservableValueCalculator(const TomoProblem & tomo, const VectorParamType & A_x)
    : _tomo(tomo),
      _A_x(tomo.matq.initVectorParamType())
  {
    _A_x = A_x;
  }

  inline ValueType getValueRho(const MatrixType & rho) const
  {
    VectorParamType x = _tomo.matq.initVectorParamType();
    param_herm_to_x(x, rho);
    return _A_x.transpose() * x;
  }
};



namespace tomo_internal
{

template<typename TomoValueCalculator, typename MatrixType>
inline auto callTomoValueCalculator(TomoValueCalculator & vcalc, const MatrixType & T)
  -> decltype(vcalc.getValueT(T)) // SFINAE: enables this prototype only if getValueT() exists.
{
  return vcalc.getValueT(T);
}

template<typename TomoValueCalculator, typename MatrixType>
inline auto callTomoValueCalculator(TomoValueCalculator & vcalc, const MatrixType & T)
  -> decltype(vcalc.getValueRho(T)) // SFINAE: enables this prototype only if vcalc.getValueRho() exists.
{
  return vcalc.getValueRho(T*T.adjoint());
}

} // namespace tomo_internal

/** \brief Stores types information for a \ref ValueHistogramMHRWStatsCollector.
 *
 * See \ref ValueHistogramMHRWStatsCollector
 */
template<typename MatrQ, typename TomoValueCalculator_>
struct ValueHistogramMHRWStatsCollectorTraits
{
  //! The matrix type with which we need to calculate e.g. fidelities
  typedef typename MatrQ::MatrixType MatrixType;
  /** \brief The type which calculates the interesting value. Should be of type interface \ref
   * pageInterfaceTomoValueCalculator.
   */
  typedef TomoValueCalculator_ TomoValueCalculator;
  //! The type to use to represent a calculated distance
  typedef typename TomoValueCalculator::ValueType ValueType;
  //! The histogram type. This is a \ref UniformBinsHistogram.
  typedef UniformBinsHistogram<ValueType> HistogramType;
  /** \brief The structure which holds the parameters (range, number of bins) of the
   * histogram we're recording
   */
  typedef typename HistogramType::Params HistogramParams;
};


/** \brief A StatsCollector which builds a histogram of values calculated with a
 * ValueCalculator for each data sample point
 *
 * This stats collector is suitable for tracking statistics during a \ref MHRandomWalk.
 *
 * The TomoValueCalculator is a type expected to implement the \ref
 * pageInterfaceTomoValueCalculator.
 *
 */
template<typename MatrQ_, typename TomoValueCalculator_, typename Log>
class ValueHistogramMHRWStatsCollector
{
public:
  //! The data types we're dealing with
  typedef MatrQ_ MatrQ;
  /** \brief The type which calculates the interesting value. Should be of type interface \ref
   * pageInterfaceTomoValueCalculator.
   */
  typedef TomoValueCalculator_ TomoValueCalculator;

  //! Matrix type we have to deal with (calculate fidelities with)
  typedef typename ValueHistogramMHRWStatsCollectorTraits<MatrQ, TomoValueCalculator>::MatrixType MatrixType;
  //! The type to use to represent a calculated distance
  typedef typename ValueHistogramMHRWStatsCollectorTraits<MatrQ, TomoValueCalculator>::ValueType ValueType;
  //! The type of the histogram. A \ref UniformBinsHistogram with \a ValueType range type
  typedef typename ValueHistogramMHRWStatsCollectorTraits<MatrQ, TomoValueCalculator>::HistogramType HistogramType;
  //! Structure which holds the parameters of the histogram we're recording
  typedef typename ValueHistogramMHRWStatsCollectorTraits<MatrQ, TomoValueCalculator>::HistogramParams HistogramParams;

private:

  HistogramType _histogram;

  /** \brief The value calculator which we will invoke to get the interesting value.
   *
   * The type should implement the \ref pageInterfaceTomoValueCalculator interface.
   */
  TomoValueCalculator & _vcalc;

  Log & _log;

public:
  //! Simple constructor, initializes with the given values
  ValueHistogramMHRWStatsCollector(ValueType fid_min, ValueType fid_max, int num_bins,
				   TomoValueCalculator & vcalc, MatrQ /*mq*/,
				   Log & logger)
    : _histogram(fid_min, fid_max, num_bins),
      _vcalc(vcalc),
      _log(logger)
  {
  }
  //! Simple alternative constructor, initializes with the given values
  ValueHistogramMHRWStatsCollector(HistogramParams histogram_params,
				   TomoValueCalculator & vcalc, MatrQ /*mq*/,
				   Log & logger)
    : _histogram(histogram_params),
      _vcalc(vcalc),
      _log(logger)
  {
  }

  //! Get the histogram data collected so far. Returns a \ref UniformBinsHistogram.
  inline const HistogramType & histogram() const
  {
    return _histogram;
  }

  // stats collector part

  //! Part of the \ref pageInterfaceMHRWStatsCollector. Initializes the histogram to zeros.
  inline void init()
  {
    // reset our array
    _histogram.reset();
  }
  //! Part of the \ref pageInterfaceMHRWStatsCollector. No-op.
  inline void thermalizing_done()
  {
  }
  //! Part of the \ref pageInterfaceMHRWStatsCollector. No-op.
  inline void done()
  {
    if (_log.enabled_for(Logger::LONGDEBUG)) {
      // _log.longdebug("ValueHistogramMHRWStatsCollector", "done()");
      _log.longdebug("ValueHistogramMHRWStatsCollector",
                     "Done walking & collecting stats. Here's the histogram:\n"
                     + _histogram.pretty_print());
    }
  }

  //! Part of the \ref pageInterfaceMHRWStatsCollector. No-op.
  template<typename LLHValueType, typename MHRandomWalk>
  void raw_move(unsigned int k, bool /*is_thermalizing*/, bool /*is_live_iter*/, bool /*accepted*/,
                double /*a*/, const MatrixType & /*newpt*/, LLHValueType /*newptval*/,
                const MatrixType & /*curpt*/, LLHValueType /*curptval*/, MHRandomWalk & /*mh*/)
  {
    _log.longdebug("ValueHistogramMHRWStatsCollector", "raw_move(): k=%lu", (unsigned long)k);
  }

  //! Part of the \ref pageInterfaceMHRWStatsCollector. Records the sample in the histogram.
  template<typename LLHValueType, typename MHRandomWalk>
  void process_sample(unsigned int k, const MatrixType & curpt, LLHValueType /*curptval*/, MHRandomWalk & /*mh*/)
  {
    ValueType val = tomo_internal::callTomoValueCalculator<TomoValueCalculator>(_vcalc, curpt);

    _log.longdebug("ValueHistogramMHRWStatsCollector", "in process_sample(): k=%lu, val=%.4g",
                   (unsigned long)k, val);

    _histogram.record(val);

    //_log.longdebug("ValueHistogramMHRWStatsCollector", "process_sample() finished");
  }
 

};








/** \brief Definitions for running multiple random walks and collecting fidelity statistics
 *
 * Provides class definitions for interfacing with a task manager/dispatcher (see \ref
 * pageTaskManagerDispatcher).
 *
 */
namespace DMLLHIntegratorTasks
{

  /** \brief Data needed to be accessible to the working code
   *
   * Stores the tomography data, as well as parameters to the random walk and ranges for
   * the fidelity histgram to take.
   */
  template<typename TomoProblem, typename TomoValueCalculator>
  struct CData
  {
    typedef typename ValueHistogramMHRWStatsCollectorTraits<typename TomoProblem::MatrQ,
							    TomoValueCalculator>::HistogramParams
    /*..*/  HistogramParams;
    
    CData(const TomoProblem &prob_, TomoValueCalculator value_calculator_,
	  int base_seed_ = 0, HistogramParams hparams = HistogramParams())
      : prob(prob_), base_seed(base_seed_),
	value_calculator(value_calculator_), histogram_params(hparams)
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

    //! The value calculator which interests us
    TomoValueCalculator value_calculator;

    //! Which histogram to record (min, max, numbins)
    HistogramParams histogram_params;
  };

  /** \brief Random Walk on the space of density matrices, collecting fidelity histogram
   * statistics
   *
   * This class can be used with \ref MultiProc::OMPTaskDispatcher, for example.
   */
  template<typename TomoProblem, typename TomoValueCalculator,
	   typename LoggerType, typename Rng = std::mt19937,
           typename CountIntType = unsigned int>
  struct MHRandomWalkTask
  {
    /** \brief Status Report for a \ref MHRandomWalkTask
     *
     * This struct can store information about the current status of a \ref
     * MHRandomWalkTask while it is running.
     *
     * This is for use with, for example, \ref OMPTaskDispatcher::request_status_report().
     */
    struct StatusReport : public MultiProc::StatusReport
    {
      /** \brief Constructor which initializes all fields */
      StatusReport(double fdone = 0.0, const std::string & msg = std::string(),
                   CountIntType kstep_ = 0, CountIntType n_sweep_ = 0,
                   CountIntType n_therm_ = 0, CountIntType n_run_ = 0,
                   double acceptance_ratio_ = 0.0)
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
      CountIntType kstep;
      /** \brief the number of iterations that form a sweep (see \ref MHRandomWalk) */
      CountIntType n_sweep;
      /** \brief the number of thermalization sweeps (see \ref MHRandomWalk) */
      CountIntType n_therm;
      /** \brief the number of live run sweeps (see \ref MHRandomWalk) */
      CountIntType n_run;
      /** \brief the current acceptance ratio of the random walk (see
       *    \ref Tomographer::MHRandomWalk::acceptance_ratio()
       * ) */
      double acceptance_ratio;
      /** \brief the total number of iterations required for this random walk
       *
       * This is calculated as \f$
       *  \textit{nTotalIters} = \textit{nSweep} \times \left( \textit{nTherm} + \textit{nRun} \right)
       * \f$.
       */
      CountIntType n_total_iters;
    };
    /** \brief Typedef for \ref StatusReport. This is needed by, e.g. \ref
     *         MultiProc::OMPTaskDispatcher.
     */
    typedef StatusReport StatusReportType;

    /** \brief convenience typedef for the \ref DMLLHIntegratorTasks::CData which we will use */
    typedef CData<TomoProblem, TomoValueCalculator> CDataType;

    /** \brief convenience typedef for our \ref ValueHistogramMHRWStatsCollector which we use here */
    typedef ValueHistogramMHRWStatsCollector<typename TomoProblem::MatrQ, TomoValueCalculator, LoggerType>
    /*..*/ ValueHistogramMHRWStatsCollectorType;

    struct Result {
      typename ValueHistogramMHRWStatsCollectorType::HistogramType histogram;
    };
    typedef Result ResultType;

  private:
    /** \brief how to seed the random number generator for this particular task (input) */
    typename Rng::result_type _seed;

    /** \brief a logger to which we can log messages */
    LoggerType & _log;

    /** \brief our \ref TomoValueCalculator instance */
    TomoValueCalculator valcalc;

    /** \brief our \ref ValueHistogramMHRWStatsCollector instance */
    ValueHistogramMHRWStatsCollectorType valstats;

    /** \brief the struct in which we hold formally the results of the current task
     *
     * This is none other than the histogram gathered by \ref valstats.
     */
    Result result;

  public:

    /** \brief Returns a random seed to seed the random number generator with for run
     * number \a k
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
    MHRandomWalkTask(int inputseed, const CDataType * pcdata, LoggerType & log)
      : _seed(inputseed),
        _log(log),
	valcalc(pcdata->value_calculator),
        valstats(pcdata->histogram_params,
		 valcalc,
		 pcdata->prob.matq, _log)
    {
    }

    /** \brief Run this task
     *
     * Runs this task, i.e. instantiates a \ref DMStateSpaceLLHRandomWalk with the provided
     * inputs and data, and runs it.
     *
     * This also takes care to call the task manager interface's
     * <code>status_report_requested()</code> and submits a status report if required. See
     * e.g. \ref OMPTaskDispatcher::request_status_report().
     */
    template<typename TaskManagerIface>
    inline void run(const CDataType * pcdata, LoggerType & /*log*/, TaskManagerIface * tmgriface)
    {
      Rng rng(_seed); // seeded random number generator

      typedef StatusReportCheck<TaskManagerIface> OurStatusReportCheck;

      OurStatusReportCheck statreportcheck(this, tmgriface);
      
      MultipleMHRWStatsCollectors<ValueHistogramMHRWStatsCollectorType, OurStatusReportCheck>
        statscollectors(valstats, statreportcheck);

      auto rwalk = makeDMStateSpaceLLHRandomWalk(
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

      result.histogram = valstats.histogram();
    }

    inline const Result & getResult() const
    {
      return result;
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
	  double accept_ratio = std::numeric_limits<double>::quiet_NaN();
	  bool warn_accept_ratio = false;
	  if (rw.has_acceptance_ratio()) {
	    accept_ratio = rw.acceptance_ratio();
	    warn_accept_ratio = (accept_ratio > 0.35 || accept_ratio < 0.2);
	  }
	  std::string msg = Tools::fmts(
              "iteration %s %lu/(%lu=%lu*(%lu+%lu)) : %5.2f%% done  [%saccept ratio=%.2f%s]",
              (is_thermalizing ? "[T]" : "   "),
              (unsigned long)k, (unsigned long)totiters, (unsigned long)rw.n_sweep(),
              (unsigned long)rw.n_therm(), (unsigned long)rw.n_run(),
              fdone*100.0,
	      warn_accept_ratio ? "!!** " : "",
	      accept_ratio,
	      warn_accept_ratio ? " **!!" : ""
              );
          tmgriface->submit_status_report(StatusReport(fdone, msg, k, rw.n_sweep(), rw.n_therm(),
                                                       rw.n_run(), accept_ratio));
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
   *
   * \bug USE AveragedHistogram here!!! CODE DUPLICATED!!
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

    template<typename... Args>
    inline void runs_finished(Args...)
    {
      final_histogram /= num_histograms;
      std_dev /= num_histograms;
      off_chart /= num_histograms;

      // std_dev = sqrt(< X^2 > - < X >^2) / sqrt(Nrepeats)
      auto finhist2 = final_histogram*final_histogram; // for array, this is c-wise product
      std_dev = ( (std_dev - finhist2) / num_histograms ).sqrt();
    }

    template<typename CountIntType, typename ResultType, typename CData>
    inline void collect_result(CountIntType /*k*/, const ResultType& t, const CData * /*pcdata*/)
    {
      // final_histogram collects the sum of the histograms
      // std_dev for now collects the sum of squares. std_dev will be normalized in run_finished().
      auto& histogram = t.histogram;
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

        s += Tools::fmts("%-6.4g | %s    %5.1f +- %5.1f\n",
			 params.min + k*(params.max-params.min)/Ntot,
			 sline.c_str(),
			 final_histogram(k), std_dev(k)
	    );
      }
      if (off_chart > 1e-6) {
        s += Tools::fmts("   ... with another (average) %.4g points off chart.\n", (double)off_chart);
      }
      return s;
    }
  };


}


} // namespace Tomographer


#endif
