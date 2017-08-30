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

#include <climits>

#include <string>
#include <limits>
#include <random>
#include <sstream>
#include <stdexcept>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>

#include <tomographer/tools/fmt.h>
#include <tomographer/tools/needownoperatornew.h>
#include <tomographer/mhrw.h>
#include <tomographer/mhrwstatscollectors.h>
#include <tomographer/multiproc.h> // StatusReport Base


namespace Tomographer {
namespace MHRWTasks {




/** \brief Data needed to be accessible to the working code
 *
 * This is only a base class for the actual CData. The actual CData must
 * additionally provide some methods, e.g. to create the random walk and stats
 * collectors. See \ref pageInterfaceMHRandomWalkTaskCData.
 *
 * Stores the parameters to the random walk.
 *
 *
 * \since Since %Tomographer 5.3, you can specify an arbitrary list of seeds as
 *        inputs to each random number generator for each task (alternative
 *        constructor).
 *
 * \since Since %Tomographer 5.3, this class can be serialized with
 *        Boost.Serialization.
 *
 */
template<typename MHWalkerParams_, typename IterCountIntType_ = int,
         typename RngSeedType_ = std::mt19937::result_type>
struct TOMOGRAPHER_EXPORT CDataBase
{
  //! Type used to count the number of iterations
  typedef IterCountIntType_ IterCountIntType;
  //! Type used to specify the step size
  typedef MHWalkerParams_ MHWalkerParams;

  /** \brief Type used to specify the seed of the random number generator
   *
   * \since Added in %Tomographer 5.0
   */
  typedef RngSeedType_ RngSeedType;

  /** \brief Type to store the parameters of the Metropolis-Hastings random walk
   *         (number of runs, sweep size, etc.)
   *
   * See \ref MHRWParams<MHWalkerParams,IterCountIntType>
   */
  typedef MHRWParams<MHWalkerParams, IterCountIntType> MHRWParamsType;


  /** \brief Constructor.
   *
   * Make sure to initialize \a base_seed to something quite random
   * (e.g. current time in seconds) so that independent runs of your program
   * won't produce the exact same results.
   */
  template<typename MHRWParamsType>
  CDataBase(MHRWParamsType&& p, RngSeedType base_seed_)
    : mhrw_params(std::forward<MHRWParamsType>(p)),
      base_seed(base_seed_),
      task_seeds()
  {
  }

  /** \brief Constructor.
   *
   * Using this constructor the seed list is set, which takes precedence over \a
   * base_seed.
   *
   * \since Introduced in %Tomographer 5.3
   */
  template<typename MHRWParamsType>
  CDataBase(MHRWParamsType&& p, std::vector<RngSeedType> task_seeds_)
    : mhrw_params(std::forward<MHRWParamsType>(p)),
      base_seed(0),
      task_seeds(std::move(task_seeds_))
  {
  }

  //! Construct an invalid object -- ONLY for use with Boost.serialization
  CDataBase() : mhrw_params(), base_seed(), task_seeds() { }


  /** \brief Parameters of the random walk
   *
   * Stores the number of iterations per sweep, the number of thermalizing
   * sweeps, the number of "live" sweeps, and the step size of the random walk.
   *
   * See \ref MHRWParams<MHWalkerParams,IterCountIntType>
   *
   * \since In %Tomographer 5.3: Removed the \c const attribute (to make
   *        serialization easier; anyway in multiprocessing implementations a
   *        const pointer to this class is kept ensuring const-ness already)
   */
  MHRWParamsType mhrw_params;

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
   *
   * \since In %Tomographer 5.3: Removed the \c const attribute (to make serialization
   *        easier; anyway in multiprocessing implementations a const pointer to this
   *        class is kept ensuring const-ness already)
   */
  RngSeedType base_seed;

  /** \brief A list of random seeds to use for each task
   *
   * If set, this list of seeds takes precedence over base_seed.  The seed at
   * index number \a k will be fed to the corresponding task.
   *
   * \since Introduced in %Tomographer 5.3
   */
  std::vector<RngSeedType> task_seeds;


  /** \brief Returns a random seed to seed the random number generator with for
   *         run number \a k
   *
   * This function provides the input of the k-th task. Each task must of course
   * have a different seed, otherwise we will just repeat the same "random"
   * walks!
   *
   * If \a task_seeds is not empty, then this function simply returns \a
   * task_seeds[k].  It is an error if \a k is not in range for that vector.  In
   * other words, if you give a list of seeds with \a task_seeds, you need to
   * specify enough seeds to cover all tasks.
   *
   * If \a task_seeds is empty, then this simply returns <code>base_seed + k</code>.
   *
   */
  template<typename TaskNoCountIntType>
  inline RngSeedType getTaskInput(TaskNoCountIntType k) const
  {
    if (task_seeds.size() > 0) {
      // specific task seeds given (e.g. from random device)
      if (k < 0 || (std::size_t)k >= task_seeds.size()) {
        throw std::out_of_range(streamstr("getTaskInput(): k="<<k<<" out of range; seed list size is = "
                                          << task_seeds.size()));
      }
      return task_seeds[(std::size_t)k];
    } else {
      // derive seed from base seed
      
      // empirically it's noticeably better to feed the RNG sequential numbers rather than
      // try to shuffle bits around (!!)
      return base_seed + (RngSeedType)k;
    }
  }


  /** \brief Get some human-readable info about the random walk as a string.
   *
   */
  inline void printBasicCDataMHRWInfo(std::ostream & str) const
  {
    str << "\tstep            : " << std::setprecision(4) << mhrw_params.mhwalker_params << "\n"
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

private:
  friend boost::serialization::access;
  template<typename Archive>
  void serialize(Archive & a, unsigned int /* version */)
  {
    a & mhrw_params;
    a & base_seed;
    a & task_seeds;
  }
};







/** \brief Result of a task run.
 *
 * This is the results from stats collectors, as well as information about the random walk
 * (e.g. acceptance ratio).
 *
 * \tparam MHRWStatsResultsType The result of the stat collector(s).  This type
 *         may be any type which can be constructed using the result of the stats
 *         collector created by the CData type (see \a createMHRWTaskComponents() in \ref
 *         pageInterfaceMHRandomWalkTaskCData).  Typically, this is just the \a ResultType
 *         of the \a MHRWStatsCollector itself which was created by \a
 *         createMHRWTaskComponents().  This type must be default-constructible.
 *
 * \tparam IterCountIntType the integer type used for counting iterations in the MHRW task.
 *
 * \tparam MHWalkerParams the real type used to describe the step size.
 *
 *
 * \since Since %Tomographer 5.3, this class can be serialized with Boost.Serialization as
 *        long as \a MHRWStatsResultsType can be serialized.
 */
template<typename MHRWStatsResultsType_, typename IterCountIntType, typename MHWalkerParams>
struct TOMOGRAPHER_EXPORT MHRandomWalkTaskResult
  : public virtual Tools::NeedOwnOperatorNew<MHRWStatsResultsType_>::ProviderType
{
  /** \brief The type which stores the results from the statistics carried out during the random walk
   *
   * This is given by the first template parameter (see class doc).
   */
  typedef MHRWStatsResultsType_ MHRWStatsResultsType;
    
  /** \brief The type to use to store the parameters of the random walk
   */
  typedef MHRWParams<MHWalkerParams, IterCountIntType> MHRWParamsType;
  
  /** \brief Constructor, initializes fields to the given values
   *
   * The first parameter is meant to be the \a ResultType of a \a MHRWStatsCollector
   * (const reference or temporary); it may be however any type accepted by a one-argument
   * constructor of the \a ResultType in question (this allows to call either the
   * rvalue-ref constructor or copy constructor automatically).
   */
  template<typename MHRWStatsResultsTypeInit,
           typename MHRWParamsTypeInit>
  MHRandomWalkTaskResult(MHRWStatsResultsTypeInit && stats_results_,
                         MHRWParamsTypeInit && mhrw_params_,
                         double acceptance_ratio_)
    : stats_results(std::forward<MHRWStatsResultsTypeInit>(stats_results_)),
      mhrw_params(std::forward<MHRWParamsTypeInit>(mhrw_params_)),
      acceptance_ratio(acceptance_ratio_)
  {
  }

  /** \brief Constructor with \ref mhrw_params initialized from a random walk instance
   *
   * \param stats_results_ the MHRWStatsResults result, or an acceptable initializer for
   *        that type
   *
   * \param mhrandomwalk should be a \ref MHRandomWalk instance
   */
  template<typename MHRWStatsResultsTypeInit, typename MHRandomWalkType>
  MHRandomWalkTaskResult(MHRWStatsResultsTypeInit && stats_results_,
                         const MHRandomWalkType & mhrandomwalk)
    : stats_results(std::forward<MHRWStatsResultsTypeInit>(stats_results_)),
      mhrw_params(mhrandomwalk.mhrwParams()),
      acceptance_ratio(mhrandomwalk.hasAcceptanceRatio() ?
                       mhrandomwalk.acceptanceRatio() :
                       std::numeric_limits<double>::quiet_NaN())
  {
  }


  //! Construct an invalid object -- ONLY for use with Boost.serialization
  TOMOGRAPHER_ENABLED_IF(std::is_default_constructible<MHRWStatsResultsType>::value)
  MHRandomWalkTaskResult() : stats_results(), mhrw_params(), acceptance_ratio() { }

    
  /** \brief The result(s) coming from stats collecting (may be processed, see \ref
   *         pageInterfaceMHRandomWalkTaskCData)
   */
  MHRWStatsResultsType stats_results;
    
  //! The parameters of the random walk (see \ref MHRWParams<MHWalkerParams,IterCountIntType>)
  MHRWParamsType mhrw_params;

  //! The acceptance ratio of the Metropolis-Hastings random walk
  double acceptance_ratio;


  MHRandomWalkTaskResult(MHRandomWalkTaskResult && ) = default;
  MHRandomWalkTaskResult(const MHRandomWalkTaskResult & ) = default;

private:
  friend boost::serialization::access;
  template<typename Archive,
           typename MHRWStatsResultsType2 = MHRWStatsResultsType>
  void serialize(Archive & a, unsigned int /* version */)
  {
    MHRWStatsResultsType2 & stats_results_ref = stats_results;
    a & stats_results_ref;
    a & mhrw_params;
    a & acceptance_ratio;
  }
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
struct TOMOGRAPHER_EXPORT MHRandomWalkTask
{
  //! The type used to count iterations (see \ref MHRWParams)
  typedef typename MHRandomWalkTaskCData::IterCountIntType IterCountIntType;
  //! The type used to describe a step size (see \ref MHRWParams)
  typedef typename MHRandomWalkTaskCData::MHWalkerParams MHWalkerParams;

  //! Type to stores the parameters of the random walk 
  typedef MHRWParams<MHWalkerParams, IterCountIntType> MHRWParamsType;

  /** \brief Result type of a single task run.
   *
   * See \ref MHRandomWalkTaskResult.
   */    
  typedef MHRandomWalkTaskResult<typename MHRandomWalkTaskCData::MHRWStatsResultsType,
                                 IterCountIntType, MHWalkerParams> ResultType;

  /** \brief Typedef for \ref MHRWStatusReport. This is needed by, e.g. \ref
   *         MultiProc::OMP::TaskDispatcher.
   */
  typedef MHRWStatusReport<MHRWParamsType> StatusReportType;

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
  MHRandomWalkTask(typename Rng::result_type inputseed, const MHRandomWalkTaskCData * /*pcdata*/,
                   LoggerType & logger)
    : _seed(inputseed), result(NULL)
  {
    logger.longdebug("MHRandomWalkTask", [&](std::ostream & stream) {
        stream << "inputseed=" << inputseed;
      });
  }

  ~MHRandomWalkTask()
  {
    if (result != NULL) {
      delete result;
    }
  }

private:

  template<typename LoggerType, typename TaskMgrIface>
  struct RunInnerCallable
  {
    const MHRandomWalkTaskCData * pcdata;
    Rng & rng;
    LoggerType & baselogger;
    TaskMgrIface * tmgriface;

    ResultType ** ppresult;

    RunInnerCallable(const MHRandomWalkTaskCData * pcdata_, Rng & rng_, LoggerType & logger_,
                     TaskMgrIface * tmgriface_, ResultType ** ppresult_)
      : pcdata(pcdata_), rng(rng_), baselogger(logger_), tmgriface(tmgriface_), ppresult(ppresult_)
    {
    }
    ~RunInnerCallable() {
    }

    template<typename MHWalkerType, typename MHRWStatsCollectorType, typename MHRWControllerType>
    inline void operator()(MHWalkerType & mhwalker, MHRWStatsCollectorType & stats,
                           MHRWControllerType & controller)
    {
      // here we actually run the stuff

      auto logger = Logger::makeLocalLogger("Tomographer::MHRWTasks::MHRandomWalkTask::run()/run", baselogger);

      // our own "stats collector" which checks if we need to send a status report back
      typedef PredStatusReportMHRWStatsCollector<MHRWParamsType> OurStatusReportCheck;
      OurStatusReportCheck statreportcheck(
          // predicate
          [&]() { return this->tmgriface->statusReportRequested(); },
          // send-status-function
          [&](StatusReportType report) { this->tmgriface->submitStatusReport(std::move(report)); }
          );

      typedef MultipleMHRWStatsCollectors<MHRWStatsCollectorType, OurStatusReportCheck>
        OurStatsCollectors;
      OurStatsCollectors ourstatscollectors(stats, statreportcheck);

      logger.longdebug("About to creat MHRandomWalk instance");

      typedef MHRandomWalk<Rng,MHWalkerType,OurStatsCollectors,MHRWControllerType,LoggerType,IterCountIntType>
        MHRandomWalkType;

      MHRandomWalkType rwalk(
          // MH random walk parameters
          pcdata->mhrw_params,
          // the MHWalker
          mhwalker,
          // our stats collectors
          ourstatscollectors,
          // the random walk controller
          controller,
          // a random number generator
          rng,
          // and a logger
          baselogger
          );
      
      logger.longdebug("MHRandomWalk object created, running...");

      rwalk.run();

      logger.longdebug("MHRandomWalk run finished.");

      *ppresult = new ResultType(stats.stealResult(), rwalk);
    }

    template<typename MHWalkerType, typename MHRWStatsCollectorType>
    inline void operator()(MHWalkerType & mhwalker, MHRWStatsCollectorType & stats)
    {
      MHRWNoController ctrl;
      operator()<MHWalkerType, MHRWStatsCollectorType, MHRWNoController>(mhwalker, stats, ctrl) ;
    }
  };


public:
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
  inline void run(const MHRandomWalkTaskCData * pcdata, LoggerType & baselogger, TaskManagerIface * tmgriface)
  {
    auto logger = Logger::makeLocalLogger("Tomographer::MHRWTasks::MHRandomWalkTask::run()", baselogger) ;

    logger.debug([&](std::ostream & stream) {
        stream << "Starting random walk, using rng seed = " << _seed;
      });
    
    Rng rng(_seed); // seeded random number generator

    RunInnerCallable<LoggerType, TaskManagerIface> run_object(pcdata, rng, baselogger, tmgriface, &result);

    pcdata->setupRandomWalkAndRun(rng, baselogger, run_object);
  }

  inline ResultType getResult() const
  {
    return *result;
  }

  inline ResultType stealResult()
  {
    return std::move(*result);
  }
};



} // namespace MHRWTasks



} // namespace Tomographer



#endif
