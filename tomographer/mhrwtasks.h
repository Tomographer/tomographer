/* This file is part of the Tomographer project, which is distributed under the
 * terms of the MIT license.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 ETH Zurich, Institute for Theoretical Physics, Philippe Faist
 * Copyright (c) 2017 Caltech, Institute for Quantum Information and Matter, Philippe Faist
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
 * \brief Multiprocessing tasks interface (see \ref pageTaskManagerDispatcher) for
 *        parallel Metropolis-Hastings random walks.
 *
 */

#include <string>
#include <limits>
#include <random>
#include <sstream>

#include <tomographer/tools/fmt.h>
#include <tomographer/tools/needownoperatornew.h>
#include <tomographer/mhrw.h>
#include <tomographer/mhrwstatscollectors.h>
#include <tomographer/multiproc.h> // StatusReport Base


namespace Tomographer {
namespace MHRWTasks {


/** \brief Data needed to be accessible to the working code
 *
 * This is only a base class for the actual CData. The actual CData must additionally
 * provide some methods, e.g. to create the random walk and stats collectors. See \ref
 * pageInterfaceMHRandomWalkTaskCData.
 *
 * Stores the parameters to the random walk.
 *
 */
template<typename IterCountIntType_ = int, typename StepRealType_ = double>
TOMOGRAPHER_EXPORT struct CDataBase
{
  /** \brief Constructor.
   *
   * Make sure to initialize \a base_seed to something quite random (e.g. current time
   * in seconds) so that independent runs of your program won't produce the exact same
   * results.
   */
  template<typename MHRWParamsType>
  CDataBase(MHRWParamsType&& p, int base_seed_ = 0)
    : mhrw_params(std::forward<MHRWParamsType>(p)), base_seed(base_seed_)
  {
  }

  //! Type used to count the number of iterations
  typedef IterCountIntType_ IterCountIntType;
  //! Type used to specify the step size
  typedef StepRealType_ StepRealType;

  /** \brief Type to store the parameters of the Metropolis-Hastings random walk (number of
   *         runs, sweep size, etc.)
   *
   * See \ref MHRWParams<IterCountIntType,StepRealType>
   */
  typedef MHRWParams<IterCountIntType, StepRealType> MHRWParamsType;

  /** \brief Parameters of the random walk
   *
   * Stores the number of iterations per sweep, the number of thermalizing sweeps, the
   * number of "live" sweeps, and the step size of the random walk.
   *
   * See \ref MHRWParams<IterCountIntType,StepRealType>
   */
  const MHRWParamsType mhrw_params;

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
  const int base_seed;

  /** \brief Returns a random seed to seed the random number generator with for run
   * number \a k
   *
   * This simply returns \code pcdata->base_seed + k \endcode
   *
   * This should be considered as the input of the k-th task. Each task must of course
   * have a different seed, otherwise we will just repeat the same "random" walks!
   *
   */
  inline int getTaskInput(int k) const
  {
    return base_seed + k;
  }
    

  /** \brief Get some human-readable info about the random walk as a string.
   */
  inline void printBasicCDataMHRWInfo(std::ostream & str) const
  {
    str << "\tstep size       = " << std::setprecision(4) << mhrw_params.step_size << "\n"
        << "\t# iter. / sweep = " << mhrw_params.n_sweep << "\n"
        << "\t# therm. sweeps = " << mhrw_params.n_therm << "\n"
        << "\t# run sweeps    = " << mhrw_params.n_run << "\n";
  }
  /** \brief Get some human-readable info about the random walk as a string, see \ref
   *         printBasicCDataMHRWInfo()
   */
  inline std::string getBasicCDataMHRWInfo() const
  {
    std::ostringstream ss;
    printBasicCDataMHRWInfo(ss);
    return ss.str();
  }
};

/** \brief Result of a task run.
 *
 * This is the stats collector's result, as well as information about the random walk
 * (e.g. acceptance ratio).
 *
 * \tparam MHRWStatsCollectorResultType_ the result type of the MHRWStatsCollector which
 *         the task will be running.
 * \tparam IterCountIntType the integer type used for counting iterations in the MHRW task.
 * \tparam StepRealType the real type used to describe the step size.
 *
 */
template<typename MHRWStatsCollectorResultType_, typename IterCountIntType, typename StepRealType>
TOMOGRAPHER_EXPORT struct MHRandomWalkTaskResult
  : public virtual Tools::NeedOwnOperatorNew<MHRWStatsCollectorResultType_>::ProviderType
{
  /** \brief The specified result type of the MHRWStatsCollector the task will be looking at
   */
  typedef MHRWStatsCollectorResultType_ MHRWStatsCollectorResultType;
    
  /** \brief The type to use to store the parameters of the random walk
   */
  typedef MHRWParams<IterCountIntType, StepRealType> MHRWParamsType;
    
  /** \brief Construct an empty task result
   *
   */
  MHRandomWalkTaskResult()
    : stats_collector_result(), mhrw_params(),
      acceptance_ratio(std::numeric_limits<double>::quiet_NaN())
  {
  }

  /** \brief Constructor, initializes fields to the given values
   *
   * The first parameter is meant to be a \ref MHRWStatsCollectorResultType const
   * reference or temporary; it may be however any type accepted by a one-argument
   * constructor of \ref MHRWStatsCollectorResultType (this allows to call either the
   * rvalue-ref constructor or copy constructor automatically).
   */
  template<typename MHRWStatsCollectorResultTypeInit,
           typename MHRWParamsType2>
  MHRandomWalkTaskResult(MHRWStatsCollectorResultTypeInit && stats_collector_result_,
                         MHRWParamsType2 && mhrw_params_,
                         double acceptance_ratio_)
    : stats_collector_result(std::forward<MHRWStatsCollectorResultTypeInit>(stats_collector_result_)),
      mhrw_params(std::forward<MHRWParamsType2>(mhrw_params_)),
      acceptance_ratio(acceptance_ratio_)
  {
  }

  /** \brief Constructor with \ref mhrw_params initialized from a random walk instance
   *
   * \param stats_collector_result_ the stats-collector result
   * \param mhrandomwalk should be a \ref MHRandomWalk instance
   */
  template<typename MHRWStatsCollectorResultTypeInit, typename MHRandomWalkType>
  MHRandomWalkTaskResult(MHRWStatsCollectorResultTypeInit && stats_collector_result_,
                         const MHRandomWalkType & mhrandomwalk)
    : stats_collector_result(std::forward<MHRWStatsCollectorResultTypeInit>(stats_collector_result_)),
      mhrw_params(mhrandomwalk.mhrwParams()),
      acceptance_ratio(mhrandomwalk.hasAcceptanceRatio() ?
                       mhrandomwalk.acceptanceRatio() :
                       std::numeric_limits<double>::quiet_NaN())
  {
  }
    
  //! The result furnished by the stats collector itself
  const MHRWStatsCollectorResultType stats_collector_result;
    
  //! The parameters of the random walk (see \ref MHRWParams<IterCountIntType,StepRealType>)
  const MHRWParamsType mhrw_params;

  //! The acceptance ratio of the Metropolis-Hastings random walk
  const double acceptance_ratio;
};


/** \brief Random Walk task, collecting statistics
 *
 * This class can be used with \ref MultiProc::OMP::TaskDispatcher, for example.
 *
 * \tparam MHRandomWalkTaskCData must comply with the \ref
 *         pageInterfaceMHRandomWalkTaskCData and inherit from \ref CDataBase with
 *         appropriate types as required.  The parameters to the random walk, as well as
 *         types to use, stats collector etc. are specified using this class.
 */
template<typename MHRandomWalkTaskCData,
         typename Rng = std::mt19937>
TOMOGRAPHER_EXPORT struct MHRandomWalkTask
{
  //! The type used to count iterations (see \ref MHRWParams)
  typedef typename MHRandomWalkTaskCData::IterCountIntType IterCountIntType;
  //! The type used to describe a step size (see \ref MHRWParams)
  typedef typename MHRandomWalkTaskCData::StepRealType StepRealType;

  //! Type to stores the parameters of the random walk 
  typedef MHRWParams<IterCountIntType, StepRealType> MHRWParamsType;

  /** \brief Result type of a single task run.
   *
   * See \ref MHRandomWalkTaskResult.
   */    
  typedef MHRandomWalkTaskResult<typename MHRandomWalkTaskCData::MHRWStatsCollectorResultType,
                                 IterCountIntType, StepRealType> ResultType;

  /** \brief Status Report for a \ref MHRandomWalkTask
   *
   * This struct can store information about the current status of a \ref
   * MHRandomWalkTask while it is running.
   *
   * This is for use with, for example, \ref
   * MultiProc::OMP::TaskDispatcher::requestStatusReport().
   */
  struct StatusReport : public MultiProc::TaskStatusReport
  {
    /** \brief Constructor which initializes all fields */
    StatusReport(double fdone = 0.0, const std::string & msg = std::string(),
                 IterCountIntType kstep_ = 0, MHRWParamsType mhrw_params_ = MHRWParamsType(),
                 double acceptance_ratio_ = 0.0)
      : MultiProc::TaskStatusReport(fdone, msg),
        kstep(kstep_),
        mhrw_params(mhrw_params_),
        acceptance_ratio(acceptance_ratio_),
        n_total_iters(mhrw_params.n_sweep*(mhrw_params.n_therm + mhrw_params.n_run))
    {
    }

    /** \brief the current iteration number */
    IterCountIntType kstep;

    /** \brief the parameters of the random walk
     *
     * Stores the number of iterations that form a sweep (\a n_sweep), the total number
     * of thermalization sweeps (\a n_therm), the total number of live run sweeps (\a
     * n_run) as well as the step size of the random walk (\a step_size).
     *
     * See also \ref MHRandomWalk
     */
    MHRWParamsType mhrw_params;

    /** \brief the current acceptance ratio of the random walk (see
     *    \ref Tomographer::MHRandomWalk::acceptanceRatio()
     * ) */
    double acceptance_ratio;

    /** \brief the total number of iterations required for this random walk
     *
     * This is calculated as \f$
     *  \textit{nTotalIters} = \textit{nSweep} \times \left( \textit{nTherm} + \textit{nRun} \right)
     * \f$.
     */
    IterCountIntType n_total_iters;
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
  ResultType * result;

public:

  /** \brief Constructs the MHRandomWalkTask
   *
   * You should never need to call this directly, except if you're writing a task
   * manager/dispatcher (e.g. \ref MultiProc::OMP::TaskDispatcher)
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
   * <code>statusReportRequested()</code> and submits a status report if required. See
   * e.g. \ref MultiProc::OMP::TaskDispatcher::requestStatusReport().
   */
  template<typename LoggerType, typename TaskManagerIface>
  inline void run(const MHRandomWalkTaskCData * pcdata, LoggerType & logger,
                  TaskManagerIface * tmgriface)
  {
    Rng rng(_seed); // seeded random number generator

    logger.longdebug("Tomographer::MHRWTasks::run()", "about to construct stats collector.");

    // the user's stats collector
    auto stats = pcdata->createStatsCollector(logger);
    typedef decltype(stats) MHRWStatsCollectorType;

    logger.longdebug("Tomographer::MHRWTasks::run()", "stats collector constructed.");

    // our own "stats collector" which checks if we need to send a status report back
    typedef StatusReportCheck<TaskManagerIface, MHRWStatsCollectorType> OurStatusReportCheck;
    OurStatusReportCheck statreportcheck(this, &stats, tmgriface);
      
    typedef MultipleMHRWStatsCollectors<MHRWStatsCollectorType, OurStatusReportCheck>
      OurStatsCollectors;
    OurStatsCollectors ourstatscollectors(stats, statreportcheck);

    logger.longdebug("Tomographer::MHRWTasks::run()", "about to create MH walker object.");

    auto mhwalker = pcdata->createMHWalker(rng, logger);
    typedef decltype(pcdata->createMHWalker(rng, logger)) MHWalkerType;

    logger.longdebug("Tomographer::MHRWTasks::run()", "MHWalker object created.");

    MHRandomWalk<Rng, MHWalkerType, OurStatsCollectors, LoggerType, IterCountIntType> rwalk(
        // MH random walk parameters
        pcdata->mhrw_params,
        // the MHWalker
        mhwalker,
        // our stats collectors
        ourstatscollectors,
        // a random number generator
        rng,
        // and a logger
        logger
        );
      
    logger.longdebug("Tomographer::MHRWTasks::run()", "MHRandomWalk object created, running...");

    rwalk.run();

    logger.longdebug("Tomographer::MHRWTasks::run()", "MHRandomWalk run finished.");

    result = new ResultType(stats.getResult(), rwalk);
  }

  inline const ResultType & getResult() const
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
    inline void thermalizingDone() { }
    inline void done() { }

    template<typename PointType, typename FnValueType, typename MHRandomWalk>
    inline void rawMove(
        IterCountIntType k, bool is_thermalizing, bool, bool, double, const PointType &, FnValueType,
        const PointType &, FnValueType, MHRandomWalk & rw
        )
    {
      // see if we should provide a status report
      //        fprintf(stderr, "StatusReportCheck::rawMove(): testing for status report requested!\n");
      // only check once per sweep, to speed up things
      if ((k % rw.nSweep() == 0) && tmgriface->statusReportRequested()) {
        // prepare & provide status report
        IterCountIntType totiters = rw.nSweep()*(rw.nTherm()+rw.nRun());
        // k restarts at zero after thermalization, so account for that:
        IterCountIntType kreal = is_thermalizing ? k : k + rw.nSweep()*rw.nTherm();
        double fdone = (double)kreal/totiters;
        double accept_ratio = std::numeric_limits<double>::quiet_NaN();
        bool warn_accept_ratio = false;
        if (rw.hasAcceptanceRatio()) {
          accept_ratio = rw.acceptanceRatio();
          warn_accept_ratio = (accept_ratio > MHRWAcceptanceRatioRecommendedMax ||
                               accept_ratio < MHRWAcceptanceRatioRecommendedMin);
        }
        std::string msg = Tools::fmts(
            "%s %lu/(%lu=%lu*(%lu+%lu)) : %5.2f%% done  [%saccept ratio=%.2f%s]",
            ( ! is_thermalizing
              ? "iteration"
              : "[therm.] "),
            (unsigned long)kreal, (unsigned long)totiters, (unsigned long)rw.nSweep(),
            (unsigned long)rw.nTherm(), (unsigned long)rw.nRun(),
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
        tmgriface->submitStatusReport(StatusReport(fdone, msg, kreal, rw.mhrwParams(), accept_ratio));
      }
      //        fprintf(stderr, "StatusReportCheck::rawMove(): done\n");
    }

    template<typename PointType, typename FnValueType, typename MHRandomWalk>
    inline void processSample(IterCountIntType, IterCountIntType, const PointType &, FnValueType, MHRandomWalk &)
    {
    }
  };
};



} // MHWalkerTasks



} // namespace Tomographer



#endif
