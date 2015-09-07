/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef TOMOGRAPHER_MHWALKER_TASKS_H
#define TOMOGRAPHER_MHWALKER_TASKS_H

/** \file mhrwtasks.h
 *
 * \brief Multiprocessing tasks interface (see \ref pageTaskManagerDispatch) for
 *        parallel Metropolis-Hastings random walks.
 *
 */

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
   * Stores the parameters to the random walk.
   *
   */
  template<typename CountIntType_ = unsigned int, typename RealType_ = double>
  struct CDataBase
  {
    /** \brief Constructor.
     *
     * Make sure to initialize \a base_seed to something quite random (e.g. current time
     * in seconds) so that independent runs of your program won't produce the exact same
     * results.
     */
    CDataBase(int base_seed_ = 0)
      : base_seed(base_seed_)
    {
    }

    //! Type used to count the number of iterations
    typedef CountIntType_ CountIntType;
    //! Type used to specify the step size
    typedef RealType_ RealType;

    //! Parameter of the random walk -- number of iterations per sweep
    CountIntType n_sweep;
    //! Parameter of the random walk -- number of thermalizing sweeps
    CountIntType n_therm;
    //! Parameter of the random walk -- number of "live" sweeps
    CountIntType n_run;
    //! Parameter of the random walk -- step size of the random walk
    RealType step_size;

    /** \brief A base random seed from which each run seed will be derived
     *
     * For each new random walk, a new pseudorandom number generator instance is created
     * with a new seed.
     *
     * In order to be able to reproduce results, the seeds are set deterministically from
     * the \a base_seed, by using for the random seed of the k-th random walk:
     * <code>seed[k] = base_seed + k</code>
     *
     * Thus, by setting the base_seed to a fixed value you can reproduce all the results
     * of a run. However, in order for your program not to output exactly the same thing
     * if it is run a second time, and to make sure the points are indeed random, you must
     * randomize \a base_seed, e.g. using the current time.
     */
    int base_seed;
  };

  /** \brief Random Walk task, collecting statistics
   *
   * This class can be used with \ref MultiProc::OMP::TaskDispatcher, for example.
   *
   * \tparam MHRandomWalkTaskCData must comply with the \ref
   *         pageInterfaceMHRandomWalkTaskCData and inherit from \ref
   *         CDataBase<CountIntType,RealType> with appropriate types as required.  The
   *         parameters to the random walk, as well as types to use, stats collector
   *         etc. are specified using this class.
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
     *         MultiProc::OMP::TaskDispatcher.
     */
    typedef StatusReport StatusReportType;

  private:
    /** \brief how to seed the random number generator for this particular task (input) */
    typename Rng::result_type _seed;

    /** \brief the struct in which we hold formally the results of the current task
     *
     * This is usually none other than the e.g. histogram gathered by \ref valstats.
     *
     * We use a pointer here because we don't want to rely on it having to be
     * default-constructible. We create the object only when requested.
     */
    Result * result;

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
      : _seed(inputseed), result(NULL)
    {
      logger.longdebug("MHRandomWalkTask", "() inputseed=%d", inputseed);
    }

    ~MHRandomWalkTask()
    {
      if (result != NULL) {
        delete result;
      }
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
      typedef StatusReportCheck<TaskManagerIface, MHRWStatsCollectorType> OurStatusReportCheck;
      OurStatusReportCheck statreportcheck(this, &stats, tmgriface);
      
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

      result = new Result(stats.getResult());
    }

    inline const Result & getResult() const
    {
      return *result;
    }

  private:
    /** \internal
     * \brief helper to provide status report
     *
     * This is in fact a StatsCollector.
     */
    template<typename TaskManagerIface, typename MHRWStatsCollectorType>
    struct StatusReportCheck
    {
      StatusReportCheck(MHRandomWalkTask * mhrwtask_, MHRWStatsCollectorType * stats_, TaskManagerIface *tmgriface_)
        : mhrwtask(mhrwtask_), stats(stats_), tmgriface(tmgriface_)
      { }

      MHRandomWalkTask *mhrwtask;
      MHRWStatsCollectorType *stats;
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
          typedef MHRWStatsCollectorStatus<MHRWStatsCollectorType> MHRWStatsCollectorStatusType;
          if (MHRWStatsCollectorStatusType::CanProvideStatus) {
            std::string nlindent = "\n    ";
            msg += nlindent;
	    std::string s = MHRWStatsCollectorStatusType::getStatus(stats);
	    for (std::size_t j = 0; j < s.size(); ++j) {
	      if (s[j] == '\n') {
		msg += nlindent;
	      } else {
		msg += s[j];
	      }
	    }
          }
          tmgriface->submit_status_report(StatusReport(fdone, msg, k, rw.n_sweep(), rw.n_therm(),
                                                       rw.n_run(), accept_ratio));
        }
        //        fprintf(stderr, "StatusReportCheck::raw_move(): done\n");
      }

      template<typename PointType, typename FnValueType, typename MHRandomWalk>
      inline void process_sample(CountIntType, CountIntType, const PointType &, FnValueType, MHRandomWalk &)
      {
      }
    };
  };



} // MHWalkerTasks



} // namespace Tomographer



#endif
