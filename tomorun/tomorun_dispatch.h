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
#include <tomographer2/valuecalculator.h>


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
	   typename Base::MHRWParamsType(opt->step_size, opt->Nsweep, opt->Ntherm, opt->Nrun),
	   base_seed),
      llh(llh_)
  {
  }

  TOMOGRAPHER_ENABLED_IF(BinningAnalysisEnabled)
  TomorunCData(const DenseLLH & llh_, ValueCalculator valcalc, const ProgOptions * opt, std::size_t base_seed)
    : Base(valcalc,
	   typename Base::HistogramParams(opt->val_min, opt->val_max, opt->val_nbins),
	   opt->binning_analysis_num_levels,
	   typename Base::MHRWParamsType(opt->step_size, opt->Nsweep, opt->Ntherm, opt->Nrun),
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

  typedef typename OurCData::template ResultsCollectorType<LoggerType>::Type OurResultsCollector;

  // seed for random number generator
  auto base_seed = std::chrono::system_clock::now().time_since_epoch().count();

  OurCData taskcdat(llh, valcalc, opt, base_seed);

  OurResultsCollector results(logger.parentLogger());

  auto tasks = Tomographer::MultiProc::OMP::makeTaskDispatcher<OurMHRandomWalkTask>(
      &taskcdat, // constant data
      &results, // results collector
      logger.parentLogger(), // the main logger object
      opt->Nrepeats, // num_runs
      opt->Nchunk // n_chunk
      );

  // set up signal handling
  auto srep = Tomographer::Tools::makeSigHandlerTaskDispatcherStatusReporter(&tasks, logger.parentLogger());
  Tomographer::Tools::installSignalHandler(SIGINT, &srep);

  // and run our tomo process

  auto time_start = TimerClock::now();

  tasks.run();

  auto time_end = TimerClock::now();

  logger.debug("Random walks done.");

  // delta-time, in seconds and fraction of seconds
  std::string elapsed_s = Tomographer::Tools::fmtDuration(time_end - time_start);

  logger.info([&](std::ostream & stream) {
      results.printFinalReport(stream, taskcdat);
    });

  // save the histogram to a CSV file if the user required it
  if (opt->write_histogram.size()) {
    std::string csvfname = opt->write_histogram + "-histogram.csv";
    std::ofstream outf;
    outf.open(csvfname);
    results.printHistogramCsv(outf);
    logger.info([&](std::ostream & str) { str << "Wrote histogram to CSV file " << csvfname << "."; });
  }

  logger.info("Computation time: %s\n\n", elapsed_s.c_str());
}








//
// And here is the dispatcher. It will call the correct variant of tomorun() (with the
// correct template parameters), depending on what we were asked to do.
//
//
template<int FixedDim, int FixedMaxDim, int FixedMaxPOVMEffects, bool UseBinningAnalysisErrorBars, typename LoggerType>
inline void tomorun_dispatch(const int dim, ProgOptions * opt, Tomographer::MAT::File * matf, LoggerType & baselogger)
{
  Tomographer::Logger::LocalLogger<LoggerType> logger(TOMO_ORIGIN, baselogger);

  logger.debug("preparing to dispatch. FixedDim=%d, FixedMaxDim=%d, FixedMaxPOVMEffects=%d",
               FixedDim, FixedMaxDim, FixedMaxPOVMEffects);

  //
  // Typedefs for tomography data types
  //

  typedef Tomographer::DenseDM::DMTypes<FixedDim, TomorunReal, FixedMaxDim> DMTypes;
  typedef Tomographer::DenseDM::IndepMeasLLH<DMTypes, TomorunReal, TomorunInt, FixedMaxPOVMEffects, true> OurDenseLLH;

  //
  // Read data from file
  //

  DMTypes dmt(dim);
  OurDenseLLH llh(dmt);

  typename Tomographer::Tools::EigenStdVector<typename DMTypes::MatrixType>::type Emn;
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
    llh.addMeasEffect(Emn[k], Nm(k), TOMORUN_DO_SLOW_POVM_CONSISTENCY_CHECKS);
  }

  logger.debug([&](std::ostream & ss) {
      ss << "\n\nExn: size="<<llh.Exn().size()<<"\n"
	 << llh.Exn() << "\n";
      ss << "\n\nNx: size="<<llh.Nx().size()<<"\n"
	 << llh.Nx() << "\n";
    });

  llh.setNMeasAmplifyFactor(opt->NMeasAmplifyFactor);

  typedef typename DMTypes::MatrixType MatrixType;
  
  //
  // Data has now been successfully read. Now, dispatch to the correct template function
  // for futher processing.
  //

  //////////////////////////////////////////////////////////////////////////////
#if TOMORUN_USE_MULTIPLEXORVALUECALCULATOR == 1 ////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////

  logger.debug("Using MultiplexorValueCalculator.");

  // Instantiate & Use a MultiplexorValueCalculator for the different possible figures of
  // merit.

  MatrixType T_ref(dmt.initMatrixType());
  MatrixType rho_ref(dmt.initMatrixType());
  MatrixType A(dmt.initMatrixType());

  // make sure that whichever fig of merit gets eventually used, each ValueCalculator has
  // valid parameters (they may error-check their argument).
  T_ref(0,0) = 1;
  rho_ref(0,0) = 1;
  // A may stay zero

  if (opt->valtype.valtype == val_type_spec::FIDELITY ||
      opt->valtype.valtype == val_type_spec::TR_DIST ||
      opt->valtype.valtype == val_type_spec::PURIF_DIST) {
    
    // read the reference state given explicitly as, e.g., "fidelity:rho_ref"
    Tomographer::MAT::EigenPosSemidefMatrixWithSqrt<MatrixType> mpsd =
      Tomographer::MAT::value<Tomographer::MAT::EigenPosSemidefMatrixWithSqrt<MatrixType> >(
          matf->var(opt->valtype.ref_obj_name.size() ? opt->valtype.ref_obj_name : std::string("rho_ref"))
          );
    rho_ref = std::move(mpsd.mat);
    T_ref = std::move(mpsd.sqrt);

    logger.debug([&](std::ostream & str) {
        str << "Using rho_ref = \n" << rho_ref << "\n\t-> T_ref = \n" << T_ref << "\n";
      });

  }
  else if (opt->valtype.valtype == val_type_spec::OBS_VALUE) {
    
    std::string obsname = "Observable";
    if (opt->valtype.ref_obj_name.size()) {
      obsname = opt->valtype.ref_obj_name;
    }

    A = Tomographer::MAT::value<MatrixType>(matf->var(obsname));
    
    ensure_valid_input(A.cols() == dim && A.rows() == dim,
		       Tomographer::Tools::fmts("Observable (%s) is expected to be a square matrix %d x %d",
						obsname.c_str(), dim, dim));
  }
  // -----------------------------------------------
  // INSERT FIGURE OF MERIT HERE: add an else-if branch, and set up appropriate parameters
  // for your ValueCalculator instance as for the other figures of merit. Just make sure
  // that your ValueCalculator gets an acceptable argument, even if your figure of merit
  // was not selected in the end, as it will be anyway constructed.
  // -----------------------------------------------
  else {
    throw invalid_input(streamstr("Unknown valtype: " << opt->valtype));
  }

  Tomographer::MultiplexorValueCalculator<
    TomorunReal,
    Tomographer::DenseDM::TSpace::FidelityToRefCalculator<DMTypes, TomorunReal>,
    Tomographer::DenseDM::TSpace::PurifDistToRefCalculator<DMTypes, TomorunReal>,
    Tomographer::DenseDM::TSpace::TrDistToRefCalculator<DMTypes, TomorunReal>,
    Tomographer::DenseDM::TSpace::ObservableValueCalculator<DMTypes>
    //...// INSERT CUSTOM FIGURE OF MERIT HERE: add your ValueCalculator instance here.
    >  multiplexor_value_calculator(
        // index of the valuecalculator to actually use:
        (opt->valtype.valtype == val_type_spec::FIDELITY ? 0 :
         (opt->valtype.valtype == val_type_spec::PURIF_DIST ? 1 :
          (opt->valtype.valtype == val_type_spec::TR_DIST ? 2 :
           (opt->valtype.valtype == val_type_spec::OBS_VALUE ? 3 :
            //...// INSERT CUSTOM FIGURE OF MERIT HERE: add check to set your figure of merit
            throw invalid_input(streamstr("Invalid valtype: " << opt->valtype.valtype))
               )))),
        // the valuecalculator instances which are available:
        Tomographer::DenseDM::TSpace::FidelityToRefCalculator<DMTypes, TomorunReal>(T_ref),
        Tomographer::DenseDM::TSpace::PurifDistToRefCalculator<DMTypes, TomorunReal>(T_ref),
        Tomographer::DenseDM::TSpace::TrDistToRefCalculator<DMTypes, TomorunReal>(rho_ref),
        Tomographer::DenseDM::TSpace::ObservableValueCalculator<DMTypes>(dmt, A)
        // INSERT CUSTOM FIGURE OF MERIT HERE: add your valuecalculator instance, with
        // valid constructor arguments.
        );

  tomorun<UseBinningAnalysisErrorBars>(
      llh,
      opt,
      multiplexor_value_calculator,
      logger.parentLogger());
        
  //////////////////////////////////////////////////////////////////////////////
#else  /////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////

  logger.debug("Not using MultiplexorValueCalculator, but directly specialized"
               " templated versions for each figure of merit.");

  //
  // Instantiate The Correct Figure Of Merit Calculator and directly delegate the call to
  // tomorun()
  //

  //
  // Figure of merit is one of the built-in distance measures. There might be a reference
  // state.
  //
  if (opt->valtype.valtype == val_type_spec::FIDELITY ||
      opt->valtype.valtype == val_type_spec::TR_DIST ||
      opt->valtype.valtype == val_type_spec::PURIF_DIST) {
    
    MatrixType rho_ref(dmt.initMatrixType());
    MatrixType T_ref(dmt.initMatrixType());
    
    // read the reference state given explicitly as, e.g., "fidelity:rho_ref"
    Tomographer::Mat::EigenPosSemidefMatrixWithSqrt<MatrixType> mpsd =
      Tomographer::MAT::value<Tomographer::MAT::EigenPosSemidefMatrixWithSqrt<MatrixType> >(
        matf->var(opt->valtype.ref_obj_name.size() ? opt->valtype.ref_obj_name : "rho_ref")
        );
    rho_ref = std::move(mpsd.mat);
    T_ref = std::move(mpsd.sqrt);

    if (opt->valtype.valtype == val_type_spec::FIDELITY) {
      tomorun<UseBinningAnalysisErrorBars>(
          llh,
          opt,
	  Tomographer::DenseDM::TSpace::FidelityToRefCalculator<DMTypes, TomorunReal>(T_ref),
          logger.parentLogger());
    } else if (opt->valtype.valtype == val_type_spec::PURIF_DIST) {
      tomorun<UseBinningAnalysisErrorBars>(
          llh,
          opt,
	  Tomographer::DenseDM::TSpace::PurifDistToRefCalculator<DMTypes, TomorunReal>(T_ref),
          logger.parentLogger());
    } else if (opt->valtype.valtype == val_type_spec::TR_DIST) {
      tomorun<UseBinningAnalysisErrorBars>(
          llh,
          opt,
	  Tomographer::DenseDM::TSpace::TrDistToRefCalculator<DMTypes, TomorunReal>(rho_ref),
          logger.parentLogger());
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
    MatrixType A(dmt.initMatrixType());

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
	Tomographer::DenseDM::TSpace::ObservableValueCalculator<DMTypes>(dmt, A),
        logger.parentLogger());

    return;
  }
  // --------------------------------------------------
  //
  // INSERT CUSTOM FIGURE OF MERIT HERE:
  //
  // Add a new "if (opt->valtype.valtype == val_type_spec::MY_NEW_CUSTOM_FIGURE_OF_MERIT)
  // { ... }" IF branch, read any possible reference state and do any other necessary
  // set-up, and then relay the call to "tomorun<UseBinningAnalysisErrorBars>( ... )".
  //
  // See also the instructions in API documentation, page 'Adding a new figure of merit to
  // the tomorun program'
  //
  // --------------------------------------------------

  throw std::logic_error(streamstr("Unknown value type: " << opt->valtype));


  //////////////////////////////////////////////////////////////////////////////
#endif /////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////
}




template<int FixedDim, int FixedMaxDim, int FixedMaxPOVMEffects, typename LoggerType>
inline void tomorun_dispatch_eb(const int dim, ProgOptions * opt, Tomographer::MAT::File * matf, LoggerType & logger)
{
  if (opt->binning_analysis_error_bars) {
    tomorun_dispatch<FixedDim, FixedMaxDim, FixedMaxPOVMEffects, true, LoggerType>(dim, opt, matf, logger);
  } else {
    tomorun_dispatch<FixedDim, FixedMaxDim, FixedMaxPOVMEffects, false, LoggerType>(dim, opt, matf, logger);
  }
}







#endif
