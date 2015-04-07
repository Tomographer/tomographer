
#include <cstdio>
#include <random>
#include <iostream>
#include <chrono>

#include <tomographer/qit/matrq.h>
#include <tomographer/loggers.h>
#include <tomographer/tomoproblem.h>
#include <tomographer/integrator.h>
#include <tomographer/dmintegrator.h>
#include <tomographer/multiprocomp.h>



int main()
{
  using namespace Tomographer;

  std::cout << "testing our integrator with Pauli meas. on a qubit ... \n";

  QubitPaulisMatrQ qmq(2);
  
  typedef IndepMeasTomoProblem<QubitPaulisMatrQ> OurTomoProblem;

  OurTomoProblem dat(qmq);

  dat.Exn = qmq.initVectorParamListType(6);
  std::cout << "Exn.size = " << dat.Exn.rows() << " x " << dat.Exn.cols() << "\n";
  dat.Exn <<
    0.5, 0.5,  0.707107,  0,
    0.5, 0.5, -0.707107,  0,
    0.5, 0.5,  0,         0.707107,
    0.5, 0.5,  0,        -0.707107,
    1,   0,    0,         0,
    0,   1,    0,         0
    ;
  dat.Nx = qmq.initFreqListType(6);
  //  dat.Nx << 1500, 800, 300, 300, 10, 30;

  // try to reproduce the nice "1qubit-test9-pureup-extreme-onlyupmeas" curve
  dat.Nx << 0, 0, 0, 0, 250, 0;

  dat.x_MLE << 1.0, 0, 0, 0; // pure up state
  dat.T_MLE <<
    1.0, 0,
    0,   0;

  /*
  typedef FidelityHistogramRWStatsCollector<QubitPaulisMatrQ,VacuumLogger>
    OurFidRWStatsCollector;

  struct OurCData {
    OurCData(const OurTomoProblem &prob_) : prob(prob_) { }
    OurTomoProblem prob;
    OurFidRWStatsCollector::HistogramType::Params histogram_params;
  };

  struct OurMHTask {
    int _seed;
    VacuumLogger _vlog;
    OurFidRWStatsCollector fidstatscollector;

    static inline int get_input(int k, const OurCData * pcdata)
    {
      return 438943 + k;
    }

    OurMHTask(int inputseed, const OurCData * pcdata)
      : _seed(inputseed),
        _vlog(),
        fidstatscollector(pcdata->histogram_params, pcdata->prob.T_MLE, pcdata->prob.matq, _vlog)
    {
    }

    inline void run(const OurCData * pcdata)
    {
      
      typedef DMStateSpaceRandomWalk<std::mt19937,OurTomoProblem,OurFidRWStatsCollector,VacuumLogger>
        MyRandomWalk;

      std::mt19937 rng(_seed); // seeded random number generator

      VacuumLogger vlog;

      MyRandomWalk rwalk(20, 500, 40000, 0.05, pcdata->prob.matq.initMatrixType(),
                         pcdata->prob, rng, fidstatscollector, vlog);
      
      rwalk.run();
    }
  };

  struct ResultsCollector {
    OurFidRWStatsCollector::HistogramType final_histogram;

    ResultsCollector(OurFidRWStatsCollector::HistogramType::Params p)
      : final_histogram(p)
    {
    }

    inline void init(unsigned int, unsigned int, const OurCData *) const { }

    inline void run_finished() const { }

    inline void collect_results(const OurMHTask& t)
    {
      final_histogram.bins += t.fidstatscollector.histogram().bins;
      final_histogram.off_chart += t.fidstatscollector.histogram().off_chart;
    }
  };
  */

  // NOW, RUN THE MH TASKS:

  SimpleFoutLogger flog(stdout, Logger::DEBUG);

  flog.debug("main()", "Starting to log stuff.");

  // MHRandomWalkTask<TomoProblem, typename Rng = std::mt19937, typename FidelityValueType = double>

  typedef DMIntegratorTasks::CData<OurTomoProblem> MyCData;
  typedef MultiProc::OMPTaskLogger<SimpleFoutLogger> MyTaskLogger;
  typedef DMIntegratorTasks::MHRandomWalkTask<OurTomoProblem,MyTaskLogger> MyMHRandomWalkTask;
  typedef DMIntegratorTasks::MHRandomWalkResultsCollector<MyMHRandomWalkTask::FidRWStatsCollector::HistogramType>
    MyResultsCollector;

  // ---------------
  
  // first, independently, test OMPTaskLogger:      --- WORKS!

  //   BufferLogger buflog(Logger::LONGDEBUG);
  //
  //   MultiProc::OMPTaskLogger<BufferLogger> testtasklogger(buflog);
  //
  //   testtasklogger.debug("main()", "test task logger: log something");
  //
  //   testtasklogger.longdebug("main()", "test task logger: log something on longdebug level");
  //
  //   std::cout << buflog.get_contents() << "\n";
  //
  //
  //   fprintf(stderr, "JUST TESTING TASK LOGGER FOR NOW. REMOVE exit(0); TO PERFORM OMP TEST.\n");
  //   ::exit(0);

  // ---------------

  MyCData taskcdat(dat);
  // seed for random number generator
  taskcdat.base_seed = std::chrono::system_clock::now().time_since_epoch().count();
  // parameters for the fidelity histogram
  taskcdat.histogram_params = MyCData::HistogramParams(0.98, 1.0, 50);
  // parameters of the random walk
  taskcdat.n_sweep = 20;
  taskcdat.n_therm = 500;
  taskcdat.n_run = 10000;
  taskcdat.step_size = 0.05;

  MyResultsCollector results(taskcdat.histogram_params);

//#define clock std::chrono::high_resolution_clock
#define clock std::chrono::system_clock

  auto time_start = clock::now();

  MultiProc::run_omp_tasks<MyMHRandomWalkTask>(
      &taskcdat, &results, 128 /* num_runs */, 1 /* n_chunk */,
      flog
      );

  auto time_end = clock::now();

  // delta-time, in seconds and fraction of seconds
  double dt = (double)(time_end - time_start).count() * clock::period::num / clock::period::den ;
  // integral part and fractional part
  double dt_i_d;
  double dt_f = std::modf(dt, &dt_i_d);
  int dt_i = (int)(dt_i_d+0.5);

  std::cout << "FINAL HISTOGRAM\n" << results.pretty_print() << "\n";

  std::cout << fmts("Total elapsed time: %d:%02d:%02d.%03d\n\n",
                    dt_i/3600, (dt_i/60)%60, dt_i%60, (int)(dt_f*1000+0.5)).c_str();
  
  return 0;
}
