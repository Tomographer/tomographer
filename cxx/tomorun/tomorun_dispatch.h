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

#include <tomographer/qit/pos_semidef_util.h>

// -----------------------------------------------------------------------------


template<typename TomoProblem_, typename ValueCalculator_>
struct TomorunCDataBase : public Tomographer::MHRWTasks::CDataBase<>
{
  typedef TomoProblem_ TomoProblem;
  typedef ValueCalculator_ ValueCalculator;

  TomorunCDataBase(const TomoProblem & tomo_, const ValueCalculator & vcalc_)
    : tomo(tomo_), vcalc(vcalc_)
  {
  }

  const TomoProblem tomo;
  const ValueCalculator vcalc;

  template<typename Rng, typename LoggerType>
  inline Tomographer::DMStateSpaceLLHMHWalker<TomoProblem,Rng,LoggerType>
  createMHWalker(Rng & rng, LoggerType & logger) const
  {
    return Tomographer::DMStateSpaceLLHMHWalker<TomoProblem,Rng,LoggerType>(
	tomo.matq.initMatrixType(),
	tomo,
	rng,
	logger
	);
  }

};


// -----------------------------------------------------------------------------
// Version without binning analysis
// -----------------------------------------------------------------------------

template<typename TomoProblem_, typename ValueCalculator_>
struct TomorunCDataSimple : public TomorunCDataBase<TomoProblem_, ValueCalculator_>
{
  typedef TomoProblem_ TomoProblem;
  typedef ValueCalculator_ ValueCalculator;

  typedef TomorunCDataBase<TomoProblem_, ValueCalculator_> Base_;

  typedef typename Tomographer::ValueHistogramMHRWStatsCollector<ValueCalculator_>::Result
  MHRWStatsCollectorResultType;
  typedef MHRWStatsCollectorResultType HistogramType;
  typedef typename HistogramType::Params HistogramParams;

  HistogramParams histogram_params;

  TomorunCDataSimple(const TomoProblem & tomo_, const ValueCalculator & vcalc_, const ProgOptions * /*opt*/)
    : Base_(tomo_, vcalc_), histogram_params()
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
    typedef TomoProblem_ TomoProblem;
    typedef ValueCalculator_ ValueCalculator;
    typedef typename Tomographer::ValueHistogramMHRWStatsCollector<ValueCalculator>::Result HistogramType;
    typedef typename HistogramType::Params HistogramParams;

    std::vector<HistogramType> collected_histograms;
    Tomographer::AveragedHistogram<HistogramType, double> finalhistogram;

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
      finalhistogram.reset(pcdata->histogram_params);
    }
    template<typename Cnt, typename CData>
    inline void collect_result(Cnt task_no, const HistogramType& taskresult, const CData *)
    {
      llogger.sublogger(TOMO_ORIGIN).debug([&](std::ostream & str) {
          str << "Got task result. Histogram is:\n" << taskresult.pretty_print();
        });
      collected_histograms[task_no] = taskresult;
      finalhistogram.add_histogram(taskresult);
    }
    template<typename Cnt, typename CData>
    inline void runs_finished(Cnt, const CData *)
    {
      finalhistogram.finalize();
    }

    inline void produce_final_report()
    {
      // produce report on runs
      auto logger = llogger.sublogger(TOMO_ORIGIN);
      logger.info([&](std::ostream & str) {
          str << "\n"
              << "               Final Report of Runs               \n"
              << "--------------------------------------------------\n";
          std::size_t j;
          int w = (int)std::ceil(std::log10(collected_histograms.size()));
          for (j = 0; j < collected_histograms.size(); ++j) {
            str << "#" << std::setw(w) << j << ": "
                << histogram_short_bar(collected_histograms[j], false, -3-w)
                << "\n";
          }
          str << "--------------------------------------------------\n"
              << "\n\n";
          // and the final histogram
          str << "                  Final Histogram                 \n"
              << "--------------------------------------------------\n";
          histogram_pretty_print(str, finalhistogram);
          str << "--------------------------------------------------\n\n";
        });
    }

    inline void write_histogram_file(const std::string & csvfname)
    {
      auto l = llogger.sublogger(TOMO_ORIGIN);
      l.info("()");

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

template<typename TomoProblem_, typename ValueCalculator_>
struct TomorunCDataBinning : public TomorunCDataBase<TomoProblem_, ValueCalculator_>
{
  typedef TomoProblem_ TomoProblem;
  typedef ValueCalculator_ ValueCalculator;
  typedef TomorunCDataBase<TomoProblem_, ValueCalculator_> Base_;

  typedef Tomographer::ValueHistogramWithBinningMHRWStatsCollectorParams<ValueCalculator>
  BinningMHRWStatsCollectorParams;
  typedef typename BinningMHRWStatsCollectorParams::Result MHRWStatsCollectorResultType;
  typedef typename BinningMHRWStatsCollectorParams::HistogramType HistogramType;
  typedef typename BinningMHRWStatsCollectorParams::HistogramParams HistogramParams;

  HistogramParams histogram_params;
  int binning_num_levels;

  TomorunCDataBinning(const TomoProblem & tomo_, const ValueCalculator & vcalc_, const ProgOptions * opt)
    : Base_(tomo_, vcalc_), histogram_params()
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
    typedef TomoProblem_ TomoProblem;
    typedef ValueCalculator_ ValueCalculator;

    typedef Tomographer::ValueHistogramWithBinningMHRWStatsCollectorParams<ValueCalculator> MHRWStatsCollectorParams;

    typedef typename MHRWStatsCollectorParams::Result Result;
    typedef typename MHRWStatsCollectorParams::HistogramType HistogramType;
    typedef typename HistogramType::Params HistogramParams;
    
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
      finalhistogram.reset(pcdata->histogram_params);
      simplefinalhistogram.reset(pcdata->histogram_params);
    }
    template<typename Cnt, typename CData>
    inline void collect_result(Cnt task_no, const Result& taskresult, const CData *)
    {
      auto logger = llogger.sublogger(TOMO_ORIGIN);

      collected_histograms[task_no] = taskresult;

      logger.debug([&](std::ostream & str) {
          str << "(). Got task result. Histogram (w/ error bars from binning analysis):\n"
              << taskresult.hist.pretty_print();
        });
      
      if ((taskresult.converged_status !=
           Eigen::ArrayXi::Constant(taskresult.hist.num_bins(), BinningAnalysisParamsType::CONVERGED)).any()) {
        logger.debug([&,this](std::ostream & str) {
            str << "Error bars have not converged! The error bars at different binning levels are:\n"
                << taskresult.error_levels << "\n"
                << "\t-> convergence analysis: \n";
            for (std::size_t k = 0; k < taskresult.hist.num_bins(); ++k) {
              str << "\t    val[" << std::setw(3) << k << "] = "
                  << std::setw(12) << taskresult.hist.bins(k)
                  << " +- " << std::setw(12) << taskresult.hist.delta(k);
              if (taskresult.converged_status(k) == BinningAnalysisParamsType::CONVERGED) {
                str << "  [CONVERGED]";
              } else if (taskresult.converged_status(k) == BinningAnalysisParamsType::NOT_CONVERGED) {
                str << "  [NOT CONVERGED]";
              } else if (taskresult.converged_status(k) == BinningAnalysisParamsType::UNKNOWN_CONVERGENCE) {
                str << "  [UNKNOWN]";
              } else {
                str << "  [UNKNOWN CONVERGENCE STATUS: " << taskresult.converged_status(k) << "]";
              }
              str << "\n";
            }
          });
      }

      // because taskresult is a histogram WITH error bars, add_histogram will do the
      // right thing and take them into account.
      finalhistogram.add_histogram(taskresult.hist);

      logger.debug("added first histogram.");


      // this one is declared for histograms WITHOUT error bars (SimpleHistogramType is a
      // UniformBinsHistogram), so it will just ignore the error bars.
      logger.debug([&](std::ostream & str) {
	  str << "Simple histogram is:\n";
	  Tomographer::histogram_pretty_print<SimpleHistogramType>(str, taskresult.hist);
	});
      simplefinalhistogram.add_histogram(taskresult.hist);
      logger.debug("done.");
    }
    template<typename Cnt, typename CData>
    inline void runs_finished(Cnt, const CData *)
    {
      finalhistogram.finalize();
      simplefinalhistogram.finalize();
    }


    inline void produce_final_report()
    {
      // produce report on runs
      auto logger = llogger.sublogger(TOMO_ORIGIN);
      logger.info([&](std::ostream & str) {
          str << "\n"
              << "                              Final Report of Runs                              \n"
              << "--------------------------------------------------------------------------------\n";
          std::size_t j;
          int w = (int)std::ceil(std::log10(collected_histograms.size()));
          for (j = 0; j < collected_histograms.size(); ++j) {
            str << "#" << std::setw(w) << j << ": "
                << histogram_short_bar(collected_histograms[j].hist, false, -3-w)
                << "\n";
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
          str << "--------------------------------------------------------------------------------\n"
              << "\n\n";
          // and the final histogram
          str << "                                 Final Histogram                                \n"
              << "--------------------------------------------------------------------------------\n";
          histogram_pretty_print(str, finalhistogram);
          str << "--------------------------------------------------------------------------------\n\n";
        });
    }

    inline void write_histogram_file(const std::string & csvfname)
    {
      auto logger = llogger.sublogger(TOMO_ORIGIN);
      logger.info("()");
      
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

template<bool BinningAnalysisErrorBars, typename TomoProblem, typename ValueCalculator>
struct TomorunModeTypes
{
};

template<typename TomoProblem, typename ValueCalculator>
struct TomorunModeTypes<true, TomoProblem, ValueCalculator>
{
  typedef TomorunCDataBinning<TomoProblem, ValueCalculator> CData;
};
template<typename TomoProblem, typename ValueCalculator>
struct TomorunModeTypes<false, TomoProblem, ValueCalculator>
{
  typedef TomorunCDataSimple<TomoProblem, ValueCalculator> CData;
};


//
// Here goes the actual program. This is templated, because then we can let Eigen use
// allocation on the stack rather than malloc'ing 2x2 matrices...
//
template<bool BinningAnalysisErrorBars, typename TomoProblem,
         typename FnMakeValueCalculator, typename LoggerType>
inline void tomorun(const TomoProblem & tomodat, const ProgOptions * opt,
		    FnMakeValueCalculator makeValueCalculator, LoggerType & baselogger)
{
  Tomographer::Logger::LocalLogger<LoggerType> logger(TOMO_ORIGIN, baselogger);
  //
  // create the OMP Task Manager and run.
  //

  auto valcalc = makeValueCalculator(tomodat);
  typedef decltype(valcalc) ValueCalculator;

  typedef typename TomorunModeTypes<BinningAnalysisErrorBars, TomoProblem, ValueCalculator>::CData OurCData;

  typedef Tomographer::MHRWTasks::MHRandomWalkTask<OurCData, std::mt19937>  OurMHRandomWalkTask;

  typedef typename OurCData::template ResultsCollector<LoggerType> OurResultsCollector;

  OurCData taskcdat(tomodat, valcalc, opt);
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

  results.produce_final_report();

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

  typedef Tomographer::MatrQ<FixedDim, FixedMaxPOVMEffects, double, unsigned int> OurMatrQ;
  typedef Tomographer::IndepMeasTomoProblem<OurMatrQ, double> OurTomoProblem;

  //
  // Read data from file
  //

  OurMatrQ matq(dim);
  OurTomoProblem tomodat(matq);

  typename Tomographer::Tools::eigen_std_vector<typename OurMatrQ::MatrixType>::type Emn;
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
  tomodat.Exn.resize(Npovmeffects, dim*dim);
  tomodat.Nx.resize(Npovmeffects);
  j = 0;
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
    auto Exn_row_as_col = tomodat.Exn.row(j).transpose();
    Tomographer::param_herm_to_x(Exn_row_as_col, Emn[k]);
    tomodat.Nx[j] = Nm[k];
    ++j;
  }
  // done.

  logger.debug("tomorun_dispatch()", [&](std::ostream & ss) {
                 ss << "\n\nExn: size="<<tomodat.Exn.size()<<"\n"
                    << tomodat.Exn << "\n";
                 ss << "\n\nNx: size="<<tomodat.Nx.size()<<"\n"
                    << tomodat.Nx << "\n";
               });

  tomodat.rho_MLE = Tomographer::MAT::value<typename OurMatrQ::MatrixType>(matf->var("rho_MLE"));

  ensure_valid_input(tomodat.rho_MLE.cols() == dim && tomodat.rho_MLE.rows() == dim,
		     Tomographer::Tools::fmts("rho_MLE is expected to be a square matrix %d x %d", dim, dim));

  tomodat.x_MLE = matq.initVectorParamType();
  Tomographer::param_herm_to_x(tomodat.x_MLE, tomodat.rho_MLE);

  tomodat.T_MLE = matq.initMatrixType();

  //  Eigen::LDLT<typename OurMatrQ::MatrixType> ldlt(tomodat.rho_MLE);
  //  typename OurMatrQ::MatrixType P = matq.initMatrixType();
  //  P = ldlt.transpositionsP().transpose() * OurMatrQ::MatrixType::Identity(dim,dim);
  //  tomodat.T_MLE.noalias() = P * ldlt.matrixL() * ldlt.vectorD().cwiseSqrt().asDiagonal();

  Eigen::SelfAdjointEigenSolver<typename OurMatrQ::MatrixType> rho_MLE_eig(tomodat.rho_MLE);
  tomodat.T_MLE = rho_MLE_eig.operatorSqrt();

  tomodat.NMeasAmplifyFactor = opt->NMeasAmplifyFactor;


  typedef typename OurMatrQ::MatrixType MatrixType;

  
  //
  // Data has now been successfully read. Now, dispatch to the correct template function
  // for futher processing.
  //

  //
  // One of the distance measures. There might be a reference state.
  //
  if (opt->valtype.valtype == val_type_spec::FIDELITY ||
      opt->valtype.valtype == val_type_spec::TR_DIST ||
      opt->valtype.valtype == val_type_spec::PURIF_DIST) {
    
    MatrixType rho_ref = matq.initMatrixType();
    MatrixType T_ref = matq.initMatrixType();
    
    std::string refname = "rho_MLE";
    if (opt->valtype.ref_obj_name.size()) {
      refname = opt->valtype.ref_obj_name;
    }

    rho_ref = Tomographer::MAT::value<MatrixType>(matf->var(refname));

    typedef typename Eigen::SelfAdjointEigenSolver<MatrixType>::RealVectorType RealVectorType;

    Eigen::SelfAdjointEigenSolver<MatrixType> eig_rho_ref( rho_ref );
    auto U = eig_rho_ref.eigenvectors();
    auto d = eig_rho_ref.eigenvalues();
    
    Tomographer::Tools::force_pos_vec_keepsum<RealVectorType>(d, 1e-12);
    
    rho_ref = U * d.asDiagonal() * U.adjoint();
    T_ref = U * d.cwiseSqrt().asDiagonal() * U.adjoint();

    logger.debug("tomorun_dispatch()", [&](std::ostream & str) {
	str << "Using rho_ref = \n" << rho_ref << "\n\t-> T_ref = \n" << T_ref << "\n";
      });

    if (opt->valtype.valtype == val_type_spec::FIDELITY) {
      tomorun<BinningAnalysisErrorBars>(
          tomodat,
          opt,
          [&T_ref](const OurTomoProblem & tomo) {
            return Tomographer::FidelityToRefCalculator<OurTomoProblem, double>(tomo, T_ref);
          },
          logger);
    } else if (opt->valtype.valtype == val_type_spec::PURIF_DIST) {
      tomorun<BinningAnalysisErrorBars>(
          tomodat,
          opt,
          [&T_ref](const OurTomoProblem & tomo) {
            return Tomographer::PurifDistToRefCalculator<OurTomoProblem, double>(tomo);
          },
          logger);
    } else if (opt->valtype.valtype == val_type_spec::TR_DIST) {
      tomorun<BinningAnalysisErrorBars>(
          tomodat,
          opt,
          [&rho_ref](const OurTomoProblem & tomo) {
              return Tomographer::TrDistToRefCalculator<OurTomoProblem, double>(tomo, rho_ref);
          },
          logger);
    } else {
      throw std::logic_error("WTF?? You shouldn't be here!");
    }

  } else if (opt->valtype.valtype == val_type_spec::OBS_VALUE) {
    
    // load the observable
    MatrixType A = tomodat.matq.initMatrixType();

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
        tomodat,
        opt,
        [&A](const OurTomoProblem & tomo) {
          return Tomographer::ObservableValueCalculator<OurTomoProblem>(tomo, A);
        },
        logger);

  } else {

    throw std::logic_error(std::string("Unknown value type: ")+streamstr(opt->valtype));

  }
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
