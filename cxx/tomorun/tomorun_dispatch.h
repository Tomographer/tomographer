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

#ifndef TOMORUN_DISPATCH
#define TOMORUN_DISPATCH

#include <tomographer2/mathtools/pos_semidef_util.h>

// -----------------------------------------------------------------------------


template<typename DenseLLH_, typename ValueCalculator_>
struct TomorunCDataBase : public Tomographer::MHRWTasks::CDataBase<>
{
  typedef Tomographer::MHRWTasks::CDataBase<> Base; // base class

  typedef DenseLLH_ DenseLLH;
  typedef ValueCalculator_ ValueCalculator;
 
  TomorunCDataBase(const DenseLLH & llh_, const ValueCalculator & vcalc_)
    : llh(llh_), vcalc(vcalc_)
  {
  }

  const DenseLLH llh;
  const ValueCalculator vcalc;

  template<typename Rng, typename LoggerType>
  inline Tomographer::DenseDM::TSpace::LLHMHWalker<DenseLLH,Rng,LoggerType>
  createMHWalker(Rng & rng, LoggerType & logger) const
  {
    return Tomographer::DenseDM::TSpace::LLHMHWalker<DenseLLH,Rng,LoggerType>(
	llh.dmt.initMatrixType(),
	llh,
	rng,
	logger
	);
  }

  inline void print_basic_cdata_mhrw_info(std::ostream & str)
  {
    str << "\t# iter. / sweep = " << Base::n_sweep << "\n"
	<< "\t# therm. sweeps = " << Base::n_therm << "\n"
	<< "\t# run sweeps    = " << Base::n_run << "\n"
	<< "\tstep size       = " << std::setprecision(4) << Base::step_size << "\n";
  }
};


struct RunTaskInfo
{
  RunTaskInfo()
    : n_run(0), n_therm(0), n_sweep(0),
      step_size(std::numeric_limits<double>::quiet_NaN()),
      acceptance_ratio(std::numeric_limits<double>::quiet_NaN())
  {
  }

  template<typename TaskResultType>
  RunTaskInfo(TaskResultType && t)
    : n_run(t.n_run),
      n_therm(t.n_therm),
      n_sweep(t.n_sweep),
      step_size(t.step_size),
      acceptance_ratio(t.acceptance_ratio)
  {
  }

  std::size_t n_run;
  std::size_t n_therm;
  std::size_t n_sweep;
  double step_size;
  double acceptance_ratio;
};



template<typename HistType>
inline void print_short_bar_and_accept_ratio(std::ostream & str, int j, HistType && hist,
					     const RunTaskInfo & inf,
					     int dig_width)
{
  std::string accept_ratio_appendstr =
    " [accept ratio = " + Tomographer::Tools::fmts("%.2f", inf.acceptance_ratio) + "]";

  str << "#" << std::setw(dig_width) << j << ": ";
  int w = histogram_short_bar(str, std::forward<HistType>(hist), false,
			      -3 - dig_width - accept_ratio_appendstr.size());
  str << std::setw(w + accept_ratio_appendstr.size()) << std::right << accept_ratio_appendstr << "\n";
  if (inf.acceptance_ratio > 0.35 || inf.acceptance_ratio < 0.2) {
    str << "    *** Accept ratio out of recommended bounds [0.20, 0.35] ! Adapt step size ***\n";
  }
}


static const std::string report_hline =
  "----------------------------------------------------------------------------------------------------\n";
static const std::string report_final_header =
  "                                        Final Report of Runs                                        \n";
static const std::string report_final_histogram =
  "                                          Final Histogram                                           \n";


// -----------------------------------------------------------------------------
// Version without binning analysis
// -----------------------------------------------------------------------------

template<typename DenseLLH_, typename ValueCalculator_>
struct TomorunCDataSimple : public TomorunCDataBase<DenseLLH_, ValueCalculator_>
{
  typedef DenseLLH_ DenseLLH;
  typedef ValueCalculator_ ValueCalculator;

  typedef TomorunCDataBase<DenseLLH_, ValueCalculator_> Base_;

  typedef typename Tomographer::ValueHistogramMHRWStatsCollector<ValueCalculator>::Result
    MHRWStatsCollectorResultType;
  typedef MHRWStatsCollectorResultType HistogramType;
  typedef typename HistogramType::Params HistogramParams;

  HistogramParams histogram_params;

  TomorunCDataSimple(const DenseLLH & llh_, const ValueCalculator & vcalc_, const ProgOptions * /*opt*/)
    : Base_(llh_, vcalc_), histogram_params()
  {
  }

  template<typename LoggerType>
  inline Tomographer::ValueHistogramMHRWStatsCollector<ValueCalculator,LoggerType,HistogramType>
  createStatsCollector(LoggerType & logger) const
  {
    return Tomographer::ValueHistogramMHRWStatsCollector<ValueCalculator,LoggerType,HistogramType>(
	histogram_params,
	Base_::vcalc,
	logger
	);
  }

  // corresponding results collector
  template<typename LoggerType>
  struct ResultsCollector
  {
    typedef DenseLLH_ DenseLLH;
    typedef ValueCalculator_ ValueCalculator;
    typedef typename Tomographer::ValueHistogramMHRWStatsCollector<ValueCalculator>::Result HistogramType;
    typedef typename HistogramType::Params HistogramParams;
    typedef Tomographer::UniformBinsHistogram<typename HistogramType::Scalar, double>
	NormalizedHistogramType;
    typedef Tomographer::AveragedHistogram<NormalizedHistogramType, typename NormalizedHistogramType::CountType>
        FinalHistogramType;

    std::vector<RunTaskInfo> collected_runtaskinfos;
    std::vector<NormalizedHistogramType> collected_histograms;
    FinalHistogramType finalhistogram;

    Tomographer::Logger::LocalLogger<LoggerType>  llogger;
    
    ResultsCollector(LoggerType & logger_)
      : finalhistogram(HistogramParams()), llogger("TomorunCDataSimple::ResultsCollector", logger_)
    {
    }
    
    template<typename Cnt, typename CData>
    inline void init(Cnt num_total_runs, Cnt /*n_chunk*/,
                     const CData * pcdata)
    {
      collected_histograms.resize(num_total_runs);
      collected_runtaskinfos.reserve(num_total_runs);
      finalhistogram.reset(pcdata->histogram_params);
    }
    template<typename Cnt, typename TaskResultType, typename CData>
    inline void collect_result(Cnt task_no, const TaskResultType& taskresult, const CData * /*pcdata*/)
    {
      llogger.sublogger(TOMO_ORIGIN).debug([&](std::ostream & str) {
          str << "Got task result. Histogram is:\n" << taskresult.stats_collector_result.pretty_print();
        });
      NormalizedHistogramType thishistogram = taskresult.stats_collector_result;
      typename NormalizedHistogramType::CountType normalization =
	thishistogram.bins.sum() + thishistogram.off_chart;
      thishistogram.bins /= normalization;
      thishistogram.off_chart /= normalization;

      collected_histograms[task_no] = thishistogram;
      collected_runtaskinfos[task_no] = RunTaskInfo(taskresult);
      finalhistogram.add_histogram(thishistogram);
    }
    template<typename Cnt, typename CData>
    inline void runs_finished(Cnt, const CData *)
    {
      finalhistogram.finalize();
    }

    template<typename CDataType>
    inline void produce_final_report(CDataType && cdata)
    {
      // produce report on runs
      auto logger = llogger.sublogger(TOMO_ORIGIN);
      logger.info([&](std::ostream & str) {
          str << "\n"
              << report_final_header
              << report_hline
            ;
	  cdata.print_basic_cdata_mhrw_info(str);
          std::size_t j;
          int w = (int)std::ceil(std::log10(collected_histograms.size()));
          for (j = 0; j < collected_histograms.size(); ++j) {
	    print_short_bar_and_accept_ratio(str, j, collected_histograms[j], collected_runtaskinfos[j], w);
          }
          str << report_hline
              << "\n";
          // and the final histogram
          str << report_final_histogram
              << report_hline;
          histogram_pretty_print(str, finalhistogram);
          str << report_hline
              << "\n";
        });
    }

    inline void write_histogram_file(const std::string & csvfname)
    {
      auto l = llogger.sublogger(TOMO_ORIGIN);
      //l.info("()");

      std::ofstream outf;
      outf.open(csvfname);
      outf << "Value\tCounts\tError\n"
           << std::scientific << std::setprecision(10);
      for (int kk = 0; kk < finalhistogram.bins.size(); ++kk) {
        outf << (double)finalhistogram.params.bin_lower_value(kk) << "\t"
             << (double)finalhistogram.bins(kk) << "\t"
             << (double)finalhistogram.delta(kk) << "\n";
      }
      l.info("Wrote histogram to CSV file %s (Value/Counts/Error)", csvfname.c_str());
    }
  };
};


// -----------------------------------------------------------------------------
// Version with the Binning Analysis
// -----------------------------------------------------------------------------

template<typename DenseLLH_, typename ValueCalculator_>
struct TomorunCDataBinning : public TomorunCDataBase<DenseLLH_, ValueCalculator_>
{
  typedef DenseLLH_ DenseLLH;
  typedef ValueCalculator_ ValueCalculator;
  typedef TomorunCDataBase<DenseLLH_, ValueCalculator_> Base_;

  typedef Tomographer::ValueHistogramWithBinningMHRWStatsCollectorParams<ValueCalculator>
    BinningMHRWStatsCollectorParams;
  typedef typename BinningMHRWStatsCollectorParams::Result MHRWStatsCollectorResultType;
  typedef typename BinningMHRWStatsCollectorParams::HistogramType HistogramType;
  typedef typename BinningMHRWStatsCollectorParams::HistogramParams HistogramParams;

  HistogramParams histogram_params;
  int binning_num_levels;

  TomorunCDataBinning(const DenseLLH & llh_, const ValueCalculator & vcalc_, const ProgOptions * opt)
    : Base_(llh_, vcalc_), histogram_params()
  {
    binning_num_levels = opt->binning_analysis_num_levels;
  }

  template<typename LoggerType>
  inline Tomographer::ValueHistogramWithBinningMHRWStatsCollector<BinningMHRWStatsCollectorParams,LoggerType>
  createStatsCollector(LoggerType & logger) const
  {
    return Tomographer::ValueHistogramWithBinningMHRWStatsCollector<BinningMHRWStatsCollectorParams,LoggerType>(
	histogram_params,
	Base_::vcalc,
        binning_num_levels,
	logger
	);
  }

  // corresponding results collector
  template<typename LoggerType>
  struct ResultsCollector
  {
    typedef DenseLLH_ DenseLLH;
    typedef ValueCalculator_ ValueCalculator;

    typedef Tomographer::ValueHistogramWithBinningMHRWStatsCollectorParams<ValueCalculator> MHRWStatsCollectorParams;

    typedef typename MHRWStatsCollectorParams::Result Result;
    typedef typename MHRWStatsCollectorParams::HistogramType HistogramType;
    typedef typename HistogramType::Params HistogramParams;
    
    std::vector<RunTaskInfo> collected_runtaskinfos;
    //! All collected histograms for each run
    std::vector<Result> collected_histograms;
    //! The final histogram, properly averaged
    Tomographer::AveragedHistogram<HistogramType, double> finalhistogram;

    /** \brief The "simple" histogram, as if without binning analysis.
     *
     * Note we need a `double` counting type, because the histograms we'll be recording
     * are normalized.
     */
    typedef Tomographer::UniformBinsHistogram<typename HistogramType::Scalar, double> SimpleHistogramType;
    //typedef typename MHRWStatsCollectorParams::BaseHistogramType SimpleHistogramType;
    Tomographer::AveragedHistogram<SimpleHistogramType, double> simplefinalhistogram;

    Tomographer::Logger::LocalLogger<LoggerType>  llogger;
    
    typedef typename MHRWStatsCollectorParams::BinningAnalysisParamsType BinningAnalysisParamsType;

    ResultsCollector(LoggerType & logger_)
      : finalhistogram(), simplefinalhistogram(), llogger("TomorunCDataBinning::ResultsCollector", logger_)
    {
    }
    
    template<typename Cnt, typename CData>
    inline void init(Cnt num_total_runs, Cnt /*n_chunk*/,
                     const CData * pcdata)
    {
      collected_histograms.resize(num_total_runs);
      collected_runtaskinfos.resize(num_total_runs);
      finalhistogram.reset(pcdata->histogram_params);
      simplefinalhistogram.reset(pcdata->histogram_params);
    }
    template<typename Cnt, typename TaskResultType, typename CData>
    inline void collect_result(Cnt task_no, TaskResultType && taskresult, const CData *)
    {
      auto logger = llogger.sublogger(TOMO_ORIGIN);

      collected_runtaskinfos[task_no] = RunTaskInfo(taskresult);
      auto stats_coll_result = taskresult.stats_collector_result;
      collected_histograms[task_no] = stats_coll_result;

      logger.debug([&](std::ostream & str) {
          str << "(). Got task result. Histogram (w/ error bars from binning analysis):\n"
              << stats_coll_result.hist.pretty_print();
        });
      
      if ((stats_coll_result.converged_status !=
           Eigen::ArrayXi::Constant(stats_coll_result.hist.num_bins(), BinningAnalysisParamsType::CONVERGED)).any()) {
        logger.debug([&,this](std::ostream & str) {
            str << "Error bars have not converged! The error bars at different binning levels are:\n"
                << stats_coll_result.error_levels << "\n"
                << "\t-> convergence analysis: \n";
            for (std::size_t k = 0; k < stats_coll_result.hist.num_bins(); ++k) {
              str << "\t    val[" << std::setw(3) << k << "] = "
                  << std::setw(12) << stats_coll_result.hist.bins(k)
                  << " +- " << std::setw(12) << stats_coll_result.hist.delta(k);
              if (stats_coll_result.converged_status(k) == BinningAnalysisParamsType::CONVERGED) {
                str << "  [CONVERGED]";
              } else if (stats_coll_result.converged_status(k) == BinningAnalysisParamsType::NOT_CONVERGED) {
                str << "  [NOT CONVERGED]";
              } else if (stats_coll_result.converged_status(k) == BinningAnalysisParamsType::UNKNOWN_CONVERGENCE) {
                str << "  [UNKNOWN]";
              } else {
                str << "  [UNKNOWN CONVERGENCE STATUS: " << stats_coll_result.converged_status(k) << "]";
              }
              str << "\n";
            }
          });
      }

      // because stats_coll_result is a histogram WITH error bars, add_histogram will do the
      // right thing and take them into account.
      finalhistogram.add_histogram(stats_coll_result.hist);

      logger.debug("added first histogram.");


      // this one is declared for histograms WITHOUT error bars (SimpleHistogramType is a
      // UniformBinsHistogram), so it will just ignore the error bars.
      logger.debug([&](std::ostream & str) {
	  str << "Simple histogram is:\n";
	  Tomographer::histogram_pretty_print<SimpleHistogramType>(str, stats_coll_result.hist);
	});
      simplefinalhistogram.add_histogram(stats_coll_result.hist);
      logger.debug("done.");
    }
    template<typename Cnt, typename CData>
    inline void runs_finished(Cnt, const CData *)
    {
      finalhistogram.finalize();
      simplefinalhistogram.finalize();
    }

    template<typename CDataType>
    inline void produce_final_report(CDataType && cdata)
    {
      // produce report on runs
      auto logger = llogger.sublogger(TOMO_ORIGIN);
      logger.info([&](std::ostream & str) {
          str << "\n"
              << report_final_header
              << report_hline
            ;
	  cdata.print_basic_cdata_mhrw_info(str);
          std::size_t j;
          int w = (int)std::ceil(std::log10(collected_histograms.size()));
          for (j = 0; j < collected_histograms.size(); ++j) {
	    print_short_bar_and_accept_ratio(str, j, collected_histograms[j].hist, collected_runtaskinfos[j], w);
	    // error bars stats:
            const int nbins = collected_histograms[j].converged_status.size();
            const int n_conv = collected_histograms[j].converged_status
              .cwiseEqual(BinningAnalysisParamsType::CONVERGED).count();
            Eigen::ArrayXi unkn_arr = (collected_histograms[j].converged_status
				       .cwiseEqual(BinningAnalysisParamsType::UNKNOWN_CONVERGENCE))
              .template cast<int>();
            const int n_unknown = unkn_arr.count();
            const int n_unknown_followingotherunknown
              = unkn_arr.segment(0,nbins-1).cwiseProduct(unkn_arr.segment(1,nbins-1)).count();
            const int n_unknown_isolated = n_unknown - n_unknown_followingotherunknown;
            const int n_notconv = collected_histograms[j].converged_status
              .cwiseEqual(BinningAnalysisParamsType::NOT_CONVERGED).count();
            str << "    error bars: " << n_conv << " converged / "
                << n_unknown << " maybe (" << n_unknown_isolated << " isolated) / "
                << n_notconv << " not converged\n";
          }
          str << report_hline
              << "\n";
          // and the final histogram
          str << report_final_histogram
              << report_hline;
          histogram_pretty_print(str, finalhistogram);
          str << report_hline
              << "\n";
        });
    }

    inline void write_histogram_file(const std::string & csvfname)
    {
      auto logger = llogger.sublogger(TOMO_ORIGIN);
      //logger.info("()");
      
      std::ofstream outf;
      outf.open(csvfname);
      outf << "Value\tCounts\tError\tSimpleError\n"
           << std::scientific << std::setprecision(10);
      for (int kk = 0; kk < finalhistogram.bins.size(); ++kk) {
        outf << (double)finalhistogram.params.bin_lower_value(kk) << "\t"
             << (double)finalhistogram.bins(kk) << "\t"
             << (double)finalhistogram.delta(kk) << "\t"
             << (double)simplefinalhistogram.delta(kk) << "\n";
      }
      logger.info("Wrote histogram to CSV file %s (Value/Counts/Error/SimpleError)", csvfname.c_str());
    }
  };
};


// =============================================================================

template<bool BinningAnalysisErrorBars, typename DenseLLH, typename ValueCalculator>
struct TomorunModeTypes
{
};

template<typename DenseLLH, typename ValueCalculator>
struct TomorunModeTypes<true, DenseLLH, ValueCalculator>
{
  typedef TomorunCDataBinning<DenseLLH, ValueCalculator> CData;
};
template<typename DenseLLH, typename ValueCalculator>
struct TomorunModeTypes<false, DenseLLH, ValueCalculator>
{
  typedef TomorunCDataSimple<DenseLLH, ValueCalculator> CData;
};


//
// Here goes the actual program. This is templated, because then we can let Eigen use
// allocation on the stack rather than malloc'ing 2x2 matrices...
//
template<bool BinningAnalysisErrorBars, typename DenseLLH,
         typename FnMakeValueCalculator, typename LoggerType>
inline void tomorun(const DenseLLH & llh, const ProgOptions * opt,
		    FnMakeValueCalculator makeValueCalculator, LoggerType & baselogger)
{
  Tomographer::Logger::LocalLogger<LoggerType> logger(TOMO_ORIGIN, baselogger);
  //
  // create the OMP Task Manager and run.
  //

  auto valcalc = makeValueCalculator();
  typedef decltype(valcalc) ValueCalculator;

  typedef typename TomorunModeTypes<BinningAnalysisErrorBars, DenseLLH, ValueCalculator>::CData OurCData;

  typedef Tomographer::MHRWTasks::MHRandomWalkTask<OurCData, std::mt19937>  OurMHRandomWalkTask;

  typedef typename OurCData::template ResultsCollector<LoggerType> OurResultsCollector;

  OurCData taskcdat(llh, valcalc, opt);
  // seed for random number generator
  taskcdat.base_seed = std::chrono::system_clock::now().time_since_epoch().count();
  // parameters for the value histogram
  taskcdat.histogram_params = typename OurCData::HistogramParams(opt->val_min, opt->val_max, opt->val_nbins);
  // parameters of the random walk
  taskcdat.n_sweep = opt->Nsweep;
  taskcdat.n_therm = opt->Ntherm;
  taskcdat.n_run = opt->Nrun;
  taskcdat.step_size = opt->step_size;

  OurResultsCollector results(logger.baselogger());

  auto tasks = Tomographer::MultiProc::OMP::makeTaskDispatcher<OurMHRandomWalkTask>(
      &taskcdat, // constant data
      &results, // results collector
      logger.baselogger(), // the main logger object
      opt->Nrepeats, // num_runs
      opt->Nchunk // n_chunk
      );

  // set up signal handling
  auto srep = Tomographer::Tools::makeSigHandlerTaskDispatcherStatusReporter(&tasks, logger.baselogger());
  Tomographer::Tools::installSignalStatusReportHandler(SIGINT, &srep);
  //  auto srep = makeSigHandlerStatusReporter(&tasks, logger.baselogger());
  //  signal_handler = &srep;
  //  signal(SIGINT, sig_int_handler);

  // and run our tomo process

  auto time_start = TimerClock::now();

  srep.time_start = time_start;

  tasks.run();

  auto time_end = TimerClock::now();

  logger.debug("Random walks done.");

  // delta-time, in seconds and fraction of seconds
  std::string elapsed_s = Tomographer::Tools::fmt_duration(time_end - time_start);

  results.produce_final_report(taskcdat);

  // save the histogram to a CSV file if the user required it
  if (opt->write_histogram.size()) {
    results.write_histogram_file(opt->write_histogram+"-histogram.csv");
  }

  logger.info("Computation time: %s\n\n", elapsed_s.c_str());
}









// -----------------------------------------------------------------------------


//
// And here is the dispatcher. It will call the correct variant of tomorun() (with the
// correct template parameters), depending on what we were asked to do.
//
//
template<int FixedDim, int FixedMaxPOVMEffects, bool BinningAnalysisErrorBars, typename LoggerType>
inline void tomorun_dispatch(unsigned int dim, ProgOptions * opt, Tomographer::MAT::File * matf, LoggerType & logger)
{
  logger.debug("tomorun_dispatch()", "Running tomography program! FixedDim=%d and FixedMaxPOVMEffects=%d",
               FixedDim, FixedMaxPOVMEffects);

  //
  // Typedefs for tomography data types
  //

  typedef Tomographer::DenseDM::DMTypes<FixedDim, double> OurDMTypes;
  typedef Tomographer::DenseDM::IndepMeasLLH<OurDMTypes, double, int, FixedMaxPOVMEffects> OurDenseLLH;

  //
  // Read data from file
  //

  OurDMTypes dmt(dim);
  OurDenseLLH llh(dmt);

  typename Tomographer::Tools::eigen_std_vector<typename OurDMTypes::MatrixType>::type Emn;
  Emn = Tomographer::MAT::value<decltype(Emn)>(matf->var("Emn"));
  Eigen::VectorXi Nm;
  Nm = Tomographer::MAT::value<Eigen::VectorXi>(matf->var("Nm"));
  ensure_valid_input((int)Emn.size() == Nm.size(),
		     "number of POVM effects in `Emn' doesn't match length of `Nm'");
  if (Emn.size() > 0) {
    ensure_valid_input(Emn[0].cols() == dim && Emn[0].rows() == dim,
		       Tomographer::Tools::fmts("POVM effects don't have dimension %d x %d", dim, dim));
  }

  // convert to x-parameterization
  unsigned int k, j;
  int Npovmeffects = 0;
  for (k = 0; k < (unsigned int)Nm.size(); ++k) {
    if (Nm[k] > 0) {
      ++Npovmeffects;
    }
  }
  logger.debug("tomorun_dispatch()", "Npovmeffects=%d", Npovmeffects);
  llh.initMeasVector(Npovmeffects);
  j = 0;
  Tomographer::DenseDM::ParamX<OurDMTypes> xp(dmt);
  for (k = 0; k < Emn.size(); ++k) {
    if (Nm[k] == 0) {
      continue;
    }
    // do some tests: positive semidefinite and non-zero
#ifdef DO_SLOW_POVM_CONSISTENCY_CHECKS
    logger.longdebug("tomorun_dispatch()", [&](std::ostream & str) {
                       str << "Emn["<<k<<"] = \n" << Emn[k] << "\n"
                           << "\tEV=" << Emn[k].eigenvalues().transpose().real() << "\n"
                           << "\tnorm=" << double(Emn[k].norm()) << "\n" ;
                     });
    ensure_valid_input(double( (Emn[k] - Emn[k].adjoint()).norm() ) < 1e-8,
		       Tomographer::Tools::fmts("POVM effect #%d is not hermitian!", k)); // Hermitian
    ensure_valid_input(double(Emn[k].eigenvalues().real().minCoeff()) >= -1e-12,
		       Tomographer::Tools::fmts("POVM effect #%d is not positive semidefinite!", k)); // Pos semidef
    ensure_valid_input(double(Emn[k].norm()) > 1e-6,
		       Tomographer::Tools::fmts("POVM effect #%d is zero!", k));
    logger.debug("tomorun_dispatch()", "Consistency checks passed for Emn[%u].", k);
#endif

    // don't need to reset `row` to zero, param_herm_to_x doesn't require it
    llh.Exn.row(j) = xp.HermToX(Emn[k]).transpose();
    llh.Nx[j] = Nm[k];
    ++j;
  }
  // done.

  logger.debug("tomorun_dispatch()", [&](std::ostream & ss) {
                 ss << "\n\nExn: size="<<llh.Exn.size()<<"\n"
                    << llh.Exn << "\n";
                 ss << "\n\nNx: size="<<llh.Nx.size()<<"\n"
                    << llh.Nx << "\n";
               });

  //tomodat.rho_MLE = Tomographer::MAT::value<typename OurMatrQ::MatrixType>(matf->var("rho_MLE"));

  //  ensure_valid_input(tomodat.rho_MLE.cols() == dim && tomodat.rho_MLE.rows() == dim,
  //		     Tomographer::Tools::fmts("rho_MLE is expected to be a square matrix %d x %d", dim, dim));

  //  tomodat.x_MLE = matq.initVectorParamType();
  //  Tomographer::param_herm_to_x(tomodat.x_MLE, tomodat.rho_MLE);

  //  tomodat.T_MLE = matq.initMatrixType();

  //  Eigen::LDLT<typename OurMatrQ::MatrixType> ldlt(tomodat.rho_MLE);
  //  typename OurMatrQ::MatrixType P = matq.initMatrixType();
  //  P = ldlt.transpositionsP().transpose() * OurMatrQ::MatrixType::Identity(dim,dim);
  //  tomodat.T_MLE.noalias() = P * ldlt.matrixL() * ldlt.vectorD().cwiseSqrt().asDiagonal();

  //  Eigen::SelfAdjointEigenSolver<typename OurMatrQ::MatrixType> rho_MLE_eig(tomodat.rho_MLE);
  //  tomodat.T_MLE = rho_MLE_eig.operatorSqrt();

  llh.NMeasAmplifyFactor = opt->NMeasAmplifyFactor;


  typedef typename OurDMTypes::MatrixType MatrixType;

  
  //
  // Data has now been successfully read. Now, dispatch to the correct template function
  // for futher processing.
  //

  //
  // Instantiate The Correct Figure Of Merit Calculator
  //

  //
  // Figure of merit is one of the built-in distance measures. There might be a reference
  // state.
  //
  if (opt->valtype.valtype == val_type_spec::FIDELITY ||
      opt->valtype.valtype == val_type_spec::TR_DIST ||
      opt->valtype.valtype == val_type_spec::PURIF_DIST) {
    
    MatrixType rho_ref = dmt.initMatrixType();
    MatrixType T_ref = dmt.initMatrixType();
    
    std::string refname = "rho_MLE";
    if (opt->valtype.ref_obj_name.size()) {
      refname = opt->valtype.ref_obj_name;
    }

    rho_ref = Tomographer::MAT::value<MatrixType>(matf->var(refname));

    typedef typename Eigen::SelfAdjointEigenSolver<MatrixType>::RealVectorType RealVectorType;

    Eigen::SelfAdjointEigenSolver<MatrixType> eig_rho_ref( rho_ref );
    auto U = eig_rho_ref.eigenvectors();
    auto d = eig_rho_ref.eigenvalues();
    
    Tomographer::MathTools::force_pos_vec_keepsum<RealVectorType>(d, 1e-12);
    
    rho_ref = U * d.asDiagonal() * U.adjoint();
    T_ref = U * d.cwiseSqrt().asDiagonal() * U.adjoint();

    logger.debug("tomorun_dispatch()", [&](std::ostream & str) {
	str << "Using rho_ref = \n" << rho_ref << "\n\t-> T_ref = \n" << T_ref << "\n";
      });

    if (opt->valtype.valtype == val_type_spec::FIDELITY) {
      tomorun<BinningAnalysisErrorBars>(
          llh,
          opt,
          [&T_ref]() {
            return Tomographer::DenseDM::TSpace::FidelityToRefCalculator<OurDMTypes, double>(T_ref);
          },
          logger);
    } else if (opt->valtype.valtype == val_type_spec::PURIF_DIST) {
      tomorun<BinningAnalysisErrorBars>(
          llh,
          opt,
          [&T_ref]() {
            return Tomographer::DenseDM::TSpace::PurifDistToRefCalculator<OurDMTypes, double>(T_ref);
          },
          logger);
    } else if (opt->valtype.valtype == val_type_spec::TR_DIST) {
      tomorun<BinningAnalysisErrorBars>(
          llh,
          opt,
          [&rho_ref]() {
	    return Tomographer::DenseDM::TSpace::TrDistToRefCalculator<OurDMTypes, double>(rho_ref);
          },
          logger);
    } else {
      throw std::logic_error("WTF?? You shouldn't be here!");
    }

    return;
  }
  //
  // Figure of merit: observable value
  //
  if (opt->valtype.valtype == val_type_spec::OBS_VALUE) {
    
    // load the observable
    MatrixType A = dmt.initMatrixType();

    std::string obsname = "Observable";
    if (opt->valtype.ref_obj_name.size()) {
      obsname = opt->valtype.ref_obj_name;
    }

    A = Tomographer::MAT::value<MatrixType>(matf->var(obsname));
    
    ensure_valid_input(A.cols() == dim && A.rows() == dim,
		       Tomographer::Tools::fmts("Observable (%s) is expected to be a square matrix %d x %d",
						obsname.c_str(), dim, dim));

    // and run our main program
    tomorun<BinningAnalysisErrorBars>(
        llh,
        opt,
        [&A, dmt]() {
          return Tomographer::DenseDM::TSpace::ObservableValueCalculator<OurDMTypes>(dmt, A);
        },
        logger);

    return;
  }
  // --------------------------------------------------
  //
  // INSERT CUSTOM FIGURE OF MERIT HERE:
  // See instructions in API documentation, page 'Adding a new figure of merit to the tomorun program'
  //
  // --------------------------------------------------

  throw std::logic_error(std::string("Unknown value type: ")+streamstr(opt->valtype));
}




template<int FixedDim, int FixedMaxPOVMEffects, typename LoggerType>
inline void tomorun_dispatch_eb(unsigned int dim, ProgOptions * opt, Tomographer::MAT::File * matf, LoggerType & logger)
{
  if (opt->binning_analysis_error_bars) {
    tomorun_dispatch<FixedDim, FixedMaxPOVMEffects, true, LoggerType>(dim, opt, matf, logger);
  } else {
    tomorun_dispatch<FixedDim, FixedMaxPOVMEffects, false, LoggerType>(dim, opt, matf, logger);
  }
}







#endif
