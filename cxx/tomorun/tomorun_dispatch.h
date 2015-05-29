
#ifndef TOMORUN_DISPATCH
#define TOMORUN_DISPATCH


// -----------------------------------------------------------------------------


template<typename TomoProblem_, typename ValueCalculator_>
struct TomorunCData : public Tomographer::MHRWTasks::CDataBase<>
{
  typedef TomoProblem_ TomoProblem;
  typedef ValueCalculator_ ValueCalculator;
  typedef typename Tomographer::ValueHistogramMHRWStatsCollector<ValueCalculator>::Result HistogramType;
  typedef typename HistogramType::Params HistogramParams;


  TomorunCData(TomoProblem & tomo_, const ValueCalculator & vcalc_)
    : tomo(tomo_), vcalc(vcalc_), histogram_params()
  {
  }

  TomoProblem tomo;
  ValueCalculator vcalc;

  typename HistogramType::Params histogram_params;

  typedef HistogramType MHRWStatsCollectorResultType;

  template<typename LoggerType>
  inline Tomographer::ValueHistogramMHRWStatsCollector<ValueCalculator,LoggerType,HistogramType>
  createStatsCollector(LoggerType & logger) const
  {
    return Tomographer::ValueHistogramMHRWStatsCollector<ValueCalculator,LoggerType,HistogramType>(
	histogram_params,
	vcalc,
	logger
	);
  }

  template<typename Rng, typename LoggerType>
  inline Tomographer::DMStateSpaceLLHMHWalker<TomoProblem,Rng,LoggerType>
  createMHWalker(Rng & rng, LoggerType & log) const
  {
    return Tomographer::DMStateSpaceLLHMHWalker<TomoProblem,Rng,LoggerType>(
	tomo.matq.initMatrixType(),
	tomo,
	rng,
	log
	);
  }

};

template<typename TomoProblem_, typename ValueCalculator_>
struct TomorunResultsCollector {
  typedef TomoProblem_ TomoProblem;
  typedef ValueCalculator_ ValueCalculator;
  typedef typename Tomographer::ValueHistogramMHRWStatsCollector<ValueCalculator>::Result HistogramType;
  typedef typename HistogramType::Params HistogramParams;

  Tomographer::AveragedHistogram<HistogramType, double> finalhistogram;

  TomorunResultsCollector()
    : finalhistogram(HistogramParams())
  {
  }

  template<typename Cnt, typename CData>
  inline void init(Cnt /*num_total_runs*/, Cnt /*n_chunk*/,
		   const CData * pcdata)
  {
    finalhistogram.reset(pcdata->histogram_params);
  }
  template<typename Cnt, typename CData>
  inline void collect_result(Cnt /*task_no*/, const HistogramType& taskresult, const CData *)
  {
    finalhistogram.add_histogram(taskresult);
  }
  template<typename Cnt, typename CData>
  inline void runs_finished(Cnt, const CData *)
  {
    finalhistogram.finalize();
  }
};




// -----------------------------------------------------------------------------



//
// Here goes the actual program. This is templated, because then we can let Eigen use
// allocation on the stack rather than malloc'ing 2x2 matrices...
//
template<typename OurTomoProblem, typename FnMakeValueCalculator, typename Logger>
inline void tomorun(OurTomoProblem & tomodat, ProgOptions * opt,
		    FnMakeValueCalculator makeValueCalculator, Logger & logger)
{
  //
  // create the OMP Task Manager and run.
  //

  auto valcalc = makeValueCalculator(tomodat);
  typedef decltype(valcalc) OurValueCalculator;

  typedef TomorunCData<OurTomoProblem, OurValueCalculator> OurCData;

  typedef Tomographer::MHRWTasks::MHRandomWalkTask<OurCData, std::mt19937>  OurMHRandomWalkTask;

  typedef TomorunResultsCollector<OurTomoProblem, OurValueCalculator>  OurResultsCollector;

  OurCData taskcdat(tomodat, valcalc);
  // seed for random number generator
  taskcdat.base_seed = std::chrono::system_clock::now().time_since_epoch().count();
  // parameters for the value histogram
  taskcdat.histogram_params = typename OurCData::HistogramParams(opt->val_min, opt->val_max, opt->val_nbins);
  // parameters of the random walk
  taskcdat.n_sweep = opt->Nsweep;
  taskcdat.n_therm = opt->Ntherm;
  taskcdat.n_run = opt->Nrun;
  taskcdat.step_size = opt->step_size;

  OurResultsCollector results;

  auto tasks = Tomographer::MultiProc::makeOMPTaskDispatcher<OurMHRandomWalkTask>(
      &taskcdat, // constant data
      &results, // results collector
      logger, // the main logger object
      opt->Nrepeats, // num_runs
      opt->Nchunk // n_chunk
      );

  // set up signal handling
  auto srep = Tomographer::Tools::makeSigHandlerTaskDispatcherStatusReporter(&tasks, logger);
  Tomographer::Tools::installSignalStatusReportHandler(SIGINT, &srep);
  //  auto srep = makeSigHandlerStatusReporter(&tasks, logger);
  //  signal_handler = &srep;
  //  signal(SIGINT, sig_int_handler);

  // and run our tomo process

  auto time_start = TimerClock::now();

  srep.time_start = time_start;

  tasks.run();

  auto time_end = TimerClock::now();

  logger.debug("tomorun()", "Random walks done.");

  // delta-time, in seconds and fraction of seconds
  std::string elapsed_s = Tomographer::Tools::fmt_duration(time_end - time_start);

  logger.info(
      "tomorun()", [&results](std::ostream & str) {
        str << "FINAL HISTOGRAM\n" << results.finalhistogram.pretty_print() << "\n";
      });


  // save the histogram to a CSV file if the user required it
  if (opt->write_histogram.size()) {
    std::string csvfname = opt->write_histogram+"-histogram.csv";
    std::ofstream outf;
    outf.open(csvfname);
    outf << "Value\tAvgCounts\tError\n"
         << std::scientific << std::setprecision(10);
    for (int kk = 0; kk < results.finalhistogram.final_histogram.size(); ++kk) {
      outf << (double)results.finalhistogram.params.bin_lower_value(kk) << "\t"
           << (double)results.finalhistogram.final_histogram(kk) << "\t"
           << (double)results.finalhistogram.std_dev(kk) << "\n";
    }
    logger.info("tomorun()", "Wrote histogram to CSV file %s", csvfname.c_str());
  }

  logger.info("tomorun()",
              "Computation time: %s\n\n",
              elapsed_s.c_str());

}









// -----------------------------------------------------------------------------


//
// And here is the dispatcher. It will call the correct variant of tomorun() (with the
// correct template parameters), depending on what we were asked to do.
//
//
template<int FixedDim, int FixedMaxPOVMEffects, typename Logger>
inline void tomorun_dispatch(unsigned int dim, ProgOptions * opt, Tomographer::MAT::File * matf, Logger & logger)
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
  Tomographer::MAT::getListOfEigenMatrices(matf->var("Emn"), & Emn, true);
  Eigen::VectorXi Nm;
  Tomographer::MAT::getEigenMatrix(matf->var("Nm"), & Nm);
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

  Tomographer::MAT::getEigenMatrix(matf->var("rho_MLE"), &tomodat.rho_MLE);

  ensure_valid_input(tomodat.rho_MLE.cols() == dim && tomodat.rho_MLE.rows() == dim,
		     Tomographer::Tools::fmts("rho_MLE is expected to be a square matrix %d x %d", dim, dim));

  tomodat.x_MLE = matq.initVectorParamType();
  Tomographer::param_herm_to_x(tomodat.x_MLE, tomodat.rho_MLE);

  tomodat.T_MLE = matq.initMatrixType();

  //  Eigen::LDLT<typename OurMatrQ::MatrixType> ldlt(tomodat.rho_MLE);
  //  typename OurMatrQ::MatrixType P = matq.initMatrixType();
  //  P = ldlt.transpositionsP().transpose() * OurMatrQ::MatrixType::Identity(dim,dim);
  //  tomodat.T_MLE.noalias() = P * ldlt.matrixL() * ldlt.vectorD().cwiseSqrt().asDiagonal();

  tomodat.T_MLE = tomodat.rho_MLE.sqrt();

  tomodat.NMeasAmplifyFactor = opt->NMeasAmplifyFactor;

  const bool use_ref_state = opt->use_ref_state;
  typename OurMatrQ::MatrixType rho_ref = matq.initMatrixType();
  typename OurMatrQ::MatrixType T_ref = matq.initMatrixType();
  if (use_ref_state) {
    Tomographer::MAT::getEigenMatrix(matf->var("rho_ref"), &rho_ref);
    T_ref = rho_ref.sqrt();
  }

  //
  // Data has now been successfully read. Now, dispatch to the correct template function
  // for futher processing.
  //

  if (opt->val_type == "fidelity") {
    tomorun(tomodat, opt,
	    [use_ref_state,&T_ref](const OurTomoProblem & tomo)
	    -> Tomographer::FidelityToRefCalculator<OurTomoProblem, double>
	    {
	      if (use_ref_state) {
		return Tomographer::FidelityToRefCalculator<OurTomoProblem, double>(tomo, T_ref);
	      } else {
		return Tomographer::FidelityToRefCalculator<OurTomoProblem, double>(tomo);
	      }
	    },
	    logger);
  } else if (opt->val_type == "purif-dist") {
    tomorun(tomodat, opt,
	    [use_ref_state,&T_ref](const OurTomoProblem & tomo)
	    -> Tomographer::PurifDistToRefCalculator<OurTomoProblem, double>
	    {
	      if (use_ref_state) {
		return Tomographer::PurifDistToRefCalculator<OurTomoProblem, double>(tomo, T_ref);
	      } else {
		return Tomographer::PurifDistToRefCalculator<OurTomoProblem, double>(tomo);
	      }
	    },
	    logger);
  } else if (opt->val_type == "tr-dist") {
    tomorun(tomodat, opt,
	    [use_ref_state,&rho_ref](const OurTomoProblem & tomo)
	    -> Tomographer::TrDistToRefCalculator<OurTomoProblem, double>
	    {
	      if (use_ref_state) {
 		return Tomographer::TrDistToRefCalculator<OurTomoProblem, double>(tomo, rho_ref);
	      } else {
 		return Tomographer::TrDistToRefCalculator<OurTomoProblem, double>(tomo);
	      }
	    },
	    logger);
  } else if (opt->val_type == "obs-value") {
    // load the observable
    typename OurMatrQ::MatrixType A = tomodat.matq.initMatrixType();
    Tomographer::MAT::getEigenMatrix(matf->var("Observable"), &A);
    ensure_valid_input(A.cols() == dim && A.rows() == dim,
		       Tomographer::Tools::fmts("Observable is expected to be a square matrix %d x %d", dim, dim));
    // and run our main program
    tomorun(tomodat, opt,
	    [&A](const OurTomoProblem & tomo) {
	      return Tomographer::ObservableValueCalculator<OurTomoProblem>(tomo, A);
	    },
	    logger);
  } else {
    throw std::logic_error((std::string("Unknown value type: ")+opt->val_type).c_str());
  }
}






#endif
