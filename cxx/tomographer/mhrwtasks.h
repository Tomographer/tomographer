
#ifndef TOMOGRAPHER_MHWALKER_TASKS_H
#define TOMOGRAPHER_MHWALKER_TASKS_H

#include <string>
#include <limits>
#include <random>

#include <tomographer/tools/fmt.h>
#include <tomographer/mhrw.h>
#include <tomographer/multiproc.h> // StatusReport Base


namespace Tomographer {


/** \brief Definitions for running multiple random walks
 *
 * Provides class definitions for interfacing with a task manager/dispatcher (see \ref
 * pageTaskManagerDispatcher).
 *
 */
namespace MHRWTasks
{

  /** \brief Data needed to be accessible to the working code
   *
   * This is only a base class for the actual CData. The actual CData must additionally
   * provide some methods, e.g. to create the random walk and stats collectors. See \ref
   * pageInterfaceMHRandomWalkTaskCData.
   *
   * Stores the tomography data, as well as parameters to the random walk and ranges for
   * the fidelity histgram to take.
   *
   */
  template<typename CountIntType_ = unsigned int, typename RealType_ = double>
  struct CDataBase
  {
    CDataBase(int base_seed_ = 0)
      : base_seed(base_seed_)
    {
    }

    typedef CountIntType_ CountIntType;
    typedef RealType_ RealType;

    //! Parameter of the random walk -- number of iterations per sweep
    CountIntType n_sweep;
    //! Parameter of the random walk -- number of thermalizing sweeps
    CountIntType n_therm;
    //! Parameter of the random walk -- number of "live" sweeps
    CountIntType n_run;
    //! Parameter of the random walk -- step size of the random walk
    RealType step_size;

    //! A base random seed from which each run seed will be derived
    int base_seed;
  };

  /** \brief Random Walk task, collecting statistics
   *
   * This class can be used with \ref MultiProc::OMPTaskDispatcher, for example.
   *
   */
  template<typename MHRandomWalkTaskCData,
	   typename Rng = std::mt19937>
  struct MHRandomWalkTask
  {
    typedef typename MHRandomWalkTaskCData::CountIntType CountIntType;
    typedef typename MHRandomWalkTaskCData::MHRWStatsCollectorResultType Result;

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

  private:
    /** \brief how to seed the random number generator for this particular task (input) */
    typename Rng::result_type _seed;

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
    static inline int get_input(int k, const MHRandomWalkTaskCData * pcdata)
    {
      return pcdata->base_seed + k;
    }

    /** \brief Constructs the MHRandomWalkTask
     *
     * You should never need to call this directly, except if you're writing a task
     * manager/dispatcher (e.g. \ref OMPTaskDispatcher)
     */
    template<typename LoggerType>
    MHRandomWalkTask(int inputseed, const MHRandomWalkTaskCData * /*pcdata*/, LoggerType & logger)
      : _seed(inputseed)
    {
      logger.longdebug("MHRandomWalkTask", "() inputseed=%d", inputseed);
    }

    /** \brief Run this task
     *
     * Runs this task, i.e. instantiates a \ref MHRandomWalk with the provided inputs and
     * data, and runs it.
     *
     * This also takes care to call the task manager interface's
     * <code>status_report_requested()</code> and submits a status report if required. See
     * e.g. \ref OMPTaskDispatcher::request_status_report().
     */
    template<typename LoggerType, typename TaskManagerIface>
    inline void run(const MHRandomWalkTaskCData * pcdata, LoggerType & logger,
		    TaskManagerIface * tmgriface)
    {
      Rng rng(_seed); // seeded random number generator

      // the user's stats collector
      auto stats = pcdata->createStatsCollector(logger);
      typedef decltype(stats) MHRWStatsCollectorType;

      // our own "stats collector" which checks if we need to send a status report back
      typedef StatusReportCheck<TaskManagerIface> OurStatusReportCheck;
      OurStatusReportCheck statreportcheck(this, tmgriface);
      
      typedef MultipleMHRWStatsCollectors<MHRWStatsCollectorType, OurStatusReportCheck>
	OurStatsCollectors;
      OurStatsCollectors ourstatscollectors(stats, statreportcheck);

      auto mhwalker = pcdata->createMHWalker(rng, logger);
      typedef decltype(mhwalker) MHWalkerType;

      MHRandomWalk<Rng, MHWalkerType, OurStatsCollectors, LoggerType, CountIntType> rwalk(
	  // MH random walk parameters
	  pcdata->n_sweep,
	  pcdata->n_therm,
	  pcdata->n_run,
	  pcdata->step_size,
	  // the MHWalker
	  mhwalker,
	  // our stats collectors
	  ourstatscollectors,
	  // a random number generator
	  rng,
	  // and a logger
	  logger
	  );
      
      rwalk.run();

      result = stats.getResult();
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
              "%s %lu/(%lu=%lu*(%lu+%lu)) : %5.2f%% done  [%saccept ratio=%.2f%s]",
              ( ! is_thermalizing
		? "iteration"
		: "[therm.] "),
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



} // MHWalkerTasks



} // namespace Tomographer



#endif