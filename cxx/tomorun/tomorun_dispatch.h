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

#include <tomographer2/tools/cxxutil.h>
#include <tomographer2/tools/loggers.h>
#include <tomographer2/tools/ezmatio.h>
#include <tomographer2/tools/signal_status_report.h>
#include <tomographer2/tools/eigenutil.h>
#include <tomographer2/densedm/dmtypes.h>
#include <tomographer2/densedm/param_herm_x.h>
#include <tomographer2/densedm/indepmeasllh.h>
#include <tomographer2/densedm/tspacefigofmerit.h>
#include <tomographer2/mhrw.h>
#include <tomographer2/mhrwtasks.h>
#include <tomographer2/mhrw_valuehist_tasks.h>
#include <tomographer2/multiprocomp.h>
#include <tomographer2/densedm/tspacellhwalker.h>
#include <tomographer2/mathtools/pos_semidef_util.h>

// -----------------------------------------------------------------------------


template<typename DenseLLH_, typename CDataBaseType_>
struct TomorunCData : public CDataBaseType_
{
  typedef CDataBaseType_ Base; // base class

  static constexpr bool BinningAnalysisEnabled = Base::UseBinningAnalysis;

  typedef DenseLLH_ DenseLLH;
  typedef typename Base::ValueCalculator ValueCalculator;

 
  TOMOGRAPHER_ENABLED_IF(!BinningAnalysisEnabled)
  TomorunCData(const DenseLLH & llh_, ValueCalculator valcalc, const ProgOptions * opt, std::size_t base_seed)
    : Base(valcalc,
	   typename Base::HistogramParams(opt->val_min, opt->val_max, opt->val_nbins),
	   typename Base::MHRWParamsType(opt->Nsweep, opt->step_size, opt->Ntherm, opt->Nrun),
	   base_seed),
      llh(llh_)
  {
  }

  TOMOGRAPHER_ENABLED_IF(BinningAnalysisEnabled)
  TomorunCData(const DenseLLH & llh_, ValueCalculator valcalc, const ProgOptions * opt, std::size_t base_seed)
    : Base(valcalc,
	   typename Base::HistogramParams(opt->val_min, opt->val_max, opt->val_nbins),
	   opt->binning_analysis_num_levels,
	   typename Base::MHRWParamsType(opt->Nsweep, opt->step_size, opt->Ntherm, opt->Nrun),
	   base_seed),
      llh(llh_)
  {
  }

  const DenseLLH llh;

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

};

// ------------------------------------------------------------------------------
// Utility to generate report
// ------------------------------------------------------------------------------


template<typename HistType, typename RunTaskInfoType>
inline void print_short_bar_and_accept_ratio(std::ostream & str, int j, HistType && hist,
					     const RunTaskInfoType & inf,
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


template<typename TomorunCDataType, typename ResultsCollector, typename LoggerType,
	 TOMOGRAPHER_ENABLED_IF_TMPL(!TomorunCDataType::BinningAnalysisEnabled)>
static inline void produce_final_report(TomorunCDataType & cdata, ResultsCollector & res,
					LoggerType & logger)
{
  logger.debug("produce_final_report()", "about to produce final report.");
  // produce report on runs
  logger.info("produce_final_report()", [&](std::ostream & str) {
      auto collected_histograms = res.collectedResults();
      auto collected_runtaskinfos = res.collectedRunTaskInfos();
      auto finalhistogram = res.finalHistogram();
      str << "\n"
	  << report_final_header
	  << report_hline
	;
      cdata.printBasicCDataMHRWInfo(str);
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

template<typename TomorunCDataType, typename ResultsCollector, typename LoggerType,
	 TOMOGRAPHER_ENABLED_IF_TMPL(TomorunCDataType::BinningAnalysisEnabled)>
inline void produce_final_report(TomorunCDataType & cdata, ResultsCollector & res,
				 LoggerType & logger)
{
  logger.debug("produce_final_report()", "about to produce final report.");
  // produce report on runs
  logger.info("produce_final_report()", [&](std::ostream & str) {
      auto collected_histograms = res.collectedResults();
      auto collected_runtaskinfos = res.collectedRunTaskInfos();
      auto finalhistogram = res.finalHistogram();
      str << "\n"
	  << report_final_header
	  << report_hline
	;
      cdata.printBasicCDataMHRWInfo(str);
      std::size_t j;
      int w = (int)std::ceil(std::log10(collected_histograms.size()));
      for (j = 0; j < collected_histograms.size(); ++j) {
	print_short_bar_and_accept_ratio(str, j, collected_histograms[j].hist, collected_runtaskinfos[j], w);
	// error bars stats:
	const int nbins = collected_histograms[j].converged_status.size();
	const int n_conv = collected_histograms[j].converged_status
	  .cwiseEqual(ResultsCollector::BinningAnalysisParamsType::CONVERGED).count();
	Eigen::ArrayXi unkn_arr = (collected_histograms[j].converged_status
				   .cwiseEqual(ResultsCollector::BinningAnalysisParamsType::UNKNOWN_CONVERGENCE))
	  .template cast<int>();
	const int n_unknown = unkn_arr.count();
	const int n_unknown_followingotherunknown
	  = unkn_arr.segment(0,nbins-1).cwiseProduct(unkn_arr.segment(1,nbins-1)).count();
	const int n_unknown_isolated = n_unknown - n_unknown_followingotherunknown;
	const int n_notconv = collected_histograms[j].converged_status
	  .cwiseEqual(ResultsCollector::BinningAnalysisParamsType::NOT_CONVERGED).count();
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





//
// Here goes the actual program. Now the program options have been translated to template
// parameters appropriately.
//
template<bool UseBinningAnalysisErrorBars, typename DenseLLH,
         typename ValueCalculator, typename LoggerType>
inline void tomorun(const DenseLLH & llh, const ProgOptions * opt,
		    ValueCalculator valcalc, LoggerType & baselogger)
{
  Tomographer::Logger::LocalLogger<LoggerType> logger(TOMO_ORIGIN, baselogger);
  //
  // create the OMP Task Manager and run.
  //

  typedef TomorunCData<
    DenseLLH,
    Tomographer::MHRWTasks::ValueHistogramTasks::CDataBase<
      ValueCalculator,
      UseBinningAnalysisErrorBars,
      TomorunInt, // CountIntType
      TomorunReal, // StepRealType
      TomorunReal> // CountRealType
    > OurCData;

  typedef Tomographer::MHRWTasks::MHRandomWalkTask<OurCData, std::mt19937>  OurMHRandomWalkTask;

  typedef typename OurCData::template ResultsCollectorType<LoggerType>::type OurResultsCollector;

  // seed for random number generator
  auto base_seed = std::chrono::system_clock::now().time_since_epoch().count();

  OurCData taskcdat(llh, valcalc, opt, base_seed);

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

  // and run our tomo process

  auto time_start = TimerClock::now();

  srep.time_start = time_start;

  tasks.run();

  auto time_end = TimerClock::now();

  logger.debug("Random walks done.");

  // delta-time, in seconds and fraction of seconds
  std::string elapsed_s = Tomographer::Tools::fmt_duration(time_end - time_start);

  produce_final_report(taskcdat, results, logger.baselogger());

  // save the histogram to a CSV file if the user required it
  if (opt->write_histogram.size()) {
    std::string csvfname = opt->write_histogram + "-histogram.csv";
    std::ofstream outf;
    outf.open(csvfname);
    results.print_histogram_csv(outf);
    logger.info([&](std::ostream & str) { str << "Wrote histogram to CSV file " << csvfname << "."; });
  }

  logger.info("Computation time: %s\n\n", elapsed_s.c_str());
}





// -----------------------------------------------------------------------------


//
// And here is the dispatcher. It will call the correct variant of tomorun() (with the
// correct template parameters), depending on what we were asked to do.
//
//
template<int FixedDim, int FixedMaxPOVMEffects, bool UseBinningAnalysisErrorBars, typename LoggerType>
inline void tomorun_dispatch(unsigned int dim, ProgOptions * opt, Tomographer::MAT::File * matf, LoggerType & logger)
{
  logger.debug("tomorun_dispatch()", "Running tomography program! FixedDim=%d and FixedMaxPOVMEffects=%d",
               FixedDim, FixedMaxPOVMEffects);

  //
  // Typedefs for tomography data types
  //

  typedef Tomographer::DenseDM::DMTypes<FixedDim, TomorunReal> OurDMTypes;
  typedef Tomographer::DenseDM::IndepMeasLLH<OurDMTypes, TomorunReal, TomorunInt, FixedMaxPOVMEffects, true> OurDenseLLH;

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

  for (std::size_t k = 0; k < Emn.size(); ++k) {
    llh.addMeasEffect(Emn[k], Nm(k),
#ifdef DO_SLOW_POVM_CONSISTENCY_CHECKS
		      true
#else
		      false
#endif
	);
  }

  logger.debug("tomorun_dispatch()", [&](std::ostream & ss) {
      ss << "\n\nExn: size="<<llh.Exn().size()<<"\n"
	 << llh.Exn() << "\n";
      ss << "\n\nNx: size="<<llh.Nx().size()<<"\n"
	 << llh.Nx() << "\n";
    });

  llh.setNMeasAmplifyFactor(opt->NMeasAmplifyFactor);

  typedef typename OurDMTypes::MatrixType MatrixType;
  
  //
  // Data has now been successfully read. Now, dispatch to the correct template function
  // for futher processing.
  //

  //
  // ::::: Instantiate The Correct Figure Of Merit Calculator :::::
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
      tomorun<UseBinningAnalysisErrorBars>(
          llh,
          opt,
	  Tomographer::DenseDM::TSpace::FidelityToRefCalculator<OurDMTypes, double>(T_ref),
          logger);
    } else if (opt->valtype.valtype == val_type_spec::PURIF_DIST) {
      tomorun<UseBinningAnalysisErrorBars>(
          llh,
          opt,
	  Tomographer::DenseDM::TSpace::PurifDistToRefCalculator<OurDMTypes, double>(T_ref),
          logger);
    } else if (opt->valtype.valtype == val_type_spec::TR_DIST) {
      tomorun<UseBinningAnalysisErrorBars>(
          llh,
          opt,
	  Tomographer::DenseDM::TSpace::TrDistToRefCalculator<OurDMTypes, double>(rho_ref),
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
    tomorun<UseBinningAnalysisErrorBars>(
        llh,
        opt,
	Tomographer::DenseDM::TSpace::ObservableValueCalculator<OurDMTypes>(dmt, A),
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
